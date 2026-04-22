#include <os/os.h>
#include <os/mem.h>

#include <avdk_check.h>
#include <components/bk_uvc_camera.h>
#include <components/cherryusb/usb_errno.h>
#include "bk_uvc_common.h"
#include "uvc_urb_list.h"
#if CONFIG_SOC_SMP
#include "spinlock.h"
#endif

#define TAG "uvc_stream"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGV(TAG, ##__VA_ARGS__)

#ifndef CONFIG_UVC_TASK_STACK_SIZE
#define CONFIG_UVC_TASK_STACK_SIZE 2048
#endif

#define UVC_TASK_STACK_SIZE (CONFIG_UVC_TASK_STACK_SIZE == 0 ? 2048 : CONFIG_UVC_TASK_STACK_SIZE)

#define UVC_HEADER_LEN_DEFAULT              12
#define UVC_HEADER_LEN_BASIC                 2
#define UVC_HEADER_LEN_WITH_PTS              6
#define UVC_HEADER_LEN_WITH_SCR              8

#define UVC_HEADER_FLAG_PTS                  0x04
#define UVC_HEADER_FLAG_SCR                  0x08
#define UVC_HEADER_FLAG_PTS_SCR              (UVC_HEADER_FLAG_PTS | UVC_HEADER_FLAG_SCR)
#define UVC_HEADER_FLAG_ERROR                0x40

#define UVC_HEADER_FLAG_STREAM_PRESENT       0x80
#define UVC_HEADER_EXTENSION_MASK            0x30

#define JPEG_MARKER_BYTE0                    0xFF
#define JPEG_MARKER_BYTE1                    0xD8

#define UVC_PAYLOAD_HEADER_LENGTH_INDEX      0
#define UVC_PAYLOAD_HEADER_INFO_INDEX        1

uvc_stream_handle_t *s_uvc_stream_handle = NULL;

#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_uvc_stream_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t uvc_stream_enter_critical(void)
{
    uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
    spin_lock(&s_uvc_stream_spin_lock);
#endif

    return flags;
}

static inline void uvc_stream_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
    spin_unlock(&s_uvc_stream_spin_lock);
#endif

    rtos_enable_int(flags);
}

static void uvc_camera_stream_receive_complete_callback(void *pCompleteParam, int nbytes);

static avdk_err_t uvc_stream_task_send_msg(uint32_t event, uint32_t param)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    uvc_msg_t msg;
    uint32_t flags = uvc_stream_enter_critical();
    uvc_stream_handle_t *stream_handle = s_uvc_stream_handle;

    if (stream_handle && stream_handle->stream_enable)
    {
        msg.event = event;
        msg.param = param;
        ret = rtos_push_to_queue(&stream_handle->stream_queue, &msg, BEKEN_NO_WAIT);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("push failed, event:%d\n", event);
        }
    } else {
        LOGE("stream_handle or stream_queue is NULL\n");
    }

    uvc_stream_exit_critical(flags);

    return ret;
}

#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
static void uvc_camera_stream_timer_handle(void *arg1)
{
    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)arg1;

    uvc_pro_config_t *pro_config = stream_handle->pro_config;
    LOGD("%s, %d, stream_handle:%p, pro_config:%p\n", __func__, __LINE__, stream_handle, pro_config);

    if (pro_config != NULL)
    {
        for (uint8_t i = 0; i < UVC_PORT_MAX; i++)
        {
            LOGI("port:%d:%u[%u %uKB]\n", i + 1,
                ((pro_config->frame_id[i] - pro_config->later_id[i]) / UVC_TIME_INTERVAL),
                pro_config->frame_id[i], (pro_config->curr_length[i] / 1024));
            pro_config->later_id[i] = pro_config->frame_id[i];
        }

        LOGI("packets[all:%u, err:%u]\n", pro_config->all_packet_num, pro_config->packet_err_num);
    }
}
#endif

static avdk_err_t uvc_camera_stream_check_mjpeg_config(uvc_param_t *param)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    if (param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, param->info->port, param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    bk_usb_hub_port_info *uvc_port_info = param->port_info;
    bk_cam_uvc_config_t *user_config = param->info;
    LOGD("%s, %d, port:%d\n", __func__, __LINE__, user_config->port);

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)uvc_port_info->usb_device_param_config;

    LOGD("PORT:0x%x\n", user_config->port);
    LOGD("VID:0x%x\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\n", uvc_device_param->device_bcd);
    uvc_device_param_config->vendor_id = uvc_device_param->vendor_id;
    uvc_device_param_config->product_id = uvc_device_param->product_id;

    uvc_device_param_config->format_index = uvc_device_param->format_index.mjpeg_format_index;
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
            uvc_device_param_config->frame_index = uvc_device_param->all_frame.mjpeg_frame[index].index;
            uvc_device_param_config->width = uvc_device_param->all_frame.mjpeg_frame[index].width;
            uvc_device_param_config->height = uvc_device_param->all_frame.mjpeg_frame[index].height;
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.mjpeg_frame[index].fps_num; i++)
        {
            LOGD("MJPEG fps:%d\r\n", uvc_device_param->all_frame.mjpeg_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.mjpeg_frame[index].fps[i] == user_config->fps)
            {
                uvc_device_param_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[i];
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[0];
                uvc_device_param_config->fps = user_config->fps;
                fps_flag = true;
            }
            break;
        }
    }

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_device_param_config->ep_desc = uvc_device_param->ep_desc;

    return AVDK_ERR_OK;
}

static avdk_err_t uvc_camera_stream_check_h264_config(uvc_param_t *param)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    if (param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, param->info->port, param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    bk_usb_hub_port_info *uvc_port_info = param->port_info;
    bk_cam_uvc_config_t *user_config = param->info;
    LOGD("%s, %d, port:%d\n", __func__, __LINE__, user_config->port);

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)uvc_port_info->usb_device_param_config;

    LOGD("PORT:0x%x\n", user_config->port);
    LOGD("VID:0x%x\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\n", uvc_device_param->device_bcd);
    uvc_device_param_config->vendor_id = uvc_device_param->vendor_id;
    uvc_device_param_config->product_id = uvc_device_param->product_id;

    uvc_device_param_config->format_index = uvc_device_param->format_index.h264_format_index;
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
            uvc_device_param_config->frame_index = uvc_device_param->all_frame.h264_frame[index].index;
            uvc_device_param_config->width = uvc_device_param->all_frame.h264_frame[index].width;
            uvc_device_param_config->height = uvc_device_param->all_frame.h264_frame[index].height;
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.h264_frame[index].fps_num; i++)
        {
            LOGD("H264 fps:%d\r\n", uvc_device_param->all_frame.h264_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.h264_frame[index].fps[i] == user_config->fps)
            {
                uvc_device_param_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[i];
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[0];
                uvc_device_param_config->fps = user_config->fps;
                fps_flag = true;
            }
            break;
        }
    }

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_device_param_config->ep_desc = uvc_device_param->ep_desc;

    return AVDK_ERR_OK;
}

static avdk_err_t uvc_camera_stream_check_h265_config(uvc_param_t *param)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    if (param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, param->info->port, param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    bk_usb_hub_port_info *uvc_port_info = param->port_info;
    bk_cam_uvc_config_t *user_config = param->info;
    LOGD("%s, %d, port:%d\n", __func__, __LINE__, user_config->port);

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)uvc_port_info->usb_device_param_config;

    LOGD("PORT:0x%x\n", user_config->port);
    LOGD("VID:0x%x\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\n", uvc_device_param->device_bcd);
    uvc_device_param_config->vendor_id = uvc_device_param->vendor_id;
    uvc_device_param_config->product_id = uvc_device_param->product_id;

    
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
            uvc_device_param_config->frame_index = uvc_device_param->all_frame.h265_frame[index].index;
            uvc_device_param_config->width = uvc_device_param->all_frame.h265_frame[index].width;
            uvc_device_param_config->height = uvc_device_param->all_frame.h265_frame[index].height;
            resolution_flag = true;
        }

        // iterate all support fps of current resolution
        for (int i = 0; i < uvc_device_param->all_frame.h265_frame[index].fps_num; i++)
        {
            LOGD("H265 fps:%d\r\n", uvc_device_param->all_frame.h265_frame[index].fps[i]);

            if (resolution_flag
                && uvc_device_param->all_frame.h265_frame[index].fps[i] == user_config->fps)
            {
                uvc_device_param_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[i];
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            // have adapt this resolution
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[0];
                uvc_device_param_config->fps = user_config->fps;
                fps_flag = true;
            }
            break;
        }
    }

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_device_param_config->ep_desc = uvc_device_param->ep_desc;

    return AVDK_ERR_OK;
}
static avdk_err_t uvc_camera_stream_check_yuv_config(uvc_param_t *param)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    uint8_t format_flag = false;

    if (param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, param->info->port, param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    bk_usb_hub_port_info *uvc_port_info = param->port_info;
    bk_cam_uvc_config_t *user_config = param->info;
    LOGI("%s, %d, port:%d, width:%d, height:%d, fps:%d\n", __func__, __LINE__, user_config->port, user_config->width, user_config->height, user_config->fps);

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    bk_uvc_config_t *uvc_device_param_config = (bk_uvc_config_t *)uvc_port_info->usb_device_param_config;

    LOGD("PORT:0x%x\n", user_config->port);
    LOGD("VID:0x%x\n", uvc_device_param->vendor_id);
    LOGD("PID:0x%x\n", uvc_device_param->product_id);
    LOGD("BCD:0x%x\n", uvc_device_param->device_bcd);
    uvc_device_param_config->vendor_id = uvc_device_param->vendor_id;
    uvc_device_param_config->product_id = uvc_device_param->product_id;

    uvc_device_param_config->format_index = uvc_device_param->format_index.yuv_format_index;
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
            uvc_device_param_config->frame_index = uvc_device_param->all_frame.yuv_frame[index].index;
            uvc_device_param_config->width = uvc_device_param->all_frame.yuv_frame[index].width;
            uvc_device_param_config->height = uvc_device_param->all_frame.yuv_frame[index].height;
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.yuv_frame[index].fps_num; i++)
        {
            LOGD("YUV fps:%d\r\n", uvc_device_param->all_frame.yuv_frame[index].fps[i]);
            uvc_device_param_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[i];
        }

        if (resolution_flag)
        {
            if (fps_flag == false)
            {
                user_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[0];
                uvc_device_param_config->fps = user_config->fps;
                fps_flag = true;
            }
        }

        if (resolution_flag)
        {
            break;
        }
    }

    if (format_flag == false)
    {
        LOGE("%s, not support this format:%d\r\n", __func__, user_config->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (resolution_flag == false)
    {
        LOGE("%s, not support this resolution:%dX%d\r\n", __func__, user_config->width, user_config->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_device_param_config->ep_desc = uvc_device_param->ep_desc;

    return AVDK_ERR_OK;
}


static avdk_err_t uvc_camera_stream_check_config(uvc_param_t *param)
{
    avdk_err_t ret = AVDK_ERR_OK;

    bk_image_format_t img_format = param->info->format;

    LOGI("%s, %d, format:%d\n", __func__, __LINE__, img_format);

    if (img_format == BK_IMAGE_FORMAT_MJPEG)
    {
        ret = uvc_camera_stream_check_mjpeg_config(param);
    }
    else if (img_format == BK_IMAGE_FORMAT_H264)
    {
        ret = uvc_camera_stream_check_h264_config(param);
    }
    else if (img_format == BK_IMAGE_FORMAT_H265)
    {
        ret = uvc_camera_stream_check_h265_config(param);
    }
    else if (img_format == BK_IMAGE_FORMAT_YUV)
    {
        ret = uvc_camera_stream_check_yuv_config(param);
    }
    else
    {
        LOGE("%s, not support this format:%d\r\n", __func__, img_format);
        ret = AVDK_ERR_UNSUPPORTED;
    }

    return ret;
}

static avdk_err_t uvc_camera_stream_packet_urb(uvc_param_t *uvc_param)
{
    LOGD("%s, %d\n", __func__, __LINE__);
    struct usbh_video *uvc_device = NULL;
    //struct usbh_hubport *hport = NULL;
    struct usbh_urb *urb = uvc_param->urb;

    if (uvc_param->stream_state != UVC_STREAM_STREAMING_STATE)
    {
        LOGE("%s, param or state error:%d...\r\n", __func__, uvc_param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    uvc_device = (struct usbh_video *)(uvc_param->port_info->usb_device);
    //hport = uvc_param->port_info->hport;
    urb->pipe = (usbh_pipe_t)(uvc_device->isoin);
    if (urb->pipe == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
    }
    urb->complete = (usbh_complete_callback_t)uvc_camera_stream_receive_complete_callback;
    urb->arg = (void *)uvc_param;
    urb->timeout = 0;//150us

    return AVDK_ERR_OK;
}

static void uvc_camera_stream_receive_complete_callback(void *pCompleteParam, int nbytes)
{
    UVC_PACKET_PUSH_START();
    struct usbh_urb *urb = NULL;//, *new_urb = NULL;
    uvc_param_t *uvc_param = (uvc_param_t *)pCompleteParam;
    int ret = BK_FAIL;
    uvc_stream_state_t stream_state;
    uint32_t flags = uvc_stream_enter_critical();
    urb = uvc_param->urb;
    uvc_param->urb = NULL;
    stream_state = uvc_param->stream_state;
    uvc_stream_exit_critical(flags);

    if (urb == NULL)
    {
        LOGD("%s, %d, %p\n", __func__, __LINE__, pCompleteParam);
        goto out;
    }

    if (nbytes < 0)
    {
        //LOGW("%s, urb:%p, nbytes:%d\n", __func__, urb, nbytes);
        urb->errorcode = nbytes;
    }

    uvc_camera_urb_push(urb);

    if (stream_state != UVC_STREAM_STREAMING_STATE)
    {
        LOGI("[%d]%s, %d, state:%d\n", uvc_param->info->port, __func__, __LINE__, uvc_param->stream_state);
        rtos_set_semaphore(&uvc_param->sem);
        goto out;
    }

    urb = uvc_camera_urb_malloc();

    if (urb)
    {
        flags = uvc_stream_enter_critical();
        uvc_param->urb = urb;
        uvc_stream_exit_critical(flags);
        ret = uvc_camera_stream_packet_urb(uvc_param);
        if (ret != AVDK_ERR_OK)
        {
            flags = uvc_stream_enter_critical();
            uvc_param->urb = NULL;
            uvc_stream_exit_critical(flags);
            uvc_camera_urb_free(urb);
            goto out;
        }

        ret = bk_usbh_hub_dev_request_data(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->urb);
        if (ret != AVDK_ERR_OK)
        {
            flags = uvc_stream_enter_critical();
            uvc_param->urb = NULL;
            uvc_stream_exit_critical(flags);
            uvc_camera_urb_free(urb);
            if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, uvc_param->info->port) != AVDK_ERR_OK)
            {
                LOGE("%s, %d send failed...\n", __func__, __LINE__);
            }
        }
    }
    else
    {
        LOGD("%s, %d, urb is NULL, send UVC_DATA_REQUEST_IND\n", __func__, __LINE__);
        if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, uvc_param->info->port) != AVDK_ERR_OK)
        {
            LOGE("%s, %d send failed...\r\n", __func__, __LINE__);
        }
    }

out:

    UVC_PACKET_PUSH_END();
}

static avdk_err_t uvc_camera_stream_start_handle(uvc_stream_handle_t *handle, uint32_t param)
{
    avdk_err_t ret = AVDK_ERR_OK;
    struct usbh_urb *urb = NULL;
    frame_buffer_t *new_frame = NULL;
    uvc_param_t *uvc_param = &handle->camera[param - 1];
    bk_image_format_t format = uvc_param->info->format;
    uvc_pro_config_t *pro_config = handle->pro_config;

    if (uvc_param->stream_state != UVC_STREAM_CONNECTED_STATE)
    {
        LOGE("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, uvc_param->info->port, uvc_param->stream_state);
        return AVDK_ERR_UNSUPPORTED;
    }

    LOGI("%s, %d, port_info:%p\r\n", __func__, __LINE__, uvc_param->port_info);
    // struct usbh_video *video_class = (struct usbh_video *)(uvc_param->port_info->usb_device);
    // bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_param->port_info->usb_device_param;
    // ret = usbh_hport_activate_epx(&video_class->isoin, video_class->hport, (struct usb_endpoint_descriptor *)uvc_device_param->ep_desc);
    // if (ret != AVDK_ERR_OK)
    // {
    //     LOGE("%s, %d, activate ep failed, port:%d\n", __func__, __LINE__, uvc_param->info->port);
    //     goto out;
    // }

    uvc_param->stream_state = UVC_STREAM_CONFIGING_STATE;

    ret = uvc_camera_stream_check_config(uvc_param);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, not support this solution, please retry...\r\n", __func__);
        goto out;
    }

    if (uvc_param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGI("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, uvc_param->info->port, uvc_param->stream_state);
        goto out;
    }

    ret = bk_usbh_hub_port_dev_open(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->port_info);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        goto out;
    }

    if (uvc_param->frame == NULL)
    {
        if (format == BK_IMAGE_FORMAT_MJPEG)
        {
            new_frame = handle->callback->frame_malloc(BK_IMAGE_FORMAT_MJPEG, UVC_FRAME_SIZE);
        }
        else if (format == BK_IMAGE_FORMAT_H264 || format == BK_IMAGE_FORMAT_H265)
        {
            new_frame = handle->callback->frame_malloc(BK_IMAGE_FORMAT_H264, UVC_FRAME_SIZE);
        }
        else if (format == BK_IMAGE_FORMAT_YUV)
        {
            new_frame = handle->callback->frame_malloc(format, uvc_param->info->width * uvc_param->info->height * 2);
        }
        else
        {
            LOGE("%s, not support this format:%d\r\n", __func__, format);
        }

        if (new_frame == NULL)
        {
            LOGE("%s, %d\r\n", __func__, __LINE__);
            ret = AVDK_ERR_NOMEM;
            goto out;
        }

        new_frame->fmt = format;
        new_frame->width = uvc_param->info->width;
        new_frame->height = uvc_param->info->height;
        new_frame->sequence = pro_config->frame_id[uvc_param->info->port - 1]++;
        uvc_param->frame = new_frame;
    }

    if (uvc_param->urb == NULL)
    {
        urb = uvc_camera_urb_malloc();
        if (urb == NULL)
        {
            ret = AVDK_ERR_NOMEM;
            goto out;
        }

        uvc_param->urb = urb;
    }

    if (uvc_param->stream_state != UVC_STREAM_CONFIGING_STATE)
    {
        LOGI("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, uvc_param->info->port, uvc_param->stream_state);
        goto out;
    }

    // make sure transmission mode
    bk_uvc_config_t *uvc_config = (bk_uvc_config_t *)uvc_param->port_info->usb_device_param_config;
    pro_config->transfer_bulk[uvc_param->info->port - 1] = ((uvc_config->ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_BULK_TRANSFER) ? true : false;
    pro_config->max_packet_size[uvc_param->info->port - 1] = uvc_config->ep_desc->wMaxPacketSize > 1024 ? 1024 : uvc_config->ep_desc->wMaxPacketSize;
    LOGI("/*****port:%d, transmission mode:%s, max_packet_zise:%d*****/\r\n", uvc_param->info->port, pro_config->transfer_bulk[uvc_param->info->port - 1] == 1 ? "BULK" : "ISO",
            pro_config->max_packet_size[uvc_param->info->port - 1]);

    // config urb
    uvc_param->stream_state = UVC_STREAM_STREAMING_STATE;
    ret = uvc_camera_stream_packet_urb(uvc_param);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        goto out;
    }

    LOGI("device_index:%d, port:%d\n", uvc_param->port_info->device_index, uvc_param->info->port);

    // requeset uvc data
    rtos_clear_event_flags(&handle->handle, UVC_PROCESS_TASK_START_BIT);
    rtos_set_event_flags(&handle->handle, UVC_PROCESS_TASK_START_BIT);
    ret = bk_usbh_hub_dev_request_data(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->urb);

out:
    if (ret != AVDK_ERR_OK)
    {
        LOGI("[%d]%s, %d, ret:%d\r\n", param, __func__, __LINE__, ret);

        if (uvc_param->frame)
        {
            handle->callback->frame_complete(uvc_param->info->port, uvc_param->frame->fmt, uvc_param->frame, ret);
            uvc_param->frame = NULL;
        }

        if (uvc_param->urb)
        {
            uvc_camera_urb_free(uvc_param->urb);
            uvc_param->urb = NULL;
        }

        uvc_param->stream_state = UVC_STREAM_CLOSED_STATE;
    }

    rtos_set_event_flags(&handle->handle, UVC_STREAM_START_BIT);

    return ret;
}

static avdk_err_t uvc_camera_stream_stop_handle(uvc_stream_handle_t *handle, uint32_t param)
{
    uvc_param_t *uvc_param = &handle->camera[param - 1];
    struct usbh_urb *urb = NULL;
    frame_buffer_t *frame = NULL;
    bk_cam_uvc_config_t *info = NULL;
    bk_usb_hub_port_info *port_info = NULL;
    uvc_stream_state_t stream_state;
    uint32_t flags = uvc_stream_enter_critical();
    stream_state = uvc_param->stream_state;

    if (stream_state != UVC_STREAM_STREAMING_STATE)
    {
        uvc_stream_exit_critical(flags);
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, uvc_param->info->port, uvc_param->stream_state);
        goto out;
    }

    uvc_param->stream_state = UVC_STREAM_CLOSING_STATE;
    uvc_stream_exit_critical(flags);

    if (rtos_get_semaphore(&uvc_param->sem, 1000) != BK_OK)
    {
        LOGE("%s, %d timeout\n", __func__, __LINE__);
    }

    flags = uvc_stream_enter_critical();
    port_info = uvc_param->port_info;
    info = uvc_param->info;
    urb = uvc_param->urb;
    frame = uvc_param->frame;
    uvc_param->urb = NULL;
    uvc_param->frame = NULL;
    uvc_param->info = NULL;
    uvc_param->stream_state = UVC_STREAM_CLOSED_STATE;
    uvc_stream_exit_critical(flags);

    if (port_info && info)
    {
        LOGI("%s, %d, port:%d, device_index:%d\n", __func__, __LINE__, info->port, port_info->device_index);
        bk_usbh_hub_port_dev_close(info->port, port_info->device_index, port_info);
    }

out:

    if (urb)
    {
        uvc_camera_urb_free(urb);
    }

    if (frame && info)
    {
        handle->callback->frame_complete(info->port, frame->fmt, frame, AVDK_ERR_INVAL);
    }

    if (info)
    {
        os_free(info);
    }

    rtos_set_event_flags(&handle->handle, UVC_STREAM_STOP_BIT);

    return AVDK_ERR_OK;
}

static void uvc_camera_stream_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)arg;

    LOGI("%s, device_index:%d, port:%d\n", __func__, port_info->device_index, port_info->port_index);

    if (stream_handle == NULL)
    {
        LOGW("%s, %d, stream_handle is NULL\r\n", __func__, __LINE__);
        return;
    }

    uvc_param_t *uvc_param = &stream_handle->camera[port_info->port_index - 1];
    uint32_t flags = uvc_stream_enter_critical();
    uvc_param->port_info = port_info;
    uvc_param->stream_state = UVC_STREAM_CONNECTED_STATE;
    uvc_stream_exit_critical(flags);

    uvc_stream_task_send_msg(UVC_CONNECT_IND, (uint32_t)port_info->port_index);
}

void uvc_camera_stream_disconnect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)arg;
    uint32_t port = port_info->port_index;

    LOGI("%s, %d, stream_handle:%p, port:%p\n", __func__, __LINE__, stream_handle, port);

    uvc_param_t *uvc_param = &stream_handle->camera[port - 1];

    uint32_t flags = uvc_stream_enter_critical();
    uvc_param->stream_state = UVC_STREAM_DISCONNECTED_STATE;
    uvc_stream_exit_critical(flags);
    //uvc_param->port_info = NULL;

    uvc_stream_task_send_msg(UVC_DISCONNECT_IND, (uint32_t)port);
}

static void uvc_camera_stream_connect_handle(uvc_stream_handle_t *handle, uint32_t param)
{
    uvc_param_t *uvc_param = &handle->camera[param - 1];

    if (uvc_param->stream_state != UVC_STREAM_CONNECTED_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, param, uvc_param->stream_state);
        return;
    }

    if (handle->callback && handle->callback->state_change_cb)
    {
        handle->callback->state_change_cb(UVC_CONNECTED, handle->callback->user_data);
    }

    if (uvc_param->info)
    {
        uvc_camera_stream_start_handle(handle, param);
    }

    LOGI("[%d]%s, %d\n", param, __func__, __LINE__);
}

static void uvc_camera_stream_disconnect_handle(uvc_stream_handle_t *handle, uint32_t param)
{
    uint8_t port = (uint8_t)param;
    uvc_param_t *uvc_param = &handle->camera[port - 1];

    if (uvc_param->stream_state == UVC_STREAM_DISCONNECTED_STATE)
    {
        LOGW("%s, %d, port:%d, state:%d\r\n", __func__, __LINE__, port, uvc_param->stream_state);
        if (handle->callback && handle->callback->state_change_cb)
        {
            handle->callback->state_change_cb(UVC_DISCONNECTED, handle->callback->user_data);
        }
    }
}

static avdk_err_t uvc_camera_stream_data_request_retry_handle(uvc_param_t *param, int value)
{
    avdk_err_t ret = AVDK_ERR_OK;
    uint8_t port = param->info->port;
    switch (-value)
    {
        case EBUSY:
            LOGD("%s port:%d, Urb is EBUSY\r\n", __func__, port);
            ret = uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, port);
            break;
        case ENODEV:
            LOGD("%s port:%d, ENODEV Please check device connect\r\n", __func__, port);
            ret = BK_FAIL;
            break;
        case EINVAL:
            LOGD("%s port:%d, EINVAL Please check pipe or urb\r\n", __func__, port);
            //bk_usb_drv_send_msg(USB_DRV_VIDEO_START, id);
            ret = BK_FAIL;
            break;
        case ESHUTDOWN:
            LOGD("%s port:%d, ESHUTDOWN Check device Disconnect\r\n", __func__, port);
            ret = BK_FAIL;
            break;
        case ETIMEDOUT:
            LOGD("%s port:%d, ETIMEDOUT Timeout wait\r\n", __func__, port);
            ret = uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, port);
            break;
        default:
            LOGD("%s port:%d, Fail to submit urb:%d\r\n", __func__, port, value);
            ret = BK_FAIL;
            break;
    }

    return ret;
}

static void uvc_camera_stream_data_request_handle(uvc_stream_handle_t *stream_handle, uint32_t param)
{
    uint8_t port = (uint8_t)param;
    struct usbh_urb *new_urb = NULL;
    struct usbh_urb *failed_urb = NULL;
    uvc_param_t *uvc_param = &stream_handle->camera[port - 1];
    avdk_err_t ret = AVDK_ERR_OK;
    uint32_t flags;

    do
    {
        flags = uvc_stream_enter_critical();
        if (uvc_param->stream_state != UVC_STREAM_STREAMING_STATE)
        {
            uvc_stream_exit_critical(flags);

            LOGW("[%d]%s, %d stream have stoped...\r\n", uvc_param->info->port, __func__, __LINE__);
            rtos_set_semaphore(&uvc_param->sem);
            break;
        }
        uvc_stream_exit_critical(flags);

        flags = uvc_stream_enter_critical();
        if (uvc_param->urb == NULL)
        {
            uvc_stream_exit_critical(flags);
            new_urb = uvc_camera_urb_malloc();

            if (new_urb)
            {
                // apply data
                flags = uvc_stream_enter_critical();
                uvc_param->urb = new_urb;
                uvc_stream_exit_critical(flags);
            }
            else
            {
                // malloc fail, retry
                stream_handle->pro_config->packet_error[uvc_param->info->port - 1] = true;
                rtos_delay_milliseconds(5);
                LOGD("%s, %d retry.....\r\n", __func__, __LINE__);
                if (uvc_stream_task_send_msg(UVC_DATA_REQUEST_IND, param) != AVDK_ERR_OK)
                {
                    LOGW("%s, %d send fail.\r\n", __func__, __LINE__);
                }
                break;
            }
        }
        else
        {
            uvc_stream_exit_critical(flags);
        }

        ret = uvc_camera_stream_packet_urb(uvc_param);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s, %d, port:%d, disconnect.....\r\n", __func__, __LINE__, uvc_param->info->port);
            break;
        }

        ret = bk_usbh_hub_dev_request_data(uvc_param->info->port, uvc_param->port_info->device_index, uvc_param->urb);
        {
            if (ret == AVDK_ERR_OK)
            {
                break;
            }
            else if (ret == AVDK_ERR_GENERIC)
            {
                LOGW("%s, %d, port:%d, disconnect.....\r\n", __func__, __LINE__, uvc_param->info->port);
                break;
            }
            else
            {
                ret = uvc_camera_stream_data_request_retry_handle(uvc_param, ret);
                if (ret != AVDK_ERR_OK)
                {
                    LOGW("%s, %d, port:%d, retry error:%d.....\r\n", __func__, __LINE__, uvc_param->info->port, ret);
                    break;
                }
            }
        }

    }
    while (0);


    if (ret != BK_OK)
    {
        flags = uvc_stream_enter_critical();
        failed_urb = uvc_param->urb;
        uvc_param->urb = NULL;
        uvc_stream_exit_critical(flags);

        if (failed_urb)
        {
            uvc_camera_urb_free(failed_urb);
            new_urb = NULL;
        }
    }
}

static void uvc_camera_stream_send_eof_handle(uvc_stream_handle_t *stream_handle, uint32_t param)
{
    uvc_eof_param_t *eof_param = (uvc_eof_param_t *)param;
    frame_buffer_t *frame = (frame_buffer_t *)eof_param->frame;

    stream_handle->callback->frame_complete(eof_param->port, frame->fmt, frame, AVDK_ERR_OK);
    os_free(eof_param);
}

static void uvc_camera_stream_task_main(beken_thread_arg_t data)
{
    avdk_err_t ret = AVDK_ERR_OK;

    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)data;
    uint32_t flags = uvc_stream_enter_critical();
    stream_handle->stream_enable = true;
    uvc_stream_exit_critical(flags);
    xEventGroupSetBits(stream_handle->handle, UVC_STREAM_TASK_ENABLE_BIT);

    while (1)
    {
        uvc_msg_t msg;
        ret = rtos_pop_from_queue(&stream_handle->stream_queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret == AVDK_ERR_OK)
        {
            LOGD("%s, %d, event:%d\r\n", __func__, __LINE__, msg.event);
            switch (msg.event)
            {
                case UVC_CONNECT_IND:
                    uvc_camera_stream_connect_handle(stream_handle, msg.param);
                    break;

                case UVC_DISCONNECT_IND:
                    uvc_camera_stream_disconnect_handle(stream_handle, msg.param);
                    break;

                case UVC_DATA_REQUEST_IND:
                    uvc_camera_stream_data_request_handle(stream_handle, msg.param);
                    break;

                case UVC_EOF_IND:
                    uvc_camera_stream_send_eof_handle(stream_handle, msg.param);
                    break;

                case UVC_STREAM_START_IND:
                    uvc_camera_stream_start_handle(stream_handle, msg.param);
                    break;

                case UVC_STREAM_STOP_IND:
                    uvc_camera_stream_stop_handle(stream_handle, msg.param);
                    break;

                case UVC_EXIT_IND:
                    goto out;
                    break;

                default:
                    break;
            }
        }
    }

out:
    LOGI("%s, exit\r\n", __func__);
    stream_handle->stream_thread = NULL;
    rtos_set_event_flags(&stream_handle->handle, UVC_STREAM_TASK_DISABLE_BIT);
    rtos_delete_thread(NULL);
}

static avdk_err_t uvc_camera_stream_task_deinit(uvc_stream_handle_t *stream_handle)
{
    if (stream_handle == NULL)
    {
        return AVDK_ERR_OK;
    }

    // free all camera resources
    for (uint8_t i = 0; i < UVC_PORT_MAX; i++)
    {
        if (stream_handle->camera[i].urb)
        {
            uvc_camera_urb_free(stream_handle->camera[i].urb);
            stream_handle->camera[i].urb = NULL;
        }

        if (stream_handle->camera[i].frame)
        {
            stream_handle->callback->frame_complete(i, stream_handle->camera[i].frame->fmt, stream_handle->camera[i].frame, AVDK_ERR_INVAL);
            stream_handle->camera[i].frame = NULL;
        }

        if (stream_handle->camera[i].info)
        {
            os_free(stream_handle->camera[i].info);
            stream_handle->camera[i].info = NULL;
        }
    }

    return AVDK_ERR_OK;
}

int uvc_camera_stream_check_frame_buffer_sof_eof_mask(frame_buffer_t *frame)
{
    int ret = AVDK_ERR_GENERIC;
    uint8_t *data = frame->frame;
    uint32_t length = frame->length;

    switch (frame->fmt)
    {
        case BK_IMAGE_FORMAT_MJPEG:
            if (data[0] == 0xFF && data[1] == 0xD8)
            {
                for (uint32_t i = length - 1; i > length - 20; i--)
                {
                    if (data[i - 1] == 0xFF && data[i] == 0xD9)
                    {
                        ret = i + 1;
                        break;
                    }
                }
            }
            break;

        case BK_IMAGE_FORMAT_H264:
        case BK_IMAGE_FORMAT_H265:
            for (uint32_t i = 0; i < 20; i++)
            {
                if ((data[i] == 0x00 && data[i + 1] == 0x00
                    && data[i + 2] == 0x00 && data[i + 3] == 0x01)
                || (data[i] == 0x00 && data[i + 1] == 0x00
                    && data[i + 2] == 0x01))
                {
                    ret = AVDK_ERR_OK;
                    break;
                }
            }
            break;

        default:
            ret = AVDK_ERR_OK;
            break;
    }

    return ret;
}

static void uvc_camera_stream_eof_handle(uvc_stream_handle_t *stream_handle, uvc_param_t *uvc_param)
{
    UVC_PACKET_EOF_START();
    uvc_pro_config_t *pro_config = stream_handle->pro_config;
    uint8_t index = uvc_param->info->port - 1;
    frame_buffer_t *new_frame = NULL, *curr_frame_buffer = uvc_param->frame;
    uint32_t sequence = pro_config->frame_id[index];

    if (pro_config->packet_error[index])
    {
        LOGD("%s, %d, length:%d\n", __func__, __LINE__, curr_frame_buffer->length);
        pro_config->packet_error[index] = false; // clear packet_error flag
        curr_frame_buffer->length = 0;
        goto out;
    }

    int check_length = uvc_camera_stream_check_frame_buffer_sof_eof_mask(curr_frame_buffer);

    if (check_length < 0)
    {
        LOGW("%s, %d, not match sof eof mask, length:%d\r\n", __func__, __LINE__, curr_frame_buffer->length);
        curr_frame_buffer->length = 0;
        goto out;
    }
    else if (check_length > 0)
    {
        curr_frame_buffer->length = check_length;
    }
    else
    {
        // h264/h265
    }

#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
    pro_config->curr_length[index] = curr_frame_buffer->length;
#endif

    LOGD("%s, %d, length:%d, fmt:%d\r\n", __func__, __LINE__, curr_frame_buffer->length, curr_frame_buffer->fmt);
    pro_config->frame_id[index]++;

    if (uvc_param->info->drop_num > 0)
    {
        uvc_param->info->drop_num--;
        LOGD("[%d]%s, drop_num:%d\r\n", index, __func__, uvc_param->info->drop_num);
    }
    else
    {
        new_frame = stream_handle->callback->frame_malloc(curr_frame_buffer->fmt, curr_frame_buffer->size);
        if (new_frame)
        {
            LOGD("%s, %d, malloc %p-%p\n", __func__, __LINE__, new_frame, new_frame->frame);
            new_frame->fmt = curr_frame_buffer->fmt;
            new_frame->width = curr_frame_buffer->width;
            new_frame->height = curr_frame_buffer->height;
            new_frame->length = 0;
            curr_frame_buffer->sequence = sequence;
            if (1)
            {
                uvc_eof_param_t *eof_param = (uvc_eof_param_t *)os_malloc(sizeof(uvc_eof_param_t));
                if (eof_param == NULL)
                {
                    LOGE("%s, %d, send eof message fail\r\n", __func__, __LINE__);
                    stream_handle->callback->frame_complete(uvc_param->info->port, uvc_param->info->format, curr_frame_buffer, AVDK_ERR_OK);
                }
                else
                {
                    eof_param->frame = (uint8_t *)curr_frame_buffer;
                    eof_param->port = uvc_param->info->port;

                    if (uvc_stream_task_send_msg(UVC_EOF_IND, (uint32_t)eof_param) != AVDK_ERR_OK)
                    {
                        LOGE("%s, %d, send eof message fail\r\n", __func__, __LINE__);
                        stream_handle->callback->frame_complete(uvc_param->info->port, uvc_param->info->format, curr_frame_buffer, AVDK_ERR_OK);
                        os_free(eof_param);
                    }
                }
            }
            else
            {
                stream_handle->callback->frame_complete(uvc_param->info->port, uvc_param->info->format, curr_frame_buffer, AVDK_ERR_OK);
            }

            uvc_param->frame = new_frame;
        }
    }

    if (new_frame == NULL)
    {
        LOGD("%s, %d malloc frame fail, length:%d, sequence:%d\n", __func__, __LINE__, curr_frame_buffer->length, sequence);
        curr_frame_buffer->length = 0;
    }

out:
    UVC_PACKET_EOF_END();
}

bk_err_t uvc_camera_stream_check_frame_buffer_length(frame_buffer_t *frame, uint32_t total_length)
{
    if (frame->size <= total_length)
    {
        return BK_FAIL;
    }

    return BK_OK;
}

static int uvc_camera_stream_check_h264_h265_header(uint8_t *data, uint32_t length)
{
    int ret = -1;

    if (length < 4)
    {
        return ret;
    }

    for (uint32_t i = 0; i < 20; i++)
    {
        if ((data[i] == 0x00 && data[i + 1] == 0x00)
        && ((data[i + 2] == 0x00 && data[i + 3] == 0x01)
            || (data[i + 2] == 0x01)))
        {
            ret = i;
            break;
        }
    }
    return ret;
}

static void uvc_camera_stream_packet_process(uvc_stream_handle_t *stream_handle, uvc_param_t *uvc_param, uint8_t *payload, uint32_t payload_len)
{
    uvc_pro_config_t *pro_config = stream_handle->pro_config;
    frame_buffer_t *curr_frame_buffer = uvc_param->frame;
    uint8_t *data = NULL;
    uint8_t header_info = 0;
    uint8_t header_len = 0;
    uint8_t flag_zlp = 0;
    uint8_t flag_lstp = 0;
    uint8_t index = uvc_param->info->port - 1;
#if 0
    uint8_t variable_offset = 0;
#endif
    uint32_t data_len = 0;
    uint32_t bulk_req_len = 0;

    uint8_t bulk_trans = pro_config->transfer_bulk[index];

    // Handle bulk transfer
    if (bulk_trans)
    {
        bulk_req_len = pro_config->max_packet_size[index];
        if (payload_len == 0)
        {
            flag_zlp = 1;
            LOGD("%s, payload_len == 0\r\n", __func__);
        }
        else
        {
            if (bulk_req_len != payload_len)
            {
                flag_lstp = 1;
            }
        }
    }
    else if (payload_len == 0)
    {
        // Ignore empty payload transfers (for ISO transfer)
        UVC_PACKET_EMPTY_START();
        UVC_PACKET_EMPTY_END();
        return;
    }

    LOGD("length:%d, port:%d\n", payload_len, uvc_param->info->port);

#if 0
    if (payload_len >= 13)
    {
        LOGI("%02x %02x %02x %02x %02x %02x %02x %02x %02x length:%d\n", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5], payload[6], payload[12], payload[13], payload_len);
    }
#endif
    /********************* Process header *******************/
    if (!flag_zlp)
    {
        LOGD("zlp=%d, lstp=%d, payload_len=%d, first=0x%02x, second=0x%02x\r\n", flag_zlp, flag_lstp, payload_len, payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX], payload_len > 1 ? payload[UVC_PAYLOAD_HEADER_INFO_INDEX] : 0);

        // Check if it's a valid header
        if (payload_len >= payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX]
            && (payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX] == UVC_HEADER_LEN_DEFAULT
                || (payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX] == UVC_HEADER_LEN_BASIC && !(payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_FLAG_PTS_SCR))
                || (payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX] == UVC_HEADER_LEN_WITH_PTS && !(payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_FLAG_SCR))
                || (payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX] == UVC_HEADER_LEN_WITH_SCR && (payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_FLAG_SCR) && !(payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_FLAG_PTS)))
            && (payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_FLAG_STREAM_PRESENT) && !(payload[UVC_PAYLOAD_HEADER_INFO_INDEX] & UVC_HEADER_EXTENSION_MASK)
            && (!bulk_trans || ((payload[payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX]] == JPEG_MARKER_BYTE0) && (payload[payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX] + 1] == JPEG_MARKER_BYTE1)))
           )
        {
            header_len = payload[UVC_PAYLOAD_HEADER_LENGTH_INDEX];
            data_len = payload_len - header_len;
            /* checking the end-of-header */
#if 0
            variable_offset = 2;
#endif
            header_info = payload[UVC_PAYLOAD_HEADER_INFO_INDEX];

            LOGD("header=%u info=0x%02x, payload_len = %u\r\n", header_len, header_info, payload_len);

            // Check error bit
            if (header_info & UVC_HEADER_FLAG_ERROR)
            {
                UVC_PACKET_ERROR_START();
                // LOGW("bad packet: %02x, head_len:%d error bit set\r\n", header_info, header_len);
                pro_config->packet_error[index] = true;
#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
                pro_config->packet_err_num++;
#endif
                UVC_PACKET_ERROR_END();
                return;
            }
        }
        else
        {
            LOGD("reassembling %u + %u\r\n", curr_frame_buffer->length, payload_len);
            data_len = payload_len;
        }
    }

    // Handle header info change
    if (header_info)
    {
        if (pro_config->head_bit0[index] != (header_info & 1))
        {
            UVC_PACKET_NEW_FRAME_BIT_START();
            if (uvc_param->frame->length > 0)
            {
                uvc_camera_stream_eof_handle(stream_handle, uvc_param);
                curr_frame_buffer = uvc_param->frame;
            }

            pro_config->head_bit0[index] = (header_info & 1);
            UVC_PACKET_NEW_FRAME_BIT_END();
        }

#if 0 // do not explain pts and last_scr
        if (header_info & (1 << 2))
        {
            pts = DW_TO_INT(payload + variable_offset);
            variable_offset += 4;
        }

        if (header_info & (1 << 3))
        {
            last_scr = DW_TO_INT(payload + variable_offset);
            variable_offset += 6;
        }
#endif
    }

    /********************* Process data *****************/
    if (data_len >= 1)
    {
        data = payload + header_len;

        // Copy data to frame buffer
        if (data_len >= 1 && !pro_config->packet_error[index])
        {
            // Fix logic bug: only set error flag when buffer space is insufficient
            if (uvc_camera_stream_check_frame_buffer_length(curr_frame_buffer, (curr_frame_buffer->length + data_len)) != BK_OK)
            {
                LOGE("Frame buffer overflow, please check the value of marco <UVC_FRAME_SIZE>: current=%d, need=%d\n", curr_frame_buffer->length, data_len);
                curr_frame_buffer->length = 0; // add this line to clear the frame buffer
                pro_config->packet_error[index] = true;
            }
            else
            {
                // check h264/h265 header in the first packet
                if (curr_frame_buffer->length == 0
                    && (uvc_param->info->format == BK_IMAGE_FORMAT_H264 || uvc_param->info->format == BK_IMAGE_FORMAT_H265))
                {
                    // check h264/h265 header
                    uint32_t header_offset = uvc_camera_stream_check_h264_h265_header(data, data_len);
                    if (header_offset > 0)
                    {
                        data_len -= header_offset;
                        data = data + header_offset;
                    }
                }

                UVC_PACKET_DMA_START();
                LOGD("uvc payload = %02x %02x...%02x %02x\n", payload[header_len], payload[header_len + 1], payload[payload_len - 2], payload[payload_len - 1]);
                os_memcpy(curr_frame_buffer->frame + curr_frame_buffer->length, data, data_len);
                curr_frame_buffer->length += data_len;
                UVC_PACKET_DMA_END();
            }
        }
    }
    else
    {
        UVC_PACKET_EMPTY_START();
        UVC_PACKET_EMPTY_END();
    }

    // Handle EOF condition
    if (((header_info & (1 << 1)) && !bulk_trans) || flag_zlp || flag_lstp)
    {
        UVC_PACKET_EOF_BIT_START();
        LOGD("eof:%d, bulk_trans:%d, flag_zlp:%d, flag_lstp:%d\r\n", header_info & 0x2, bulk_trans, flag_zlp, flag_lstp);

        // Publish complete frame
        if (curr_frame_buffer->length != 0)
        {
            if (curr_frame_buffer->fmt == BK_IMAGE_FORMAT_MJPEG)
            {
                // Check SOF and EOF markers for JPEG frame
                if (uvc_camera_stream_check_frame_buffer_sof_eof_mask(curr_frame_buffer) > 0)
                {
                    uvc_camera_stream_eof_handle(stream_handle, uvc_param);
                }
                else
                {
                    LOGD("[EOF_bit]id:%d, %02x-%02x-%02x-%02x-%02x-%02x\r\n", index,
                         curr_frame_buffer->frame[0],
                         curr_frame_buffer->frame[1],
                         curr_frame_buffer->frame[2],
                         curr_frame_buffer->frame[3],
                         curr_frame_buffer->frame[curr_frame_buffer->length - 2],
                         curr_frame_buffer->frame[curr_frame_buffer->length - 1]);
                }
            }
            else
            {
                // Directly handle EOF for other formats
                uvc_camera_stream_eof_handle(stream_handle, uvc_param);
            }
        }

        pro_config->packet_error[index] = false;
        UVC_PACKET_EOF_BIT_END();
    }
}

static void uvc_camera_process_task_main(beken_thread_arg_t data)
{
    struct usbh_urb *urb = NULL;
    uint8_t *payload = NULL;
    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)data;
    uvc_pro_config_t *pro_config = stream_handle->pro_config;
    uint32_t flags = uvc_stream_enter_critical();

    stream_handle->pro_enable = true;
    uvc_stream_exit_critical(flags);
    rtos_set_event_flags(&stream_handle->handle, UVC_PROCESS_TASK_ENABLE_BIT);
    rtos_wait_for_event_flags(&stream_handle->handle, UVC_PROCESS_TASK_START_BIT, true, true, BEKEN_WAIT_FOREVER);

    while (1)
    {
        flags = uvc_stream_enter_critical();
        if (!stream_handle->pro_enable)
        {
            uvc_stream_exit_critical(flags);
            break;
        }
        uvc_stream_exit_critical(flags);

        urb = uvc_camera_urb_pop();

        if (urb == NULL)
        {
            continue;
        }

        uvc_param_t *uvc_param = (uvc_param_t *)urb->arg;

        // complete urb error, do not need process
        if (urb->errorcode != 0)
        {
            UVC_PACKET_ERROR_START();
            pro_config->packet_error[uvc_param->info->port - 1] = true;
            // clear error code
            LOGW("%s, %d, errorcode:%d\n", __func__, __LINE__, urb->errorcode);
            urb->errorcode = 0;
#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
            pro_config->packet_err_num += 8;
            pro_config->all_packet_num += 8;
#endif
            UVC_PACKET_ERROR_END();
        }
        else
        {
            for (uint8_t i = 0; i < urb->num_of_iso_packets; i++)
            {
#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
                pro_config->all_packet_num++;
#endif
                UVC_PACKET_PROCESS_START();
                payload = urb->iso_packet[i].transfer_buffer;
                if (urb->iso_packet[i].errorcode != BK_OK)
                {
                    UVC_PACKET_ERROR_START();
                    LOGD("[%d]%s, %d packet error:%d...\r\n", uvc_param->info->port, __func__, __LINE__, urb->iso_packet[i].errorcode);
                    pro_config->packet_error[uvc_param->info->port - 1] = true;
                    // clear error code
                    urb->iso_packet[i].errorcode = 0;
#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
                    pro_config->packet_err_num++;
#endif
                    UVC_PACKET_ERROR_END();
                }
                else
                {
                    uvc_camera_stream_packet_process(stream_handle, uvc_param, payload, urb->iso_packet[i].actual_length);
                }
                UVC_PACKET_PROCESS_END();
            }
        }

        uvc_camera_urb_free(urb);
    };

    LOGI("%s, %d\n", __func__, __LINE__);
    stream_handle->pro_thread = NULL;
    rtos_set_event_flags(&stream_handle->handle, UVC_PROCESS_TASK_DISABLE_BIT);
    rtos_delete_thread(NULL);
}

avdk_err_t bk_uvc_camera_stream_ioctl(uvc_stream_handle_t *handle, uint32_t event, void *arg)
{
    // TODO: implement ioctl
    (void)handle;
    (void)event;
    (void)arg;

    return AVDK_ERR_UNSUPPORTED;
}

avdk_err_t bk_uvc_camera_stream_suspend(uvc_stream_handle_t *handle, uint8_t port)
{
    uvc_param_t *uvc_param = &handle->camera[port - 1];
    if (uvc_param->stream_state != UVC_STREAM_STREAMING_STATE)
    {
        return AVDK_ERR_UNSUPPORTED;
    }
    uvc_param->stream_state = UVC_STREAM_CONNECTED_STATE;
    return AVDK_ERR_OK;
}

avdk_err_t bk_uvc_camera_stream_resume(uvc_stream_handle_t *handle, uint8_t port)
{
    uvc_param_t *uvc_param = &handle->camera[port - 1];
    if (uvc_param->stream_state != UVC_STREAM_CONNECTED_STATE)
    {
        return AVDK_ERR_UNSUPPORTED;
    }
    uvc_param->stream_state = UVC_STREAM_STREAMING_STATE;
    return AVDK_ERR_OK;
}

static avdk_err_t uvc_camera_memcpy_port_info(uvc_stream_handle_t *handle, bk_usb_hub_port_info *src)
{
    uint8_t port = src->port_index;
    uvc_param_t *uvc_param = &handle->camera[port - 1];

    if (uvc_param->port_info == NULL)
    {
        uvc_param->port_info = (bk_usb_hub_port_info *)os_malloc(sizeof(bk_usb_hub_port_info));
        AVDK_RETURN_ON_FALSE(uvc_param->port_info, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
        os_memcpy(uvc_param->port_info, src, sizeof(bk_usb_hub_port_info));
    }

    os_memcpy(uvc_param->port_info, src, sizeof(bk_usb_hub_port_info));
    return AVDK_ERR_OK;
}

avdk_err_t bk_uvc_camera_stream_start(uvc_stream_handle_t *handle, bk_cam_uvc_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    UVC_INIT_START();

    LOGI("%s, %d, handle:%p, config:%p\n", __func__, __LINE__, handle, config);

    if (handle == NULL || config == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&handle->lock);

    uvc_param_t *uvc_param = &handle->camera[config->port - 1];

    if (uvc_param->stream_state == UVC_STREAM_STREAMING_STATE)
    {
        LOGW("%s, %d, camera already streaming\r\n", __func__, __LINE__);
        ret = AVDK_ERR_OK;
        goto out;
    }

    bk_usbh_hub_port_register_connect_callback(config->port, USB_UVC_H26X_DEVICE, uvc_camera_stream_connect_callback, handle);
    bk_usbh_hub_port_register_disconnect_callback(config->port, USB_UVC_H26X_DEVICE, uvc_camera_stream_disconnect_callback, handle);
    bk_usbh_hub_port_register_connect_callback(config->port, USB_UVC_DEVICE, uvc_camera_stream_connect_callback, handle);
    bk_usbh_hub_port_register_disconnect_callback(config->port, USB_UVC_DEVICE, uvc_camera_stream_disconnect_callback, handle);

    if (config->format == BK_IMAGE_FORMAT_H264 || config->format == BK_IMAGE_FORMAT_H265)
    {
        ret = bk_usbh_hub_port_check_device(config->port, USB_UVC_H26X_DEVICE, &uvc_param->port_info);
    }
    else
    {
        ret = bk_usbh_hub_port_check_device(config->port, USB_UVC_DEVICE, &uvc_param->port_info);
    }

    if (ret != AVDK_ERR_OK)
    {
        ret = bk_usbh_hub_port_check_device(config->port, USB_UVC_DEVICE, &uvc_param->port_info);
    }


    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, check device failed, port:%d not connected!\n", __func__, __LINE__, config->port);
        goto out;
    }

    if (uvc_param->info == NULL)
    {
        uvc_param->info = (bk_cam_uvc_config_t *)os_malloc(sizeof(bk_cam_uvc_config_t));
        AVDK_RETURN_ON_FALSE(uvc_param->info, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
        os_memcpy(uvc_param->info, config, sizeof(bk_cam_uvc_config_t));
    }

    {
        uint32_t flags = uvc_stream_enter_critical();
    uvc_param->stream_state = UVC_STREAM_CONNECTED_STATE;
        uvc_stream_exit_critical(flags);
    }

    rtos_clear_event_flags(&handle->handle, config->port);

    ret = uvc_stream_task_send_msg(UVC_STREAM_START_IND, (uint32_t)config->port);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, send connect ind failed, port:%d\n", __func__, __LINE__, config->port);
        goto out;
    }

    rtos_wait_for_event_flags(&handle->handle, UVC_STREAM_START_BIT, true, true, BEKEN_WAIT_FOREVER);

    if (uvc_param->stream_state != UVC_STREAM_STREAMING_STATE)
    {
        LOGE("%s, %d, failed to start stream, port:%d\n", __func__, __LINE__, config->port);
        ret = AVDK_ERR_UNSUPPORTED;
    }

out:
    UVC_INIT_END();
    if (uvc_param->info && ret != AVDK_ERR_OK)
    {
        os_free(uvc_param->info);
        uvc_param->info = NULL;
        bk_usbh_hub_port_register_connect_callback(config->port, USB_UVC_H26X_DEVICE, NULL, NULL);
        bk_usbh_hub_port_register_disconnect_callback(config->port, USB_UVC_H26X_DEVICE, NULL, NULL);
        bk_usbh_hub_port_register_connect_callback(config->port, USB_UVC_DEVICE, NULL, NULL);
        bk_usbh_hub_port_register_disconnect_callback(config->port, USB_UVC_DEVICE, NULL, NULL);
    }
    rtos_unlock_mutex(&handle->lock);
    return ret;
}


avdk_err_t bk_uvc_camera_stream_stop(uvc_stream_handle_t *handle, uint8_t port)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (handle == NULL || port == 0)
    {
        return AVDK_ERR_INVAL;
    }

    bk_usbh_hub_port_register_connect_callback(port, USB_UVC_H26X_DEVICE, NULL, NULL);
    bk_usbh_hub_port_register_disconnect_callback(port, USB_UVC_H26X_DEVICE, NULL, NULL);
    bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, NULL, NULL);
    bk_usbh_hub_port_register_disconnect_callback(port, USB_UVC_DEVICE, NULL, NULL);

    uvc_param_t *uvc_param = &handle->camera[port - 1];

    LOGI("%s, %d, handle:%p, port:%d\n", __func__, __LINE__, handle, port);

    {
        uint32_t flags = uvc_stream_enter_critical();
        ret = (uvc_param->stream_state == UVC_STREAM_STREAMING_STATE) ? AVDK_ERR_OK : AVDK_ERR_UNSUPPORTED;
        uvc_stream_exit_critical(flags);
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, camera already stopped\n", __func__, __LINE__);
        ret = AVDK_ERR_OK;
        goto out;
    }

    rtos_clear_event_flags(&handle->handle, UVC_STREAM_STOP_BIT);

    ret = uvc_stream_task_send_msg(UVC_STREAM_STOP_IND, (uint32_t)port);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d, send disconnect ind failed, port:%d\n", __func__, __LINE__, port);
        goto out;
    }

    rtos_wait_for_event_flags(&handle->handle, UVC_STREAM_STOP_BIT, true, true, BEKEN_WAIT_FOREVER);

    if (uvc_param->stream_state != UVC_STREAM_CLOSED_STATE)
    {
        LOGE("%s, %d, failed to stop stream, port:%d\n", __func__, __LINE__, port);
        ret = AVDK_ERR_UNSUPPORTED;
    }

    LOGI("%s, %d, port:%d success\n", __func__, __LINE__, port);

out:
    return ret;
}

avdk_err_t bk_uvc_camera_stream_deinit(uvc_stream_handle_t *handle)
{
    if (handle == NULL)
    {
        return AVDK_ERR_OK;
    }

    if (handle->lock)
    {
        rtos_lock_mutex(&handle->lock);
    }

    for (int i = 0; i < UVC_PORT_MAX; i++)
    {
        if (handle->camera[i].stream_state != UVC_STREAM_CLOSED_STATE)
        {
            if (handle->lock)
            {
                rtos_unlock_mutex(&handle->lock);
            }
            LOGI("%s, %d, port:%d is busy\n", __func__, __LINE__, i + 1);
            return AVDK_ERR_OK;
        }
    }

    if (handle->pro_thread)
    {
        rtos_clear_event_flags(&handle->handle, UVC_PROCESS_TASK_DISABLE_BIT);
        {
            uint32_t flags = uvc_stream_enter_critical();
        handle->pro_enable = false;
            uvc_stream_exit_critical(flags);
        }
        rtos_wait_for_event_flags(&handle->handle, UVC_PROCESS_TASK_DISABLE_BIT, true, true, BEKEN_WAIT_FOREVER);
        LOGI("%s, %d, pro_thread deleted success\n", __func__, __LINE__);
    }

    if (handle->pro_config)
    {
#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
        if (handle->pro_config->timer.handle)
        {
            rtos_stop_timer(&handle->pro_config->timer);
            rtos_deinit_timer(&handle->pro_config->timer);
            uvc_camera_stream_timer_handle(handle);
        }
#endif

        os_free(handle->pro_config);
        handle->pro_config = NULL;
    }

    if (handle->stream_thread)
    {
        rtos_clear_event_flags(&handle->handle, UVC_STREAM_TASK_DISABLE_BIT);
        if (uvc_stream_task_send_msg(UVC_EXIT_IND, 0) != AVDK_ERR_OK)
        {
            LOGE("%s, %d\n", __func__, __LINE__);
        }
        {
            uint32_t flags = uvc_stream_enter_critical();
        handle->stream_enable = false;
            uvc_stream_exit_critical(flags);
        }
        rtos_wait_for_event_flags(&handle->handle, UVC_STREAM_TASK_DISABLE_BIT, true, true, BEKEN_WAIT_FOREVER);
        uvc_camera_stream_task_deinit(handle);
    }

    for (uint8_t i = 0; i < UVC_PORT_MAX; i++)
    {
        if (handle->camera[i].sem)
        {
            rtos_deinit_semaphore(&handle->camera[i].sem);
            handle->camera[i].sem = NULL;
        }
    }

    if (handle->stream_queue)
    {
        rtos_deinit_queue(&handle->stream_queue);
        handle->stream_queue = NULL;
    }

    if (handle->lock)
    {
        rtos_unlock_mutex(&handle->lock);
        rtos_deinit_mutex(&handle->lock);
        handle->lock = NULL;
    }

    if (handle->handle)
    {
        rtos_deinit_event_flags(&handle->handle);
        handle->handle = NULL;
    }

    os_free(handle);

    uvc_camera_urb_list_deinit();

    LOGI("%s, %d success\n", __func__, __LINE__);

    {
        uint32_t flags = uvc_stream_enter_critical();
        s_uvc_stream_handle = NULL;
        uvc_stream_exit_critical(flags);
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_uvc_camera_stream_init(uvc_stream_handle_t **handle, const bk_uvc_callback_t *callback)
{
    {
        uint32_t flags = uvc_stream_enter_critical();
        if (s_uvc_stream_handle)
        {
            LOGE("%s, %d already initialized\n", __func__, __LINE__);
            *handle = s_uvc_stream_handle;
            uvc_stream_exit_critical(flags);
            return AVDK_ERR_OK;
        }
        uvc_stream_exit_critical(flags);
    }

    avdk_err_t ret = uvc_camera_urb_list_init();
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }

    uvc_stream_handle_t *stream_handle = (uvc_stream_handle_t *)os_malloc(sizeof(uvc_stream_handle_t));
    if (stream_handle == NULL)
    {
        LOGE("s_uvc_stream_handle malloc failed\n");
        ret = AVDK_ERR_NOMEM;
        return ret;
    }

    os_memset(stream_handle, 0, sizeof(uvc_stream_handle_t));
    stream_handle->callback = callback;

    stream_handle->pro_config = (uvc_pro_config_t *)os_malloc(sizeof(uvc_pro_config_t));
    if (stream_handle->pro_config == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }

    os_memset(stream_handle->pro_config, 0, sizeof(uvc_pro_config_t));

    ret = rtos_init_event_flags(&stream_handle->handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s, %d event flags init failed\n", __func__, __LINE__);
        goto error;
    }

    ret = rtos_init_mutex(&stream_handle->lock);
    if (AVDK_ERR_OK != ret)
    {
        LOGE("%s uvc_stream->lock init failed\n", __func__);
        goto error;
    }

    for (uint8_t i = 0; i < UVC_PORT_MAX; i++)
    {
        ret = rtos_init_semaphore(&stream_handle->camera[i].sem, 1);
        if (AVDK_ERR_OK != ret)
        {
            LOGE("%s uvc_stream->camera[i].sem init failed\n", __func__);
            goto error;
        }
    }

    ret = rtos_init_queue(&stream_handle->stream_queue,
                          "uvc_stream_que",
                          sizeof(uvc_msg_t),
                          10);
    if (AVDK_ERR_OK != ret)
    {
        LOGE("%s stream_queue init failed\n", __func__);
        goto error;
    }

    rtos_clear_event_flags(&stream_handle->handle, UVC_STREAM_TASK_ENABLE_BIT);

    ret = rtos_create_thread(&stream_handle->stream_thread,
                                    BEKEN_DEFAULT_WORKER_PRIORITY,
                                    "uvc_stream_task",
                                    (beken_thread_function_t)uvc_camera_stream_task_main,
                                    UVC_TASK_STACK_SIZE,
                                    (beken_thread_arg_t)stream_handle);

    if (AVDK_ERR_OK != ret)
    {
        LOGE("%s uvc stream task init failed\n", __func__);
        goto error;
    }

    rtos_wait_for_event_flags(&stream_handle->handle, UVC_STREAM_TASK_ENABLE_BIT, true, true, BEKEN_WAIT_FOREVER);

    rtos_clear_event_flags(&stream_handle->handle, UVC_PROCESS_TASK_ENABLE_BIT);

    ret = rtos_create_thread(&stream_handle->pro_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY - 3,
                             "uvc_pro_task",
                             (beken_thread_function_t)uvc_camera_process_task_main,
                             1024 * 2,
                             (beken_thread_arg_t)stream_handle);

    if (AVDK_ERR_OK != ret)
    {
        LOGE("%s uvc pro task init failed\n", __func__);
        goto error;
    }

#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
    ret = rtos_init_timer(&stream_handle->pro_config->timer, UVC_TIME_INTERVAL * 1000,
                          uvc_camera_stream_timer_handle, stream_handle);
    if (ret != BK_OK)
    {
        LOGE("%s, init stream_handle->pro_config->timer fail....\r\n", __func__);
        goto error;
    }

    rtos_start_timer(&stream_handle->pro_config->timer);
#endif

    rtos_wait_for_event_flags(&stream_handle->handle, UVC_PROCESS_TASK_ENABLE_BIT, true, true, BEKEN_WAIT_FOREVER);

    {
        uint32_t flags = uvc_stream_enter_critical();
        s_uvc_stream_handle = stream_handle;
        uvc_stream_exit_critical(flags);
    }
    *handle = stream_handle;

    LOGI("%s, %d success\n", __func__, __LINE__);

    return ret;

error:
    LOGE("%s, %d fail\n", __func__, __LINE__);
    bk_uvc_camera_stream_deinit(stream_handle);
    return ret;
}
