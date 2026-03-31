

#include <components/bk_frame_buffer.h>

#include <components/usb_types.h>
#include <components/bk_uvc_camera.h>
#include <avdk_error.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include "cli.h"
#define TAG "cli_uvc"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static beken_semaphore_t s_uvc_connect_sem = NULL;
static bk_uvc_ctlr_handle_t s_uvc_handle[UVC_PORT_MAX] = {NULL};

static frame_buffer_t *uvc_camera_frame_malloc(bk_image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, size + sizeof(frame_buffer_t));
    if (frame == NULL)
    {
        return NULL;
    }

    os_memset(frame, 0, sizeof(frame_buffer_t));

    frame->frame = (uint8_t *)(frame + 1);
    frame->size = size;
    return frame;
}

static void uvc_camera_frame_complete(uint8_t port, bk_image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame->sequence % 30 == 0)
    {
        LOGI("%s, port:%d, format:%d, result:%d, frame->sequence:%d, frame->length:%d\n", __func__, port, format, result, frame->sequence, frame->length);
    }

    bk_frame_buffer_free(frame);
}

static void uvc_event_callback(uvc_state_t state, void *user_data)
{
    LOGI("%s, state:%d, user_data:%p\n", __func__, state, *((bk_uvc_ctlr_handle_t *)user_data));
}

static const bk_uvc_callback_t uvc_camera_cbs =
{
    .frame_malloc = uvc_camera_frame_malloc,
    .frame_complete = uvc_camera_frame_complete,
    .state_change_cb = uvc_event_callback,
    .user_data = &s_uvc_handle,
};

static avdk_err_t uvc_check_mjpeg_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.mjpeg_frame_num;
    LOGD("%s, %d, frame_num:%d\n", __func__, __LINE__, frame_num);
    for (index = 0; index < frame_num; index++)
    {
        LOGD("MJPEG width:%d heigth:%d index:%d\r\n",
             uvc_device_param->all_frame.mjpeg_frame[index].width,
             uvc_device_param->all_frame.mjpeg_frame[index].height,
             uvc_device_param->all_frame.mjpeg_frame[index].index);

        if (uvc_device_param->all_frame.mjpeg_frame[index].width == user_config->width
            && uvc_device_param->all_frame.mjpeg_frame[index].height == user_config->height)
        {
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.mjpeg_frame[index].fps_num; i++)
        {
            LOGD("MJPEG fps:%d\r\n", uvc_device_param->all_frame.mjpeg_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.mjpeg_frame[index].fps[i] == user_config->fps)
            {
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag)
    {
        ret = AVDK_ERR_OK;
    }

    return ret;
}

static avdk_err_t uvc_check_yuv_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.yuv_frame_num;
    LOGI("%s, %d, frame_num:%d\n", __func__, __LINE__, frame_num);
    for (index = 0; index < frame_num; index++)
    {
        LOGD("YUV width:%d heigth:%d index:%d\r\n",
             uvc_device_param->all_frame.yuv_frame[index].width,
             uvc_device_param->all_frame.yuv_frame[index].height,
             uvc_device_param->all_frame.yuv_frame[index].index);

        if (uvc_device_param->all_frame.yuv_frame[index].width == user_config->width
            && uvc_device_param->all_frame.yuv_frame[index].height == user_config->height)
        {
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.yuv_frame[index].fps_num; i++)
        {
            LOGD("YUV fps:%d\r\n", uvc_device_param->all_frame.yuv_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.yuv_frame[index].fps[i] == user_config->fps)
            {
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag)
    {
        ret = AVDK_ERR_OK;
    }

    return ret;
}


static avdk_err_t uvc_check_h264_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.h264_frame_num;
    LOGI("%s, %d, frame_num:%d\n", __func__, __LINE__, frame_num);
    for (index = 0; index < frame_num; index++)
    {
        LOGD("H264 width:%d heigth:%d index:%d\r\n",
             uvc_device_param->all_frame.h264_frame[index].width,
             uvc_device_param->all_frame.h264_frame[index].height,
             uvc_device_param->all_frame.h264_frame[index].index);

        if (uvc_device_param->all_frame.h264_frame[index].width == user_config->width
            && uvc_device_param->all_frame.h264_frame[index].height == user_config->height)
        {
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.h264_frame[index].fps_num; i++)
        {
            LOGD("H264 fps:%d\r\n", uvc_device_param->all_frame.h264_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.h264_frame[index].fps[i] == user_config->fps)
            {
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag)
    {
        ret = AVDK_ERR_OK;
    }

    return ret;

}

static avdk_err_t uvc_check_h265_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.h265_frame_num;
    LOGI("%s, %d, frame_num:%d\n", __func__, __LINE__, frame_num);
    for (index = 0; index < frame_num; index++)
    {
        LOGD("H265 width:%d heigth:%d index:%d\r\n",
             uvc_device_param->all_frame.h265_frame[index].width,
             uvc_device_param->all_frame.h265_frame[index].height,
             uvc_device_param->all_frame.h265_frame[index].index);

        if (uvc_device_param->all_frame.h265_frame[index].width == user_config->width
            && uvc_device_param->all_frame.h265_frame[index].height == user_config->height)
        {
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.h265_frame[index].fps_num; i++)
        {
            LOGD("H265 fps:%d\r\n", uvc_device_param->all_frame.h265_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.h265_frame[index].fps[i] == user_config->fps)
            {
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag)
    {
        ret = AVDK_ERR_OK;
    }

    return ret;
}


static avdk_err_t uvc_checkout_port_info(bk_cam_uvc_config_t *user_config)
{
    bk_usb_hub_port_info *uvc_port_info = NULL;
    avdk_err_t ret = AVDK_ERR_OK;
 
    if (user_config->format == BK_IMAGE_FORMAT_H264 || user_config->format == BK_IMAGE_FORMAT_H265)
    {
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_H26X_DEVICE, &uvc_port_info);
    }
    else
    {
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_DEVICE, &uvc_port_info);
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, bk_usbh_hub_port_check_device failed, retry get <USB_UVC_DEVICE>\n", __func__, __LINE__);
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_DEVICE, &uvc_port_info);
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, bk_usbh_hub_port_check_device failed, format:%d\n", __func__, __LINE__, user_config->format);
        return ret;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    if (uvc_device_param == NULL)
    {
        LOGE("%s, %d, uvc_device_param is NULL\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    LOGD("PORT:0x%x\r\n", user_config->port);
    LOGD("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\r\n", uvc_device_param->device_bcd);
    LOGD("format:%d, width:%d, height:%d, fps:%d\r\n", user_config->format, user_config->width, user_config->height, user_config->fps);
    switch (user_config->format)
    {
        case BK_IMAGE_FORMAT_YUV:
            ret = uvc_check_yuv_config(uvc_device_param, user_config);
            break;

        case BK_IMAGE_FORMAT_MJPEG:
            ret = uvc_check_mjpeg_config(uvc_device_param, user_config);
            break;

        case BK_IMAGE_FORMAT_H264:
            ret = uvc_check_h264_config(uvc_device_param, user_config);
            break;

        case BK_IMAGE_FORMAT_H265:
            ret = uvc_check_h265_config(uvc_device_param, user_config);
            break;

        default:
            LOGE("%s, please check usb output format:%d\r\n", __func__, user_config->format);
            break;
    }

    LOGI("%s, %d, ret:%d\r\n", __func__, __LINE__, ret);

    return ret;
}

/**
 * @brief UVC device connection callback
 * 
 * This callback handles UVC device connection and creates backup of device parameters.
 */
static void uvc_device_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    LOGI("%s: UVC device connected\n", __func__);

    if (!port_info) {
        LOGE("%s: port_info is NULL\n", __func__);
        return;
    }

    beken_semaphore_t sem = (beken_semaphore_t)arg;
    if (sem)
    {
        rtos_set_semaphore(&sem);
    }
}

static avdk_err_t uvc_camera_power_on(uint32_t timeout)
{
    // Power on the camera device and check have connected already
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_uvc_connect_sem == NULL)
    {
        ret = rtos_init_semaphore(&s_uvc_connect_sem, 1);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d, rtos_init_semaphore failed, timeout:%d\n", __func__, __LINE__, timeout);
            return ret;
        }
    }

    rtos_get_semaphore(&s_uvc_connect_sem, BEKEN_NO_WAIT);

    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, uvc_device_connect_callback, s_uvc_connect_sem);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_DEVICE);
    }

    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_H26X_DEVICE, uvc_device_connect_callback, s_uvc_connect_sem);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_H26X_DEVICE);
    }

    bk_usb_hub_port_info *port_info = NULL;
    bk_usb_hub_port_info *port_info_h26x = NULL;
    avdk_err_t ret1 = AVDK_ERR_OK;
    avdk_err_t ret2 = AVDK_ERR_OK;
    // check if devices are already connected
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        ret1 = bk_usbh_hub_port_check_device(port, USB_UVC_DEVICE, &port_info);
        ret2 = bk_usbh_hub_port_check_device(port, USB_UVC_H26X_DEVICE, &port_info_h26x);
        if (ret1 == AVDK_ERR_OK || ret2 == AVDK_ERR_OK)
        {
            break;
        }
    }

    // if not connected, wait for connection
    if (ret1 != AVDK_ERR_OK && ret2 != AVDK_ERR_OK)
    {
        ret = rtos_get_semaphore(&s_uvc_connect_sem, timeout);
    }

    return ret;
}

static avdk_err_t uvc_camera_power_off(void)
{
    for (uint8_t index = 0; index < UVC_PORT_MAX; index++)
    {
        if (s_uvc_handle[index] != NULL)
        {
            LOGW("%s, %d, uvc port:%d not closed\n", __func__, __LINE__, index + 1);
            return AVDK_ERR_GENERIC;
        }
    }
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_DEVICE);
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_H26X_DEVICE, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_H26X_DEVICE);
    }
    return AVDK_ERR_OK;
}

bk_uvc_ctlr_handle_t uvc_camera_turn_on(bk_cam_uvc_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    bk_uvc_ctlr_handle_t handle = NULL;

    if (config == NULL)
    {
        LOGE("%s: parameters is NULL\n", __func__);
        return NULL;
    }

    // Power on the camera device and check have connected
    ret = uvc_camera_power_on(4000);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: uvc_camera_power_on failed\n", __func__);
        goto exit;
    }

    // suggest add delay to ensure the camera is stable connected
    // rtos_delay_milliseconds(3000);

    // Check the port info and input resolution/format uvc support or not
    // user should check the port info by yourself
#if 1
    ret = uvc_checkout_port_info(config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: uvc_checkout_port_info failed\n", __func__, __LINE__);
        goto exit;
    }
#endif
    ret = bk_uvc_ctrl_new(&handle, &uvc_camera_cbs);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_uvc_camera_ctlr_new failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_init(handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_uvc_init failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_open(handle, config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d: bk_uvc_open failed\n", __func__, __LINE__);
        goto exit;
    }

    LOGD("%s open successful\n", __func__);

    return handle;

exit:
    if (handle != NULL)
    {
        bk_uvc_deinit(handle);
        bk_uvc_delete(handle);
    }
    uvc_camera_power_off();
    return NULL;
}

avdk_err_t uvc_camera_turn_off(bk_uvc_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (handle == NULL)
    {
        return ret;
    }

    ret = bk_uvc_close(handle);
    if (ret != BK_OK)
    {
        LOGE("%s: bk_uvc_close failed\n", __func__);
        return ret;
    }

    ret = bk_uvc_deinit(handle);
    if (ret != BK_OK)
    {
        LOGE("%s: bk_uvc_deinit failed\n", __func__);
        return ret;
    }

    ret = bk_uvc_delete(handle);
    if (ret != BK_OK)
    {
        LOGE("%s: bk_uvc_delete failed\n", __func__);
        return ret;
    }

    uvc_camera_power_off();

    return ret;
}

static avdk_err_t get_arg_value(int argc, char **argv, const char *name)
{
    for (int i = 0; i < argc; i++)
    {
        if (os_strcmp(argv[i], name) == 0)
        {
            return AVDK_ERR_OK;
        }
    }

    return AVDK_ERR_INVAL;
}


void cli_uvc_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        LOGE("Usage: uvc <command>\n");
        return;
    }

    if (os_strcmp(argv[1], "open") == 0)
    {
        bk_cam_uvc_config_t config = MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG();
        if (argc >= 5)
        {
            config.port = os_strtoul(argv[2], NULL, 10);
            config.width = os_strtoul(argv[3], NULL, 10);
            config.height = os_strtoul(argv[4], NULL, 10);
        }

        if (config.port > UVC_PORT_MAX || config.port == 0)
        {
            LOGE("port:%d out of range, please check the port\n", config.port);
            return;
        }

        if (s_uvc_handle[config.port - 1] != NULL)
        {
            LOGE("uvc port:%d already open, please close first\n", config.port);
            return;
        }

        if (get_arg_value(argc, argv, "h264") == 0)
        {
            config.format = BK_IMAGE_FORMAT_H264;
        }
        else if (get_arg_value(argc, argv, "h265") == 0)
        {
            config.format = BK_IMAGE_FORMAT_H265;
        }
        else if (get_arg_value(argc, argv, "mjpeg") == 0)
        {
            config.format = BK_IMAGE_FORMAT_MJPEG;
        }
        else if (get_arg_value(argc, argv, "yuv") == 0)
        {
            config.format = BK_IMAGE_FORMAT_YUV;
        }
        else
        {
            LOGE("Usage: uvc open <port> <width> <height> <h264|h265|mjpeg|yuv>\n");
            config.format = BK_IMAGE_FORMAT_MJPEG;
        }

        LOGI("uvc open\n");
        s_uvc_handle[config.port - 1] = uvc_camera_turn_on(&config);
        if (s_uvc_handle[config.port - 1] == NULL)
        {
            LOGE("uvc open failed\n");
            return;
        }

        LOGI("uvc open successful\n");
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        if (argc < 3)
        {
            LOGE("Usage: uvc close <port>\n");
            return;
        }
        uint8_t port = os_strtoul(argv[2], NULL, 10);
        if (port > UVC_PORT_MAX || port == 0)
        {
            LOGE("port:%d out of range, please check the port\n", port);
            return;
        }
        if (s_uvc_handle[port - 1] == NULL)
        {
            LOGE("uvc port:%d already closed\n", port);
            return;
        }
        LOGI("uvc close\n");
        if (uvc_camera_turn_off(s_uvc_handle[port - 1]) != AVDK_ERR_OK)
        {
            LOGE("uvc port:%d close failed\n", port);
        }
        else
        {
            LOGI("uvc port:%d close successful\n", port);
            s_uvc_handle[port - 1] = NULL;
        }
    }
    else
    {
        LOGE("Usage: uvc <command>\n");
    }

    return;
}

static void cli_uvc_api_func_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    if (argc < 2)
    {
        LOGE("Usage: uvc_api <command>\n");
        return;
    }
    else if (os_strcmp(argv[1], "power_on") == 0)
    {
        LOGI("uvc_api power_on\n");
        ret = uvc_camera_power_on(4000);
    }
    else if (os_strcmp(argv[1], "power_off") == 0)
    {
        LOGI("uvc_api power_off\n");
        ret = uvc_camera_power_off();
    }
    else if (os_strcmp(argv[1], "new") == 0)
    {
        LOGI("uvc_api new\n");
        ret = bk_uvc_ctrl_new(&s_uvc_handle[0], &uvc_camera_cbs);
    }
    else if (os_strcmp(argv[1], "delete") == 0)
    {
        LOGI("uvc_api delete\n");
        ret = bk_uvc_delete(s_uvc_handle[0]);
        if (ret == AVDK_ERR_OK)
        {
            LOGI("uvc_api delete successful\n");
            s_uvc_handle[0] = NULL;
        }
    }
    else if (os_strcmp(argv[1], "init") == 0)
    {
        LOGI("uvc_api init\n");
        ret = bk_uvc_init(s_uvc_handle[0]);
    }
    else if (os_strcmp(argv[1], "deinit") == 0)
    {
        LOGI("uvc_api deinit\n");
        ret = bk_uvc_deinit(s_uvc_handle[0]);
    }
    else if (os_strcmp(argv[1], "open") == 0)
    {
        LOGI("uvc_api open\n");

        bk_cam_uvc_config_t config = MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG();
        if (argc >= 5)
        {
            config.port = os_strtoul(argv[2], NULL, 10);
            config.width = os_strtoul(argv[3], NULL, 10);
            config.height = os_strtoul(argv[4], NULL, 10);
        }

        if (get_arg_value(argc, argv, "h264") == 0)
        {
            config.format = BK_IMAGE_FORMAT_H264;
        }
        else if (get_arg_value(argc, argv, "h265") == 0)
        {
            config.format = BK_IMAGE_FORMAT_H265;
        }
        else if (get_arg_value(argc, argv, "mjpeg") == 0)
        {
            config.format = BK_IMAGE_FORMAT_MJPEG;
        }
        else if (get_arg_value(argc, argv, "yuv") == 0)
        {
            config.format = BK_IMAGE_FORMAT_YUV;
        }
        else
        {
            LOGE("Usage: uvc_api open <width> <height> <h264|h265|mjpeg|yuv>\n");
            config.format = BK_IMAGE_FORMAT_MJPEG;
        }

        ret = uvc_checkout_port_info(&config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("uvc_api checkout_port_info failed\n");
        }
        else
        {
            ret = bk_uvc_open(s_uvc_handle[0], &config);
        }
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        LOGI("uvc_api close\n");
        ret = bk_uvc_close(s_uvc_handle[0]);
    }
    else if (os_strcmp(argv[1], "suspend") == 0)
    {
        LOGI("uvc_api suspend\n");
        ret = bk_uvc_suspend(s_uvc_handle[0]);
    }
    else if (os_strcmp(argv[1], "resume") == 0)
    {
        LOGI("uvc_api resume\n");
        ret = bk_uvc_resume(s_uvc_handle[0]);
    }
    else
    {
        LOGE("Usage: uvc_api <command>\n");
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("execute %s failed\n", argv[1]);
    }
    else
    {
        LOGI("execute %s successful\n", argv[1]);
    }
}



#define CMDS_COUNT  (sizeof(s_uvc_test_commands) / sizeof(struct cli_command))

static const struct cli_command s_uvc_test_commands[] =
{
    {"uvc", " uvc open | close", cli_uvc_func_test_cmd},
    {"uvc_api", "uvc_api power_on | power_off | new | delete | init | deinit | open | close | suspend | resume ...", cli_uvc_api_func_test_cmd},
};

int cli_uvc_test_init(void)
{
    return cli_register_commands(s_uvc_test_commands, CMDS_COUNT);
}