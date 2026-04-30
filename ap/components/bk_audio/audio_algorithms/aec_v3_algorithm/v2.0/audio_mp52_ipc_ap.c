#include "audio_mp52_ipc_ap.h"

#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#include <modules/audio_mp52_ipc.h>

#define TAG "aec_m52_ap"

#ifndef AEC_M52_MB_CHNL
#define AEC_M52_MB_CHNL MB_CHNL_AUD_AEC
#endif

typedef struct {
    aec_m52_slot_desc_t desc;
    uint32_t busy;
    uint32_t done;
    int16_t *ref_buf;
    int16_t *mic_buf;
    int16_t *out_buf;
    uint8_t *ecout_buf;
} aec_m52_slot_t;

struct aec_m52_proxy {
    uint32_t ref_bytes;
    uint32_t mic_bytes;
    uint32_t out_bytes;
    uint32_t next_seq;
    volatile int32_t last_done_slot;
    volatile uint32_t last_done_seq;
    aec_m52_ctrl_cfg_t ctrl_cfg;
    aec_m52_slot_t slots[AEC_M52_SLOT_COUNT];
};

typedef struct {
    uint32_t done_cnt;
    uint32_t submit_fail_cnt;
    uint32_t busy_drop_cnt;
} aec_m52_ap_debug_stats_t;

static uint8_t s_ipc_opened = 0;
static aec_m52_proxy_t *s_proxy = NULL;
static aec_m52_ap_debug_stats_t s_ap_dbg_stats = {0};

static void aec_m52_ap_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
    (void)param;
    (void)ack_buf;
}

static void aec_m52_ap_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
    aec_m52_proxy_t *proxy = (aec_m52_proxy_t *)param;
    uint32_t seq    = cmd_buf->param1;
    uint32_t slot   = cmd_buf->param2;
    uint32_t status = cmd_buf->param3;

    if ((proxy != NULL) && (cmd_buf->hdr.cmd == AEC_M52_IPC_CMD_DONE) &&
        (slot < AEC_M52_SLOT_COUNT) && (status == AEC_M52_IPC_STATUS_OK)) {
        proxy->slots[slot].busy = 0;
        proxy->slots[slot].done = 1;
        proxy->last_done_slot = (int32_t)slot;
        proxy->last_done_seq = seq;
        s_ap_dbg_stats.done_cnt++;
        if ((s_ap_dbg_stats.done_cnt % 200) == 0) {
            BK_LOGV(TAG, "rx done cnt = %d, seq = %d, slot = %d\n", s_ap_dbg_stats.done_cnt, seq, slot);
        }
    } else if (cmd_buf->hdr.cmd == AEC_M52_IPC_CMD_DONE) {
        BK_LOGV(TAG, "rx done invalid seq = %d, slot = %d, status = %d\n", seq, slot, status);
    }

    cmd_buf->param1 = seq;
    cmd_buf->param2 = slot;
    cmd_buf->param3 = AEC_M52_IPC_STATUS_OK;
}

static bk_err_t aec_m52_ipc_open(aec_m52_proxy_t *proxy)
{
    bk_err_t ret = BK_OK;

    if (s_ipc_opened) {
        return BK_OK;
    }

    ret = mb_chnl_open(AEC_M52_MB_CHNL, proxy);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "mb_chnl_open failed ret = %d\n", ret);
        return ret;
    }

    ret = mb_chnl_ctrl(AEC_M52_MB_CHNL, MB_CHNL_SET_RX_ISR, aec_m52_ap_rx_isr);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "set rx isr failed ret = %d\n", ret);
        return ret;
    }

    ret = mb_chnl_ctrl(AEC_M52_MB_CHNL, MB_CHNL_SET_TX_CMPL_ISR, aec_m52_ap_tx_cmpl_isr);
    if (ret != BK_OK) {
        BK_LOGE(TAG, "set tx cmpl isr failed ret = %d\n", ret);
        return ret;
    }

    s_ipc_opened = 1;
    BK_LOGI(TAG, "audio_mp52_ipc open done chnl = %d\n", AEC_M52_MB_CHNL);
    return BK_OK;
}

static bk_err_t aec_m52_send_ctrl(aec_m52_proxy_t *proxy)
{
    mb_chnl_cmd_t cmd;
    os_memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.cmd = AEC_M52_IPC_CMD_CTRL;
    cmd.param1  = (uint32_t)(uintptr_t)&proxy->ctrl_cfg;
    cmd.param2  = sizeof(proxy->ctrl_cfg);
    cmd.param3  = proxy->ctrl_cfg.fs;
    if (mb_chnl_write(AEC_M52_MB_CHNL, &cmd) != BK_OK) {
        BK_LOGW(TAG, "send ctrl failed, fs = %d, frame = %d\n", proxy->ctrl_cfg.fs, proxy->ctrl_cfg.frame_bytes);
        return BK_FAIL;
    } else {
        BK_LOGD(TAG, "send ctrl fs = %d, frame = %d, init_flags = 0x%x\n",
                proxy->ctrl_cfg.fs,
                proxy->ctrl_cfg.frame_bytes,
                proxy->ctrl_cfg.init_flags);
        return BK_OK;
    }
}

aec_m52_proxy_t *aec_m52_proxy_create(const aec_m52_ctrl_cfg_t *ctrl_cfg,
                                      uint32_t ref_bytes,
                                      uint32_t mic_bytes,
                                      uint32_t out_bytes)
{
    if (s_proxy != NULL) {
        return s_proxy;
    }
    if ((ctrl_cfg == NULL) || (ctrl_cfg->magic != AEC_M52_CTRL_MAGIC)) {
        BK_LOGE(TAG, "invalid ctrl cfg\n");
        return NULL;
    }

    aec_m52_proxy_t *proxy = (aec_m52_proxy_t *)os_zalloc(sizeof(aec_m52_proxy_t));
    if (proxy == NULL) {
        return NULL;
    }

    proxy->ctrl_cfg = *ctrl_cfg;
    proxy->ref_bytes = ref_bytes;
    proxy->mic_bytes = mic_bytes;
    proxy->out_bytes = out_bytes;
    proxy->last_done_slot = -1;

    for (uint32_t i = 0; i < AEC_M52_SLOT_COUNT; i++)
    {
        aec_m52_slot_t *slot = &proxy->slots[i];
        slot->ref_buf   = (int16_t *)os_malloc(ref_bytes);
        slot->mic_buf   = (int16_t *)os_malloc(mic_bytes);
        slot->out_buf   = (int16_t *)os_malloc(out_bytes);
        slot->ecout_buf = (uint8_t *)os_malloc(out_bytes);
        if ((slot->ref_buf == NULL) || (slot->mic_buf == NULL) ||
            (slot->out_buf == NULL) || (slot->ecout_buf == NULL)) {
            aec_m52_proxy_destroy(proxy);
            return NULL;
        }

        slot->desc.magic = AEC_M52_SLOT_MAGIC;
        slot->desc.ref_bytes   = ref_bytes;
        slot->desc.mic_bytes   = mic_bytes;
        slot->desc.out_bytes   = out_bytes;
        slot->desc.ecout_bytes = out_bytes;

        slot->desc.ref_addr   = slot->ref_buf;
        slot->desc.mic_addr   = slot->mic_buf;
        slot->desc.out_addr   = slot->out_buf;
        slot->desc.ecout_addr = slot->ecout_buf;
    }

    if (aec_m52_ipc_open(proxy) != BK_OK) {
        aec_m52_proxy_destroy(proxy);
        return NULL;
    }

    (void)aec_m52_send_ctrl(proxy);
    s_proxy = proxy;
    BK_LOGD(TAG, "proxy create fs = %d, ref = %d, mic = %d, out = %d, dual = %d, ns = %d\n",
            proxy->ctrl_cfg.fs,
            ref_bytes, mic_bytes, out_bytes,
            proxy->ctrl_cfg.dual_ch,
            proxy->ctrl_cfg.ns_type);
    return proxy;
}

bk_err_t aec_m52_proxy_update_ctrl(aec_m52_proxy_t *proxy, const aec_m52_ctrl_cfg_t *ctrl_cfg)
{
    if ((proxy == NULL) || (ctrl_cfg == NULL) || (ctrl_cfg->magic != AEC_M52_CTRL_MAGIC)) {
        return BK_FAIL;
    }

    proxy->ctrl_cfg = *ctrl_cfg;
    return aec_m52_send_ctrl(proxy);
}

void aec_m52_proxy_destroy(aec_m52_proxy_t *proxy)
{
    if (proxy == NULL) {
        return;
    }

    for (uint32_t i = 0; i < AEC_M52_SLOT_COUNT; i++) {
        os_free(proxy->slots[i].ref_buf);
        os_free(proxy->slots[i].mic_buf);
        os_free(proxy->slots[i].out_buf);
        os_free(proxy->slots[i].ecout_buf);
    }

    if (s_proxy == proxy) {
        s_proxy = NULL;
    }
    os_free(proxy);
}

bk_err_t aec_m52_proxy_copy_last_output(aec_m52_proxy_t *proxy, int16_t *out, uint32_t out_bytes)
{
    if ((proxy == NULL) || (out == NULL)) {
        return BK_FAIL;
    }

    int32_t slot = proxy->last_done_slot;
    if ((slot < 0) || (slot >= (int32_t)AEC_M52_SLOT_COUNT)) {
        os_memset(out, 0, out_bytes);
        return BK_FAIL;
    }

    os_memcpy(out, proxy->slots[slot].out_buf, out_bytes);
    return BK_OK;
}

bk_err_t aec_m52_proxy_copy_last_feedback(aec_m52_proxy_t *proxy, aec_m52_feedback_t *feedback)
{
    if ((proxy == NULL) || (feedback == NULL)) {
        return BK_FAIL;
    }

    int32_t slot = proxy->last_done_slot;
    if ((slot < 0) || (slot >= (int32_t)AEC_M52_SLOT_COUNT)) {
        return BK_FAIL;
    }

    const aec_m52_slot_desc_t *desc = &proxy->slots[slot].desc;
    feedback->test  = desc->aec_test;
    feedback->spcnt = desc->aec_spcnt;
    feedback->dcnt  = desc->aec_dcnt;
    feedback->dc    = desc->aec_dc;
    feedback->mic_max = desc->aec_mic_max;
    feedback->vad_hr  = desc->aec_vad_hr;
    feedback->phs_cur = desc->aec_phs_cur;
    return BK_OK;
}

bk_err_t aec_m52_proxy_copy_last_ecout(aec_m52_proxy_t *proxy, uint8_t *ecout, uint32_t ecout_bytes)
{
    if ((proxy == NULL) || (ecout == NULL)) {
        return BK_FAIL;
    }

    int32_t slot = proxy->last_done_slot;
    if ((slot < 0) || (slot >= (int32_t)AEC_M52_SLOT_COUNT)) {
        return BK_FAIL;
    }

    const aec_m52_slot_t *last_slot = &proxy->slots[slot];
    if ((last_slot->ecout_buf == NULL) || (ecout_bytes > last_slot->desc.ecout_bytes)) {
        return BK_FAIL;
    }

    os_memcpy(ecout, last_slot->ecout_buf, ecout_bytes);
    return BK_OK;
}

bk_err_t aec_m52_proxy_submit(aec_m52_proxy_t *proxy, const int16_t *ref, const int16_t *mic)
{
    if ((proxy == NULL) || (ref == NULL) || (mic == NULL)) {
        return BK_FAIL;
    }

    uint32_t slot_id = proxy->next_seq % AEC_M52_SLOT_COUNT;
    aec_m52_slot_t *slot = &proxy->slots[slot_id];
    if (slot->busy) {
        s_ap_dbg_stats.busy_drop_cnt++;
        if ((s_ap_dbg_stats.busy_drop_cnt % 50) == 1) {
            BK_LOGW(TAG, "submit busy drop cnt = %d, seq = %d, slot = %d\n",
                    s_ap_dbg_stats.busy_drop_cnt,
                    proxy->next_seq,
                    slot_id);
        }
        return BK_FAIL;
    }

    slot->busy = 1;
    slot->done = 0;
    slot->desc.seq = proxy->next_seq;

    os_memcpy(slot->ref_buf, ref, proxy->ref_bytes);
    os_memcpy(slot->mic_buf, mic, proxy->mic_bytes);

    mb_chnl_cmd_t cmd;
    os_memset(&cmd, 0x00, sizeof(cmd));
    cmd.hdr.cmd = AEC_M52_IPC_CMD_RUN;
    cmd.param1 = (uint32_t)(uintptr_t)&slot->desc;
    cmd.param2 = slot->desc.seq;
    cmd.param3 = slot_id;

    if (mb_chnl_write(AEC_M52_MB_CHNL, &cmd) != BK_OK) {
        slot->busy = 0;
        s_ap_dbg_stats.submit_fail_cnt++;
        BK_LOGE(TAG, "submit write fail cnt = %d, seq = %d, slot = %d\n",
                s_ap_dbg_stats.submit_fail_cnt,
                slot->desc.seq,
                slot_id);
        return BK_FAIL;
    }

    proxy->next_seq++;
    return BK_OK;
}
