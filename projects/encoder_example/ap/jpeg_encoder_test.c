#include <avdk_error.h>
#include <os/str.h>
#include <components/bk_jpeg_encode_ctlr.h>
#include "frame_buffer.h"
#include "encoder_cli.h"

#define TAG "jpeg_encode_test"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_jpeg_encode_ctlr_handle_t s_jpeg_encode_handle = NULL;

static void jpeg_encode_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("jpeg_encode open\n");
    LOGD("- width: image width, default is 640\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- yuv_format: yuv format, default is YUV_FORMAT_YUYV\n");
    LOGD("jpeg_encode encode\n");
    LOGD("jpeg_encode set_compress min_size_bytes max_size_bytes\n");
    LOGD("jpeg_encode get_compress\n");
    LOGD("jpeg_encode close\n");
}

static avdk_err_t jpeg_encode_open(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    bk_jpeg_encode_ctlr_config_t config = {
        .width = YUV_TEST_BUF_WIDTH,
        .height = YUV_TEST_BUF_HEIGHT,
        .yuv_format = YUV_FORMAT_YUYV,
    };

    ret = bk_jpeg_encode_ctlr_new(&s_jpeg_encode_handle, &config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_jpeg_encode_ctlr_new failed, ret:%d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = bk_jpeg_encode_open(s_jpeg_encode_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_jpeg_encode_open failed, ret:%d\n", __func__, __LINE__, ret);
        bk_jpeg_encode_delete(s_jpeg_encode_handle);
        s_jpeg_encode_handle = NULL;
    }

    return ret;
}

static avdk_err_t jpeg_encode_close(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    ret = bk_jpeg_encode_close(s_jpeg_encode_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_jpeg_encode_close failed, ret:%d\n", __func__, __LINE__, ret);
        return ret;
    }
    ret = bk_jpeg_encode_delete(s_jpeg_encode_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_jpeg_encode_delete failed, ret:%d\n", __func__, __LINE__, ret);
        return ret;
    }

    s_jpeg_encode_handle = NULL;
    return ret;
}

static avdk_err_t jpeg_encode_encode(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    frame_buffer_t *yuv_frame = (frame_buffer_t *)os_malloc(sizeof(frame_buffer_t));
    if (yuv_frame == NULL)
    {
        LOGE("%s, %d: frame_buffer_display_malloc failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }

    yuv_frame->frame = get_yuv_test_buf();

    frame_buffer_t *jpeg_frame = frame_buffer_encode_malloc(100 * 1024);
    if (jpeg_frame == NULL)
    {
        LOGE("%s, %d: frame_buffer_encode_malloc failed\n", __func__, __LINE__);
        os_free(yuv_frame);
        return AVDK_ERR_NOMEM;
    }

    ret = bk_jpeg_encode_encode(s_jpeg_encode_handle, yuv_frame, jpeg_frame);
    if (ret == AVDK_ERR_OK)
    {
        extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
        stack_mem_dump((uint32_t)jpeg_frame->frame, (uint32_t)jpeg_frame->frame + jpeg_frame->length);
    }

    frame_buffer_encode_free(jpeg_frame);
    os_free(yuv_frame);

    return ret;
}

void cli_jpeg_encoder_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;

    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        jpeg_encode_cmd_help();
        return;
    }

    if (strcmp(argv[1], "open") == 0)
    {
        if (s_jpeg_encode_handle != NULL)
        {
            LOGE("%s, %d: jpeg_encode handle already opened\n", __func__, __LINE__);
            return;
        }
        ret = jpeg_encode_open();
    }
    else if (strcmp(argv[1], "close") == 0)
    {
        if (s_jpeg_encode_handle == NULL)
        {
            LOGE("%s, %d: jpeg_encode handle not opened\n", __func__, __LINE__);
            return;
        }
        ret = jpeg_encode_close();
    }
    else if (strcmp(argv[1], "encode") == 0)
    {
        if (s_jpeg_encode_handle == NULL)
        {
            LOGE("%s, %d: jpeg_encode handle not opened\n", __func__, __LINE__);
            return;
        }
        ret = jpeg_encode_encode();
    }
    else if (strcmp(argv[1], "set_compress") == 0)
    {
        if (s_jpeg_encode_handle == NULL)
        {
            LOGE("%s, %d: jpeg_encode handle not opened\n", __func__, __LINE__);
            return;
        }
        if (argc < 4)
        {
            LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
            return;
        }

        uint32_t min_size = os_strtoul(argv[2], NULL, 10);
        uint32_t max_size = os_strtoul(argv[3], NULL, 10);
        if (min_size >= max_size)
        {
            LOGE("%s, %d: min_size >= max_size\n", __func__, __LINE__);
            return;
        }
        bk_jpeg_encode_frame_compress_t compress = {
            .min_size_bytes = min_size,
            .max_size_bytes = max_size,
        };
        ret = bk_jpeg_encode_ioctl(s_jpeg_encode_handle, JPEG_ENCODE_IOCTL_CMD_SET_COMPRESS_PARAM, &compress);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: set compress failed, ret:%d\n", __func__, __LINE__, ret);
            return;
        }
    }
    else if (strcmp(argv[1], "get_compress") == 0)
    {
        if (s_jpeg_encode_handle == NULL)
        {
            LOGE("%s, %d: jpeg_encode handle not opened\n", __func__, __LINE__);
            return;
        }
        bk_jpeg_encode_frame_compress_t compress = {
            .min_size_bytes = 0,
            .max_size_bytes = 0,
        };
        ret = bk_jpeg_encode_ioctl(s_jpeg_encode_handle, JPEG_ENCODE_IOCTL_CMD_GET_COMPRESS_PARAM, &compress);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: get compress failed, ret:%d\n", __func__, __LINE__, ret);
            return;
        }
        LOGI("%s, %d: min_size_bytes:%d, max_size_bytes:%d\n", __func__, __LINE__, compress.min_size_bytes, compress.max_size_bytes);
    }
    else
    {
        LOGE("%s, %d: invalid command\n", __func__, __LINE__);
        jpeg_encode_cmd_help();
    }

    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}