#include <os/os.h>
#include <os/mem.h>

#include <common/avdk_pixel_types.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <driver/isp.h>
#include <driver/i2c.h>
#include <driver/io_matrix.h>

#include <avdk_check.h>
#include "isp_cam_sensor.h"
#include "isp_camera_ctlr.h"
#include "isp_camera_utils.h"

#define TAG "bk_cam_isp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static void isp_camera_ctlr_task_entry(void *param)
{
    // only support sp
    int ret = BK_FAIL;
    bk_camera_isp_ctlr_t *cam_control = (bk_camera_isp_ctlr_t *)param;
    isp_control_t *isp_control = (isp_control_t *)cam_control->isp_handle;
    cam_control->chnl = 0;
    isp_channel_config_t *config = NULL;
    while (cam_control->thread_enable)
    {
        uint8_t chnl_id = cam_control->chnl;
        config = &isp_control->chn[chnl_id];
        if (config == NULL || config->enable == false)
        {
            //LOGD("%s, %d config error:%p\n", __func__, __LINE__, config);
            rtos_delay_milliseconds(10);
            continue;
        }

        if (config->enable_flexa)
        {
            LOGE("%s, %d flexa mode not supported read frame\n", __func__, __LINE__);
            rtos_delay_milliseconds(1000);
            continue;
        }

        VIDEO_BUF_S buf;
        ret = isp_control->pop_buf(config->channel, &buf, cam_control->read_timeout);
        if (ret != BK_OK)
        {
            continue;
        }

        if (cam_control->read_register && cam_control->read_enable && cam_control->frame)
        {
            uint8_t *dst_frame = (uint8_t *)(uintptr_t)buf.planes[0].dmaPhyAddr;
            uint32_t length = config->chn_attr.chnFormat.imageSize;
            if (length <= cam_control->size)
            {
                //LOGD("%s, %p %p %d, %d\n", __func__, cam_control->frame, dst_frame, length, chnl_id);
                os_memcpy(cam_control->frame, dst_frame, length);
            }
            else
            {
                LOGE("%s, frame size overflow, %d > %d\n", __func__, length, cam_control->size);
                cam_control->size = 0;
            }

            cam_control->read_enable = false;
            rtos_set_semaphore(&cam_control->sem);
        }

        isp_control->free_buf(config->channel, &buf);
    }

    cam_control->thread = NULL;
    rtos_set_semaphore(&cam_control->sem);
    rtos_delete_thread(NULL);
}


static void camera_frame_complete_callback(uint32_t seqence, uint32_t line, uint8_t chnl, uint8_t ok, void *param)
{

}

static bk_err_t isp_camera_ctlr_dev_init(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    control->state = CAM_FSM_INIT;
    AVDK_RETURN_ON_ERROR(bk_isp_dev_init(&control->isp_handle), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static bk_err_t isp_camera_ctlr_port_init(bk_isp_camera_ctlr_handle_t handle, void *config)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    control->state = CAM_FSM_INIT;
    bk_isp_camera_ctlr_config_t *ctrl_cfg = (bk_isp_camera_ctlr_config_t *)config;

    if (ctrl_cfg->port_id >= ISP_PORT_CNT)
    {
        LOGE("error port id\r\n");
        return BK_FAIL;
    }

    control->attr[ctrl_cfg->port_id].snsRect.top = ctrl_cfg->input_rect.top;
    control->attr[ctrl_cfg->port_id].snsRect.left = ctrl_cfg->input_rect.left;
    control->attr[ctrl_cfg->port_id].snsRect.width = ctrl_cfg->input_rect.width;
    control->attr[ctrl_cfg->port_id].snsRect.height = ctrl_cfg->input_rect.height;

    control->attr[ctrl_cfg->port_id].inFormRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].inFormRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].inFormRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].inFormRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].outFormRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].outFormRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].outFormRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].outFormRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].iSRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].iSRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].iSRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].iSRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].snsFps = ctrl_cfg->fps * ISP_SNS_FPS_ACCU;
    control->attr[ctrl_cfg->port_id].ispInputType = ctrl_cfg->input_type;
    control->attr[ctrl_cfg->port_id].ispMode = ctrl_cfg->isp_mode;
    control->attr[ctrl_cfg->port_id].hdrMode = ctrl_cfg->hdr_mode;
    control->attr[ctrl_cfg->port_id].pixelFormat = isp_camera_format_convert(ctrl_cfg->input_pixel_fmt);
    control->attr[ctrl_cfg->port_id].pSnsObj = (void*)ctrl_cfg->sensor_object;
    control->attr[ctrl_cfg->port_id].port_id = ctrl_cfg->port_id;

    LOGI("port id %d \r\n", ctrl_cfg->port_id);
    AVDK_RETURN_ON_ERROR(bk_isp_port_init(&control->isp_handle, &(control->attr[ctrl_cfg->port_id])), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static bk_err_t isp_camera_ctlr_port_change(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    AVDK_RETURN_ON_ERROR(bk_isp_port_change(&control->isp_handle, control->chnl), TAG, "exe fail");
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_deinit(bk_isp_camera_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control != NULL, ret, TAG, "control is NULL");

    if (control->state != CAM_FSM_INIT)
    {
        LOGE("%s, %d camera not in init state\n", __func__, __LINE__);
        return ret;
    }

    // Stop cam_thread if running
    if (control->thread_enable)
    {
        control->thread_enable = false;
        // Wait for thread to exit (thread will set semaphore before exiting)
        if (control->sem)
        {
            rtos_get_semaphore(&control->sem, 1000);  // Wait up to 1 second
            rtos_deinit_semaphore(&control->sem);
        }
    }

    // Deinitialize ISP core resources (threads, buffers, semaphores, etc.)
    if (control->isp_handle)
    {
        bk_err_t isp_ret = bk_isp_deinit(&control->isp_handle);
        if (isp_ret != BK_OK)
        {
            LOGW("%s, %d, failed to deinit ISP core: %d\n", __func__, __LINE__, isp_ret);
        }
        control->isp_handle = NULL;
    }

    ret = BK_OK;

    return ret;
}

static avdk_err_t isp_camera_csi_sensor_open(bk_camera_isp_ctlr_t *control, void *is_handler)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    if (control->state != CAM_FSM_INIT)
    {
        LOGE("%s, %d camera not init\n", __func__, __LINE__);
        return ret;
    }

    ret = rtos_init_semaphore(&control->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d sem init error\n", __func__, __LINE__);
        return ret;
    }

    control->isp_handle = is_handler;
    control->thread_enable = true;
    ret = rtos_create_hsram_thread(&control->thread,
                            BEKEN_DEFAULT_WORKER_PRIORITY,
                            "cam_thread",
                            (beken_thread_function_t)isp_camera_ctlr_task_entry,
                            1024 * 2,
                            (beken_thread_arg_t)control);
    if (ret != BK_OK)
    {
        LOGE("%s, %d cam task create fail\n", __func__, __LINE__);
        control->thread_enable = false;
        if (control->sem)
        {
            rtos_deinit_semaphore(&control->sem);
        }
        return ret;
    }

    if (ret != BK_OK)
    {
        LOGE("%s, %d cannot find camera sensor\n", __func__, __LINE__);
        return ret;
    }

    LOGI("%s, %d\n", __func__, __LINE__);

    control->state = CAM_FSM_ENABLE;

    ret = BK_OK;

    return ret;
}

static avdk_err_t isp_camera_ctlr_read(bk_isp_camera_ctlr_handle_t handle, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control != NULL, ret, TAG, "control is NULL");

    if (control->state != CAM_FSM_ENABLE || control->thread_enable == false)
    {
        LOGE("%s, %d camera not enable\n", __func__, __LINE__);
        return ret;
    }

    isp_control_t *isp_control = (isp_control_t *)control->isp_handle;

    if (control->read_register == false)
    {
        // register frame end cb
        bk_isp_register_isr_callback((isp_handle_t *)&isp_control, ISP_FRAME_END_DONE,
            camera_frame_complete_callback, control);

        control->read_register = true;
    }

    if (control->read_enable)
    {
        LOGW("%s, %d state error!\n", __func__, __LINE__);
        return ret;
    }

    control->frame = frame;
    control->chnl = id;
    control->size = size;
    control->read_enable = true;
    control->read_timeout = timeout;

    ret = rtos_get_semaphore(&control->sem, timeout);
    if (ret != BK_OK)
    {
        LOGW("%s, %d read timeout %dms\n", __func__, __LINE__, timeout);
    }

    if (control->size == 0)
    {
        LOGW("%s, %d, frame size is 0\n", __func__, __LINE__);
        ret = AVDK_ERR_GENERIC;
    }

    control->size = 0;
    control->read_enable = false;
    control->frame = NULL;

    return ret;
}

static avdk_err_t isp_camera_ctlr_delete(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    //TODO

    os_free(control);

    return AVDK_ERR_OK;
}


static avdk_err_t isp_camera_ctlr_channel_open(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, bk_isp_camera_channel_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(channel < 2, AVDK_ERR_INVAL, TAG, "channel out of range");

    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    LOGI("%s, %d, chnl_id: %d\n", __func__, __LINE__, channel);

    if (controller->channel_state[channel] != ISP_CHANNEL_STATE_TURN_OFF)
    {
        LOGE("%s, %d, channel %d state busy %d\n", __func__, __LINE__, channel, controller->channel_state[channel]);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURNING_ON;

    //return false
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_NO_RESOURCE, TAG, AVDK_ERR_NO_RESOURCE_TEXT);

    //TODO
    isp_config_ext_t isp_config = {
        .buf_cnt = config->buf_cnt,
        .chnl_id = channel,
        .port_id = config->port_id,
        .enable_flexa = config->enable_flexa,
        .work_mode = config->work_mode,
        .width = config->width,
        .height = config->height,
        .format = isp_camera_format_convert(config->format),
    };

    LOGI("%s, buf_cnt: %d, chnl_id: %d, port_id: %d, enable_flexa: %d, work_mode: %d, width: %d, height: %d, format: %d\n",
        __func__,
        isp_config.buf_cnt,
        isp_config.chnl_id,
        isp_config.port_id,
        isp_config.enable_flexa,
        isp_config.work_mode,
        isp_config.width,
        isp_config.height,
        isp_config.format
    );

    AVDK_RETURN_ON_ERROR(bk_isp_open(&controller->isp_handle, &isp_config), TAG, "exe fail");

    if (controller->sensor_ctlr == 0 && isp_config.work_mode == 0)
    {
        AVDK_RETURN_ON_ERROR(isp_camera_csi_sensor_open(controller, controller->isp_handle), TAG, "exe fail");
        controller->sensor_ctlr++;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_ON;

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_channel_close(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(channel < 2, AVDK_ERR_INVAL, TAG, "channel out of range");

    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);

    if (controller->channel_state[channel] != ISP_CHANNEL_STATE_TURN_ON)
    {
        LOGE("%s, %d, channel %d state not turn on %d\n", __func__, __LINE__, channel, controller->channel_state[channel]);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURNING_OFF;

    // Close ISP channel
    uint8_t chnl_id = channel;
    bk_err_t ret = bk_isp_close(&controller->isp_handle, chnl_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, failed to close ISP channel %d\n", __func__, __LINE__, chnl_id);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_OFF;

    LOGI("%s, %d, channel %d closed\n", __func__, __LINE__, channel);


    if (controller->channel_state[ISP_MP_CHN_ID] == ISP_CHANNEL_STATE_TURN_OFF
        && controller->channel_state[ISP_SP_CHN_ID] == ISP_CHANNEL_STATE_TURN_OFF
        && controller->state == CAM_FSM_ENABLE)
    {
        controller->state = CAM_FSM_INIT;
        LOGI("%s, %d, all channels closed, state changed to INIT\n", __func__, __LINE__);
    }

    return AVDK_ERR_OK;
}

static bk_isp_camera_channel_state_t isp_camera_ctlr_channel_state_get(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");
    return controller->channel_state[channel];
}

/**
 * @brief Register ISP interrupt service routine callback
 * @param handle Camera controller handle
 * @param type ISR type
 * @param cb ISR callback function
 * @param arg Callback argument
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_camera_ctlr_register_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");
    AVDK_RETURN_ON_FALSE(cb, AVDK_ERR_INVAL, TAG, "cb is NULL");
    AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");

    bk_err_t ret = bk_isp_register_isr_callback(&controller->isp_handle, (isp_isr_type_t)type, cb, arg);
    AVDK_RETURN_ON_ERROR((avdk_err_t)ret, TAG, "bk_isp_register_isr_callback failed");

    LOGI("%s: registered ISR callback type=%d\n", __func__, type);
    return AVDK_ERR_OK;
}

/**
 * @brief Deregister ISP interrupt service routine callback
 * @param handle Camera controller handle
 * @param type ISR type
 * @param arg Callback argument
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_camera_ctlr_deregister_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");
    AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");

    bk_err_t ret = bk_isp_deregister_isr_callback(&controller->isp_handle, type, arg);
    AVDK_RETURN_ON_ERROR((avdk_err_t)ret, TAG, "bk_isp_deregister_isr_callback failed");

    LOGI("%s: deregistered ISR callback type=%d\n", __func__, type);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_ioctl(bk_isp_camera_ctlr_handle_t handle, bk_cam_interface_ioctl_t ioctl, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");

    switch (ioctl)
    {
        case BK_CAM_IOCTL_SOFTRESET:
            bk_isp_soft_reset(&controller->isp_handle);
            break;
        default:
            return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

avdk_err_t bk_camera_isp_ctlr_new(bk_isp_camera_ctlr_handle_t *handle)
{
    bk_camera_isp_ctlr_t *controller = os_malloc(sizeof(bk_camera_isp_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(bk_camera_isp_ctlr_t));

    // os_memcpy(&controller->config, config, sizeof(bk_isp_camera_ctlr_config_t));
    controller->ops.dev_init = isp_camera_ctlr_dev_init;
    controller->ops.port_init = isp_camera_ctlr_port_init;
    controller->ops.port_change = isp_camera_ctlr_port_change;
    controller->ops.deinit = isp_camera_ctlr_deinit;
    controller->ops.read = isp_camera_ctlr_read;
    controller->ops.del = isp_camera_ctlr_delete;
    controller->ops.channel_open = isp_camera_ctlr_channel_open;
    controller->ops.channel_close = isp_camera_ctlr_channel_close;
    controller->ops.channel_state_get = isp_camera_ctlr_channel_state_get;
    controller->ops.register_isr_callback = isp_camera_ctlr_register_isr_callback;
    controller->ops.deregister_isr_callback = isp_camera_ctlr_deregister_isr_callback;
    controller->ops.ioctl = isp_camera_ctlr_ioctl;

    // controller->attr.snsRect.top = config->input_rect.top;
    // controller->attr.snsRect.left = config->input_rect.left;
    // controller->attr.snsRect.width = config->input_rect.width;
    // controller->attr.snsRect.height = config->input_rect.height;

    // controller->attr.inFormRect.top = config->input_crop.top;
    // controller->attr.inFormRect.left = config->input_crop.left;
    // controller->attr.inFormRect.width = config->input_crop.width;
    // controller->attr.inFormRect.height = config->input_crop.height;
    // controller->attr.outFormRect.top = config->input_crop.top;
    // controller->attr.outFormRect.left = config->input_crop.left;
    // controller->attr.outFormRect.width = config->input_crop.width;
    // controller->attr.outFormRect.height = config->input_crop.height;
    // controller->attr.iSRect.top = config->input_crop.top;
    // controller->attr.iSRect.left = config->input_crop.left;
    // controller->attr.iSRect.width = config->input_crop.width;
    // controller->attr.iSRect.height = config->input_crop.height;
    // controller->attr.snsFps = config->fps * ISP_SNS_FPS_ACCU;
    // controller->attr.ispInputType = config->input_type;
    // controller->attr.ispMode = config->isp_mode;
    // controller->attr.hdrMode = config->hdr_mode;
    // controller->attr.pixelFormat = isp_camera_format_convert(config->input_pixel_fmt);
    // controller->attr.pSnsObj = (void*)config->sensor_object;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}