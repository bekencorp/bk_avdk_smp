#ifndef LV_CAMERA_BLEND_H
#define LV_CAMERA_BLEND_H

#include <stdbool.h>
#include <stdint.h>
#include <common/bk_include.h>
#include <common/avdk_pixel_types.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lv_camera_blend_ctx *lv_camera_blend_handle_t;
typedef struct lv_camera_blend_async_ctx *lv_camera_blend_async_handle_t;
typedef int (*lv_camera_blend_free_cb_t)(void *args);
typedef bk_err_t (*lv_camera_blend_lock_cb_t)(void *user_data);
typedef bk_err_t (*lv_camera_blend_output_cb_t)(void *user_data,
                                                void *frame_buffer,
                                                lv_camera_blend_free_cb_t free_cb);
typedef void (*lv_camera_blend_overlay_cb_t)(void *user_data, vg_lite_buffer_t *output);

typedef struct {
    uint16_t width;
    uint16_t height;
    bk_pixel_format_t lvgl_format;
    bool lvgl_compress;
    bk_pixel_format_t output_format;
    bool output_compress;
} lv_camera_blend_config_t;

typedef struct {
    void *buffer;
    uint16_t src_x;
    uint16_t src_y;
    uint16_t src_width;
    uint16_t src_height;
    bk_pixel_format_t src_format;
    bool src_compress;
    uint16_t dst_x;
    uint16_t dst_y;
    uint16_t rotate_degree;
    bool alpha_blend;
} lv_camera_blend_camera_frame_t;

typedef struct {
    lv_camera_blend_config_t blend;
    uint32_t output_buffer_size;
    uint32_t camera_frame_cache_size;
    uint32_t queue_depth;
    uint32_t thread_stack_size;
    uint32_t thread_priority;
    lv_camera_blend_lock_cb_t lock_cb;
    lv_camera_blend_lock_cb_t unlock_cb;
    void *lock_user_data;
    lv_camera_blend_output_cb_t output_cb;
    void *output_user_data;
    lv_camera_blend_overlay_cb_t overlay_cb;
    void *overlay_user_data;
} lv_camera_blend_async_config_t;

bk_err_t lv_camera_blend_init(lv_camera_blend_handle_t *handle, const lv_camera_blend_config_t *config);
void lv_camera_blend_deinit(lv_camera_blend_handle_t handle);

/*
 * Use this as lv_vnd_config.flush_cb and set lv_vnd_config.args to the handle.
 * The callback copies the LVGL frame to an internal background buffer and then
 * releases the input frame through free_cb; it does not send the frame to DPU.
 */
void lv_camera_blend_lvgl_flush_cb(void *args, void *frame_buffer, int (*free_cb)(void *args));

/*
 * Compose the latest published LVGL background and one camera foreground frame
 * into output_buffer. The caller owns output_buffer and is responsible for
 * sending it to DPU after this function returns BK_OK.
 */
bk_err_t lv_camera_blend_process(lv_camera_blend_handle_t handle,
                                 void *output_buffer,
                                 const lv_camera_blend_camera_frame_t *camera);

uint32_t lv_camera_blend_get_bg_sequence(lv_camera_blend_handle_t handle);

/*
 * Async camera/LVGL blend service. It owns the camera frame after
 * lv_camera_blend_async_push_camera_frame() returns and sends composed output
 * frames through output_cb. Set camera_frame_cache_size to 0 to disable camera
 * frame caching; otherwise the latest camera frame is cached for LVGL refreshes.
 * lock_cb/unlock_cb are optional but should be provided when VG-Lite is shared.
 */
bk_err_t lv_camera_blend_async_start(lv_camera_blend_async_handle_t *handle,
                                     const lv_camera_blend_async_config_t *config);
void lv_camera_blend_async_stop(lv_camera_blend_async_handle_t handle);
bool lv_camera_blend_async_is_active(lv_camera_blend_async_handle_t handle);
bk_err_t lv_camera_blend_async_push_camera_frame(lv_camera_blend_async_handle_t handle,
                                                 const lv_camera_blend_camera_frame_t *camera,
                                                 uint32_t frame_size,
                                                 lv_camera_blend_free_cb_t free_cb);
bk_err_t lv_camera_blend_async_update_lvgl_frame(lv_camera_blend_async_handle_t handle,
                                                 void *frame_buffer,
                                                 lv_camera_blend_free_cb_t free_cb);

#ifdef __cplusplus
}
#endif

#endif /* LV_CAMERA_BLEND_H */
