#include <components/bk_uvc_camera.h>
#include <avdk_check.h>

#define TAG "uvc"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


avdk_err_t bk_uvc_init(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);

    return handle->init(handle);
}

avdk_err_t bk_uvc_deinit(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->deinit(handle);
}

avdk_err_t bk_uvc_open(bk_uvc_ctlr_handle_t handle, bk_cam_uvc_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle, config);
}

avdk_err_t bk_uvc_close(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->close(handle);
}

avdk_err_t bk_uvc_suspend(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->suspend, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->suspend(handle);
}

avdk_err_t bk_uvc_resume(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->resume, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->resume(handle);
}

avdk_err_t bk_uvc_ioctl(bk_uvc_ctlr_handle_t handle, uint32_t event, void *arg)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, event, arg);
}

avdk_err_t bk_uvc_delete(bk_uvc_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->del(handle);
}