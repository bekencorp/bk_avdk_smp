#include <components/avdk_utils/avdk_error.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <components/bk_camera_ctlr.h>
#include "frame_buffer.h"
#include "media_utils.h"
#include "uvc_cli.h"
#include "uvc_common.h"
#include "cli.h"

#define TAG "uvc_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static uvc_test_info_t *uvc_test_info = NULL;

bool cmd_contain(int argc, char **argv, char *string)
{
    int i;
    bool ret = false;

    for (i = 0; i < argc; i++)
    {
        if (os_strcmp(argv[i], string) == 0)
        {
            ret = true;
        }
    }

    return ret;
}

static void uvc_api_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("uvc_api new port width height img_format\n");
    LOGD("- port: uvc port number, default is 1, range [1- %d]\n", UVC_PORT_MAX);
    LOGD("- width: image width, default is 864\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- img_format: image format, default is MJPEG, support MJPEG, H264, H265, YUV\n");
    LOGD("uvc_api delete port\n");
    LOGD("uvc_api open port\n");
    LOGD("uvc_api close port\n");
    LOGD("uvc_api suspend port\n");
    LOGD("uvc_api resume port\n");
}

static void uvc_func_cmd_help(void)
{
    LOGD("*************support commands list:********************\n");
    LOGD("uvc open port width height img_format\n");
    LOGD("- port: uvc port number, default is 1, range [1- %d]\n", UVC_PORT_MAX);
    LOGD("- width: image width, default is 864\n");
    LOGD("- height: image height, default is 480\n");
    LOGD("- img_format: image format, default is MJPEG, support MJPEG, H264, H265, YUV\n");
    LOGD("uvc close port\n");
}


static avdk_err_t uvc_test_info_init(void)
{
    if (uvc_test_info == NULL)
    {
        uvc_test_info = os_malloc(sizeof(uvc_test_info_t));
        if (uvc_test_info == NULL)
        {
            LOGE("%s: malloc uvc_test_info failed\n", __func__);
            return AVDK_ERR_NOMEM;
        }
        os_memset(uvc_test_info, 0, sizeof(uvc_test_info_t));

        if (rtos_init_semaphore(&uvc_test_info->uvc_connect_semaphore, 1) != AVDK_ERR_OK)
        {
            LOGE("%s: init semaphore failed\n", __func__);
            os_free(uvc_test_info);
            uvc_test_info = NULL;
            return AVDK_ERR_NOMEM;
        }

        if (rtos_init_mutex(&uvc_test_info->uvc_mutex) != AVDK_ERR_OK)
        {
            LOGE("%s: init mutex failed\n", __func__);
            rtos_deinit_semaphore(&uvc_test_info->uvc_connect_semaphore);
            os_free(uvc_test_info);
            uvc_test_info = NULL;
            return AVDK_ERR_NOMEM;
        }
    }

    return AVDK_ERR_OK;
}

static avdk_err_t uvc_test_info_deinit(void)
{
    if (uvc_test_info)
    {
        for (int i = 0; i < UVC_PORT_MAX; i++)
        {
            if (uvc_test_info->port_info[i])
            {
                LOGE("%s, %d, port:%d not closed\n", __func__, __LINE__, i + 1);
                return AVDK_ERR_INVAL;
            }
        }
        rtos_deinit_mutex(&uvc_test_info->uvc_mutex);
        rtos_deinit_semaphore(&uvc_test_info->uvc_connect_semaphore);
        os_free(uvc_test_info);
        uvc_test_info = NULL;
    }

    return AVDK_ERR_OK;
}


static avdk_err_t uvc_checkout_user_info(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (user_config == NULL)
    {
        LOGE("%s: config is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    // No need to lock in the main function, as each format processing function handles locking internally
    LOGD("%s, %d, port:%d, format:%d, W*H:%d*%d\r\n", __func__, __LINE__, user_config->port, user_config->img_format,
        user_config->width, user_config->height);

    avdk_err_t ret = AVDK_ERR_OK;
    switch (user_config->img_format)
    {
        case IMAGE_YUV:
            ret = uvc_checkout_port_info_yuv(info, user_config);
            break;

        case IMAGE_MJPEG:
            ret = uvc_checkout_port_info_mjpeg(info, user_config);
            break;

        case IMAGE_H264:
            ret = uvc_checkout_port_info_h264(info, user_config);
            break;

        case IMAGE_H265:
            ret = uvc_checkout_port_info_h265(info, user_config);
            break;

        default:
            LOGE("%s, please check usb output format:%d\r\n", __func__, user_config->img_format);
            ret = AVDK_ERR_UNSUPPORTED;
            break;
    }

    return ret;
}


static frame_buffer_t *uvc_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = NULL;
    if (format == IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        frame = frame_buffer_encode_malloc(size);
    }

    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }

    return frame;
}

static void uvc_frame_complete(uint8_t port, image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame->sequence % 30 == 0)
    {
        LOGD("%s: port:%d, seq:%d, length:%d, format:%s, h264_type:%x, result:%d\n", __func__, port, frame->sequence,
            frame->length, (format == IMAGE_MJPEG) ? "mjpeg" : "h264", frame->h264_type, result);
    }

    if (format == IMAGE_YUV)
    {
        frame_buffer_display_free(frame);
    }
    else
    {
        frame_buffer_encode_free(frame);
    }
}

static void uvc_event_callback(bk_usb_hub_port_info *port_info,void *arg, uvc_error_code_t code)
{
    LOGD("uvc_event_callback: %d", code);
}

static const bk_uvc_callback_t uvc_cbs = {
    .malloc = uvc_frame_malloc,
    .complete = uvc_frame_complete,
    .uvc_event_callback = uvc_event_callback,
};

static avdk_err_t uvc_open_handle(uvc_test_info_t *info, bk_uvc_ctlr_config_t *uvc_ctlr_config)
{
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    uint8_t port = uvc_ctlr_config->config.port;

    // Check if the resolution input by the user is supported, can be selectively enabled
    avdk_err_t ret = uvc_checkout_user_info(info, &uvc_ctlr_config->config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: uvc_checkout_user_info failed\n", __func__, __LINE__);
        return ret;
    }

    ret = bk_camera_uvc_ctlr_new(&info->handle[port - 1], uvc_ctlr_config);
    if (ret == BK_OK)
    {
        ret = bk_camera_open(info->handle[port - 1]);
        if (ret != BK_OK)
        {
            LOGE("%s, %d: bk_camera_open failed, ret:%d\n", __func__, __LINE__, ret);
            bk_camera_delete(info->handle[port - 1]);
            info->handle[port - 1] = NULL;
        }
        else
        {
            LOGD("%s open successful\n", __func__);
        }
    }
    else
    {
        LOGE("%s, %d: bk_camera_uvc_ctlr_new failed, ret:%d\n", __func__, __LINE__, ret);
    }

    return ret;
}

static avdk_err_t uvc_close_handle(uvc_test_info_t *info, uint8_t port)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if (info == NULL)
    {
        LOGE("%s: info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    if (uvc_test_info->handle[port - 1] != NULL)
    {
        ret = bk_camera_close(info->handle[port - 1]);
        if (ret == BK_OK)
        {
            bk_camera_delete(info->handle[port - 1]);
            info->handle[port - 1] = NULL;
        }
        else
        {
            LOGE("%s, %d: bk_camera_close failed, ret:%d\n", __func__, __LINE__, ret);
        }
    }

    return ret;
}

void cli_uvc_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    uint16_t output_format = IMAGE_MJPEG;
    uint8_t port = 1;
    uvc_test_info_t *test_info = uvc_test_info;

    // 检查参数数量是否足够
    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        uvc_func_cmd_help();
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

    if (CMD_CONTAIN("dual"))
    {
        output_format = IMAGE_MJPEG | IMAGE_H264;
    }

    if (strcmp(argv[1], "open") == 0)
    {
        //  check arguments
        if (argc < 5) {
            LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
            uvc_func_cmd_help();
            goto exit;
        }

        LOGD("step 1: power on usb port, and wait default 4000ms\n");

        ret = uvc_camera_power_on_handle(test_info, 4000);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: uvc_camera_power_on_handle failed\n", __func__, __LINE__);
            goto exit;
        }

        // step 2: open handle
        bk_uvc_ctlr_config_t uvc_ctlr_config = {
            .config = BK_UVC_864X480_30FPS_MJPEG_CONFIG(),
            .cbs = &uvc_cbs,
        };

        uvc_ctlr_config.config.img_format = output_format;
        port = os_strtoul(argv[2], NULL, 10);
        uvc_ctlr_config.config.width = os_strtoul(argv[3], NULL, 10);
        uvc_ctlr_config.config.height = os_strtoul(argv[4], NULL, 10);
        uvc_ctlr_config.config.user_data = test_info;
        if (port > UVC_PORT_MAX || port == 0)
        {
            LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
            goto exit;
        }

        uvc_ctlr_config.config.port = port;

        // step 2: check uvc open already
        LOGD("step 2: check uvc port:%d already open or not:%p\n", port, test_info->handle[port - 1]);
        if (test_info->handle[port - 1] != NULL)
        {
            LOGE("%s, %d: uvc handle already opened\n", __func__, __LINE__);
            goto exit;
        }

        LOGD("step 3: open uvc port:%d, img_format:%d\n", port, uvc_ctlr_config.config.img_format);
        ret = uvc_open_handle(test_info, &uvc_ctlr_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: uvc_open_handle failed, ret:%d\n", __func__, __LINE__, ret);
            goto exit;
        }
    }
    else if (strcmp(argv[1], "close") == 0)
    {
        LOGD("step 1: close uvc port:%d\n", port);
        if (argc > 2)
        {
            port = os_strtoul(argv[2], NULL, 10);
            if (port > UVC_PORT_MAX || port == 0)
            {
                LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
                goto exit;
            }
        }

        // step 2: check uvc close already
        LOGD("step 2: check uvc port:%d already close or not:%p\n", port, test_info->handle[port - 1]);
        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s, %d: uvc handle already closed\n", __func__, __LINE__);
            goto exit;
        }

        // step 3: close handle
        ret = uvc_close_handle(test_info, port);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: uvc_close_handle failed, ret:%d\n", __func__, __LINE__, ret);
            goto exit;
        }

        LOGD("step 3: power off usb port:%d\n", port);

        ret = uvc_camera_power_off_handle(test_info);
    }
    else
    {
        LOGE("%s, %d: unknown command %s\n", __func__, __LINE__, argv[1]);
        uvc_func_cmd_help();
    }

exit:
    if (ret != AVDK_ERR_OK)
    {
        msg = CLI_CMD_RSP_ERROR;
        uvc_camera_power_off_handle(test_info);
    }
    else
    {
        msg = CLI_CMD_RSP_SUCCEED;
    }

    LOGI("%s ---complete\n", __func__);

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

void cli_uvc_api_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNKNOWN;
    char *msg = NULL;
    uint16_t output_format = IMAGE_MJPEG;
    uint8_t port = 1;
    uvc_test_info_t *test_info = uvc_test_info;

    // 检查参数数量是否足够
    if (argc < 2) {
        LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
        uvc_api_cmd_help();
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

    if (CMD_CONTAIN("dual"))
    {
        output_format = IMAGE_MJPEG | IMAGE_H264;
    }

    if (strcmp(argv[1], "power_on") == 0)
    {
        ret = uvc_camera_power_on_handle(test_info, 4000);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: uvc_camera_power_on_handle failed\n", __func__, __LINE__);
            ret = uvc_camera_power_off_handle(test_info);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s, %d: uvc_camera_power_off_handle failed\n", __func__, __LINE__);
            }
            goto exit;
        }
    }
    else if (strcmp(argv[1], "power_off") == 0)
    {
        ret = uvc_camera_power_off_handle(test_info);
    }
    else if (strcmp(argv[1], "new") == 0)
    {
        bk_uvc_ctlr_config_t uvc_ctlr_config = {
            .config = BK_UVC_864X480_30FPS_MJPEG_CONFIG(),
            .cbs = &uvc_cbs,
        };

        if (argc < 5)
        {
            LOGE("%s, %d: insufficient arguments\n", __func__, __LINE__);
            uvc_api_cmd_help();
            goto exit;
        }

        uvc_ctlr_config.config.img_format = output_format;
        port = os_strtoul(argv[2], NULL, 10);
        uvc_ctlr_config.config.width = os_strtoul(argv[3], NULL, 10);
        uvc_ctlr_config.config.height = os_strtoul(argv[4], NULL, 10);
        uvc_ctlr_config.config.user_data = test_info;

        if (port > UVC_PORT_MAX || port == 0)
        {
            LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
            goto exit;
        }

        uvc_ctlr_config.config.port = port;

        // check uvc new already
        if (test_info->handle[port - 1] != NULL)
        {
            LOGE("%s, %d: uvc handle already new\n", __func__, __LINE__);
            goto exit;
        }

        ret = uvc_checkout_user_info(test_info, &uvc_ctlr_config.config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: uvc_checkout_user_info failed, ret:%d\n", __func__, __LINE__, ret);
            goto exit;
        }

        ret = bk_camera_uvc_ctlr_new(&test_info->handle[port - 1], &uvc_ctlr_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d: bk_camera_uvc_ctlr_new failed, ret:%d\n", __func__, __LINE__, ret);
        }
    }
    else if (strcmp(argv[1], "delete") == 0)
    {
        if (argc > 2)
        {
            port = os_strtoul(argv[2], NULL, 10);
            if (port > UVC_PORT_MAX || port == 0)
            {
                LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
                goto exit;
            }
        }

        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s, %d: handle is NULL\n", __func__, __LINE__);
            goto exit;
        }

        ret = bk_camera_delete(test_info->handle[port - 1]);
        if (ret == AVDK_ERR_OK)
        {
            test_info->handle[port - 1] = NULL;
        }
    }
    else if (strcmp(argv[1], "open") == 0)
    {
        if (argc > 2)
        {
            port = os_strtoul(argv[2], NULL, 10);
            if (port > UVC_PORT_MAX || port == 0)
            {
                LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
                goto exit;
            }
        }

        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s, %d: handle is NULL\n", __func__, __LINE__);
            goto exit;
        }

        ret = bk_camera_open(test_info->handle[port - 1]);
    }
    else if (strcmp(argv[1], "close") == 0)
    {
        if (argc > 2)
        {
            port = os_strtoul(argv[2], NULL, 10);
            if (port > UVC_PORT_MAX || port == 0)
            {
                LOGE("%s, %d: invalid port %d\n", __func__, __LINE__, port);
                goto exit;
            }
        }

        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s, %d: handle is NULL\n", __func__, __LINE__);
            goto exit;
        }

        ret = bk_camera_close(test_info->handle[port - 1]);
    }
    else if (strcmp(argv[1], "suspend") == 0)
    {
        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s: handle is NULL\n", __func__);
            goto exit;
        }

        ret = bk_camera_suspend(test_info->handle[port - 1]);
    }
    else if (strcmp(argv[1], "resume") == 0)
    {
        if (test_info->handle[port - 1] == NULL)
        {
            LOGE("%s: handle is NULL\n", __func__);
            goto exit;
        }

        ret = bk_camera_resume(test_info->handle[port - 1]);
    }
    else
    {
        LOGE("%s, %d: invalid arguments\n", __func__, __LINE__);
        uvc_api_cmd_help();
        ret = AVDK_ERR_INVAL;
    }

exit:
    if (ret != BK_OK)
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

#define CMDS_COUNT  (sizeof(s_uvc_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_uvc_test_commands[] =
{
    {"uvc", " uvc open | close", cli_uvc_func_test_cmd},
    {"uvc_api", "uvc power_on|power_off|open|close|new|delete|suspend|resume [port] [width] [height] [type]", cli_uvc_api_test_cmd},
};

int cli_uvc_test_init(void)
{
    int ret = uvc_test_info_init();
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: uvc_test_info_init failed\n", __func__, __LINE__);
        return ret;
    }
    return cli_register_commands(s_uvc_test_commands, CMDS_COUNT);
}