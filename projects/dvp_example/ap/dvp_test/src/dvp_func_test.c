#include <components/avdk_utils/avdk_error.h>
#include <components/bk_camera_ctlr.h>
#include "dvp_cli.h"
#include "dvp_frame_list.h"

#define TAG "func_test"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_camera_ctlr_handle_t s_dvp_func_handle = NULL;

static void dvp_func_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("dvp open width height img_format\n");
    LOGD("- width: image width, default is 864\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- img_format: image format, default is MJPEG, support MJPEG, H264, H265, YUV\n");
    LOGD("dvp close\n");
}


extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

void dvp_api_dump_jpeg(void)
{
    frame_buffer_t *frame = dvp_frame_queue_get_frame(IMAGE_MJPEG, 500);
    if (frame)
    {
        stack_mem_dump((uint32_t)frame->frame, (uint32_t)frame->frame + frame->length);

        dvp_frame_queue_free(IMAGE_MJPEG, frame);
    } else {
        LOGE("%s, %d: dvp_frame_queue_get_frame failed\n", __func__, __LINE__);
    }
}

void dvp_api_dump_h264(void)
{
    frame_buffer_t *frame = dvp_frame_queue_get_frame(IMAGE_H264, 500);
    if (frame)
    {
        stack_mem_dump((uint32_t)frame->frame, (uint32_t)frame->frame + frame->length);
        dvp_frame_queue_free(IMAGE_H264, frame);
    } else {
        LOGE("%s, %d: dvp_frame_queue_get_frame failed\n", __func__, __LINE__);
    }
}

void dvp_api_dump_yuv(void)
{
    frame_buffer_t *frame = dvp_frame_queue_get_frame(IMAGE_YUV, 500);
    if (frame)
    {
        stack_mem_dump((uint32_t)frame->frame, (uint32_t)frame->frame + frame->size);
        dvp_frame_queue_free(IMAGE_YUV, frame);
    } else {
        LOGE("%s, %d: dvp_frame_queue_get_frame failed\n", __func__, __LINE__);
    }
}

static frame_buffer_t *dvp_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = dvp_frame_queue_malloc(format, size);

    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }

    return frame;
}

static void dvp_frame_complete(image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame->sequence % 30 == 0)
    {
        LOGD("%s: seq:%d, length:%d, format:%s, h264_type:%x, result:%d\n", __func__, frame->sequence,
            frame->length, (format == IMAGE_MJPEG) ? "mjpeg" : (format == IMAGE_H264) ? "h264" : "YUV", frame->h264_type, result);
    }

    if (result != AVDK_ERR_OK)
    {
        dvp_frame_queue_free(format, frame);
    }
    else
    {
        dvp_frame_queue_complete(format, frame);
    }
}

static const bk_dvp_callback_t dvp_cbs = {
    .malloc = dvp_frame_malloc,
    .complete = dvp_frame_complete,
};

void cli_dvp_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    uint16_t output_format = IMAGE_MJPEG;

    dvp_frame_queue_init_all();

    // 检查参数数量是否足够
    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        dvp_func_cmd_help();
        goto exit;
    }

    if (CMD_CONTAIN("jpeg"))
    {
        output_format = IMAGE_MJPEG;
    }

    if (CMD_CONTAIN("h264"))
    {
        output_format = IMAGE_H264;
    }

    if (CMD_CONTAIN("h265"))
    {
        output_format = IMAGE_H265;
    }

    if (CMD_CONTAIN("yuv"))
    {
        output_format = IMAGE_YUV;
    }

    if (CMD_CONTAIN("enc_yuv"))
    {
        output_format |= IMAGE_YUV;
    }

    if (strcmp(argv[1], "open") == 0)
    {
        if (s_dvp_func_handle != NULL)
        {
            LOGE("%s, %d: dvp handle already opened\n", __func__, __LINE__);
            goto exit;
        }

        if (argc < 4) {
            LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
            dvp_func_cmd_help();
            goto exit;
        }

        // step 1: power on dvp camera
        if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_UP(DVP_POWER_GPIO_ID);
        }

        bk_dvp_ctlr_config_t dvp_ctrl_config = {
            .config = BK_DVP_864X480_30FPS_MJPEG_CONFIG(),
            .cbs = &dvp_cbs,
        };

        dvp_ctrl_config.config.img_format = output_format;
        dvp_ctrl_config.config.width = os_strtoul(argv[2], NULL, 10);
        dvp_ctrl_config.config.height = os_strtoul(argv[3], NULL, 10);
        ret = bk_camera_dvp_ctlr_new(&s_dvp_func_handle, &dvp_ctrl_config);
        if (ret == AVDK_ERR_OK)
        {
            ret = bk_camera_open(s_dvp_func_handle);
            if (ret != AVDK_ERR_OK)
            {
                bk_camera_delete(s_dvp_func_handle);
                s_dvp_func_handle = NULL;
            }
            else
            {
                LOGD("%s open successful\n", __func__);
            }
        }
    }
    else if (strcmp(argv[1], "close") == 0)
    {
        if (s_dvp_func_handle != NULL)
        {
            ret = bk_camera_close(s_dvp_func_handle);
            if (ret == AVDK_ERR_OK)
            {
                bk_camera_delete(s_dvp_func_handle);
                s_dvp_func_handle = NULL;
                // power off dvp camera
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
            }
        }
        else
        {
            LOGE("%s, %d: dvp handle is NULL\n", __func__, __LINE__);
            ret = AVDK_ERR_OK;
        }
    }
    else if (strcmp(argv[1], "dump") == 0)
    {
        ret = AVDK_ERR_OK;
        if (output_format == IMAGE_MJPEG)
        {
            dvp_api_dump_jpeg();
        }
        else if (output_format == IMAGE_H264)
        {
            dvp_api_dump_h264();
        }
        else if (output_format == IMAGE_YUV)
        {
            dvp_api_dump_yuv();
        }
        else
        {
            ret = AVDK_ERR_INVAL;
            LOGE("%s, %d: invalid arguments\n", __func__, __LINE__);
            dvp_func_cmd_help();
        }
    }
    else
    {
        LOGE("%s, %d: invalid arguments\n", __func__, __LINE__);
        dvp_func_cmd_help();
    }

exit:
    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
        // power off dvp camera
        if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_DOWN(DVP_POWER_GPIO_ID);
        }
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}