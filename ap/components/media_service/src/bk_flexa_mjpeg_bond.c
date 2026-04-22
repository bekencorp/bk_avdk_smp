// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>
#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_gpu.h>

#include "private_jpeg_decode_ctlr.h"

#include "bk_flexa_bond_types.h"

#define TAG "bk_flexa_mjpeg_bond"

#define LOGD(...) BK_LOGD(TAG, __VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, __VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, __VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, __VA_ARGS__)

static void h264e_flexa_done(uint32_t wr_ptr, void *args)
{
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
    if (out_stream == NULL || out_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
    if (in_stream == NULL) {
        return;
    }
    out_stream->last_lines = wr_ptr;
    bk_jpeg_decode_ctlr_handle_t src = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
    if (src != NULL) {
        bk_jpeg_decode_port_rd_t rd_cmd = {
            .port_ptr = in_stream,
            .rd_blocks = wr_ptr,
        };
        (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
    }
}

static void h264e_frame_done(uint32_t status, void *args)
{
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
    if (out_stream == NULL || out_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
    if (in_stream == NULL) {
        return;
    }
    bk_jpeg_decode_ctlr_handle_t src = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
    if (src != NULL) {
        if (status != BK_OK) {
            bk_jpeg_decode_port_rd_t rd_cmd = {
                .port_ptr = in_stream,
                .rd_blocks = out_stream->max_lines_per_frame,
            };
            (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
        }
        (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE,
                        (void *)in_stream);
    }
}

static void h264e_bond_mjpegd_flexa_done(uint32_t wr_ptr, void *args)
{
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
    if (in_stream == NULL || in_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)in_stream->bond_config->out_stream;
    if (out_stream == NULL) {
        return;
    }
    in_stream->last_lines = wr_ptr;
    bk_h264_encode_ctlr_handle_t enc = (bk_h264_encode_ctlr_handle_t)out_stream->handle;
    if (enc != NULL) {
        if (wr_ptr == 1) {
            bk_h264_encode_ioctl(enc, BK_H264_ENCODE_IOCTL_SET_FRAME_READY, NULL);
        }
        bk_h264_encode_ioctl(enc, BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY, (void *)wr_ptr);
    }
}

static void h264e_bond_mjpegd_decode_error(uint32_t reason, void *args)
{
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
    (void)in_stream;
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)in_stream->bond_config->out_stream;
    if (out_stream == NULL || out_stream->handle == NULL) {
        return;
    }
    bk_h264_encode_ctlr_handle_t enc = (bk_h264_encode_ctlr_handle_t)out_stream->handle;
    if (enc == NULL) {
        return;
    }
    LOGE("%s %d in:%d out:%d\r\n", __func__, __LINE__, in_stream->last_lines, out_stream->last_lines);
    if (reason == BK_FAIL) {
        bk_h264_encode_ioctl(enc, BK_H264_ENCODE_IOCTL_STOP_ENCODE, NULL);
    }
}

avdk_err_t bk_flexa_mjpegd_h264e_bond_start(void **bond,
                        bk_jpeg_decode_ctlr_handle_t jpeg,
                        bk_h264_encode_ctlr_handle_t h264)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_flexa_bond_config_t *bond_new = NULL;
    bk_flexa_bond_t *in_stream = NULL;
    bk_flexa_bond_t *out_stream = NULL;
    uint8_t jpeg_registered = 0;
    uint8_t h264_registered = 0;

    if (bond == NULL) {
        LOGE("%s %d bond is NULL\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }
    if (*bond != NULL) {
        LOGE("%s %d already started\r\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }
    if (jpeg == NULL || h264 == NULL) {
        LOGE("%s jpeg %p, h264 %p invalid parameters\r\n", __func__, jpeg, h264);
        return AVDK_ERR_INVAL;
    }

    bond_new = (bk_flexa_bond_config_t *)os_malloc(sizeof(bk_flexa_bond_config_t));
    if (bond_new == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(bond_new, 0, sizeof(bk_flexa_bond_config_t));

    in_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
    if (in_stream == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }
    os_memset(in_stream, 0, sizeof(bk_flexa_bond_t));

    out_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
    if (out_stream == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }
    os_memset(out_stream, 0, sizeof(bk_flexa_bond_t));

    bond_new->in_stream = in_stream;
    bond_new->out_stream = out_stream;
    bond_new->in_stream_type = BK_FLEXA_TYPE_MJPEG;
    bond_new->out_stream_type = BK_FLEXA_TYPE_H264E;

    in_stream->handle = (void *)jpeg;
    in_stream->flexa_done = h264e_bond_mjpegd_flexa_done;
    in_stream->error = h264e_bond_mjpegd_decode_error;
    in_stream->bond_config = bond_new;

    {
        private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)jpeg;

        out_stream->max_lines_per_frame = (ctrl->config.out_height + 15) / 16;
    }

    out_stream->handle = (void *)h264;
    out_stream->flexa_done = h264e_flexa_done;
    out_stream->frame_done = h264e_frame_done;
    out_stream->bond_config = bond_new;

    ret = bk_jpeg_decode_ioctl(jpeg, BK_JPEG_DECODE_IOCTL_REGISTER_BOND, in_stream);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d failed to register bond\r\n", __func__, __LINE__);
        goto error;
    }
    jpeg_registered = 1;

    ret = bk_h264_encode_ioctl(h264, BK_H264_ENCODE_IOCTL_REGISTER_BOND, out_stream);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d failed to register bond\r\n", __func__, __LINE__);
        goto error;
    }
    h264_registered = 1;

    *bond = bond_new;

    LOGI("%s %d bond started\r\n", __func__, __LINE__);
    return ret;

error:
    if (h264_registered && out_stream != NULL && out_stream->handle != NULL) {
        (void)bk_h264_encode_ioctl((bk_h264_encode_ctlr_handle_t)out_stream->handle,
                       BK_H264_ENCODE_IOCTL_UNREGISTER_BOND, out_stream);
    }
    if (jpeg_registered && in_stream != NULL && in_stream->handle != NULL) {
        (void)bk_jpeg_decode_ioctl((bk_jpeg_decode_ctlr_handle_t)in_stream->handle,
                       BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
    }
    if (bond_new != NULL) {
        os_free(bond_new);
    }
    if (in_stream != NULL) {
        os_free(in_stream);
    }
    if (out_stream != NULL) {
        os_free(out_stream);
    }
    LOGE("%s %d bond failed\r\n", __func__, __LINE__);
    return ret;
}

void bk_flexa_mjpegd_h264e_bond_stop(void *bond)
{
    bk_flexa_bond_config_t *bond_p = (bk_flexa_bond_config_t *)bond;
    if (bond_p == NULL) {
        return;
    }
    bk_flexa_bond_t *out_stream = bond_p->out_stream;
    if (out_stream != NULL && out_stream->handle != NULL) {
        (void)bk_h264_encode_ioctl((bk_h264_encode_ctlr_handle_t)out_stream->handle,
                       BK_H264_ENCODE_IOCTL_UNREGISTER_BOND, out_stream);
    }
    bk_flexa_bond_t *in_stream = bond_p->in_stream;
    if (in_stream != NULL && in_stream->handle != NULL) {
        (void)bk_jpeg_decode_ioctl((bk_jpeg_decode_ctlr_handle_t)in_stream->handle,
                       BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
    }
    if (in_stream != NULL) {
        os_free(in_stream);
    }
    if (out_stream != NULL) {
        os_free(out_stream);
    }
    os_free(bond_p);
    LOGI("%s %d bond stopped\r\n", __func__, __LINE__);
}

static void gpu_bond_mjpegd_flexa_done(uint32_t wr_ptr, void *args)
{
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
    if (in_stream == NULL || in_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)in_stream->bond_config->out_stream;
    if (out_stream == NULL) {
        return;
    }
    in_stream->last_lines = wr_ptr;
    bk_gpu_ctlr_handle_t gpu = (bk_gpu_ctlr_handle_t)out_stream->handle;
    if (gpu != NULL) {
        bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_FLEXA_LINES_READY, (void *)wr_ptr);
    }
}

static void gpu_bond_mjpegd_decode_error(uint32_t reason, void *args)
{
    (void)reason;
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)args;
    (void)in_stream;
}

static void gpu_flexa_done(uint32_t wr_ptr, void *args)
{
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
    if (out_stream == NULL || out_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
    if (in_stream == NULL) {
        return;
    }
    out_stream->last_lines = wr_ptr;
    bk_jpeg_decode_ctlr_handle_t src = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
    if (src != NULL) {
        bk_jpeg_decode_port_rd_t rd_cmd = {
            .port_ptr = in_stream,
            .rd_blocks = wr_ptr,
        };
        (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
    }
}

static void gpu_frame_done(uint32_t status, void *args)
{
    bk_flexa_bond_t *out_stream = (bk_flexa_bond_t *)args;
    if (out_stream == NULL || out_stream->bond_config == NULL) {
        return;
    }
    bk_flexa_bond_t *in_stream = (bk_flexa_bond_t *)out_stream->bond_config->in_stream;
    if (in_stream == NULL) {
        return;
    }
    bk_jpeg_decode_ctlr_handle_t src = (bk_jpeg_decode_ctlr_handle_t)in_stream->handle;
    if (src != NULL) {
        if (status != BK_OK) {
            bk_jpeg_decode_port_rd_t rd_cmd = {
                .port_ptr = in_stream,
                .rd_blocks = out_stream->max_lines_per_frame,
            };
            (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR, &rd_cmd);
        }
        {
            (void)bk_jpeg_decode_ioctl(src, BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE,
                           (void *)in_stream);
        }
    }
}

avdk_err_t bk_flexa_mjpegd_gpu_bond_start(void **bond,
                      bk_jpeg_decode_ctlr_handle_t jpeg,
                      bk_gpu_ctlr_handle_t gpu)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_flexa_bond_config_t *bond_new = NULL;
    bk_flexa_bond_t *in_stream = NULL;
    bk_flexa_bond_t *out_stream = NULL;
    uint8_t jpeg_registered = 0;
    uint8_t gpu_registered = 0;

    if (bond == NULL) {
        LOGE("%s bond out is NULL\r\n", __func__);
        return AVDK_ERR_INVAL;
    }
    if (*bond != NULL) {
        LOGE("%s already started\r\n", __func__);
        return AVDK_ERR_INVAL;
    }
    if (jpeg == NULL || gpu == NULL) {
        LOGE("%s jpeg %p, gpu %p invalid parameters\r\n", __func__, jpeg, gpu);
        return AVDK_ERR_INVAL;
    }

    bond_new = (bk_flexa_bond_config_t *)os_malloc(sizeof(bk_flexa_bond_config_t));
    if (bond_new == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(bond_new, 0, sizeof(bk_flexa_bond_config_t));

    in_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));

    if (in_stream == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }
    os_memset(in_stream, 0, sizeof(bk_flexa_bond_t));

    out_stream = (bk_flexa_bond_t *)os_malloc(sizeof(bk_flexa_bond_t));
    if (out_stream == NULL) {
        LOGE("%s %d failed to allocate memory\r\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }
    os_memset(out_stream, 0, sizeof(bk_flexa_bond_t));

    bond_new->in_stream = in_stream;
    bond_new->out_stream = out_stream;
    bond_new->in_stream_type = BK_FLEXA_TYPE_MJPEG;
    bond_new->out_stream_type = BK_FLEXA_TYPE_GPU;

    in_stream->handle = (void *)jpeg;
    in_stream->flexa_done = gpu_bond_mjpegd_flexa_done;
    in_stream->error = gpu_bond_mjpegd_decode_error;
    in_stream->bond_config = bond_new;

    {
        private_jpeg_decode_flexa_ctlr_t *ctrl = (private_jpeg_decode_flexa_ctlr_t *)jpeg;

        out_stream->max_lines_per_frame = (ctrl->config.out_height + 15) / 16;
    }

    out_stream->handle = (void *)gpu;
    out_stream->flexa_done = gpu_flexa_done;
    out_stream->frame_done = gpu_frame_done;
    out_stream->bond_config = bond_new;

    ret = bk_jpeg_decode_ioctl((bk_jpeg_decode_ctlr_handle_t)in_stream->handle, BK_JPEG_DECODE_IOCTL_REGISTER_BOND, in_stream);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d REGISTER_BOND JPEG failed: %d\r\n", __func__, __LINE__, ret);
        goto error;
    }
    jpeg_registered = 1;

    ret = bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle, BK_GPU_IOCTL_REGISTER_BOND, out_stream);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s %d REGISTER_BOND GPU failed: %d\r\n", __func__, __LINE__, ret);
        goto error;
    }
    gpu_registered = 1;

    *bond = bond_new;
    LOGI("%s %d bond started\r\n", __func__, __LINE__);
    return AVDK_ERR_OK;

error:
    if (gpu_registered && out_stream != NULL && out_stream->handle != NULL) {
        (void)bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle,
                   BK_GPU_IOCTL_UNREGISTER_BOND, out_stream);
    }
    if (jpeg_registered && in_stream != NULL && in_stream->handle != NULL) {
        (void)bk_jpeg_decode_ioctl((bk_jpeg_decode_ctlr_handle_t)in_stream->handle,
                       BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
    }
    if (bond_new != NULL) {
        os_free(bond_new);
    }
    if (in_stream != NULL) {
        os_free(in_stream);
    }
    if (out_stream != NULL) {
        os_free(out_stream);
    }
    LOGE("%s %d bond failed\r\n", __func__, __LINE__);
    return ret;
}

void bk_flexa_mjpegd_gpu_bond_stop(void *bond)
{
    bk_flexa_bond_config_t *bond_p = (bk_flexa_bond_config_t *)bond;
    if (bond_p == NULL) {
        return;
    }
    bk_flexa_bond_t *in_stream = bond_p->in_stream;
    bk_flexa_bond_t *out_stream = bond_p->out_stream;

    if (out_stream != NULL && out_stream->handle != NULL) {
        (void)bk_gpu_ioctl((bk_gpu_ctlr_handle_t)out_stream->handle,
                   BK_GPU_IOCTL_UNREGISTER_BOND, out_stream);
    }
    if (in_stream != NULL && in_stream->handle != NULL) {
        (void)bk_jpeg_decode_ioctl((bk_jpeg_decode_ctlr_handle_t)in_stream->handle,
                       BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND, in_stream);
    }
    if (in_stream != NULL) {
        os_free(in_stream);
    }
    if (out_stream != NULL) {
        os_free(out_stream);
    }
    os_free(bond_p);
    LOGI("%s %d bond stopped\r\n", __func__, __LINE__);
}
