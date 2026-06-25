#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <components/log.h>
#include <driver/int.h>
#include <common/bk_err.h>

#include "components/bk_frame_buffer.h"
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "components/bk_encode/bk_jpeg_encode_ctlr.h"

#include "driver/isp.h"
#include "app_camera.h"
#include "app_codec.h"
#include "doorbell_img_manager.h"
#if CONFIG_NTWK_H264_DROP_POLICY
#include "h264_backpressure_drop.h"
#endif

#define TAG "db-codec"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

bk_h264_encode_ctlr_handle_t doorbell_enc_handler = NULL;
bk_jpeg_encode_ctlr_handle_t doorbell_jpeg_enc_handler = NULL;
beken_queue_t h264e_request_que = NULL;
beken_queue_t h264e_complete_que = NULL;
#define MAX_QUEUE_LEN 5
//#define DUMP_ENCODE_DATA_ENABLE

static uint32_t app_codec_isp_channel_buffer_size(const isp_channel_config_t *chn)
{
    uint32_t buffer_size = 0;

    if (chn == NULL)
    {
        return 0;
    }

    if (chn->u_addr > chn->y_addr)
    {
        uint32_t y_size = chn->u_addr - chn->y_addr;
        return y_size + y_size / 2U;
    }

    for (uint8_t i = 0; i < chn->chn_attr.chnFormat.numPlanes; i++)
    {
        buffer_size += chn->chn_attr.chnFormat.planeFmt[i].size;
    }

    return buffer_size;
}

typedef enum
{
	H264E_MSG_REQUEST = 0,
	H264E_MSG_COMPLETE,
    H264E_MSG_ENC_FREE,
} h264e_msg_type_t;


typedef struct
{
    uint32_t event;
    uint32_t param;
} h264e_msg_t;

#define ENCODE_QUENE_ENABLE (0)

void *encoder_buffer_request(uint32_t buffer_len, void *args)
{
    frame_buffer_t *temp_buffer = NULL;
    if (buffer_len > 0)
    {
        temp_buffer = (frame_buffer_t *)bk_encoded_data_request();
        if(temp_buffer == NULL) {
            return NULL;
        }
    }

    return temp_buffer != NULL ? temp_buffer->frame : NULL;
}

uint32_t encoder_buffer_complete(bk_h264_encode_outbuf_info_t *info)
{
    bk_err_t ret = BK_OK;
    if (info == NULL || info->outbuf == NULL) {
        return BK_FAIL;
    }

    uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
    frame_buffer_t *buffer = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
    if (info->status == BK_OK)
    {
        buffer->length = info->length;
        buffer->h264_type = info->type;
        buffer->fmt = PIXEL_FMT_H264;
        buffer->sequence = info->sequence;
        bk_encoded_data_complete_request((uint8_t *)buffer);
#if CONFIG_NTWK_H264_DROP_POLICY
        if (ntwk_h264_backpressure_drop_consume_force_idr() && doorbell_enc_handler != NULL) {
            bk_h264_encode_force_idr(doorbell_enc_handler);
        }
#endif
    }
    else
    {
        bk_encoded_data_free_request((uint8_t *)buffer);
    }
    return ret;
}

static uint32_t jpeg_encoder_buffer_complete(bk_jpeg_encode_outbuf_info_t *info)
{
    if (info == NULL || info->outbuf == NULL) {
        return BK_FAIL;
    }

    uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
    frame_buffer_t *buffer = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
    if (info->status == BK_OK)
    {
        buffer->length = info->length;
        buffer->h264_type = 0;
        buffer->fmt = PIXEL_FMT_JPEG;
        buffer->sequence = info->sequence;
        void *isp_handle = app_isp_handle_get();
        if (isp_handle != NULL) {
            isp_control_t *isp_control = (isp_control_t *)isp_handle;
            buffer->width = isp_control->chn[ISP_MP_CHN_ID].chn_attr.chnFormat.width;
            buffer->height = isp_control->chn[ISP_MP_CHN_ID].chn_attr.chnFormat.height;
        }
        bk_encoded_data_complete_request((uint8_t *)buffer);
    }
    else
    {
        bk_encoded_data_free_request((uint8_t *)buffer);
    }
    return BK_OK;
}

int app_h264e_turn_off(void)
{
    if (doorbell_enc_handler == NULL) {
        return BK_OK;
    }

    // 停止调试
    bk_h264_encode_ioctl(doorbell_enc_handler, BK_H264_ENCODE_IOCTL_DEBUG_STOP, NULL);

    // 关闭编码器
    bk_h264_encode_close(doorbell_enc_handler);
    
    // 去初始化
    bk_h264_encode_deinit(doorbell_enc_handler);
    
    // 删除编码器
    bk_h264_encode_delete(doorbell_enc_handler);
    doorbell_enc_handler = NULL;

    bk_encoded_data_manager_deinit(1);
    return BK_OK;
}

int app_h264e_turn_on(void)
{
    bk_err_t ret = BK_OK;

    bk_encoded_data_manager_init();

    void *isp_handle = app_isp_handle_get();

    isp_control_t *isp_control = (isp_control_t *)isp_handle;
    uint8_t chnl_id = ISP_MP_CHN_ID;

    const uint32_t enc_aligned_height =
        ((uint32_t)isp_control->chn[chnl_id].chn_attr.chnFormat.height + 15U) & ~15U;
    const uint32_t input_buffer_size = app_codec_isp_channel_buffer_size(&isp_control->chn[chnl_id]);
    if (input_buffer_size == 0)
    {
        LOGE("Invalid H.264 encoder input buffer size\r\n");
        return BK_FAIL;
    }

    // 配置H.264编码器
    bk_h264_encode_hw_flexa_config_t config = {
        .width = isp_control->chn[chnl_id].chn_attr.chnFormat.width,
        .height = isp_control->chn[chnl_id].chn_attr.chnFormat.height,
        .input_format = BK_PIXEL_FORMAT_NV12,
        .gop_frame_count = 30,
        .input_flexa_cnt = 3,
        .input_buf = isp_control->chn[chnl_id].y_addr,
        .input_size = input_buffer_size,
        .outbuf_malloc = encoder_buffer_request,
        .outbuf_malloc_args = NULL,
        .outbuf_complete = encoder_buffer_complete,
        .outbuf_complete_args = NULL,
    };

    // 创建编码器
    ret = bk_h264_encode_hw_flexa_new(&doorbell_enc_handler, &config);
    if (ret != BK_OK) {
        LOGE("Create H.264 encoder failed: %d\r\n", ret);
        return ret;
    }

    // 初始化编码器
    ret = bk_h264_encode_init(doorbell_enc_handler);
    if (ret != BK_OK) {
        LOGE("Init H.264 encoder failed: %d\r\n", ret);
        bk_h264_encode_delete(doorbell_enc_handler);
        doorbell_enc_handler = NULL;
        return ret;
    }

    // 打开编码器
    ret = bk_h264_encode_open(doorbell_enc_handler);
    if (ret != BK_OK) {
        LOGE("Open H.264 encoder failed: %d\r\n", ret);
        bk_h264_encode_deinit(doorbell_enc_handler);
        bk_h264_encode_delete(doorbell_enc_handler);
        doorbell_enc_handler = NULL;
        return ret;
    }

    // 启动调试 (2秒间隔)
#ifndef DUMP_ENCODE_DATA_ENABLE
    uint32_t debug_interval = 2000;
    bk_h264_encode_ioctl(doorbell_enc_handler, BK_H264_ENCODE_IOCTL_DEBUG_START, &debug_interval);
#endif
    return ret;
}

void *app_h264_encode_handle_get(void)
{
    return doorbell_enc_handler;
}

int app_jpege_turn_off(void)
{
    if (doorbell_jpeg_enc_handler == NULL) {
        return BK_OK;
    }

    bk_jpeg_encode_close(doorbell_jpeg_enc_handler);
    bk_jpeg_encode_deinit(doorbell_jpeg_enc_handler);
    bk_jpeg_encode_delete(doorbell_jpeg_enc_handler);
    doorbell_jpeg_enc_handler = NULL;

    bk_encoded_data_manager_deinit(1);
    return BK_OK;
}

int app_jpege_turn_on(void)
{
    bk_err_t ret = BK_OK;

    bk_encoded_data_manager_init();

    void *isp_handle = app_isp_handle_get();
    isp_control_t *isp_control = (isp_control_t *)isp_handle;
    uint8_t chnl_id = ISP_MP_CHN_ID;
    const uint32_t input_flexa_blocks = isp_control->chn[chnl_id].buf_cnt;
    if (input_flexa_blocks == 0)
    {
        LOGE("Invalid JPEG encoder input flexa blocks\r\n");
        return BK_FAIL;
    }

    bk_jpeg_encode_hw_flexa_config_t config = {
        .width = isp_control->chn[chnl_id].chn_attr.chnFormat.width,
        .height = isp_control->chn[chnl_id].chn_attr.chnFormat.height,
        .input_format = BK_PIXEL_FORMAT_NV12,
        .input_flexa_cnt = input_flexa_blocks,
        .input_buf = isp_control->chn[chnl_id].y_addr,
        .input_size = input_flexa_blocks,
        .quality = 2,
        .outbuf_malloc = encoder_buffer_request,
        .outbuf_malloc_args = NULL,
        .outbuf_complete = jpeg_encoder_buffer_complete,
        .outbuf_complete_args = NULL,
    };

    ret = bk_jpeg_encode_hw_flexa_new(&doorbell_jpeg_enc_handler, &config);
    if (ret != BK_OK) {
        LOGE("Create JPEG encoder failed: %d\r\n", ret);
        return ret;
    }

    ret = bk_jpeg_encode_init(doorbell_jpeg_enc_handler);
    if (ret != BK_OK) {
        LOGE("Init JPEG encoder failed: %d\r\n", ret);
        bk_jpeg_encode_delete(doorbell_jpeg_enc_handler);
        doorbell_jpeg_enc_handler = NULL;
        return ret;
    }

    ret = bk_jpeg_encode_open(doorbell_jpeg_enc_handler);
    if (ret != BK_OK) {
        LOGE("Open JPEG encoder failed: %d\r\n", ret);
        bk_jpeg_encode_deinit(doorbell_jpeg_enc_handler);
        bk_jpeg_encode_delete(doorbell_jpeg_enc_handler);
        doorbell_jpeg_enc_handler = NULL;
        return ret;
    }

    return ret;
}

void *app_jpeg_encode_handle_get(void)
{
    return doorbell_jpeg_enc_handler;
}
