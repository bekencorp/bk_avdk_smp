#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <media_service.h>
#include <components/bk_frame_buffer.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264_encode_test.h"

#define TAG "h264_enc_regular"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_h264_encode_ctlr_handle_t h264_encode_handle = NULL;
static bk_h264_encode_frame_config_t h264_encode_config = {
    .buffer_request_cb = h264_encode_buffer_request_cb,
    .buffer_complete_cb = h264_encode_buffer_complete_cb,
    .chnl_id = 0,
    .param = NULL,
};

// Regular test
void cli_h264_encode_regular_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;

    if (argc < 2) {
        LOGE("%s, %d, param error!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }

    if (os_strcmp(argv[1], "normal_test") == 0) {
        // Create and open encoder
        ret = create_and_open_encoder((void **)&h264_encode_handle, &h264_encode_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, create_and_open_encoder failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Perform normal scenario encoding test
        ret = perform_h264_encode_test(h264_encode_handle, "normal_test");
        if (ret != BK_OK) {
            LOGE("%s, %d, h264 encode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Close and delete encoder
        close_and_delete_encoder((void **)&h264_encode_handle);

        LOGI("%s, %d, H264 encode normal scenario test completed!\n", __func__, __LINE__);
    }
    else if (os_strcmp(argv[1], "async_test") == 0) {
        // Create and open encoder
        ret = create_and_open_encoder((void **)&h264_encode_handle, &h264_encode_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, create_and_open_encoder failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Perform asynchronous encoding test
        ret = perform_h264_encode_async_test(h264_encode_handle, "async_test");
        if (ret != BK_OK) {
            LOGE("%s, %d, h264 async encode test failed! ret: %d\n", __func__, __LINE__, ret);
            goto exit;
        }

        // Wait a bit for async encoding to complete
        rtos_delay_milliseconds(100);

        // Close and delete encoder
        close_and_delete_encoder((void **)&h264_encode_handle);

        LOGI("%s, %d, H264 encode async test completed!\n", __func__, __LINE__);
    }
    else {
        LOGE("%s, %d, not found this test type!\n", __func__, __LINE__);
        ret = BK_FAIL;
    }

exit:
    {
        char *msg = NULL;
        if (ret != BK_OK) {
            msg = CLI_CMD_RSP_ERROR;
        } else {
            msg = CLI_CMD_RSP_SUCCEED;
        }

        LOGI("%s ---complete\n", __func__);
        os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    }
}

