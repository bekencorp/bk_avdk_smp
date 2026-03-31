#include <os/os.h>
#include <os/mem.h>


#include <components/bk_gpu_ctlr.h>
#include <components/bk_gpu_types.h>

#define TAG "bk_gpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


avdk_err_t bk_gpu_init(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->init, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->init(handle);
}

avdk_err_t bk_gpu_deinit(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->deinit, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->deinit(handle);
}

avdk_err_t bk_gpu_open(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->open, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->open(handle);
}

avdk_err_t bk_gpu_close(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->close, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->close(handle);
}

avdk_err_t bk_gpu_ioctl(bk_gpu_ctlr_handle_t handle, uint32_t cmd, void *args)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, cmd, args);
}

avdk_err_t bk_gpu_delete(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->del, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->del(handle);
}

avdk_err_t bk_gpu_draw_path_clear(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_path_clear, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->draw_path_clear(handle);
}

avdk_err_t bk_gpu_draw_path_build(bk_gpu_ctlr_handle_t handle, bk_gpu_draw_path_set_t *path_set)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_path_build, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->draw_path_build(handle, path_set);
}

avdk_err_t bk_gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer, bk_gpu_blit_config_t *blit_config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->blit_set, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->blit_set(handle, src_buffer, blit_config);
}

avdk_err_t bk_gpu_blit_clear(bk_gpu_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->blit_clear, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->blit_clear(handle);
}
