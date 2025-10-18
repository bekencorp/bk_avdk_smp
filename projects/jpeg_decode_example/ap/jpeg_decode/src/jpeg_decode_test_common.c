#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include "jpeg_data.h"
#include <media_service.h>
#include "frame_buffer.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "jpeg_decode_test.h"

#define TAG "jdec_common"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

bk_err_t jpeg_decode_in_complete(frame_buffer_t *in_frame)
{
    frame_buffer_encode_free(in_frame);
    return BK_OK;
}

frame_buffer_t *jpeg_decode_out_malloc(uint32_t size)
{
    return frame_buffer_display_malloc(size);
}

bk_err_t jpeg_decode_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    if (result == BK_OK)
    {
        LOGD("%s, %d, jpeg decode success! format_type: %d, out_frame: %p\n", __func__, __LINE__, format_type, out_frame);
    }
    else
    {
        LOGE("%s, %d, jpeg decode failed! format_type: %d, result: %d, out_frame: %p\n", __func__, __LINE__, format_type, result, out_frame);
    }
    frame_buffer_display_free(out_frame);

    return BK_OK;
}

// Helper function: Create and open decoder
bk_err_t create_and_open_decoder(void **jpeg_decode_handle, void *jpeg_decode_config, jpeg_decode_test_type_t jpeg_decode_test_type)
{
    bk_err_t ret = BK_OK;

    // Ensure decoder is released
    if (*jpeg_decode_handle != NULL) {
        close_and_delete_decoder(jpeg_decode_handle, jpeg_decode_test_type);
        *jpeg_decode_handle = NULL;
    }

    // Create decoder
    if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
        ret = bk_hardware_jpeg_decode_new((bk_jpeg_decode_hw_handle_t *)jpeg_decode_handle, (bk_jpeg_decode_hw_config_t *)jpeg_decode_config);
    } else if (jpeg_decode_test_type == JPEG_DECODE_MODE_SOFTWARE) {
        ret = bk_software_jpeg_decode_new((bk_jpeg_decode_sw_handle_t *)jpeg_decode_handle, (bk_jpeg_decode_sw_config_t *)jpeg_decode_config);
    } else if (jpeg_decode_test_type == JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1) {
        ((bk_jpeg_decode_sw_config_t *)jpeg_decode_config)->core_id = JPEG_DECODE_CORE_ID_1;
        ret = bk_software_jpeg_decode_on_multi_core_new((bk_jpeg_decode_sw_handle_t *)jpeg_decode_handle, (bk_jpeg_decode_sw_config_t *)jpeg_decode_config);
    } else if (jpeg_decode_test_type == JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2) {
        ((bk_jpeg_decode_sw_config_t *)jpeg_decode_config)->core_id = JPEG_DECODE_CORE_ID_2;
        ret = bk_software_jpeg_decode_on_multi_core_new((bk_jpeg_decode_sw_handle_t *)jpeg_decode_handle, (bk_jpeg_decode_sw_config_t *)jpeg_decode_config);
    }

    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode new failed! ret: %d\n", __func__, __LINE__, ret);
        goto exit;
    }
    LOGD("%s, %d, jpeg decode new success!\n", __func__, __LINE__);

    // Open decoder
    if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
        ret = bk_jpeg_decode_hw_open((bk_jpeg_decode_hw_handle_t)*jpeg_decode_handle);
    } else {
        ret = bk_jpeg_decode_sw_open((bk_jpeg_decode_sw_handle_t)*jpeg_decode_handle);
    }

    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode open failed! ret: %d\n", __func__, __LINE__, ret);
        goto cleanup_jpeg_handle;
    }

    LOGD("%s, %d, jpeg decode open success!\n", __func__, __LINE__);

    return ret;

cleanup_jpeg_handle:
    if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
        bk_jpeg_decode_hw_delete((bk_jpeg_decode_hw_handle_t)*jpeg_decode_handle);
    } else {
        bk_jpeg_decode_sw_delete((bk_jpeg_decode_sw_handle_t)*jpeg_decode_handle);
    }
    *jpeg_decode_handle = NULL;

exit:
    return ret;
}

// Helper function: Close and delete decoder
bk_err_t close_and_delete_decoder(void **jpeg_decode_handle, jpeg_decode_test_type_t jpeg_decode_test_type)
{
    bk_err_t ret = BK_OK;

    if (*jpeg_decode_handle != NULL) {
        if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
            ret = bk_jpeg_decode_hw_close((bk_jpeg_decode_hw_handle_t)*jpeg_decode_handle);
        } else {
            ret = bk_jpeg_decode_sw_close((bk_jpeg_decode_sw_handle_t)*jpeg_decode_handle);
        }

        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode close failed! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGD("%s, %d, jpeg decode close success!\n", __func__, __LINE__);
        }

        if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
            ret = bk_jpeg_decode_hw_delete((bk_jpeg_decode_hw_handle_t)*jpeg_decode_handle);
        } else {
            ret = bk_jpeg_decode_sw_delete((bk_jpeg_decode_sw_handle_t)*jpeg_decode_handle);
        }

        *jpeg_decode_handle = NULL;
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode delete failed! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGD("%s, %d, jpeg decode delete success!\n", __func__, __LINE__);
        }
    }

    return ret;
}

// Helper function: Perform JPEG decoding test
bk_err_t perform_jpeg_decode_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type)
{
    bk_err_t ret = BK_OK;
    frame_buffer_t *in_frame = NULL;
    frame_buffer_t *out_frame = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};

    LOGI("%s, %d, Start %s!\n", __func__, __LINE__, test_name);

    // 1. Allocate and fill input buffer
    in_frame = frame_buffer_encode_malloc(jpeg_length);
    if (in_frame == NULL) {
        LOGE("%s, %d, frame_buffer_encode_malloc failed!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }
    in_frame->length = jpeg_length;
    in_frame->size = jpeg_length;
    os_memcpy(in_frame->frame, jpeg_data, jpeg_length);

    // 2. Get image dimensions information
    img_info.frame = in_frame;
    if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
        ret = bk_jpeg_decode_hw_get_img_info((bk_jpeg_decode_hw_handle_t)jpeg_decode_handle, &img_info);
    } else {
        ret = bk_jpeg_decode_sw_get_img_info((bk_jpeg_decode_sw_handle_t)jpeg_decode_handle, &img_info);
    }

    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode get img info failed! ret: %d\n", __func__, __LINE__, ret);
        goto exit;
    }
    LOGD("%s, %d, jpeg decode get img info success! %dx%d %d\n", __func__, __LINE__,
            img_info.width, img_info.height, img_info.format);

    // 3. Allocate output buffer
    out_frame = frame_buffer_display_malloc(img_info.width * img_info.height * 2);
    if (out_frame == NULL) {
        LOGE("%s, %d, frame_buffer_display_malloc failed!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }
    out_frame->width = img_info.width;
    out_frame->height = img_info.height;
    out_frame->fmt = PIXEL_FMT_YUYV;

    // For software decoder, can set rotation angle, output format and byte order
    if (jpeg_decode_test_type != JPEG_DECODE_MODE_HARDWARE) {

        // Set output format
        bk_jpeg_decode_sw_out_frame_info_t jpeg_decode_sw_config = {0};
        jpeg_decode_sw_config.out_format = JPEG_DECODE_SW_OUT_FORMAT_YUYV;
        jpeg_decode_sw_config.byte_order = JPEG_DECODE_LITTLE_ENDIAN;
        bk_jpeg_decode_sw_set_config((bk_jpeg_decode_sw_handle_t)jpeg_decode_handle, &jpeg_decode_sw_config);
    }

    // 4. Perform decoding and measure time
    uint32_t start_time = 0, end_time = 0;
    beken_time_get_time(&start_time);
    if (jpeg_decode_test_type == JPEG_DECODE_MODE_HARDWARE) {
        ret = bk_jpeg_decode_hw_decode((bk_jpeg_decode_hw_handle_t)jpeg_decode_handle, in_frame, out_frame);
    } else {
        ret = bk_jpeg_decode_sw_decode((bk_jpeg_decode_sw_handle_t)jpeg_decode_handle, in_frame, out_frame);
    }

    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode start failed! ret: %d\n", __func__, __LINE__, ret);
        goto exit;
    }

    beken_time_get_time(&end_time);

    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode start failed! ret: %d\n", __func__, __LINE__, ret);
    } else {
        LOGD("%s, %d, jpeg decode start success! Decode time: %d ms\n", __func__, __LINE__, end_time - start_time);
    }
    return ret;

    // 5. Release resources
exit:
    if (in_frame != NULL) {
        frame_buffer_encode_free(in_frame);
    }
    if (out_frame != NULL) {
        frame_buffer_display_free(out_frame);
    }

    return ret;
}

// Helper function: Perform JPEG asynchronous decoding test
bk_err_t perform_jpeg_decode_async_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, const char *test_name, jpeg_decode_test_type_t jpeg_decode_test_type)
{
    bk_err_t ret = BK_OK;
    frame_buffer_t *in_frame = NULL;
    bk_jpeg_decode_img_info_t img_info = {0};

    LOGI("%s, %d, Start %s!\n", __func__, __LINE__, test_name);

    // 1. Allocate and fill input buffer
    in_frame = frame_buffer_encode_malloc(jpeg_length);
    if (in_frame == NULL) {
        LOGE("%s, %d, frame_buffer_encode_malloc failed!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }
    in_frame->length = jpeg_length;
    in_frame->size = jpeg_length;
    os_memcpy(in_frame->frame, jpeg_data, jpeg_length);

    // 2. Get image dimensions information
    img_info.frame = in_frame;
    ret = bk_jpeg_decode_hw_get_img_info((bk_jpeg_decode_hw_handle_t)jpeg_decode_handle, &img_info);
    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg decode get img info failed! ret: %d\n", __func__, __LINE__, ret);
        goto exit;
    }
    LOGD("%s, %d, jpeg decode get img info success! %dx%d %d\n", __func__, __LINE__,
            img_info.width, img_info.height, img_info.format);

    // 3. Perform asynchronous decoding
    // For async decoding, the output buffer will be allocated and returned in the callback
    ret = bk_jpeg_decode_hw_decode_async((bk_jpeg_decode_hw_handle_t)jpeg_decode_handle, in_frame);
    if (ret != BK_OK) {
        LOGE("%s, %d, jpeg async decode start failed! ret: %d\n", __func__, __LINE__, ret);
        goto exit;
    }

    LOGD("%s, %d, jpeg async decode complete\n", __func__, __LINE__);
    return ret;

    // 4. Release resources
exit:
    LOGE("%s, %d, jpeg async decode failed! ret: %d\n", __func__, __LINE__, ret);

    if (in_frame != NULL) {
        frame_buffer_encode_free(in_frame);
    }

    return ret;
}

// Helper function: Perform multiple JPEG asynchronous decoding tests (burst mode)
bk_err_t perform_jpeg_decode_async_burst_test(void *jpeg_decode_handle, uint32_t jpeg_length, const uint8_t *jpeg_data, 
                                           const char *test_name, uint32_t burst_count)
{
    bk_err_t ret = BK_OK;
    uint32_t total_time = 0;
    uint32_t i = 0;

    LOGI("%s, %d, Start %s with %d bursts!\n", __func__, __LINE__, test_name, burst_count);

    // Run multiple asynchronous decoding tests in sequence
    for (i = 0; i < burst_count; i++) {
        uint32_t start_time = 0, end_time = 0;
        LOGD("%s, %d, Burst test %d/%d\n", __func__, __LINE__, i+1, burst_count);

        beken_time_get_time(&start_time);
        ret = perform_jpeg_decode_async_test(jpeg_decode_handle, jpeg_length, jpeg_data, test_name, JPEG_DECODE_MODE_HARDWARE);
        beken_time_get_time(&end_time);
        
        if (ret != BK_OK) {
            LOGE("%s, %d, Burst test %d/%d failed! ret: %d\n", __func__, __LINE__, i+1, burst_count, ret);
            break;
        }
        
        total_time += (end_time - start_time);
    }

    if (ret == BK_OK) {
        LOGI("%s, %d, JPEG async burst test completed! Total time: %d ms, Average time: %d ms\n", 
             __func__, __LINE__, total_time, total_time / burst_count);
    }

    return ret;
}