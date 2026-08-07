#include <os/os.h>
#include <os/mem.h>

#include <avdk_check.h>
#include <components/bk_uvc_camera.h>

#include "bk_uvc_common.h"
#include "uvc_urb_list.h"

#define TAG "uvc_ctlr"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static avdk_err_t bk_uvc_ctrl_init(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_init(&controller->stream_handle, controller->callback), TAG, "exe fail");
    LOGI("%s, %d, controller:%p, stream_handle:%p, init successful\n", __func__, __LINE__, controller, controller->stream_handle);
    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_deinit(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    LOGI("%s, %d, controller:%p, stream_handle:%p\n", __func__, __LINE__, controller, controller->stream_handle);
    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_deinit(controller->stream_handle), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_open(bk_uvc_ctlr_handle_t handle, bk_cam_uvc_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->port > 0 && config->port <= UVC_PORT_MAX, AVDK_ERR_INVAL, TAG, "port out of range");
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->stream_handle, AVDK_ERR_INVAL, TAG, "stream_handle is NULL");

    os_memcpy(&controller->config, config, sizeof(bk_cam_uvc_config_t));
    LOGI("%s, %d, controller:%p, stream_handle:%p, config:%p\n",
         __func__, __LINE__, controller, controller->stream_handle, config);
    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_start(controller->stream_handle, config), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_close(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    uint8_t port = controller->config.port;
    LOGI("%s, %d, controller:%p, stream_handle:%p, port:%d\n", __func__, __LINE__, controller, controller->stream_handle, port);
    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_stop(controller->stream_handle, port), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_suspend(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    uint8_t port = controller->config.port;
    LOGI("%s, %d, controller:%p, stream_handle:%p\n", __func__, __LINE__, controller, controller->stream_handle);
    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_suspend(controller->stream_handle, port), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_resume(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    uint8_t port = controller->config.port;
    LOGI("%s, %d, controller:%p, stream_handle:%p, port:%d\n", __func__, __LINE__, controller, controller->stream_handle, port);
    AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_resume(controller->stream_handle, port), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_uvc_ctrl_ioctl(bk_uvc_ctlr_handle_t handle, bk_uvc_ioctl_cmd_t event, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    switch (event)
    {
        default:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
            AVDK_RETURN_ON_FALSE(controller->stream_handle, AVDK_ERR_INVAL, TAG, "stream_handle is NULL");
            LOGI("%s, %d, controller:%p, stream_handle:%p\n", __func__, __LINE__, controller, controller->stream_handle);
            AVDK_RETURN_ON_ERROR(bk_uvc_camera_stream_ioctl(controller->stream_handle, event, arg), TAG, "exe fail");
            return AVDK_ERR_OK;
    }
}

static avdk_err_t bk_uvc_ctrl_del(bk_uvc_ctlr_handle_t handle)
{
    private_uvc_ctlr_t *controller = __containerof(handle, private_uvc_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    LOGI("%s, %d, controller:%p\n", __func__, __LINE__, controller);
    os_free(controller);

    return AVDK_ERR_OK;
}

avdk_err_t bk_uvc_ctrl_new(bk_uvc_ctlr_handle_t *handle, const bk_uvc_callback_t *callbacks)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(callbacks, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    private_uvc_ctlr_t *controller = os_malloc(sizeof(private_uvc_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(private_uvc_ctlr_t));

    LOGI("%s, %d, controller:%p\n", __func__, __LINE__, controller);

    controller->callback = callbacks;

    controller->ops.init = bk_uvc_ctrl_init;
    controller->ops.deinit = bk_uvc_ctrl_deinit;
    controller->ops.open = bk_uvc_ctrl_open;
    controller->ops.close = bk_uvc_ctrl_close;
    controller->ops.suspend = bk_uvc_ctrl_suspend;
    controller->ops.resume = bk_uvc_ctrl_resume;
    controller->ops.ioctl = bk_uvc_ctrl_ioctl;
    controller->ops.del = bk_uvc_ctrl_del;

    *handle = &controller->ops;

    return AVDK_ERR_OK;
}