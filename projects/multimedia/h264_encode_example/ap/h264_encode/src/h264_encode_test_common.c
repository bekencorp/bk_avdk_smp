#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <media_service.h>
#include <components/bk_frame_buffer.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264_encode_test.h"

#define TAG "h264_enc_common"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Buffer request callback - allocate output buffer for encoded data
void *h264_encode_outbuf_malloc(uint32_t size, void *args)
{
    (void)args;
    frame_buffer_t *buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size + sizeof(frame_buffer_t));
    if (buffer != NULL) {
        LOGD("%s, %d, Allocated buffer: %p, size: %d\n", __func__, __LINE__, buffer, size);
        buffer->frame = (uint8_t *)((((uint32_t)(buffer + 1) >> 5) + 1) << 5);
    } else {
        LOGE("%s, %d, Failed to allocate buffer, size: %d\n", __func__, __LINE__, size);
    }
    return buffer;
}

// Buffer complete callback - handle encoded frame
uint32_t h264_encode_outbuf_complete(void *buffer, uint32_t result, void *args)
{
    (void)args;
    frame_buffer_t *frame = (frame_buffer_t *)buffer;
    if (result == BK_OK) {
        LOGD("%s, %d, H264 encode success! frame: %p, length: %d, h264_type: %d\n", 
             __func__, __LINE__, frame, frame ? frame->length : 0, frame ? frame->h264_type : 0);
    } else {
        LOGE("%s, %d, H264 encode failed! frame: %p, result: %d\n", 
             __func__, __LINE__, frame, result);
    }

    // Free the buffer after encoding
    if (frame != NULL) {
        bk_frame_buffer_free(frame);
    }

    return BK_OK;
}

// Helper function: Create and open encoder
bk_err_t create_and_open_encoder(void **h264_encode_handle, void *h264_encode_config)
{
    bk_err_t ret = BK_OK;
    avdk_err_t avdk_ret = AVDK_ERR_OK;

    // Ensure encoder is released
    if (*h264_encode_handle != NULL) {
        close_and_delete_encoder(h264_encode_handle);
        *h264_encode_handle = NULL;
    }

    // Create encoder
    avdk_ret = bk_h264_encode_frame_new((bk_h264_encode_ctlr_handle_t *)h264_encode_handle, 
                                   (bk_h264_encode_frame_config_t *)h264_encode_config);
    if (avdk_ret != AVDK_ERR_OK) {
        LOGE("%s, %d, h264 encode new failed! ret: %d\n", __func__, __LINE__, avdk_ret);
        ret = BK_FAIL;
        goto exit;
    }
    LOGD("%s, %d, h264 encode new success!\n", __func__, __LINE__);

    // Initialize encoder
    avdk_ret = bk_h264_encode_init((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
    if (avdk_ret != AVDK_ERR_OK) {
        LOGE("%s, %d, h264 encode init failed! ret: %d\n", __func__, __LINE__, avdk_ret);
        ret = BK_FAIL;
        goto cleanup_encoder;
    }
    LOGD("%s, %d, h264 encode init success!\n", __func__, __LINE__);

    // Open encoder
    avdk_ret = bk_h264_encode_open((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
    if (avdk_ret != AVDK_ERR_OK) {
        LOGE("%s, %d, h264 encode open failed! ret: %d\n", __func__, __LINE__, avdk_ret);
        ret = BK_FAIL;
        goto cleanup_encoder;
    }
    LOGD("%s, %d, h264 encode open success!\n", __func__, __LINE__);

    return ret;

cleanup_encoder:
    bk_h264_encode_delete((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
    *h264_encode_handle = NULL;

exit:
    return ret;
}

// Helper function: Close and delete encoder
bk_err_t close_and_delete_encoder(void **h264_encode_handle)
{
    bk_err_t ret = BK_OK;
    avdk_err_t avdk_ret = AVDK_ERR_OK;

    if (*h264_encode_handle != NULL) {
        // Close encoder
        avdk_ret = bk_h264_encode_close((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
        if (avdk_ret != AVDK_ERR_OK) {
            LOGE("%s, %d, h264 encode close failed! ret: %d\n", __func__, __LINE__, avdk_ret);
            ret = BK_FAIL;
        } else {
            LOGD("%s, %d, h264 encode close success!\n", __func__, __LINE__);
        }

        // Deinitialize encoder
        avdk_ret = bk_h264_encode_deinit((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
        if (avdk_ret != AVDK_ERR_OK) {
            LOGE("%s, %d, h264 encode deinit failed! ret: %d\n", __func__, __LINE__, avdk_ret);
            ret = BK_FAIL;
        } else {
            LOGD("%s, %d, h264 encode deinit success!\n", __func__, __LINE__);
        }

        // Delete encoder
        avdk_ret = bk_h264_encode_delete((bk_h264_encode_ctlr_handle_t)*h264_encode_handle);
        *h264_encode_handle = NULL;
        if (avdk_ret != AVDK_ERR_OK) {
            LOGE("%s, %d, h264 encode delete failed! ret: %d\n", __func__, __LINE__, avdk_ret);
            ret = BK_FAIL;
        } else {
            LOGD("%s, %d, h264 encode delete success!\n", __func__, __LINE__);
        }
    }

    return ret;
}

// Helper function: Perform H264 encoding test
bk_err_t perform_h264_encode_test(void *h264_encode_handle, const char *test_name)
{
    bk_err_t ret = BK_OK;
    avdk_err_t avdk_ret = AVDK_ERR_OK;

    LOGI("%s, %d, Start %s!\n", __func__, __LINE__, test_name);

    // Note: For H264 encoding, the input frame should be provided by the encoder's internal mechanism
    // The encoder will request input frames through its internal pipeline
    // Here we just trigger the encoding process

    // Perform encoding
    uint32_t start_time = 0, end_time = 0;
    beken_time_get_time(&start_time);

    avdk_ret = bk_h264_encode_start((bk_h264_encode_ctlr_handle_t)h264_encode_handle);
    if (avdk_ret != AVDK_ERR_OK) {
        LOGE("%s, %d, h264 encode start failed! ret: %d\n", __func__, __LINE__, avdk_ret);
        ret = BK_FAIL;
        goto exit;
    }

    beken_time_get_time(&end_time);
    LOGD("%s, %d, h264 encode started! Start time: %d ms\n", 
         __func__, __LINE__, end_time - start_time);

    // Note: Encoding is asynchronous, the actual encoding time will be reported in the callback
    // For synchronous test, we may need to wait for the callback

    return ret;

exit:
    return ret;
}

// Helper function: Perform H264 asynchronous encoding test
bk_err_t perform_h264_encode_async_test(void *h264_encode_handle, const char *test_name)
{
    bk_err_t ret = BK_OK;
    avdk_err_t avdk_ret = AVDK_ERR_OK;

    LOGI("%s, %d, Start %s!\n", __func__, __LINE__, test_name);

    // Note: For H264 encoding, the input frame should be provided by the encoder's internal mechanism
    // The encoder will request input frames through its internal pipeline
    // Here we just trigger the encoding process

    // Perform asynchronous encoding
    avdk_ret = bk_h264_encode_start((bk_h264_encode_ctlr_handle_t)h264_encode_handle);
    if (avdk_ret != AVDK_ERR_OK) {
        LOGE("%s, %d, h264 async encode start failed! ret: %d\n", __func__, __LINE__, avdk_ret);
        ret = BK_FAIL;
        goto exit;
    }

    LOGD("%s, %d, h264 async encode started successfully\n", __func__, __LINE__);

    // Note: For async encoding, the input frame will be provided by the encoder's internal mechanism
    // The output buffer will be allocated and returned in the buffer_complete_cb callback

    return ret;

exit:
    return ret;
}

