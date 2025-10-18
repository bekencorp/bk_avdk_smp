#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include "jpeg_data.h"
#include <media_service.h>
#include "frame_buffer.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "jpeg_decode_test.h"

#define TAG "jdec_regular"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static void *jpeg_decode_handle = NULL;
static bk_jpeg_decode_sw_config_t jpeg_decode_sw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    },
    .core_id = 0,
    .out_format = JPEG_DECODE_SW_OUT_FORMAT_YUYV,
    .byte_order = JPEG_DECODE_LITTLE_ENDIAN,
};

static bk_jpeg_decode_hw_config_t jpeg_decode_hw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    }
};

// Regular test
void cli_jpeg_decode_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    jpeg_decode_test_type_t test_type = JPEG_DECODE_MODE_HARDWARE;
    void *config = NULL;

    if (os_strcmp(argv[1], "hardware_test") == 0) {
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;

        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform normal scenario decoding test
        ret = perform_jpeg_decode_test(jpeg_decode_handle, jpeg_length_422_864_480, jpeg_data_422_864_480, "hardware_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, hardware jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "hardware_async_test") == 0) {
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;

        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform async hardware decoding test
        ret = perform_jpeg_decode_async_test(jpeg_decode_handle, jpeg_length_422_864_480, jpeg_data_422_864_480, "hardware_async_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg async decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        //wait decode complete
        rtos_delay_milliseconds(1000);
        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, hardware jpeg async decode test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "hardware_async_burst_test") == 0) {
        test_type = JPEG_DECODE_MODE_HARDWARE;
        config = &jpeg_decode_hw_config;

        // Create and open hardware decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        perform_jpeg_decode_async_burst_test(jpeg_decode_handle, jpeg_length_422_864_480, jpeg_data_422_864_480, "hardware_test", 10);

        //wait all frame decode complete
        rtos_delay_milliseconds(5000);

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, hardware jpeg async decode test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform normal scenario decoding test
        ret = perform_jpeg_decode_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform normal scenario decoding test
        ret = perform_jpeg_decode_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp1_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp2_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform normal scenario decoding test
        ret = perform_jpeg_decode_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp2_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg decode Normal scenario JPEG decoding test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_async_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform async software decoding test on CP1
        ret = perform_jpeg_decode_sw_async_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp1_async_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg async decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Wait decode complete
        rtos_delay_milliseconds(1000);
        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async decode on CP1 test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_async_burst_test") == 0) {
        int burst_count = 10;
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        perform_jpeg_decode_sw_async_burst_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp1_async_burst_test", test_type, burst_count);

        // Wait all frame decode complete
        rtos_delay_milliseconds(burst_count * 400);

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async burst decode on CP1 test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp2_async_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform async software decoding test on CP2
        ret = perform_jpeg_decode_sw_async_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp2_async_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg async decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Wait decode complete
        rtos_delay_milliseconds(1000);
        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async decode on CP2 test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp2_async_burst_test") == 0) {
        int burst_count = 10;
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP2;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        perform_jpeg_decode_sw_async_burst_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp2_async_burst_test", test_type, burst_count);

        // Wait all frame decode complete
        rtos_delay_milliseconds(burst_count * 400);

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async burst decode on CP2 test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_cp2_async_test") == 0) {
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1_CP2;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        // Perform async software decoding test on CP2
        ret = perform_jpeg_decode_sw_async_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp1_cp2_async_test", test_type);
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg async decode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Wait decode complete
        rtos_delay_milliseconds(1000);
        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async decode on CP2 test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "software_dtcm_cp1_cp2_async_burst_test") == 0) {
        int burst_count = 10;
        test_type = JPEG_DECODE_MODE_SOFTWARE_DTCM_CP1_CP2;
        config = &jpeg_decode_sw_config;

        // Create and open software decoder
        ret = create_and_open_decoder(&jpeg_decode_handle, config, test_type);
        if (ret != BK_OK) {
            goto exit;
        }

        perform_jpeg_decode_sw_async_burst_test(jpeg_decode_handle, jpeg_length_420_864_480, jpeg_data_420_864_480, "software_dtcm_cp1_cp2_async_burst_test", test_type, burst_count);

        // Wait all frame decode complete
        rtos_delay_milliseconds(burst_count * 400);

        // Close and delete decoder
        close_and_delete_decoder(&jpeg_decode_handle, test_type);

        LOGI("%s, %d, software jpeg async burst decode on CP2 test completed!\n", __func__, __LINE__);
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }

exit:
    if (jpeg_decode_handle != NULL) {
        close_and_delete_decoder(&jpeg_decode_handle, test_type);
    }

    char *msg = NULL;
    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}