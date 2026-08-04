// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/avdk_utils/avdk_check.h>

#include "bk_flexa_bond_types.h"
#include "components/bk_decode/bk_jpeg_decode_ctlr.h"
#include "private_jpeg_decode_ctlr.h"
#include "bk_decode_pp_helper.h"
#include "hw_decoder_ctlr.h"
#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
#include "cache.h"
#endif
#include "modules/vcdec/vcdec_jpeg_api.h"
#include "avdk_monitor.h"
#include "common/avdk_pixel_types.h"

#define TAG "bk_jpeg_dec"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_DECODE_PORT_DONE_BIT(id) (1U << (id))

/*
 * Compute and advance the hardware read pointer. Caller must hold the critical
 * section (rtos_enter_critical).
 * Key fix: exclude ports that already reported the full-frame value (done) from
 * the backpressure min. An aborted/finished consumer must not pin the watermark
 * at the full-frame value; min then tracks only the slowest port still reading,
 * so the hardware read pointer can climb from 0 again on a new frame, avoiding
 * the previous frame's full-frame report latching the watermark across frames.
 */
static void jpeg_decode_apply_min_rd_to_hw(private_jpeg_decode_flexa_ctlr_t *ctrl)
{
    uint32_t min_rd = 0xFFFFFFFFU;

    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond != NULL && ctrl->port[i].first_bond == 0 && ctrl->port[i].done == 0) {
            if (ctrl->port[i].rd_blocks < min_rd) {
                min_rd = ctrl->port[i].rd_blocks;
            }
        }
    }

    if (min_rd != 0xFFFFFFFFU) {
        if (min_rd > ctrl->all_ports_min_rd) {
            ctrl->all_ports_min_rd = min_rd;
            DECODE_LINE_START;
            vcdec_jpeg_set_rd_ptr(ctrl->vcdec_handle, min_rd);
        }
        else if (min_rd < ctrl->all_ports_min_rd) {
            /* Keep this warning: a rolling report below the frame's current watermark
             * indicates a dropped/stale-frame anomaly; use it to observe whether the
             * done-exclusion defense holds during long soak. */
            LOGW("%s %d min_rd %u is less than all_ports_min_rd %u\r\n",
                 __func__, __LINE__, min_rd, ctrl->all_ports_min_rd);
        }
    }
}

static void jpeg_decode_flexa_clear_port_done_events(private_jpeg_decode_flexa_ctlr_t *ctrl)
{
    uint32_t mask = 0;

    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond != NULL) {
            mask |= JPEG_DECODE_PORT_DONE_BIT(i);
        }
    }
    if (mask != 0U && ctrl->port_done_events != NULL) {
        (void)rtos_clear_event_flags(&ctrl->port_done_events, mask);
    }
}

static avdk_err_t jpeg_decode_wait_flexa_registered_ports_done(private_jpeg_decode_flexa_ctlr_t *ctrl)
{
    uint32_t mask = 0;
    if (ctrl->port_done_events == NULL) {
        LOGE("%s %d port_done_events not initialized\r\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond != NULL && ctrl->port[i].first_bond == 0) {
            mask |= JPEG_DECODE_PORT_DONE_BIT(i);
        }
    }

    if (mask == 0U) {
        return AVDK_ERR_OK;
    }

    /* Wait for all already-registered downstream bonds in the same frame (same mask as clear, including first_bond ports). */
    beken_event_flags_t ux = rtos_wait_for_event_flags(&ctrl->port_done_events, mask,
                               true, WAIT_FOR_ALL_EVENTS, 800U);
    if ((ux & mask) != mask) {
        for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            uint32_t bit = JPEG_DECODE_PORT_DONE_BIT(i);

            if ((mask & bit) != 0U && (ux & bit) == 0U) {
                bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
                bk_flexa_bond_t *b_out = (bk_flexa_bond_t *)b->bond_config->out_stream;
                LOGW("%s %d flexa port[%u] decode timeout last:%d all_ports_min_rd:%d %d %d %d\r\n",
                    __func__, __LINE__, (unsigned)i, ctrl->port[i].rd_blocks, ctrl->all_ports_min_rd,
                    b->last_lines, b_out->last_lines, ctrl->last_flexa_line);
            }
        }
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

static void jpeg_decode_notify_flexa_bonds_error(private_jpeg_decode_flexa_ctlr_t *ctrl)
{
    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond == NULL) {
            continue;
        }
        bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
        if (b->error != NULL) {
            b->error(BK_FAIL, b);
        }
    }
}

static void frame_done_cb(int status, void *args)
{
    DECODE_FRAME_DONE;
    private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)args;
    if(ctrl == NULL) {
        LOGE("control is NULL\r\n");
        return;
    }
#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
    if (status == BK_OK && ctrl->decode_config.output_buffer != NULL &&
        ctrl->decode_config.output_size > 0U) {
        flush_dcache(ctrl->decode_config.output_buffer,
                     (long)ctrl->decode_config.output_size);
    }
#endif
    if (ctrl->config.frame_done_cb != NULL)
    {
        ctrl->config.frame_done_cb(status, ctrl->config.frame_done_args);
    }
}

static void flexa_done_cb(uint32_t wr_ptr, void *args)
{
    DECODE_LINE_END;
    private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)args;
    if(ctrl == NULL) {
        LOGE("control is NULL\r\n");
        return;
    }
    ctrl->last_flexa_line = wr_ptr;
    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond == NULL) {
            continue;
        }
        bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
        /* Hand the current decode frame_seq to the consumer with its lines, so a
         * report generated for this frame can be seq-matched by the decoder. */
        b->last_seq = ctrl->frame_seq;
        /* On the first REGISTER_BOND, do not dispatch the remaining flexa line interrupts of the current frame; normal dispatch resumes from wr_ptr == 1 in the next frame. */
        if (ctrl->port[i].first_bond) {
            if (wr_ptr == 1U) {
                ctrl->port[i].first_bond = 0;
                if (b->flexa_done != NULL) {
                    b->flexa_done(wr_ptr, b);
                }
            }
        } else {
            if (b->flexa_done != NULL) {
                b->flexa_done(wr_ptr, b);
            }
        }
    }
    if (ctrl->config.flexa_done_cb != NULL)
    {
        ctrl->config.flexa_done_cb(wr_ptr, ctrl->config.flexa_done_args);
    }
}

static avdk_err_t jpeg_decode_ctlr_init(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    avdk_err_t ret = hw_decoder_register(HW_DECODER_TYPE_JPEG, ctrl);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d register to hw decoder controller failed: %d\r\n", __func__, __LINE__, ret);
        goto error;
    }

    ret = rtos_init_semaphore(&ctrl->decode_done_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d init decode_done_sem failed\r\n", __func__, __LINE__);
        goto error;
    }

    ret = rtos_init_event_flags(&ctrl->port_done_events);
    if (ret != kNoErr) {
        LOGE("%s %d init port_done_events failed\r\n", __func__, __LINE__);
        goto error;
    }

    vcdec_config_t cfg = {0};
    cfg.mode = ctrl->mode;
    cfg.timeout_ms = (ctrl->config.timeout_ms != 0U) ? ctrl->config.timeout_ms : 1000U;
    cfg.frame_done_cb = frame_done_cb;
    cfg.flexa_done_cb = flexa_done_cb;
    cfg.args = ctrl;

    ret = vcdec_jpeg_init(&ctrl->vcdec_handle, &cfg);
    if (ret != VCDEC_OK) {
        LOGE("vcdec_jpeg_init failed: %d\r\n", (int)ret);
        goto error;
    }

    LOGI("%s %d JPEG decoder registered to hw controller\r\n", __func__, __LINE__);
    return AVDK_ERR_OK;

error:
    if (ctrl->port_done_events != NULL) {
        (void)rtos_deinit_event_flags(&ctrl->port_done_events);
    }
    if (ctrl->decode_done_sem != NULL) {
        rtos_deinit_semaphore(&ctrl->decode_done_sem);
        ctrl->decode_done_sem = NULL;
    }
    hw_decoder_unregister(ctrl);
    return ret;
}

static avdk_err_t jpeg_decode_ctlr_open(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    vcdec_ret_e ret = vcdec_jpeg_open(ctrl->vcdec_handle);
    if (ret != VCDEC_OK) {
        LOGE("vcdec_jpeg_open failed: %d\r\n", (int)ret);
        ctrl->vcdec_handle = NULL;
        return AVDK_ERR_GENERIC;
    }

    LOGI("JPEG decoder opened\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_callback(void *param)
{
    if (!param) {
        return AVDK_ERR_INVAL;
    }

    private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)param;
    if (!ctrl->vcdec_handle) {
        return AVDK_ERR_INVAL;
    }

    if (ctrl->mode == BK_JPEG_DECODE_FLEXA_MODE_FLEXA) {
        jpeg_decode_flexa_clear_port_done_events(ctrl);
    }
    /* Frame-boundary reset (lock-free): clears the watermark and per-port state at
     * the start of each frame. No critical section is taken; correctness relies on
     * the done flag, which excludes full-frame ports from the backpressure min so a
     * concurrent/late full-frame report cannot pin the watermark. Shared word writes
     * are atomic on SMP and apply's monotonic guard only advances the watermark. */
    ctrl->all_ports_min_rd = 0;
    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond == NULL) {
            continue;
        }
        ctrl->port[i].rd_blocks = 0;
        ctrl->port[i].done = 0;
    }
    /* Advance the monotonic decode frame counter (never 0). Consumers echo the
     * value they were handed (bond->last_seq) in their reports, letting the
     * decoder drop reports that belong to an already-finished frame. */
    ctrl->frame_seq++;
    if (ctrl->frame_seq == 0U) {
        ctrl->frame_seq = 1U;
    }
#if CONFIG_L2_CACHE_ENABLE || CONFIG_DCACHE
    if (ctrl->decode_config.input_stream != NULL &&
        ctrl->decode_config.input_stream_len > 0U) {
        flush_dcache(ctrl->decode_config.input_stream,
                     (long)ctrl->decode_config.input_stream_len);
    }
#endif

    DECODE_FRAME_START;
    DECODE_LINE_START;
    vcdec_ret_e ret = vcdec_jpeg_decode_frame(ctrl->vcdec_handle, &ctrl->decode_config);
    if (ret != VCDEC_FRAME_READY && ret != VCDEC_OK) {
        LOGE("%s %d vcdec_jpeg_decode_frame failed: %d\r\n", __func__, __LINE__, ret);
        ctrl->decode_result = AVDK_ERR_GENERIC;
        DECODE_FRAME_END;
        return AVDK_ERR_GENERIC;
    }
    DECODE_FRAME_END;
    ctrl->decode_result = BK_OK;

    if (ctrl->mode == BK_JPEG_DECODE_FLEXA_MODE_FLEXA) {
        ret = jpeg_decode_wait_flexa_registered_ports_done(ctrl);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s %d wait flexa registered ports done failed: %d\r\n", __func__, __LINE__, ret);
        }
    }

    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_decode_frame(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_input_t *input)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(input, AVDK_ERR_INVAL, TAG, "input is NULL");
    AVDK_RETURN_ON_FALSE(input->stream && input->stream_len > 0, AVDK_ERR_INVAL, TAG, "invalid stream");
    AVDK_RETURN_ON_FALSE(ctrl->vcdec_handle, AVDK_ERR_INVAL, TAG, "decoder not open");

    uint8_t *out_buffer = input->out_buffer;
    uint32_t out_buffer_size = input->out_buffer_size;

    const uint16_t seg_ht_mb = ctrl->config.segment_height;
    const uint8_t seg_num = ctrl->config.segment_number;
    const uint32_t out_w = (uint32_t)ctrl->config.out_width;
    uint32_t rb_size;
    vcdec_pp_out_format_e out_fmt;

    if (ctrl->config.out_format == BK_PIXEL_FORMAT_RGB565 ||
        ctrl->config.out_format == BK_PIXEL_FORMAT_RGB888) {
        LOGE("JPEG flexa RGB output is not supported; use frame RGB mode instead\r\n");
        return AVDK_ERR_INVAL;
    }
    out_fmt = bk_decode_pp_map_out_format(ctrl->config.out_format);
    rb_size = bk_decode_pp_flexa_rb_size(ctrl->config.out_width, seg_ht_mb, seg_num);

    if (out_w == 0U) {
        LOGE("Flexa mode requires out_width set via ioctl\r\n");
        return AVDK_ERR_INVAL;
    }

    if (!out_buffer || out_buffer_size == 0U) {
        LOGE("No output buffer or size\r\n");
        return AVDK_ERR_INVAL;
    }

    if (out_buffer_size < rb_size) {
        LOGE("output buffer too small: have=%u need=%u\r\n", out_buffer_size, rb_size);
        return AVDK_ERR_NOMEM;
    }

    ctrl->decode_config.input_stream = input->stream;
    ctrl->decode_config.input_stream_len = input->stream_len;
    ctrl->decode_config.output_buffer = out_buffer;
    ctrl->decode_config.output_size = out_buffer_size;
    ctrl->decode_config.out_width = ctrl->config.out_width;
    ctrl->decode_config.out_height = ctrl->config.out_height;
    ctrl->decode_config.out_format = out_fmt;
    ctrl->decode_config.segment_height = seg_ht_mb;
    ctrl->decode_config.segment_number = seg_num;

    hw_decoder_msg_t msg = {
        .decoder_type = HW_DECODER_TYPE_JPEG,
        .type = HW_DECODER_MSG_DECODE,
        .callback = jpeg_decode_callback,
        .param = ctrl,
        .sem = &ctrl->decode_done_sem,
    };

    avdk_err_t ret = hw_decoder_send_msg(&msg, BEKEN_WAIT_FOREVER);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    ret = rtos_get_semaphore(&ctrl->decode_done_sem, 2000);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d rtos_get_semaphore failed: %d\r\n", __func__, __LINE__, ret);
        if (ctrl->config.frame_done_cb != NULL)
        {
            ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
        }
        for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond != NULL) {
                bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
                if (b->frame_done != NULL) {
                    b->frame_done(BK_FAIL, b);
                }
            }
        }
        jpeg_decode_notify_flexa_bonds_error(ctrl);
        return AVDK_ERR_GENERIC;
    }

    if(ctrl->decode_result != BK_OK) {
        if (ctrl->config.frame_done_cb != NULL)
        {
            ctrl->config.frame_done_cb(BK_FAIL, ctrl->config.frame_done_args);
        }
        for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond != NULL) {
                bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
                if (b->frame_done != NULL) {
                    b->frame_done(BK_FAIL, b);
                }
            }
        }
        jpeg_decode_notify_flexa_bonds_error(ctrl);
        return AVDK_ERR_GENERIC;
    }
    for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
        if (ctrl->port[i].bond != NULL) {
            bk_flexa_bond_t *b = (bk_flexa_bond_t *)ctrl->port[i].bond;
            if (b->frame_done != NULL) {
                b->frame_done(BK_OK, b);
            }
        }
    }
    if (ctrl->config.frame_done_cb != NULL) {
        ctrl->config.frame_done_cb(BK_OK, ctrl->config.frame_done_args);
    }
    return AVDK_ERR_OK;
}

static void jpeg_decode_resources_deinit(private_jpeg_decode_flexa_ctlr_t *ctrl)
{
    if (ctrl->port_done_events != NULL) {
        (void)rtos_deinit_event_flags(&ctrl->port_done_events);
    }
    if (ctrl->decode_done_sem != NULL) {
        rtos_deinit_semaphore(&ctrl->decode_done_sem);
        ctrl->decode_done_sem = NULL;
    }
}

static avdk_err_t jpeg_decode_ctlr_close(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (ctrl->vcdec_handle) {
        vcdec_jpeg_close(ctrl->vcdec_handle);
    }
    LOGI("JPEG decoder closed\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_deinit(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    avdk_err_t ret = hw_decoder_unregister(ctrl);
    if (ret != AVDK_ERR_OK) {
        LOGE("Unregister from hw decoder controller failed: %d\r\n", ret);
        return ret;
    }
    if (ctrl->vcdec_handle) {
        vcdec_jpeg_deinit(ctrl->vcdec_handle);
        ctrl->vcdec_handle = NULL;
    }
    jpeg_decode_resources_deinit(ctrl);
    LOGI("JPEG decoder unregistered from hw controller\r\n");
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_ioctl(bk_jpeg_decode_ctlr_handle_t handle, uint32_t cmd, void *arg)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (cmd) {
    case BK_JPEG_DECODE_IOCTL_GET_INFO: {
        vcdec_ret_e ret = bk_jpeg_decode_get_img_info((bk_jpeg_decode_img_info_t *)arg);
        if (ret != VCDEC_OK) {
            LOGE("%s %d get_img_info failed: %d\r\n", __func__, __LINE__, (int)ret);
            return AVDK_ERR_GENERIC;
        }
        break;
    }
    case BK_JPEG_DECODE_IOCTL_ABORT:
        vcdec_jpeg_abort(ctrl->vcdec_handle);
        break;

    case BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR: {
        bk_jpeg_decode_port_rd_t *p = (bk_jpeg_decode_port_rd_t *)arg;
        AVDK_RETURN_ON_FALSE(p, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
        uint32_t max_lines = ((uint32_t)ctrl->config.out_height + 15U) / 16U;

        uint32_t flags = rtos_enter_critical();
        uint32_t i = 0;
        uint32_t drop = 0U;
        for (i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond == p->port_ptr) {
                uint32_t is_full = (p->rd_blocks >= max_lines) ? 1U : 0U;
                /* Scheme B seq-gate: a report carrying a known (non-zero) frame_seq that
                 * differs from the current decode frame is a stale cross-frame report --
                 * the consumer worker finished/aborted an older frame after the decoder
                 * already advanced. Drop it so it cannot re-pin this frame's watermark.
                 * frame_seq == 0 (legacy/rolling reports) is always accepted. The current
                 * frame's own (matching-seq) report is always accepted, so backpressure is
                 * still released once per frame (hard constraint preserved). */
                if (p->frame_seq != 0U && p->frame_seq != ctrl->frame_seq) {
                    drop = 1U;
                    break;
                }
                /* Same-frame duplicate full-frame report (e.g. GPU decode_error broadcast
                 * + worker abort both fire for this frame): the first already released
                 * backpressure; drop the rest. The first full-frame report is kept. */
                if (is_full && ctrl->port[i].done) {
                    drop = 1U;
                    break;
                }
                ctrl->port[i].rd_blocks = p->rd_blocks;
                /* A full-frame value = the consumer's "frame end/abort" signal:
                 * mark done and exclude it from the backpressure min so it no longer
                 * pins the watermark at the full-frame value. A later normal rolling
                 * report (< full-frame) clears done and rejoins backpressure. */
                ctrl->port[i].done = is_full;
                break;
            }
        }
        if (i == BK_JPEG_DECODE_RD_PORT_MAX) {
            rtos_exit_critical(flags);
            LOGE("%s %d no free port\r\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        if (!drop) {
            jpeg_decode_apply_min_rd_to_hw(ctrl);
        }
        rtos_exit_critical(flags);
        break;
    }

    case BK_JPEG_DECODE_IOCTL_REGISTER_BOND: {
        void *bond_ptr = (void *)arg;
        AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);

        uint32_t i = 0;
        for (i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond == NULL) {
                ctrl->port[i].bond = bond_ptr;
                ctrl->port[i].first_bond = 1U;
                ctrl->port[i].rd_blocks = 0;
                ctrl->port[i].done = 0;
                break;
            }
        }
        if (i == BK_JPEG_DECODE_RD_PORT_MAX) {
            LOGE("%s %d no free port\r\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        break;
    }

    case BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND: {
        void *bond_ptr = (void *)arg;
        AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
        for (uint32_t i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond == bond_ptr) {
                ctrl->port[i].bond = NULL;
                ctrl->port[i].first_bond = 0;
                ctrl->port[i].rd_blocks = 0;
                ctrl->port[i].done = 0;
                break;
            }
        }
        break;
    }

    case BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE: {
        void *bond_ptr = (void *)arg;
        AVDK_RETURN_ON_FALSE(bond_ptr, AVDK_ERR_INVAL, TAG, "%s %d arg is NULL", __func__, __LINE__);
        uint32_t i = 0;
        for (i = 0; i < BK_JPEG_DECODE_RD_PORT_MAX; i++) {
            if (ctrl->port[i].bond == bond_ptr) {
                break;
            }
        }
        if (i == BK_JPEG_DECODE_RD_PORT_MAX) {
            LOGE("%s %d no found port\r\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        if (ctrl->port_done_events == NULL) {
            LOGE("%s %d port_done_events is NULL\r\n", __func__, __LINE__);
            break;
        }
        rtos_set_event_flags(&ctrl->port_done_events, JPEG_DECODE_PORT_DONE_BIT(i));
        break;
    }

    default:
        LOGE("Unknown ioctl: %u\r\n", (unsigned)cmd);
        return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t jpeg_decode_ctlr_delete(bk_jpeg_decode_ctlr_handle_t handle)
{
    private_jpeg_decode_flexa_ctlr_t *ctrl = __containerof(handle, private_jpeg_decode_flexa_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_INVAL, TAG, "control is NULL");
    os_free(ctrl);
    LOGI("JPEG decoder deleted\r\n");
    return AVDK_ERR_OK;
}

avdk_err_t bk_jpeg_decode_flexa_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_flexa_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)os_malloc(sizeof(private_jpeg_decode_flexa_ctlr_t));
    AVDK_RETURN_ON_FALSE(ctrl, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(ctrl, 0, sizeof(private_jpeg_decode_flexa_ctlr_t));
    os_memcpy(&ctrl->config, config, sizeof(bk_jpeg_decode_flexa_config_t));

    ctrl->mode = BK_JPEG_DECODE_FLEXA_MODE_FLEXA;

    ctrl->ops.init = jpeg_decode_ctlr_init;
    ctrl->ops.open = jpeg_decode_ctlr_open;
    ctrl->ops.decode_frame = jpeg_decode_ctlr_decode_frame;
    ctrl->ops.close = jpeg_decode_ctlr_close;
    ctrl->ops.deinit = jpeg_decode_ctlr_deinit;
    ctrl->ops.ioctl = jpeg_decode_ctlr_ioctl;
    ctrl->ops.del = jpeg_decode_ctlr_delete;

    *handle = &ctrl->ops;
    LOGI("JPEG decoder controller created\r\n");
    return AVDK_ERR_OK;
}
