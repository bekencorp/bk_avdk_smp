#include <components/avdk_utils/avdk_error.h>
#include "frame_buffer.h"
#include "media_utils.h"
#include "uvc_common.h"

#define TAG "uvc_com"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t uvc_checkout_port_info_yuv(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    // Need to lock protection when accessing port_info
    rtos_lock_mutex(&info->uvc_mutex);
    bk_usb_hub_port_info *uvc_port_info = info->port_info[user_config->port - 1];
    if (uvc_port_info == NULL)
    {
        rtos_unlock_mutex(&info->uvc_mutex);
        LOGE("%s: uvc_port_info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;

    // Add PID/VID and other log information
    LOGD("PORT:0x%x\r\n", user_config->port);
    LOGD("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\r\n", uvc_device_param->device_bcd);

    frame_num = uvc_device_param->all_frame.yuv_frame_num;
    for (index = 0; index < frame_num; index++)
    {
        format_flag = true;
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

    // Unlock at the end of the function to ensure all access to shared resources is safe
    rtos_unlock_mutex(&info->uvc_mutex);

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->img_format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

avdk_err_t uvc_checkout_port_info_mjpeg(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    // Need to lock protection when accessing port_info
    rtos_lock_mutex(&info->uvc_mutex);
    bk_usb_hub_port_info *uvc_port_info = info->port_info[user_config->port - 1];
    if (uvc_port_info == NULL)
    {
        rtos_unlock_mutex(&info->uvc_mutex);
        LOGE("%s: uvc_port_info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;

    // Add PID/VID and other log information
    LOGD("PORT:0x%x\r\n", user_config->port);
    LOGD("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\r\n", uvc_device_param->device_bcd);

    frame_num = uvc_device_param->all_frame.mjpeg_frame_num;
    for (index = 0; index < frame_num; index++)
    {
        format_flag = true;
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

    // Unlock at the end of the function to ensure all access to shared resources is safe
    rtos_unlock_mutex(&info->uvc_mutex);

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->img_format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

avdk_err_t uvc_checkout_port_info_h264(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    // Need to lock protection when accessing port_info
    rtos_lock_mutex(&info->uvc_mutex);
    bk_usb_hub_port_info *uvc_port_info = info->port_info[user_config->port - 1];
    if (uvc_port_info == NULL)
    {
        rtos_unlock_mutex(&info->uvc_mutex);
        LOGE("%s: uvc_port_info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;

    // Add PID/VID and other log information
    LOGD("PORT:0x%x\r\n", user_config->port);
    LOGD("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\r\n", uvc_device_param->device_bcd);

    frame_num = uvc_device_param->all_frame.h264_frame_num;
    for (index = 0; index < frame_num; index++)
    {
        format_flag = true;
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

    // Unlock at the end of the function to ensure all access to shared resources is safe
    rtos_unlock_mutex(&info->uvc_mutex);

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->img_format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

avdk_err_t uvc_checkout_port_info_h265(uvc_test_info_t *info, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    // Need to lock protection when accessing port_info
    rtos_lock_mutex(&info->uvc_mutex);
    bk_usb_hub_port_info *uvc_port_info = info->port_info[user_config->port - 1];
    if (uvc_port_info == NULL)
    {
        rtos_unlock_mutex(&info->uvc_mutex);
        LOGE("%s: uvc_port_info is NULL\n", __func__);
        return AVDK_ERR_INVAL;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;

    // Add PID/VID and other log information
    LOGD("PORT:0x%x\r\n", user_config->port);
    LOGD("VID:0x%x\r\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\r\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\r\n", uvc_device_param->device_bcd);

    frame_num = uvc_device_param->all_frame.h265_frame_num;
    for (index = 0; index < frame_num; index++)
    {
        format_flag = true;
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

    // Unlock at the end of the function to ensure all access to shared resources is safe
    rtos_unlock_mutex(&info->uvc_mutex);

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->img_format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    return AVDK_ERR_OK;
}

static void uvc_connect_successful_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    LOGI("%s, %d\n", __func__, __LINE__);
    uvc_test_info_t *info = (uvc_test_info_t *)arg;
    if (info == NULL)
        return;
    rtos_lock_mutex(&info->uvc_mutex);
    info->port_info[port_info->port_index - 1] = port_info;
    rtos_set_semaphore(&info->uvc_connect_semaphore);
    rtos_unlock_mutex(&info->uvc_mutex);
}

static void uvc_disconnect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    LOGI("%s, %d\n", __func__, __LINE__);
    uvc_test_info_t *info = (uvc_test_info_t *)arg;
    if (info == NULL)
        return;
    rtos_lock_mutex(&info->uvc_mutex);
    info->port_info[port_info->port_index - 1] = NULL;
    rtos_unlock_mutex(&info->uvc_mutex);
}

static avdk_err_t uvc_camera_device_power_on(E_USB_DEVICE_T device, uvc_test_info_t *info)
{
    avdk_err_t ret = AVDK_ERR_OK;
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, device, uvc_connect_successful_callback, info);
        bk_usbh_hub_port_register_disconnect_callback(port, device, uvc_disconnect_callback, info);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, device);
    }

    // After power-on is completed, check if connection is successful
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        ret = bk_usbh_hub_port_check_device(port, device, &info->port_info[port - 1]);
        if (ret == AVDK_ERR_OK)
        {
            break;
        }
    }

    return ret;
}

static void uvc_camera_device_power_off(E_USB_DEVICE_T device)
{
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
    {
        bk_usbh_hub_port_register_connect_callback(port, device, NULL, NULL);
        bk_usbh_hub_port_register_disconnect_callback(port, device, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, device);
    }
}

avdk_err_t uvc_camera_power_on_handle(uvc_test_info_t **p_info, uint32_t timeout)
{
    // Power on the camera device and check have connected already
    avdk_err_t ret = AVDK_ERR_OK;
    uvc_test_info_t *info = NULL;

    /*USB_UVC_DEVICE: for mjpeg stream*/
    // there only consider single stream, not consider multistream(mjpeg/h26x)
    info = (uvc_test_info_t *)p_info[USB_UVC_DEVICE];
    ret = uvc_camera_device_power_on(USB_UVC_DEVICE, info);

    // If not checked successfully, wait for the connection success callback
    if (ret != AVDK_ERR_OK)
    {
        ret = rtos_get_semaphore(&info->uvc_connect_semaphore, timeout);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d, timeout:%d\n", __func__, __LINE__, timeout);
        }
    }

    /*USB_UVC_H26X_DEVICE: for h26x stream, this can not execute*/
    info = (uvc_test_info_t *)p_info[USB_UVC_H26X_DEVICE];
    ret = uvc_camera_device_power_on(USB_UVC_H26X_DEVICE, info);

    if (ret != AVDK_ERR_OK)
    {
        ret = rtos_get_semaphore(&info->uvc_connect_semaphore, timeout);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s, %d, timeout:%d\n", __func__, __LINE__, timeout);
        }
    }
    return ret;
}

avdk_err_t uvc_camera_power_off_handle(uvc_test_info_t **info)
{
    // Before power-off, check if all ports are closed
    avdk_err_t ret = AVDK_ERR_GENERIC;
    if (info == NULL)
    {
        LOGE("%s, %d: info is NULL\n", __func__, __LINE__);
        return ret;
    }
    uint8_t all_closed = true;
    
    // Check all stream types for all ports
    for (int stream_idx = 0; stream_idx < 2; stream_idx++) { // Check both MJPEG and H26X streams
        if (info[stream_idx]) {
            for (uint8_t port = 1; port <= UVC_PORT_MAX; port++)
            {
                if (info[stream_idx]->handle[port - 1] != NULL)
                {
                    all_closed = false;
                    LOGW("%s, %d: port %d not closed\n", __func__, __LINE__, port);
                    break;
                }
            }
            if (!all_closed) break; // Break outer loop if any handle is open
        }
    }

    if (all_closed == true)
    {
        // Handle MJPEG stream device
        if (info[USB_UVC_DEVICE]) {
            rtos_get_semaphore(&info[USB_UVC_DEVICE]->uvc_connect_semaphore, BEKEN_NO_WAIT);
            os_memset(info[USB_UVC_DEVICE]->port_info, 0, sizeof(bk_usb_hub_port_info *) * UVC_PORT_MAX);
        }
        // Handle H26X stream device
        if (info[USB_UVC_H26X_DEVICE]) {
            rtos_get_semaphore(&info[USB_UVC_H26X_DEVICE]->uvc_connect_semaphore, BEKEN_NO_WAIT);
            os_memset(info[USB_UVC_H26X_DEVICE]->port_info, 0, sizeof(bk_usb_hub_port_info *) * UVC_PORT_MAX);
        }
        
        // Power off the devices
        uvc_camera_device_power_off(USB_UVC_H26X_DEVICE);
        uvc_camera_device_power_off(USB_UVC_DEVICE);
        
        ret = AVDK_ERR_OK;
    }
    else
    {
        LOGE("%s, %d: still have opened handle, can't power off\n", __func__, __LINE__);
        return ret;
    }

    return ret;
}
