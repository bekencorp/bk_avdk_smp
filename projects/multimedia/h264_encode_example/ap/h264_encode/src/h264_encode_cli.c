#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <media_service.h>
#include <components/bk_frame_buffer.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264_encode_test.h"

#define TAG "h264_enc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_h264_encode_ctlr_handle_t h264_encode_handle = NULL;
static bk_h264_encode_frame_config_t h264_encode_config = {
    .outbuf_malloc = h264_encode_outbuf_malloc,
    .outbuf_malloc_args = NULL,
    .outbuf_complete = h264_encode_outbuf_complete,
    .outbuf_complete_args = NULL,
};

void cli_h264_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    avdk_err_t avdk_ret = AVDK_ERR_OK;

    if (argc < 2) {
        LOGE("%s, %d, param error!\n", __func__, __LINE__);
        ret = BK_FAIL;
        goto exit;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        ret = create_and_open_encoder((void **)&h264_encode_handle, &h264_encode_config);
        if (ret != BK_OK) {
            LOGE("%s, %d, h264 encode init failed!\n", __func__, __LINE__);
        } else {
            LOGD("%s, %d, h264 encode init success!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "delete") == 0) {
        ret = close_and_delete_encoder((void **)&h264_encode_handle);
        if (ret != BK_OK) {
            LOGE("%s, %d, h264 encode delete failed! ret: %d\n", __func__, __LINE__, ret);
        } else {
            LOGD("%s, %d, h264 encode delete success!\n", __func__, __LINE__);
        }
    }
    else if (os_strcmp(argv[1], "open") == 0) {
        if (h264_encode_handle != NULL) {
            avdk_ret = bk_h264_encode_open(h264_encode_handle);
            if (avdk_ret != AVDK_ERR_OK) {
                LOGE("%s, %d, h264 encode open failed!\n", __func__, __LINE__);
                ret = BK_FAIL;
            } else {
                LOGD("%s, %d, h264 encode open success!\n", __func__, __LINE__);
            }
        } else {
            LOGE("%s, %d, h264 encode handle is NULL, please init first!\n", __func__, __LINE__);
            ret = BK_FAIL;
        }
    }
    else if (os_strcmp(argv[1], "close") == 0) {
        if (h264_encode_handle != NULL) {
            avdk_ret = bk_h264_encode_close(h264_encode_handle);
            if (avdk_ret != AVDK_ERR_OK) {
                LOGE("%s, %d, h264 encode close failed!\n", __func__, __LINE__);
                ret = BK_FAIL;
            } else {
                LOGD("%s, %d, h264 encode close success!\n", __func__, __LINE__);
            }
        } else {
            LOGE("%s, %d, h264 encode handle is NULL!\n", __func__, __LINE__);
            ret = BK_FAIL;
        }
    }
    else if (os_strcmp(argv[1], "encode") == 0) {
        if (h264_encode_handle != NULL) {
            ret = perform_h264_encode_test(h264_encode_handle, "manual");
        } else {
            LOGE("%s, %d, h264 encode handle is NULL, please init first!\n", __func__, __LINE__);
            ret = BK_FAIL;
        }
    }
    else if (os_strcmp(argv[1], "force_idr") == 0) {
        if (h264_encode_handle != NULL) {
            avdk_ret = bk_h264_encode_force_idr(h264_encode_handle);
            if (avdk_ret != AVDK_ERR_OK) {
                LOGE("%s, %d, h264 encode force_idr failed!\n", __func__, __LINE__);
                ret = BK_FAIL;
            } else {
                LOGD("%s, %d, h264 encode force_idr success!\n", __func__, __LINE__);
            }
        } else {
            LOGE("%s, %d, h264 encode handle is NULL, please init first!\n", __func__, __LINE__);
            ret = BK_FAIL;
        }
    }
    else {
        LOGE("%s, %d, not found this cmd!\n", __func__, __LINE__);
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

