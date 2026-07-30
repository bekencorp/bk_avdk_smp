/*
 * bk_draw_osd public API (thin wrapper).
 *
 * Validates handle/vtable, then forwards to handle->op(...). Logic lives in:
 *   - bk_draw_osd_ctlr.c: controller (assets, display list, mutex, add/remove)
 *   - bk_osd_engine.c: compositor (sprite + external pipeline GPU submit)
 *
 * Pipeline model: register composed sprites with external GPU for per-frame SRC_OVER;
 * no standalone vg_lite. MIPI and UVC use independent instances.
 */
#include <os/os.h>
#include "components/bk_draw_osd.h"
#include "bk_draw_osd_ctlr.h"

#define TAG "draw_osd"

avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle && config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(*handle == NULL, AVDK_ERR_INVAL, TAG, "handle is not NULL\n");
    return osd_ctlr_new(handle, config);
}

avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->delete, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->delete(handle);
}

avdk_err_t bk_draw_osd_element(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_element, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->draw_element(handle, info);
}

avdk_err_t bk_draw_osd_text(bk_draw_osd_ctlr_handle_t handle, osd_font_kind_t kind,
                            const void *font, const char *utf8,
                            uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_text, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(font && utf8, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->draw_text(handle, kind, font, utf8, x, y, argb, scale);
}

avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *list)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->draw_osd_array, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->draw_osd_array(handle, list);
}

avdk_err_t bk_draw_osd_clear(bk_draw_osd_ctlr_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->clear, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->clear(handle);
}

avdk_err_t bk_draw_osd_add_or_update(bk_draw_osd_ctlr_handle_t handle, const char *name, const char *content)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->add_or_update, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->add_or_update(handle, name, content);
}

avdk_err_t bk_draw_osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->remove, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    AVDK_RETURN_ON_FALSE(name, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    return handle->remove(handle, name);
}

avdk_err_t bk_draw_osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t ioctl_cmd,
                             uint32_t param1, uint32_t param2, uint32_t param3)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(handle->ioctl, AVDK_ERR_UNSUPPORTED, TAG, AVDK_ERR_UNSUPPORTED_FUNCTION_TEXT);
    return handle->ioctl(handle, ioctl_cmd, param1, param2, param3);
}
