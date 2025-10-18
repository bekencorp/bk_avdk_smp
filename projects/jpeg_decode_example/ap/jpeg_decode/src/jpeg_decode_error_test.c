#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include "jpeg_data.h"
#include <media_service.h>
#include "frame_buffer.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "jpeg_decode_test.h"

#define TAG "jdec_error"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static void *jpeg_decode_handle = NULL;
static bk_jpeg_decode_hw_config_t jpeg_decode_hw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    }
};

static bk_jpeg_decode_sw_config_t jpeg_decode_sw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    }
};

static jpeg_decode_test_type_t test_type = JPEG_DECODE_MODE_HARDWARE;
static void *config = &jpeg_decode_hw_config;

void cli_jpeg_decode_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_FAIL;
    frame_buffer_t *in_frame = NULL;
    frame_buffer_t *out_frame = NULL;
    char *msg = NULL;

    if (os_strcmp(argv[1], "input_buffer_null_1") == 0) {
        // Create and open decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }
        in_frame = frame_buffer_encode_malloc(10);
        if (in_frame == NULL) {
            LOGE("%s, %d, Failed to malloc input frame buffer!\n", __func__, __LINE__);
            ret = BK_FAIL;
            goto exit;
        }
        out_frame = frame_buffer_display_malloc(100 * 100 * 2);
        if (out_frame == NULL) {
            LOGE("%s, %d, Failed to malloc output frame buffer!\n", __func__, __LINE__);
            ret = BK_FAIL;
            goto exit;
        }

        /* Test image decoding under abnormal scenarios */
        LOGI("%s, %d, Start abnormal scenario JPEG decoding test!\n", __func__, __LINE__);

        // Test case 1: jpeg_decode_handle is NULL pointer input
        LOGD("%s, %d, Test case 1: jpeg_decode_handle is NULL pointer input\n", __func__, __LINE__);
        ret = bk_jpeg_decode_hw_decode(NULL, in_frame, out_frame);
        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for jpeg_decode_handle is NULL pointer test! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for jpeg_decode_handle is NULL pointer test!\n", __func__, __LINE__);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }
    else if (os_strcmp(argv[1], "input_buffer_null_2") == 0) {
        // Create and open decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        /* Test image decoding under abnormal scenarios */
        LOGI("%s, %d, Start abnormal scenario JPEG decoding test!\n", __func__, __LINE__);

        out_frame = frame_buffer_display_malloc(100 * 100 * 2);
        if (out_frame == NULL) {
            LOGE("%s, %d, Failed to malloc output frame buffer!\n", __func__, __LINE__);
            ret = BK_FAIL;
            goto exit;
        }
        // Test case 2: input_frame is NULL pointer input
        LOGD("%s, %d, Test case 2: input_frame is NULL pointer input\n", __func__, __LINE__);
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, NULL, out_frame);
        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for input_frame is  NULL pointer test! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for input_frame is NULL pointer test!\n", __func__, __LINE__);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }
    else if (os_strcmp(argv[1], "input_buffer_null_3") == 0) {
        // Create and open decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        /* Test image decoding under abnormal scenarios */
        LOGI("%s, %d, Start abnormal scenario JPEG decoding test!\n", __func__, __LINE__);
        in_frame = frame_buffer_encode_malloc(10);
        if (in_frame == NULL) {
            LOGE("%s, %d, Failed to malloc input frame buffer!\n", __func__, __LINE__);
            ret = BK_FAIL;
            goto exit;
        }

        // Test case 3: out_frame is NULL pointer input
        LOGD("%s, %d, Test case 3: out_frame is NULL pointer input\n", __func__, __LINE__);
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, in_frame, NULL);
        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for out_frame is NULL pointer test! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for out_frame is NULL pointer test!\n", __func__, __LINE__);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }
    else if (os_strcmp(argv[1], "invalid_input_data") == 0) {
        // Create and open decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Test case 4: Invalid JPEG data
        LOGD("%s, %d, Test case 4: Invalid JPEG data\n", __func__, __LINE__);

        // Create a small buffer with invalid JPEG data
        frame_buffer_t *invalid_frame = frame_buffer_encode_malloc(10);
        if (invalid_frame != NULL) {
            invalid_frame->length = 10;
            invalid_frame->size = 10;
            // Fill with some random data, not valid JPEG data
            os_memset(invalid_frame->frame, 0xAA, 10);

            frame_buffer_t *out_frame = frame_buffer_display_malloc(100 * 100 * 2);
            if (out_frame != NULL) {
                out_frame->width = 100;
                out_frame->height = 100;
                out_frame->fmt = PIXEL_FMT_YUYV;

                ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, invalid_frame, out_frame);
                if (ret != BK_OK) {
                    LOGD("%s, %d, Expected failure for invalid JPEG data test! ret: %d\n", __func__, __LINE__, ret);
                } else {
                    LOGE("%s, %d, Unexpected success for invalid JPEG data test!\n", __func__, __LINE__);
                }
            }
            else
            {
                frame_buffer_encode_free(invalid_frame);
            }
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }
    else if (os_strcmp(argv[1], "output_buffer_small") == 0) {
        // Create and open decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Test case 5: Output buffer too small
        LOGD("%s, %d, Test case 5: Output buffer too small\n", __func__, __LINE__);

        // Use valid JPEG data
        uint32_t jpeg_length = jpeg_length_422_864_480;
        const uint8_t *jpeg_data = jpeg_data_422_864_480;

        frame_buffer_t *in_frame = frame_buffer_encode_malloc(jpeg_length);
        if (in_frame != NULL) {
            in_frame->length = jpeg_length;
            in_frame->size = jpeg_length;
            os_memcpy(in_frame->frame, jpeg_data, jpeg_length);

            // Allocate a very small output buffer
            frame_buffer_t *small_out_frame = frame_buffer_display_malloc(10);
            if (small_out_frame != NULL) {
                small_out_frame->width = 864;
                small_out_frame->height = 480;
                small_out_frame->fmt = PIXEL_FMT_YUYV;

                ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, in_frame, small_out_frame);
                if (ret != BK_OK) {
                    LOGD("%s, %d, Expected failure for small output buffer test! ret: %d\n", __func__, __LINE__, ret);
                } else {
                    LOGE("%s, %d, Unexpected success for small output buffer test!\n", __func__, __LINE__);
                }
            }
            else
            {
                frame_buffer_encode_free(in_frame);
            }
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);

    }
    else if (os_strcmp(argv[1], "hardware_decode_error_1") == 0) {
        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        uint32_t jpeg_length = jpeg_length_420_864_480;
        const uint8_t *jpeg_data = jpeg_data_420_864_480;
        bk_jpeg_decode_img_info_t img_info = {0};

        // Test case 6: Hardware decode 420 864_480_jpeg
        LOGD("%s, %d, Test case 6: Hardware decode 420 864_480_jpeg\n", __func__, __LINE__);

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
        ret = bk_jpeg_decode_hw_get_img_info(jpeg_decode_handle, &img_info);
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

        // 4. Perform decoding and measure time
        uint32_t start_time = 0, end_time = 0;
        beken_time_get_time(&start_time);
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, in_frame, out_frame);
        beken_time_get_time(&end_time);

        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for hardware decode yuv420 860_480_jpeg! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for hardware decode yuv420 860_480_jpeg time %d!\n", __func__, __LINE__, end_time - start_time);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "hardware_decode_error_2") == 0) {
        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        uint32_t jpeg_length = jpeg_length_422_865_480;
        const uint8_t *jpeg_data = jpeg_data_422_865_480;
        bk_jpeg_decode_img_info_t img_info = {0};

        // Test case 7: Hardware decode 422 865_480_jpeg
        LOGD("%s, %d, Test case 7: Hardware decode 422 865_480_jpeg\n", __func__, __LINE__);

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
        ret = bk_jpeg_decode_hw_get_img_info(jpeg_decode_handle, &img_info);
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

        // 4. Perform decoding and measure time
        uint32_t start_time = 0, end_time = 0;
        beken_time_get_time(&start_time);
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, in_frame, out_frame);
        beken_time_get_time(&end_time);

        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for hardware decode yuv420 860_480_jpeg! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for hardware decode yuv420 860_480_jpeg time %d!\n", __func__, __LINE__, end_time - start_time);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "hardware_decode_error_3") == 0) {
        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        uint32_t jpeg_length = jpeg_length_422_864_479;
        const uint8_t *jpeg_data = jpeg_data_422_864_479;
        bk_jpeg_decode_img_info_t img_info = {0};

        // Test case 8: Hardware decode 422 864_479_jpeg
        LOGD("%s, %d, Test case 8: Hardware decode 422 864_479_jpeg\n", __func__, __LINE__);

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
        ret = bk_jpeg_decode_hw_get_img_info(jpeg_decode_handle, &img_info);
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

        // 4. Perform decoding and measure time
        uint32_t start_time = 0, end_time = 0;
        beken_time_get_time(&start_time);
        ret = bk_jpeg_decode_hw_decode(jpeg_decode_handle, in_frame, out_frame);
        beken_time_get_time(&end_time);

        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for hardware decode yuv420 860_480_jpeg! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for hardware decode yuv420 860_480_jpeg time %d!\n", __func__, __LINE__, end_time - start_time);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_async_decode_error_1") == 0) {
        // Create and open software decoder for async error test
        test_type = JPEG_DECODE_MODE_SOFTWARE;
        config = &jpeg_decode_sw_config;
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        /* Test image decoding under abnormal scenarios with software async decode */
        LOGI("%s, %d, Start abnormal scenario Software Async JPEG decoding test!\n", __func__, __LINE__);

        // Test case 9: Invalid JPEG data with software async decode
        LOGD("%s, %d, Test case 9: Invalid JPEG data with software async decode\n", __func__, __LINE__);

        // Create a small buffer with invalid JPEG data
        frame_buffer_t *invalid_frame = frame_buffer_encode_malloc(10);
        if (invalid_frame != NULL) {
            invalid_frame->length = 10;
            invalid_frame->size = 10;
            // Fill with some random data, not valid JPEG data
            os_memset(invalid_frame->frame, 0xAA, 10);

            // For async decode, the output buffer is managed by the callback
            ret = bk_jpeg_decode_sw_decode_async(jpeg_decode_handle, invalid_frame);
            // Wait for async operation to complete
            rtos_delay_milliseconds(500);
            if (ret != BK_OK) {
                LOGD("%s, %d, Expected failure for invalid JPEG data with software async decode! ret: %d\n", __func__, __LINE__, ret);
            } else {
                LOGE("%s, %d, Unexpected success for invalid JPEG data with software async decode!\n", __func__, __LINE__);
            }
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_async_decode_error_1") == 0) {
        // Create and open software dtcm cp1 decoder for async error test
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1;
        config = &jpeg_decode_sw_config;
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        /* Test image decoding under abnormal scenarios with software dtcm cp1 async decode */
        LOGI("%s, %d, Start abnormal scenario Software DTCM CP1 Async JPEG decoding test!\n", __func__, __LINE__);

        // Test case 10: Invalid parameters with software dtcm cp1 async decode
        LOGD("%s, %d, Test case 10: Invalid parameters with software dtcm cp1 async decode\n", __func__, __LINE__);

        // Test NULL input frame
        ret = bk_jpeg_decode_sw_decode_async(jpeg_decode_handle, NULL);
        if (ret != BK_OK) {
            LOGD("%s, %d, Expected failure for NULL input frame with software dtcm cp1 async decode! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGE("%s, %d, Unexpected success for NULL input frame with software dtcm cp1 async decode!\n", __func__, __LINE__);
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp2_async_decode_error_1") == 0) {
        // Create and open software dtcm cp2 decoder for async error test
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2;
        config = &jpeg_decode_sw_config;
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        /* Test image decoding under abnormal scenarios with software dtcm cp2 async decode */
        LOGI("%s, %d, Start abnormal scenario Software DTCM CP2 Async JPEG decoding test!\n", __func__, __LINE__);

        // Test case 11: Invalid input frame with software dtcm cp2 async decode
        LOGD("%s, %d, Test case 11: Invalid input frame with software dtcm cp2 async decode\n", __func__, __LINE__);

        // Create a frame buffer with NULL data pointer
        in_frame = frame_buffer_encode_malloc(100);
        if (in_frame != NULL) {
            // Set frame pointer to NULL to test error handling
            in_frame->frame = NULL;
            in_frame->length = 100;
            in_frame->size = 100;

            ret = bk_jpeg_decode_sw_decode_async(jpeg_decode_handle, in_frame);
            // Wait for async operation to complete
            rtos_delay_milliseconds(500);
            if (ret != BK_OK) {
                LOGD("%s, %d, Expected failure for NULL frame data with software dtcm cp2 async decode! ret: %d\n", __func__, __LINE__, ret);
            } else {
                LOGE("%s, %d, Unexpected success for NULL frame data with software dtcm cp2 async decode!\n", __func__, __LINE__);
            }
        }

        // Close and delete decoder
        ret = close_and_delete_decoder(&jpeg_decode_handle, test_type);
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

    return ;
exit:
    if (in_frame != NULL) {
        frame_buffer_encode_free(in_frame);
    }
    if (out_frame != NULL) {
        frame_buffer_display_free(out_frame);
    }
    if (jpeg_decode_handle != NULL) {
        close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }

    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }
    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


