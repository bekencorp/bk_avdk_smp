#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <media_service.h>
#include <components/bk_frame_buffer.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264_encode_test.h"

#define TAG "h264_enc_error"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Error test
void cli_h264_encode_error_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;

    if (argc < 2) {
        LOGE("%s, %d, param error!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }

    if (os_strcmp(argv[1], "null_handle_test") == 0) {
        // Test encoding with NULL handle
        LOGI("%s, %d, Testing encoding with NULL handle\n", __func__, __LINE__);
        ret = perform_h264_encode_test(NULL, "null_handle_test");
        if (ret == BK_FAIL) {
            LOGI("%s, %d, NULL handle test passed (expected failure)\n", __func__, __LINE__);
            ret = BK_OK; // Expected to fail
        } else {
            LOGE("%s, %d, NULL handle test failed (should have failed)\n", __func__, __LINE__);
            ret = BK_FAIL;
        }
    }
    else if (os_strcmp(argv[1], "invalid_config_test") == 0) {
        // Test encoding with invalid config
        LOGI("%s, %d, Testing encoding with invalid config\n", __func__, __LINE__);
        bk_h264_encode_frame_config_t invalid_config = {0};
        bk_h264_encode_ctlr_handle_t handle = NULL;
        avdk_err_t avdk_ret = bk_h264_encode_frame_new(&handle, &invalid_config);
        if (avdk_ret != AVDK_ERR_OK) {
            LOGI("%s, %d, Invalid config test passed (expected failure)\n", __func__, __LINE__);
            ret = BK_OK; // Expected to fail
        } else {
            LOGE("%s, %d, Invalid config test failed (should have failed)\n", __func__, __LINE__);
            ret = BK_FAIL;
            if (handle != NULL) {
                bk_h264_encode_delete(handle);
            }
        }
    }
    else {
        LOGE("%s, %d, not found this error test type!\n", __func__, __LINE__);
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

