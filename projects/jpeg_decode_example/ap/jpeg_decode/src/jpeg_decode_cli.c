#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include "jpeg_data.h"
#include <media_service.h>
#include "frame_buffer.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "jpeg_decode_test.h"

#define TAG "jdec_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_jpeg_decode_sw_handle_t jpeg_decode_sw_handle = NULL;
static bk_jpeg_decode_sw_config_t jpeg_decode_sw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    }
};

static bk_jpeg_decode_hw_handle_t jpeg_decode_hw_handle = NULL;
static bk_jpeg_decode_hw_config_t jpeg_decode_hw_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    }
};

static bk_jpeg_decode_hw_opt_config_t jpeg_decode_hw_opt_config = {
    .decode_cbs = {
        .in_complete = jpeg_decode_in_complete,
        .out_malloc = jpeg_decode_out_malloc,
        .out_complete = jpeg_decode_out_complete,
    },
    .sram_buffer = NULL,
    .image_max_width = 864,
    .is_pingpong = 0,
    .lines_per_block = 16,  // Using plain number instead of enum
    .copy_method = 0,       // Using plain number instead of enum
};

static jpeg_decode_test_type_t jpeg_decode_mode = JPEG_DECODE_MODE_HARDWARE;

void cli_jpeg_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
    bk_err_t ret = BK_OK;

    if (os_strcmp(argv[1], "init_hw") == 0) {
        ret = bk_hardware_jpeg_decode_new(&jpeg_decode_hw_handle, &jpeg_decode_hw_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_hardware_jpeg_decode_new failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_hardware_jpeg_decode_new success!\n", __func__, __LINE__);
        }
        jpeg_decode_mode = JPEG_DECODE_MODE_HARDWARE;
    }
    else if (os_strcmp(argv[1], "init_sw") == 0) {
        ret = bk_software_jpeg_decode_new(&jpeg_decode_sw_handle, &jpeg_decode_sw_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_software_jpeg_decode_new failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_software_jpeg_decode_new success!\n", __func__, __LINE__);
        }
        jpeg_decode_mode = JPEG_DECODE_MODE_SOFTWARE;
    }
    else if (os_strcmp(argv[1], "init_sw_on_dtcm") == 0) {
        if (argc > 2)
        {
            if (os_strcmp(argv[2], "1") == 0)
            {
                jpeg_decode_sw_config.core_id = JPEG_DECODE_CORE_ID_1;
            }
            else if (os_strcmp(argv[2], "2") == 0)
            {
                jpeg_decode_sw_config.core_id = JPEG_DECODE_CORE_ID_2;
            }
            else
            {
                LOGE("%s, %d, param error!\n", __func__, __LINE__);
                return;
            }
        }
        ret = bk_software_jpeg_decode_on_multi_core_new(&jpeg_decode_sw_handle, &jpeg_decode_sw_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_software_jpeg_decode_on_multi_core_new failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_software_jpeg_decode_on_multi_core_new success!\n", __func__, __LINE__);
        }
        jpeg_decode_mode = JPEG_DECODE_MODE_SOFTWARE;
    }
    else if (os_strcmp(argv[1], "init_hw_line") == 0) {
        ret = bk_hardware_jpeg_decode_opt_new(&jpeg_decode_hw_handle, &jpeg_decode_hw_opt_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, bk_hardware_jpeg_decode_opt_new failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, bk_hardware_jpeg_decode_opt_new success!\n", __func__, __LINE__);
        }
        jpeg_decode_mode = JPEG_DECODE_MODE_HARDWARE;
    }
    else if (os_strcmp(argv[1], "delete") == 0) {
        if(jpeg_decode_mode == JPEG_DECODE_MODE_HARDWARE)
        {
            ret = bk_jpeg_decode_hw_delete(jpeg_decode_hw_handle);
        }
        else
        {
            ret = bk_jpeg_decode_sw_delete(jpeg_decode_sw_handle);
        }
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode delete failed! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGD("%s, %d, jpeg decode delete success!\n", __func__, __LINE__);
            if(jpeg_decode_mode == JPEG_DECODE_MODE_HARDWARE)
            {
                jpeg_decode_hw_handle = NULL;
            }
            else
            {
                jpeg_decode_sw_handle = NULL;
            }
        }
    }
    else if (os_strcmp(argv[1], "open") == 0) {
        if(jpeg_decode_mode == JPEG_DECODE_MODE_HARDWARE)
        {
            ret = bk_jpeg_decode_hw_open(jpeg_decode_hw_handle);
        }
        else
        {
            ret = bk_jpeg_decode_sw_open(jpeg_decode_sw_handle);
        }
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode open failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, jpeg decode open success!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "close") == 0) {
        if (jpeg_decode_mode == JPEG_DECODE_MODE_HARDWARE)
        {
            if(jpeg_decode_hw_handle != NULL)
            {
                ret = bk_jpeg_decode_hw_close(jpeg_decode_hw_handle);
            }
        }
        else
        {
            if(jpeg_decode_sw_handle != NULL)
            {
                ret = bk_jpeg_decode_sw_close(jpeg_decode_sw_handle);
            }
        }
        if (ret != BK_OK) {
            LOGE("%s, %d, jpeg decode close failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, jpeg decode close success!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "dec") == 0) {
        if (argc < 3) {
            LOGE("%s, %d, param error!\n", __func__, __LINE__);
            return;
        }

        uint32_t jpeg_length = 0;
        const uint8_t *jpeg_data = NULL;

        if (os_strcmp(argv[2], "422_864_480") == 0) {
            jpeg_length = jpeg_length_422_864_480;
            jpeg_data = jpeg_data_422_864_480;
        }
        else if (os_strcmp(argv[2], "420_864_480") == 0) {
            jpeg_length = jpeg_length_420_864_480;
            jpeg_data = jpeg_data_420_864_480;
        }

        else if (os_strcmp(argv[2], "422_865_480") == 0) {
            jpeg_length = jpeg_length_422_865_480;
            jpeg_data = jpeg_data_422_865_480;
        }
        else if (os_strcmp(argv[2], "422_864_479") == 0) {
            jpeg_length = jpeg_length_422_864_479;
            jpeg_data = jpeg_data_422_864_479;
        }
        else if (os_strcmp(argv[2], "420_865_480") == 0) {
            jpeg_length = jpeg_length_420_865_480;
            jpeg_data = jpeg_data_420_865_480;
        }
        else if (os_strcmp(argv[2], "420_864_479") == 0) {
            jpeg_length = jpeg_length_420_864_479;
            jpeg_data = jpeg_data_420_864_479;
        }

        if(jpeg_decode_mode == JPEG_DECODE_MODE_HARDWARE)
        {
            ret = perform_jpeg_decode_test(jpeg_decode_hw_handle, jpeg_length, jpeg_data, "manual", jpeg_decode_mode);
        }
        else
        {
            ret = perform_jpeg_decode_test(jpeg_decode_sw_handle, jpeg_length, jpeg_data, "manual", jpeg_decode_mode);
        }
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
    }

    char *msg = NULL;
    if (ret != BK_OK) {
        msg = CLI_CMD_RSP_ERROR;
    } else {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
#endif
}