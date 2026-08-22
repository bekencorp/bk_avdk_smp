#include <os/os.h>
#include <os/mem.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_gpu.h>
#include "bk_gpu_overlay.h"
#include <components/log.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#include "avdk_monitor.h"
#include "gpu_vn_ctlr.h"

#define TAG "gpu_overlay"
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGV(...)
#define OVERLAY_DEFERRED_RELEASE_MAX (BK_GPU_OVERLAY_LAYER_MAX * 2U)
#define OVERLAY_COMPRESS_ALIGN_X 16U
#define OVERLAY_COMPRESS_ALIGN_Y 4U
#define OVERLAY_COMPRESSED_TILE_BYTES \
    (OVERLAY_COMPRESS_ALIGN_X * OVERLAY_COMPRESS_ALIGN_Y)
/* Keep the clean backing captured from the latest real GPU frame while the
 * compositor refreshes a copied, retained frame. Set to 1 only for A/B testing
 * the former behavior that re-captured backing after every idle refresh. */
#define OVERLAY_IDLE_RECAPTURE_BACKING 0

typedef struct
{
    void *buffer;
    void (*free)(void *frame, void *args);
    void *args;
} overlay_release_record_t;

typedef struct
{
    uint8_t count;
    overlay_release_record_t records[OVERLAY_DEFERRED_RELEASE_MAX];
} overlay_release_batch_t;

typedef struct
{
    bool allocated;
    bool dirty;
    uint16_t generation;
    bk_gpu_overlay_layer_desc_t desc;
    void *update_buffer;
    bk_gpu_overlay_layer_update_config_t update_config;
    void *display_buffer;
    bk_gpu_overlay_layer_update_config_t display_config;
    void *backing_buffer;
    uint16_t backing_x;
    uint16_t backing_y;
    uint16_t backing_w;
    uint16_t backing_h;
    void *capture_buffer;
    uint16_t capture_x;
    uint16_t capture_y;
    uint16_t capture_w;
    uint16_t capture_h;
    bool capture_failed;
} bk_gpu_overlay_layer_t;

struct bk_gpu_overlay
{
    bk_gpu_ctlr_handle_t gpu;
    beken_mutex_t mutex;
    bool deleting;
    bool flexa_frame_compose_failed;
    bk_gpu_overlay_layer_t layers[BK_GPU_OVERLAY_LAYER_MAX];
};

static bool overlay_rotation_valid(uint16_t rotation)
{
    return rotation == 0U || rotation == 90U ||
           rotation == 180U || rotation == 270U;
}

static bool overlay_rect_valid(const bk_gpu_overlay_rect_t *rect,
                               bool allow_empty)
{
    bool empty = rect->width == 0U || rect->height == 0U;
    return allow_empty
               ? (!empty || (rect->width == 0U && rect->height == 0U))
               : !empty;
}

static bool overlay_rect_contains(const bk_gpu_overlay_rect_t *outer,
                                  const bk_gpu_overlay_rect_t *inner)
{
    if (outer->width == 0U)
    {
        return true;
    }
    return inner->x >= outer->x && inner->y >= outer->y &&
           (uint32_t)inner->x + inner->width <=
               (uint32_t)outer->x + outer->width &&
           (uint32_t)inner->y + inner->height <=
               (uint32_t)outer->y + outer->height;
}

static bool overlay_source_format_valid(bk_pixel_format_t format)
{
    switch (format)
    {
        case BK_PIXEL_FORMAT_RGB565:
        case BK_PIXEL_FORMAT_BGR565:
        case BK_PIXEL_FORMAT_RGB888:
        case BK_PIXEL_FORMAT_BGR888:
        case BK_PIXEL_FORMAT_ARGB8888:
        case BK_PIXEL_FORMAT_ABGR8888:
        case BK_PIXEL_FORMAT_RGBA8888:
        case BK_PIXEL_FORMAT_BGRA8888:
        case BK_PIXEL_FORMAT_NV12:
        case BK_PIXEL_FORMAT_YUYV:
            return true;
        default:
            return false;
    }
}

static bool overlay_submit_geometry_valid(
    const bk_gpu_overlay_layer_update_config_t *config,
    const bk_gpu_output_info_t *target)
{
    if (config->buffer_width == 0U || config->buffer_height == 0U ||
        config->src_width == 0U || config->src_height == 0U ||
        config->enable_alpha_blend > 1U ||
        !overlay_source_format_valid(config->src_format) ||
        (uint32_t)config->src_x + config->src_width >
            config->buffer_width ||
        (uint32_t)config->src_y + config->src_height >
            config->buffer_height)
    {
        return false;
    }

    if (config->src_format == BK_PIXEL_FORMAT_NV12 &&
        (((config->buffer_width | config->buffer_height |
           config->src_x | config->src_y |
           config->src_width | config->src_height) & 1U) != 0U))
    {
        return false;
    }
    if (config->src_format == BK_PIXEL_FORMAT_YUYV &&
        (((config->buffer_width | config->src_x |
           config->src_width) & 1U) != 0U))
    {
        return false;
    }

    bk_gpu_overlay_rect_t draw_rect = {
        .x = config->dst_x,
        .y = config->dst_y,
        .width = (config->rotation_degree == 90U ||
                  config->rotation_degree == 270U)
                     ? config->src_height
                     : config->src_width,
        .height = (config->rotation_degree == 90U ||
                   config->rotation_degree == 270U)
                      ? config->src_width
                      : config->src_height,
    };
    bk_gpu_overlay_rect_t target_rect = {
        .width = target->width,
        .height = target->height,
    };

    return target->width != 0U && target->height != 0U &&
           overlay_rect_contains(&config->footprint_rect, &draw_rect) &&
           overlay_rect_contains(&target_rect,
                                 &config->footprint_rect);
}

static bool overlay_layer_uses_backing(
    const bk_gpu_overlay_layer_t *layer)
{
    return layer->desc.backing_policy !=
           BK_GPU_OVERLAY_BACKING_NONE;
}

/* Caller holds overlay->mutex. Callback execution is deferred until unlocked. */
static void overlay_buffer_take(
    void **buffer, bk_gpu_overlay_layer_update_config_t *config,
                                overlay_release_batch_t *batch)
{
    if (*buffer != NULL)
    {
        /*
         * A batch can take at most update+display for every layer (delete or
         * clear-all); all other paths take fewer records.
         */
        overlay_release_record_t *record =
            &batch->records[batch->count++];
        record->buffer = *buffer;
        record->free = config->buffer_release_cb;
        record->args = config->release_user_data;
        *buffer = NULL;
        os_memset(config, 0, sizeof(*config));
    }
}

static void overlay_release_batch_run(overlay_release_batch_t *batch)
{
    for (uint8_t i = 0U; i < batch->count; i++)
    {
        overlay_release_record_t *record = &batch->records[i];
        if (record->free != NULL)
        {
            record->free(record->buffer, record->args);
        }
    }
    batch->count = 0U;
}

static void overlay_backing_free(bk_gpu_overlay_layer_t *layer)
{
    if (layer->backing_buffer != NULL)
    {
        bk_frame_buffer_free(layer->backing_buffer);
        layer->backing_buffer = NULL;
    }
    layer->backing_x = 0U;
    layer->backing_y = 0U;
    layer->backing_w = 0U;
    layer->backing_h = 0U;
}

static void overlay_capture_free(bk_gpu_overlay_layer_t *layer)
{
    if (layer->capture_buffer != NULL)
    {
        bk_frame_buffer_free(layer->capture_buffer);
        layer->capture_buffer = NULL;
    }
    layer->capture_x = 0U;
    layer->capture_y = 0U;
    layer->capture_w = 0U;
    layer->capture_h = 0U;
    layer->capture_failed = false;
}

static void overlay_layer_clear_locked(bk_gpu_overlay_layer_t *layer,
                                       overlay_release_batch_t *batch)
{
    overlay_buffer_take(&layer->update_buffer, &layer->update_config, batch);
    overlay_buffer_take(&layer->display_buffer, &layer->display_config, batch);
    layer->dirty = true;
    /*
     * Keep the last clean backing until the next refresh. A cleared/released
     * layer must first restore the pixels it covered on a retained frame.
     */
}

static bool overlay_layer_decode_locked(
    bk_gpu_overlay_handle_t overlay,
    bk_gpu_overlay_layer_handle_t handle,
    uint8_t *index)
{
    uint32_t encoded_index = handle & 0xFFU;
    uint16_t generation = (uint16_t)(handle >> 8);

    if (encoded_index == 0U || encoded_index > BK_GPU_OVERLAY_LAYER_MAX)
    {
        return false;
    }
    uint8_t slot = (uint8_t)(encoded_index - 1U);
    if (!overlay->layers[slot].allocated ||
        overlay->layers[slot].generation != generation)
    {
        return false;
    }
    *index = slot;
    return true;
}

static uint8_t overlay_draw_order_locked(
    const bk_gpu_overlay_handle_t overlay, uint32_t layer_mask,
    uint8_t order[BK_GPU_OVERLAY_LAYER_MAX])
{
    uint8_t count = 0U;

    for (uint8_t index = 0U; index < BK_GPU_OVERLAY_LAYER_MAX; index++)
    {
        if ((layer_mask & (1UL << index)) == 0U ||
            overlay->layers[index].display_buffer == NULL)
        {
            continue;
        }
        uint8_t pos = count;
        while (pos > 0U)
        {
            uint8_t previous = order[pos - 1U];
            if (overlay->layers[previous].desc.z_order <=
                overlay->layers[index].desc.z_order)
            {
                break;
            }
            order[pos] = previous;
            pos--;
        }
        order[pos] = index;
        count++;
    }
    return count;
}

static void overlay_promote_locked(bk_gpu_overlay_layer_t *layer,
                                   overlay_release_batch_t *batch)
{
    if (layer->display_buffer != NULL && layer->update_buffer != NULL)
    {
        overlay_buffer_take(&layer->display_buffer,
                            &layer->display_config, batch);
    }
    if (layer->update_buffer != NULL)
    {
        layer->display_buffer = layer->update_buffer;
        layer->display_config = layer->update_config;
        layer->update_buffer = NULL;
        os_memset(&layer->update_config, 0, sizeof(layer->update_config));
        if (!overlay_layer_uses_backing(layer))
        {
            overlay_backing_free(layer);
            overlay_capture_free(layer);
        }
    }
}

/*
 * Public pixel format to VG-Lite format map for overlay sprites. Kept local so
 * the overlay stays independent of the GPU controller's internal headers; must
 * stay in sync with the controller's own video-path converter.
 */
static vg_lite_buffer_format_t overlay_pixel_format_to_vglite(
    bk_pixel_format_t bk_format)
{
    switch (bk_format)
    {
        case BK_PIXEL_FORMAT_RGB565:   return VG_LITE_BGR565;
        case BK_PIXEL_FORMAT_BGR565:   return VG_LITE_RGB565;
        case BK_PIXEL_FORMAT_RGB888:   return VG_LITE_BGR888;
        case BK_PIXEL_FORMAT_BGR888:   return VG_LITE_RGB888;
        case BK_PIXEL_FORMAT_ARGB8888: return VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_ABGR8888: return VG_LITE_RGBA8888;
        case BK_PIXEL_FORMAT_RGBA8888: return VG_LITE_ABGR8888;
        case BK_PIXEL_FORMAT_BGRA8888: return VG_LITE_ARGB8888;
        case BK_PIXEL_FORMAT_NV12:     return VG_LITE_NV12;
        case BK_PIXEL_FORMAT_YUYV:     return VG_LITE_YUYV;
        default:                       return (vg_lite_buffer_format_t)-1;
    }
}

static vg_lite_buffer_format_t overlay_blit_format_convert(
    bk_pixel_format_t bk_format)
{
    if (bk_format == BK_PIXEL_FORMAT_BGRA8888)
    {
        /*
         * The ISP SP path uses this public format as 32bpp BGRX. When a
         * rotated blit forces an alpha-aware path in VG-Lite, treating X as A
         * can make the overlay fully transparent.
         */
        return VG_LITE_BGRX8888;
    }
    return overlay_pixel_format_to_vglite(bk_format);
}

static void *overlay_blit_uv_plane_get(
    const bk_gpu_overlay_layer_update_config_t *blit_config,
    void *front_frame)
{
    if (blit_config == NULL || front_frame == NULL)
    {
        return NULL;
    }
    if (blit_config->src_format == BK_PIXEL_FORMAT_NV12)
    {
        return (uint8_t *)front_frame +
               ((uint32_t)blit_config->buffer_width *
                (uint32_t)blit_config->buffer_height);
    }
    return NULL;
}

/*
 * Blend one prepared overlay layer onto a complete GPU output frame. display
 * must already wrap the destination frame. The caller holds overlay->mutex and
 * the backend GPU lock (BK_GPU_IOCTL_LOCK).
 */
static avdk_err_t overlay_blit_layer_to_frame(
    vg_lite_buffer_t *display,
    const bk_gpu_overlay_layer_update_config_t *blit_config,
    void *front_frame)
{
    vg_lite_buffer_t front_buffer;
    vg_lite_matrix_t display_matrix;
    avdk_err_t result = AVDK_ERR_OK;

    if (display == NULL || blit_config == NULL || front_frame == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    os_memset(&front_buffer, 0, sizeof(front_buffer));
    /* buffer_width/height give the true stride; src_* may be a sub-rect. */
    front_buffer.width = blit_config->buffer_width
                             ? blit_config->buffer_width
                             : blit_config->src_width;
    front_buffer.height = blit_config->buffer_height
                              ? blit_config->buffer_height
                              : blit_config->src_height;
    front_buffer.format = overlay_blit_format_convert(blit_config->src_format);
    vg_lite_allocate_with_data(&front_buffer,
                               front_frame,
                               overlay_blit_uv_plane_get(blit_config, front_frame),
                               NULL,
                               NULL);

    os_memset(&display_matrix, 0, sizeof(display_matrix));
    vg_lite_identity(&display_matrix);

    vg_lite_rectangle_t rect = {
        .x = blit_config->src_x,
        .y = blit_config->src_y,
        .width = blit_config->src_width,
        .height = blit_config->src_height,
    };

    switch (blit_config->rotation_degree)
    {
        case 90:
            vg_lite_rotate(90.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x +
                                     (vg_lite_float_t)blit_config->src_height;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y;
            break;
        case 180:
            vg_lite_rotate(180.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x +
                                     (vg_lite_float_t)blit_config->src_width;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y +
                                     (vg_lite_float_t)blit_config->src_height;
            break;
        case 270:
            vg_lite_rotate(270.0f, &display_matrix);
            display_matrix.m[0][2] = (vg_lite_float_t)blit_config->dst_x;
            display_matrix.m[1][2] = (vg_lite_float_t)blit_config->dst_y +
                                     (vg_lite_float_t)blit_config->src_width;
            break;
        case 0:
        default:
            vg_lite_translate(blit_config->dst_x, blit_config->dst_y,
                              &display_matrix);
            break;
    }

    /*
     * Default keeps the legacy opaque copy; enable_alpha_blend selects SRC_OVER
     * so a transparent ARGB8888 OSD sprite composites correctly over the video.
     */
    vg_lite_blend_t blend_mode = blit_config->enable_alpha_blend
                                     ? VG_LITE_BLEND_SRC_OVER
                                     : VG_LITE_BLEND_NONE;
    /*
     * vg_lite_allocate_with_data marks the compressed destination as
     * screen_copy. VG-Lite may then reduce SRC_OVER to an opaque copy, which
     * exposes transparent sprite pixels as an intermittent black rectangle.
     * Clear it only for alpha composition and restore the descriptor afterward.
     */
    vg_lite_uint8_t saved_screen_copy = display->screen_copy;
    if (blend_mode != VG_LITE_BLEND_NONE)
    {
        display->screen_copy = 0U;
    }

    int ret = vg_lite_blit_rect(
        display, &front_buffer, &rect, &display_matrix,
        blend_mode, 0, VG_LITE_FILTER_POINT);
    if (ret != VG_LITE_SUCCESS)
    {
        LOGE("vg_lite_blit_rect failed, ret %d\r\n", ret);
        result = AVDK_ERR_HWERROR;
    }

    ret = vg_lite_finish();
    if (ret != VG_LITE_SUCCESS)
    {
        LOGE("vg_lite_finish failed, ret %d\r\n", ret);
        result = AVDK_ERR_HWERROR;
    }
    display->screen_copy = saved_screen_copy;

    vg_lite_free_without_free_data(&front_buffer);
    return result;
}

/* Blend one prepared overlay layer onto the current hardware FLEXA block. */
static avdk_err_t overlay_blit_layer_to_flexa_block(
    const bk_gpu_flexa_block_t *block,
    const bk_gpu_overlay_layer_update_config_t *bc,
    void *sprite, int32_t local_dst_x, int32_t local_dst_y)
{
    if (block == NULL || block->native_target == NULL ||
        bc == NULL || sprite == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    vg_lite_buffer_t *target = (vg_lite_buffer_t *)block->native_target;

    vg_lite_buffer_t front;
    os_memset(&front, 0, sizeof(front));
    front.width = bc->buffer_width ? bc->buffer_width : bc->src_width;
    front.height = bc->buffer_height ? bc->buffer_height : bc->src_height;
    front.format = overlay_blit_format_convert(bc->src_format);
    vg_lite_allocate_with_data(&front, sprite,
                               overlay_blit_uv_plane_get(bc, sprite),
                               NULL, NULL);

    /* Placement is already translated into block-local coordinates. */
    vg_lite_matrix_t m;
    os_memset(&m, 0, sizeof(m));
    vg_lite_identity(&m);
    switch (bc->rotation_degree)
    {
        case 90:
            vg_lite_rotate(90.0f, &m);
            m.m[0][2] = (vg_lite_float_t)local_dst_x +
                        (vg_lite_float_t)bc->src_height;
            m.m[1][2] = (vg_lite_float_t)local_dst_y;
            break;
        case 180:
            vg_lite_rotate(180.0f, &m);
            m.m[0][2] = (vg_lite_float_t)local_dst_x +
                        (vg_lite_float_t)bc->src_width;
            m.m[1][2] = (vg_lite_float_t)local_dst_y +
                        (vg_lite_float_t)bc->src_height;
            break;
        case 270:
            vg_lite_rotate(270.0f, &m);
            m.m[0][2] = (vg_lite_float_t)local_dst_x;
            m.m[1][2] = (vg_lite_float_t)local_dst_y +
                        (vg_lite_float_t)bc->src_width;
            break;
        case 0:
        default:
            vg_lite_translate(local_dst_x, local_dst_y, &m);
            break;
    }

    vg_lite_rectangle_t rect = {
        .x = bc->src_x,
        .y = bc->src_y,
        .width = bc->src_width,
        .height = bc->src_height,
    };

    vg_lite_blend_t blend_mode = bc->enable_alpha_blend
                                     ? VG_LITE_BLEND_SRC_OVER
                                     : VG_LITE_BLEND_NONE;

    /* compress path: clear screen_copy so SRC_OVER is not forced to BLEND_NONE. */
    vg_lite_uint8_t saved_screen_copy = target->screen_copy;
    if (blend_mode != VG_LITE_BLEND_NONE)
    {
        target->screen_copy = 0;
    }

    int ret = vg_lite_blit_rect(target, &front, &rect, &m,
                                blend_mode, 0, VG_LITE_FILTER_POINT);
    if (ret != VG_LITE_SUCCESS)
    {
        LOGE("%s, osd block blit failed %d\n", __func__, ret);
    }
    int finish_ret = vg_lite_finish();
    if (finish_ret != VG_LITE_SUCCESS)
    {
        LOGE("%s, osd block finish failed %d\n", __func__, finish_ret);
    }

    target->screen_copy = saved_screen_copy;
    vg_lite_free_without_free_data(&front);
    return (ret == VG_LITE_SUCCESS && finish_ret == VG_LITE_SUCCESS)
               ? AVDK_ERR_OK
               : AVDK_ERR_HWERROR;
}

static avdk_err_t overlay_display_init(
    bk_gpu_overlay_handle_t overlay, void *dst_buffer,
    vg_lite_buffer_t *display)
{
    bk_gpu_output_info_t target;
    avdk_err_t ret =
        bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_GET_OUTPUT_INFO, &target);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    if (!target.is_flexa || !target.is_compressed ||
        target.format != BK_PIXEL_FORMAT_ARGB8888)
    {
        return AVDK_ERR_UNSUPPORTED;
    }

    os_memset(display, 0, sizeof(*display));
    display->width = target.width;
    display->height = target.height;
    display->format = VG_LITE_BGRA8888;
    display->tiled = VG_LITE_TILED;
    display->compress_mode = VG_LITE_DEC_HV_SAMPLE;
    vg_lite_allocate_with_data(display, dst_buffer, NULL, NULL, NULL);
    return AVDK_ERR_OK;
}

static avdk_err_t overlay_backing_rect(
    const bk_gpu_overlay_layer_t *layer, const vg_lite_buffer_t *display,
    uint16_t *out_x, uint16_t *out_y, uint16_t *out_w, uint16_t *out_h)
{
    const bk_gpu_overlay_rect_t *rect =
        &layer->display_config.footprint_rect;
    uint32_t x = rect->x;
    uint32_t y = rect->y;
    uint32_t width = rect->width;
    uint32_t height = rect->height;

    /*
     * HV compression stores a fixed 16x4 pixel tile in 64 bytes. Backing must
     * cover complete tiles so capture and restore can copy the encoded bytes
     * verbatim without decoding and re-encoding edge pixels.
     */
    uint32_t x1 = x + width;
    uint32_t y1 = y + height;
    x &= ~(OVERLAY_COMPRESS_ALIGN_X - 1U);
    y &= ~(OVERLAY_COMPRESS_ALIGN_Y - 1U);
    x1 = (x1 + OVERLAY_COMPRESS_ALIGN_X - 1U) &
         ~(OVERLAY_COMPRESS_ALIGN_X - 1U);
    y1 = (y1 + OVERLAY_COMPRESS_ALIGN_Y - 1U) &
         ~(OVERLAY_COMPRESS_ALIGN_Y - 1U);
    width = x1 - x;
    height = y1 - y;

    if (x >= display->width || y >= display->height)
    {
        return AVDK_ERR_INVAL;
    }
    if (x + width > display->width)
    {
        width = display->width - x;
    }
    if (y + height > display->height)
    {
        height = display->height - y;
    }
    if (width == 0U || height == 0U)
    {
        return AVDK_ERR_INVAL;
    }

    *out_x = (uint16_t)x;
    *out_y = (uint16_t)y;
    *out_w = (uint16_t)width;
    *out_h = (uint16_t)height;
    return AVDK_ERR_OK;
}

static avdk_err_t overlay_compressed_tile_copy(
    void *dst, uint16_t dst_width, uint16_t dst_x, uint16_t dst_y,
    const void *src, uint16_t src_width, uint16_t src_x, uint16_t src_y,
    uint16_t width, uint16_t height)
{
    if (dst == NULL || src == NULL || dst_width == 0U || src_width == 0U ||
        width == 0U || height == 0U ||
        (dst_width % OVERLAY_COMPRESS_ALIGN_X) != 0U ||
        (src_width % OVERLAY_COMPRESS_ALIGN_X) != 0U ||
        (dst_x % OVERLAY_COMPRESS_ALIGN_X) != 0U ||
        (src_x % OVERLAY_COMPRESS_ALIGN_X) != 0U ||
        (dst_y % OVERLAY_COMPRESS_ALIGN_Y) != 0U ||
        (src_y % OVERLAY_COMPRESS_ALIGN_Y) != 0U ||
        (width % OVERLAY_COMPRESS_ALIGN_X) != 0U ||
        (height % OVERLAY_COMPRESS_ALIGN_Y) != 0U)
    {
        return AVDK_ERR_INVAL;
    }

    const uint32_t dst_stride =
        ((uint32_t)dst_width / OVERLAY_COMPRESS_ALIGN_X) *
        OVERLAY_COMPRESSED_TILE_BYTES;
    const uint32_t src_stride =
        ((uint32_t)src_width / OVERLAY_COMPRESS_ALIGN_X) *
        OVERLAY_COMPRESSED_TILE_BYTES;
    const uint32_t row_bytes =
        ((uint32_t)width / OVERLAY_COMPRESS_ALIGN_X) *
        OVERLAY_COMPRESSED_TILE_BYTES;
    uint8_t *dst_row = (uint8_t *)dst +
                       ((uint32_t)dst_y / OVERLAY_COMPRESS_ALIGN_Y) *
                           dst_stride +
                       ((uint32_t)dst_x / OVERLAY_COMPRESS_ALIGN_X) *
                           OVERLAY_COMPRESSED_TILE_BYTES;
    const uint8_t *src_row = (const uint8_t *)src +
                             ((uint32_t)src_y /
                              OVERLAY_COMPRESS_ALIGN_Y) *
                                 src_stride +
                             ((uint32_t)src_x /
                              OVERLAY_COMPRESS_ALIGN_X) *
                                 OVERLAY_COMPRESSED_TILE_BYTES;

    for (uint32_t row = 0U;
         row < (uint32_t)height / OVERLAY_COMPRESS_ALIGN_Y; row++)
    {
        os_memcpy(dst_row, src_row, row_bytes);
        dst_row += dst_stride;
        src_row += src_stride;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t overlay_capture_prepare(
    bk_gpu_overlay_layer_t *layer, const vg_lite_buffer_t *display)
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    avdk_err_t ret = overlay_backing_rect(
        layer, display, &x, &y, &width, &height);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    if (layer->capture_buffer == NULL ||
        layer->capture_w != width || layer->capture_h != height)
    {
        overlay_capture_free(layer);
        layer->capture_buffer = bk_frame_buffer_malloc(
            MEM_SLAB_HEAP_UNCODED, (uint32_t)width * height);
        if (layer->capture_buffer == NULL)
        {
            return AVDK_ERR_NOMEM;
        }
    }
    layer->capture_x = x;
    layer->capture_y = y;
    layer->capture_w = width;
    layer->capture_h = height;
    return AVDK_ERR_OK;
}

static void overlay_capture_commit(bk_gpu_overlay_layer_t *layer)
{
    void *buffer = layer->backing_buffer;
    uint16_t x = layer->backing_x;
    uint16_t y = layer->backing_y;
    uint16_t width = layer->backing_w;
    uint16_t height = layer->backing_h;

    layer->backing_buffer = layer->capture_buffer;
    layer->backing_x = layer->capture_x;
    layer->backing_y = layer->capture_y;
    layer->backing_w = layer->capture_w;
    layer->backing_h = layer->capture_h;
    layer->capture_buffer = buffer;
    layer->capture_x = x;
    layer->capture_y = y;
    layer->capture_w = width;
    layer->capture_h = height;
}

static bool overlay_backing_matches_display(
    const bk_gpu_overlay_layer_t *layer, const vg_lite_buffer_t *display)
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;

    if (layer->backing_buffer == NULL ||
        overlay_backing_rect(layer, display, &x, &y, &width, &height) !=
            AVDK_ERR_OK)
    {
        return false;
    }
    return layer->backing_x == x && layer->backing_y == y &&
           layer->backing_w == width && layer->backing_h == height;
}

static bool overlay_layer_retains_idle_backing(
    const bk_gpu_overlay_layer_t *layer)
{
    return layer->display_config.footprint_rect.width != 0U &&
           layer->display_config.footprint_rect.height != 0U;
}

static avdk_err_t overlay_backing_capture(
    bk_gpu_overlay_layer_t *layer, vg_lite_buffer_t *display)
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    avdk_err_t ret = overlay_backing_rect(
        layer, display, &x, &y, &width, &height);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    overlay_capture_free(layer);

    if (layer->backing_buffer == NULL ||
        layer->backing_w != width || layer->backing_h != height)
    {
        overlay_backing_free(layer);
        layer->backing_buffer = bk_frame_buffer_malloc(
            MEM_SLAB_HEAP_UNCODED, (uint32_t)width * height);
        if (layer->backing_buffer == NULL)
        {
            return AVDK_ERR_NOMEM;
        }
    }

    ret = overlay_compressed_tile_copy(
        layer->backing_buffer, width, 0U, 0U,
        display->memory, (uint16_t)display->width, x, y, width, height);
    if (ret != AVDK_ERR_OK)
    {
        overlay_backing_free(layer);
        return ret;
    }
    layer->backing_x = x;
    layer->backing_y = y;
    layer->backing_w = width;
    layer->backing_h = height;
    return AVDK_ERR_OK;
}

static avdk_err_t overlay_backing_restore(
    bk_gpu_overlay_layer_t *layer, vg_lite_buffer_t *display)
{
    if (layer->backing_buffer == NULL)
    {
        return AVDK_ERR_OK;
    }

    return overlay_compressed_tile_copy(
        display->memory, (uint16_t)display->width,
        layer->backing_x, layer->backing_y,
        layer->backing_buffer, layer->backing_w, 0U, 0U,
        layer->backing_w, layer->backing_h);
}

/* overlay->mutex and the backend GPU lock are held by the caller. */
static avdk_err_t overlay_compose_locked(
    bk_gpu_overlay_handle_t overlay, void *dst_buffer,
    uint32_t layer_mask, bool restore_previous, bool promote,
    overlay_release_batch_t *releases)
{
    vg_lite_buffer_t display;
    OSD_BLIT_START();
    avdk_err_t ret = overlay_display_init(overlay, dst_buffer, &display);
    if (ret != AVDK_ERR_OK)
    {
        OSD_BLIT_END();
        return ret;
    }

    if (restore_previous)
    {
        for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
        {
            if ((layer_mask & (1UL << i)) == 0U)
            {
                continue;
            }
            if (!overlay_layer_uses_backing(&overlay->layers[i]))
            {
                continue;
            }
            ret = overlay_backing_restore(&overlay->layers[i], &display);
            if (ret != AVDK_ERR_OK)
            {
                /*
                 * The destination may now be partially restored. Abort before
                 * promotion/draw and invalidate every retained backing so a
                 * later refresh cannot silently reuse an inconsistent set.
                 */
                for (uint8_t j = 0U; j < BK_GPU_OVERLAY_LAYER_MAX; j++)
                {
                    overlay_backing_free(&overlay->layers[j]);
                    overlay_capture_free(&overlay->layers[j]);
                }
                vg_lite_free_without_free_data(&display);
                OSD_BLIT_END();
                return ret;
            }
        }
    }

    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if ((layer_mask & (1UL << i)) == 0U)
        {
            continue;
        }
        if (promote)
        {
            overlay_promote_locked(layer, releases);
        }
        if (layer->display_buffer == NULL)
        {
            overlay_backing_free(layer);
            overlay_capture_free(layer);
        }
    }

    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if ((layer_mask & (1UL << i)) != 0U &&
            layer->display_buffer != NULL &&
            overlay_layer_uses_backing(layer))
        {
            bool capture_backing =
                !restore_previous || OVERLAY_IDLE_RECAPTURE_BACKING ||
                !overlay_layer_retains_idle_backing(layer) ||
                !overlay_backing_matches_display(layer, &display);
            if (capture_backing)
            {
                ret = overlay_backing_capture(layer, &display);
                if (ret != AVDK_ERR_OK)
                {
                    break;
                }
            }
        }
    }
    if (ret != AVDK_ERR_OK)
    {
        vg_lite_free_without_free_data(&display);
        for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
        {
            overlay_backing_free(&overlay->layers[i]);
            overlay_capture_free(&overlay->layers[i]);
        }
        OSD_BLIT_END();
        return ret;
    }

    uint8_t draw_order[BK_GPU_OVERLAY_LAYER_MAX];
    uint8_t draw_count =
        overlay_draw_order_locked(overlay, layer_mask, draw_order);
    for (uint8_t pos = 0U; pos < draw_count; pos++)
    {
        uint8_t i = draw_order[pos];
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        OSD_SLOT_START();
        ret = overlay_blit_layer_to_frame(
            &display, &layer->display_config,
            layer->display_buffer);
        OSD_SLOT_END();
        if (ret != AVDK_ERR_OK)
        {
            break;
        }
    }
    vg_lite_free_without_free_data(&display);
    if (ret == AVDK_ERR_OK)
    {
        for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
        {
            if ((layer_mask & (1UL << i)) != 0U)
            {
                overlay->layers[i].dirty = false;
            }
        }
    }
    OSD_BLIT_END();
    return ret;
}

/* Allocate + init an overlay object without claiming a compositor role. */
static avdk_err_t overlay_alloc(bk_gpu_overlay_handle_t *handle,
                                bk_gpu_ctlr_handle_t gpu)
{
    bk_gpu_overlay_handle_t overlay = os_malloc(sizeof(*overlay));
    if (overlay == NULL)
    {
        return AVDK_ERR_NOMEM;
    }
    os_memset(overlay, 0, sizeof(*overlay));
    overlay->gpu = gpu;
    avdk_err_t ret = rtos_init_mutex(&overlay->mutex);
    if (ret != AVDK_ERR_OK)
    {
        os_free(overlay);
        return ret;
    }
    *handle = overlay;
    return AVDK_ERR_OK;
}

/* Tear down an overlay object; does not touch controller compositor ownership. */
static void overlay_destroy(bk_gpu_overlay_handle_t overlay)
{
    overlay_release_batch_t releases = {0};

    rtos_lock_mutex(&overlay->mutex);
    overlay->deleting = true;
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        overlay_layer_clear_locked(&overlay->layers[i], &releases);
        overlay_backing_free(&overlay->layers[i]);
        overlay_capture_free(&overlay->layers[i]);
        overlay->layers[i].allocated = false;
    }
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    rtos_deinit_mutex(&overlay->mutex);
    os_free(overlay);
}

avdk_err_t bk_gpu_overlay_new(bk_gpu_overlay_handle_t *handle,
                              bk_gpu_ctlr_handle_t gpu)
{
    if (handle == NULL || gpu == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    /*
     * Claim the OVERLAY compositor role first so a controller already driven by
     * the legacy bk_gpu_blit_set() path is rejected instead of silently running
     * two compositors on the same output frame.
     */
    bk_gpu_compositor_owner_t owner = BK_GPU_COMPOSITOR_OWNER_OVERLAY;
    avdk_err_t ret = bk_gpu_ioctl(gpu, BK_GPU_IOCTL_CLAIM_COMPOSITOR, &owner);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    ret = overlay_alloc(handle, gpu);
    if (ret != AVDK_ERR_OK)
    {
        owner = BK_GPU_COMPOSITOR_OWNER_NONE;
        (void)bk_gpu_ioctl(gpu, BK_GPU_IOCTL_CLAIM_COMPOSITOR, &owner);
        return ret;
    }
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_overlay_delete(bk_gpu_overlay_handle_t overlay)
{
    if (overlay == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    bk_gpu_ctlr_handle_t gpu = overlay->gpu;
    overlay_destroy(overlay);
    bk_gpu_compositor_owner_t owner = BK_GPU_COMPOSITOR_OWNER_NONE;
    (void)bk_gpu_ioctl(gpu, BK_GPU_IOCTL_CLAIM_COMPOSITOR, &owner);
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_overlay_layer_acquire(
    bk_gpu_overlay_handle_t overlay,
    const bk_gpu_overlay_layer_desc_t *desc,
    bk_gpu_overlay_layer_handle_t *layer_handle)
{
    if (overlay == NULL || desc == NULL || layer_handle == NULL ||
        (desc->backing_policy != BK_GPU_OVERLAY_BACKING_REQUIRED &&
         desc->backing_policy != BK_GPU_OVERLAY_BACKING_NONE))
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if (!layer->allocated && layer->update_buffer == NULL &&
            layer->display_buffer == NULL &&
            layer->backing_buffer == NULL &&
            layer->capture_buffer == NULL && !layer->dirty)
        {
            uint16_t generation = (uint16_t)(layer->generation + 1U);
            if (generation == 0U)
            {
                generation = 1U;
            }
            layer->generation = generation;
            layer->desc = *desc;
            layer->allocated = true;
            *layer_handle = ((uint32_t)generation << 8) |
                            (uint32_t)(i + 1U);
            rtos_unlock_mutex(&overlay->mutex);
            return AVDK_ERR_OK;
        }
    }
    rtos_unlock_mutex(&overlay->mutex);
    return AVDK_ERR_NOMEM;
}

avdk_err_t bk_gpu_overlay_layer_submit_region(
    bk_gpu_overlay_handle_t overlay,
    bk_gpu_overlay_layer_handle_t layer_handle,
    void *src_buffer,
    const bk_gpu_overlay_layer_update_config_t *config)
{
    uint8_t index;
    bk_gpu_output_info_t target;
    overlay_release_batch_t releases = {0};
    if (overlay == NULL || src_buffer == NULL || config == NULL ||
        config->buffer_release_cb == NULL ||
        !overlay_rotation_valid(config->rotation_degree) ||
        !overlay_rect_valid(&config->footprint_rect, false))
    {
        return AVDK_ERR_INVAL;
    }
    avdk_err_t ret =
        bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_GET_OUTPUT_INFO, &target);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    if (!overlay_submit_geometry_valid(config, &target))
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting ||
        !overlay_layer_decode_locked(overlay, layer_handle, &index))
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    bk_gpu_overlay_layer_t *layer = &overlay->layers[index];
    overlay_buffer_take(&layer->update_buffer, &layer->update_config,
                        &releases);
    layer->update_buffer = src_buffer;
    layer->update_config = *config;
    layer->dirty = true;
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_overlay_layer_submit(
    bk_gpu_overlay_handle_t overlay,
    bk_gpu_overlay_layer_handle_t layer_handle,
    void *src_buffer,
    const bk_gpu_overlay_layer_submit_config_t *config)
{
    if (config == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    bk_gpu_overlay_layer_update_config_t update = {
        .buffer_width = config->width,
        .buffer_height = config->height,
        .src_width = config->width,
        .src_height = config->height,
        .src_format = config->src_format,
        .dst_x = config->dst_x,
        .dst_y = config->dst_y,
        .rotation_degree = config->rotation_degree,
        .enable_alpha_blend = config->enable_alpha_blend,
        .footprint_rect = {
            .x = config->dst_x,
            .y = config->dst_y,
            .width = (config->rotation_degree == 90U ||
                      config->rotation_degree == 270U)
                         ? config->height
                         : config->width,
            .height = (config->rotation_degree == 90U ||
                       config->rotation_degree == 270U)
                          ? config->width
                          : config->height,
        },
        .release_user_data = config->release_user_data,
        .buffer_release_cb = config->buffer_release_cb,
    };
    return bk_gpu_overlay_layer_submit_region(
        overlay, layer_handle, src_buffer, &update);
}

avdk_err_t bk_gpu_overlay_layer_clear(
    bk_gpu_overlay_handle_t overlay,
    bk_gpu_overlay_layer_handle_t layer_handle)
{
    uint8_t index;
    overlay_release_batch_t releases = {0};
    if (overlay == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting ||
        !overlay_layer_decode_locked(overlay, layer_handle, &index))
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    overlay_layer_clear_locked(&overlay->layers[index], &releases);
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_overlay_layer_release(
    bk_gpu_overlay_handle_t overlay,
    bk_gpu_overlay_layer_handle_t layer_handle)
{
    uint8_t index;
    overlay_release_batch_t releases = {0};
    if (overlay == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting ||
        !overlay_layer_decode_locked(overlay, layer_handle, &index))
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    overlay_layer_clear_locked(&overlay->layers[index], &releases);
    overlay->layers[index].allocated = false;
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return AVDK_ERR_OK;
}

uint8_t bk_gpu_overlay_get_available_layer_count(
    bk_gpu_overlay_handle_t overlay)
{
    uint8_t count = 0U;
    if (overlay == NULL)
    {
        return 0U;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return 0U;
    }
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if (!layer->allocated && layer->update_buffer == NULL &&
            layer->display_buffer == NULL &&
            layer->backing_buffer == NULL && !layer->dirty)
        {
            count++;
        }
    }
    rtos_unlock_mutex(&overlay->mutex);
    return count;
}

static avdk_err_t overlay_compose(
    bk_gpu_overlay_handle_t overlay, void *dst_buffer,
    uint32_t layer_mask, bool restore_previous)
{
    overlay_release_batch_t releases = {0};

    if (overlay == NULL || dst_buffer == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_LOCK, NULL);
    avdk_err_t ret = overlay_compose_locked(
        overlay, dst_buffer, layer_mask, restore_previous, true, &releases);
    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_UNLOCK, NULL);
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return ret;
}

avdk_err_t bk_gpu_overlay_compose_frame(
    bk_gpu_overlay_handle_t overlay, void *dst_buffer)
{
    return overlay_compose(overlay, dst_buffer,
                           BK_GPU_OVERLAY_ALL_LAYERS, false);
}

avdk_err_t bk_gpu_overlay_refresh_dirty_layers(
    bk_gpu_overlay_handle_t overlay, void *dst_buffer)
{
    overlay_release_batch_t releases = {0};
    uint32_t dirty_mask = 0U;

    if (overlay == NULL || dst_buffer == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        if (overlay->layers[i].dirty)
        {
            dirty_mask |= (1UL << i);
        }
    }
    if (dirty_mask == 0U)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_OK;
    }

    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_LOCK, NULL);
    avdk_err_t ret = overlay_compose_locked(
        overlay, dst_buffer, dirty_mask, true, true, &releases);
    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_UNLOCK, NULL);
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return ret;
}

avdk_err_t bk_gpu_overlay_commit_flexa_frame(
    bk_gpu_overlay_handle_t overlay)
{
    overlay_release_batch_t releases = {0};

    if (overlay == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    bool flexa_frame_failed = overlay->flexa_frame_compose_failed;
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        bool promote_pending = layer->update_buffer != NULL;
        if (layer->display_buffer != NULL)
        {
            if (flexa_frame_failed)
            {
                /*
                 * Keep the backing associated with the last complete frame.
                 * A failed FLEXA frame must not publish a partial candidate or
                 * mark any layer clean.
                 */
                overlay_capture_free(layer);
                layer->capture_failed = false;
            }
            else if (!overlay_layer_uses_backing(layer))
            {
                overlay_backing_free(layer);
                overlay_capture_free(layer);
                layer->dirty = false;
            }
            else if (!layer->capture_failed &&
                     layer->capture_buffer != NULL)
            {
                overlay_capture_commit(layer);
                layer->dirty = false;
            }
            else
            {
                overlay_backing_free(layer);
            }
        }
        overlay_promote_locked(&overlay->layers[i], &releases);
        if (promote_pending)
        {
            layer->dirty = true;
        }
    }
    overlay->flexa_frame_compose_failed = false;
    rtos_unlock_mutex(&overlay->mutex);
    overlay_release_batch_run(&releases);
    return flexa_frame_failed ? AVDK_ERR_HWERROR : AVDK_ERR_OK;
}

static bool overlay_flexa_rect_intersection(
    const bk_gpu_overlay_rect_t *rect,
    const bk_gpu_flexa_block_t *block,
    uint16_t *x, uint16_t *y, uint16_t *width, uint16_t *height)
{
    int32_t x0 = (int32_t)rect->x > block->frame_x
                     ? (int32_t)rect->x : block->frame_x;
    int32_t y0 = (int32_t)rect->y > block->frame_y
                     ? (int32_t)rect->y : block->frame_y;
    int32_t rect_x1 = (int32_t)rect->x + rect->width;
    int32_t rect_y1 = (int32_t)rect->y + rect->height;
    int32_t block_x1 = block->frame_x + block->frame_width;
    int32_t block_y1 = block->frame_y + block->frame_height;
    int32_t x1 = rect_x1 < block_x1 ? rect_x1 : block_x1;
    int32_t y1 = rect_y1 < block_y1 ? rect_y1 : block_y1;

    if (x0 >= x1 || y0 >= y1)
    {
        return false;
    }
    if (x != NULL)
    {
        *x = (uint16_t)x0;
    }
    if (y != NULL)
    {
        *y = (uint16_t)y0;
    }
    if (width != NULL)
    {
        *width = (uint16_t)(x1 - x0);
    }
    if (height != NULL)
    {
        *height = (uint16_t)(y1 - y0);
    }
    return true;
}

static bk_gpu_overlay_rect_t overlay_layer_frame_rect(
    const bk_gpu_overlay_layer_update_config_t *config)
{
    bk_gpu_overlay_rect_t rect = config->footprint_rect;
    if (rect.width == 0U || rect.height == 0U)
    {
        rect.x = config->dst_x;
        rect.y = config->dst_y;
        rect.width = (config->rotation_degree == 90U ||
                      config->rotation_degree == 270U)
                         ? config->src_height
                         : config->src_width;
        rect.height = (config->rotation_degree == 90U ||
                       config->rotation_degree == 270U)
                          ? config->src_width
                          : config->src_height;
    }
    return rect;
}

static avdk_err_t overlay_flexa_capture_backing(
    const bk_gpu_flexa_block_t *block,
    bk_gpu_overlay_layer_t *layer)
{
    bk_gpu_overlay_rect_t capture_rect = {
        .x = layer->capture_x,
        .y = layer->capture_y,
        .width = layer->capture_w,
        .height = layer->capture_h,
    };
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;

    if (!overlay_flexa_rect_intersection(
            &capture_rect, block, &x, &y, &width, &height))
    {
        return AVDK_ERR_OK;
    }
    if (block->compress_mode != VG_LITE_DEC_HV_SAMPLE)
    {
        return AVDK_ERR_INVAL;
    }
    return overlay_compressed_tile_copy(
        layer->capture_buffer, layer->capture_w,
        (uint16_t)(x - layer->capture_x),
        (uint16_t)(y - layer->capture_y),
        block->memory, block->local_width,
        (uint16_t)(x - block->frame_x),
        (uint16_t)(y - block->frame_y),
        width, height);
}

avdk_err_t bk_gpu_overlay_compose_flexa_block(
    bk_gpu_overlay_handle_t overlay,
    const bk_gpu_flexa_block_t *block)
{
    vg_lite_buffer_t display;
    avdk_err_t ret = AVDK_ERR_OK;

    if (overlay == NULL || block == NULL || block->block_index == 0U ||
        block->memory == NULL || block->native_target == NULL ||
        block->frame_width == 0U || block->frame_height == 0U ||
        block->frame_total_width == 0U || block->frame_total_height == 0U)
    {
        return AVDK_ERR_INVAL;
    }
    rtos_lock_mutex(&overlay->mutex);
    if (overlay->deleting)
    {
        rtos_unlock_mutex(&overlay->mutex);
        return AVDK_ERR_INVAL;
    }
    os_memset(&display, 0, sizeof(display));
    display.width = block->frame_total_width;
    display.height = block->frame_total_height;

    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_LOCK, NULL);
    OSD_BLIT_START();
    if (block->block_index == 1U)
    {
        overlay->flexa_frame_compose_failed = false;
    }
    /*
     * Capture every clean intersection before drawing any layer. This keeps
     * overlapping layers from becoming part of one another's retained backing.
     * capture_buffer is committed only at frame end, so an aborted FLEXA frame
     * cannot corrupt the backing associated with the frame still on screen.
     */
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if (layer->display_buffer != NULL &&
            overlay_layer_uses_backing(layer))
        {
            if (block->block_index == 1U)
            {
                layer->capture_failed = false;
            }
            if (!layer->capture_failed)
            {
                avdk_err_t capture_ret =
                    overlay_capture_prepare(layer, &display);
                if (capture_ret == AVDK_ERR_OK)
                {
                    capture_ret =
                        overlay_flexa_capture_backing(block, layer);
                }
                if (capture_ret != AVDK_ERR_OK)
                {
                    layer->capture_failed = true;
                    overlay->flexa_frame_compose_failed = true;
                    if (ret == AVDK_ERR_OK)
                    {
                        ret = capture_ret;
                    }
                }
            }
        }
    }
    uint8_t draw_order[BK_GPU_OVERLAY_LAYER_MAX];
    uint8_t draw_count = overlay_draw_order_locked(
        overlay, BK_GPU_OVERLAY_ALL_LAYERS, draw_order);
    for (uint8_t pos = 0U; pos < draw_count; pos++)
    {
        uint8_t i = draw_order[pos];
        bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        bk_gpu_overlay_rect_t layer_rect =
            overlay_layer_frame_rect(&layer->display_config);
        if (!overlay_flexa_rect_intersection(
                &layer_rect, block, NULL, NULL, NULL, NULL))
        {
            continue;
        }
        OSD_SLOT_START();
        avdk_err_t blit_ret =
            overlay_blit_layer_to_flexa_block(
                block, &layer->display_config, layer->display_buffer,
                (int32_t)layer->display_config.dst_x - block->frame_x,
                (int32_t)layer->display_config.dst_y - block->frame_y);
        OSD_SLOT_END();
        if (ret == AVDK_ERR_OK && blit_ret != AVDK_ERR_OK)
        {
            ret = blit_ret;
        }
        if (blit_ret != AVDK_ERR_OK)
        {
            overlay->flexa_frame_compose_failed = true;
        }
    }
    OSD_BLIT_END();
    (void)bk_gpu_ioctl(overlay->gpu, BK_GPU_IOCTL_UNLOCK, NULL);
    rtos_unlock_mutex(&overlay->mutex);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* Public blit API (bk_gpu.h): bk_gpu_blit_set/clear/clear_slot.              */
/*                                                                           */
/* These map the slot-based blit API onto the one shared overlay bound to the */
/* controller. The controller drives compose through the registered           */
/* frame_composer hook every output frame, so blit, bk_draw_osd and any other */
/* layers on the shared overlay coexist and are composited together.          */
/* ------------------------------------------------------------------------- */

/*
 * Controller-driven compose hooks: the GPU controller calls these on the
 * shared overlay so applications never touch the overlay compose API. The
 * controller selects the hook by its compose timing (frame-done vs per-FLEXA
 * block); see bk_gpu_frame_composer_t.
 */
static void overlay_compose_frame_thunk(void *ctx, void *dst_frame,
                                        uint32_t frame_size)
{
    (void)frame_size;
    bk_gpu_overlay_handle_t overlay = (bk_gpu_overlay_handle_t)ctx;
    if (overlay != NULL && dst_frame != NULL)
    {
        (void)bk_gpu_overlay_compose_frame(overlay, dst_frame);
    }
}

static void overlay_compose_flexa_block_thunk(void *ctx,
                                              const bk_gpu_flexa_block_t *block)
{
    bk_gpu_overlay_handle_t overlay = (bk_gpu_overlay_handle_t)ctx;
    if (overlay != NULL && block != NULL)
    {
        (void)bk_gpu_overlay_compose_flexa_block(overlay, block);
    }
}

static void overlay_commit_flexa_frame_thunk(void *ctx)
{
    bk_gpu_overlay_handle_t overlay = (bk_gpu_overlay_handle_t)ctx;
    if (overlay != NULL)
    {
        (void)bk_gpu_overlay_commit_flexa_frame(overlay);
    }
}

/* Driven by BK_GPU_IOCTL_REFRESH_DIRTY to push OSD/blit changes during a stall. */
static avdk_err_t overlay_refresh_dirty_thunk(void *ctx, void *bg_frame)
{
    bk_gpu_overlay_handle_t overlay = (bk_gpu_overlay_handle_t)ctx;
    if (overlay == NULL || bg_frame == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    return bk_gpu_overlay_refresh_dirty_layers(overlay, bg_frame);
}

/* Full compose-hook bundle registered on the controller for the shared overlay. */
static bk_gpu_frame_composer_t overlay_frame_composer(bk_gpu_overlay_handle_t overlay,
                                                      void (*destroy)(void *))
{
    bk_gpu_frame_composer_t reg = {
        .ctx = overlay,
        .compose_frame = overlay_compose_frame_thunk,
        .compose_flexa_block = overlay_compose_flexa_block_thunk,
        .commit_flexa_frame = overlay_commit_flexa_frame_thunk,
        .refresh_dirty = overlay_refresh_dirty_thunk,
        .destroy = destroy,
    };
    return reg;
}

/* Invoked from gpu_ctlr_delete so a controller-owned overlay dies with it.
 * Shared by the legacy blit shim and bk_gpu_get_overlay(). */
static void overlay_ctlr_destroy_thunk(void *ctx)
{
    bk_gpu_overlay_handle_t overlay = (bk_gpu_overlay_handle_t)ctx;
    if (overlay != NULL)
    {
        overlay_destroy(overlay);
    }
}

/* Get, or lazily create + register, the hidden legacy overlay bound to gpu. */
static avdk_err_t legacy_overlay_get(bk_gpu_ctlr_handle_t gpu,
                                     bk_gpu_overlay_handle_t *out)
{
    bk_gpu_frame_composer_t composer;
    avdk_err_t ret =
        bk_gpu_ioctl(gpu, BK_GPU_IOCTL_GET_FRAME_COMPOSER, &composer);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    if (composer.ctx != NULL)
    {
        /* Overlay already exists (created by OSD/PIP/blit). get_overlay always
         * registers the full compose-hook bundle, so nothing to upgrade here. */
        *out = (bk_gpu_overlay_handle_t)composer.ctx;
        return AVDK_ERR_OK;
    }

    bk_gpu_overlay_handle_t overlay = NULL;
    ret = overlay_alloc(&overlay, gpu);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    bk_gpu_frame_composer_t reg =
        overlay_frame_composer(overlay, overlay_ctlr_destroy_thunk);
    ret = bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_FRAME_COMPOSER, &reg);
    if (ret == AVDK_ERR_OK)
    {
        *out = overlay;
        return AVDK_ERR_OK;
    }
    /* Registration lost: free our object. */
    overlay_destroy(overlay);
    if (ret == AVDK_ERR_RDYDONE)
    {
        /* Another thread registered first; reuse the winner. */
        ret = bk_gpu_ioctl(gpu, BK_GPU_IOCTL_GET_FRAME_COMPOSER, &composer);
        if (ret == AVDK_ERR_OK && composer.ctx != NULL)
        {
            *out = (bk_gpu_overlay_handle_t)composer.ctx;
            return AVDK_ERR_OK;
        }
        return AVDK_ERR_GENERIC;
    }
    /* AVDK_ERR_BUSY: the bk_gpu_overlay_* API already owns this controller. */
    return ret;
}

avdk_err_t bk_gpu_get_overlay(bk_gpu_ctlr_handle_t gpu,
                              bk_gpu_overlay_handle_t *out)
{
    if (gpu == NULL || out == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    /*
     * Return the single overlay bound to this controller, creating it on first
     * use. bk_draw_osd, the PIP/display compositor and product code all share
     * this one instance so OSD and PIP layers land on the same output frame.
     * The pointer + teardown live in the controller's frame_composer slot along
     * with the compose hooks the controller drives every frame, so the overlay
     * is composited automatically and destroyed together with the controller.
     */
    bk_gpu_frame_composer_t composer;
    avdk_err_t ret =
        bk_gpu_ioctl(gpu, BK_GPU_IOCTL_GET_FRAME_COMPOSER, &composer);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    if (composer.ctx != NULL)
    {
        *out = (bk_gpu_overlay_handle_t)composer.ctx;
        return AVDK_ERR_OK;
    }

    bk_gpu_overlay_handle_t overlay = NULL;
    ret = bk_gpu_overlay_new(&overlay, gpu); /* claims the OVERLAY compositor */
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    bk_gpu_frame_composer_t reg =
        overlay_frame_composer(overlay, overlay_ctlr_destroy_thunk);
    ret = bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_FRAME_COMPOSER, &reg);
    if (ret == AVDK_ERR_OK)
    {
        *out = overlay;
        return AVDK_ERR_OK;
    }
    /*
     * Store failed: tear down the object directly (overlay_destroy, not
     * bk_gpu_overlay_delete) so we do not release the OVERLAY compositor claim
     * that a concurrent winner may already rely on.
     */
    overlay_destroy(overlay);
    return ret;
}

/*
 * Map a blit osd_slot back to its leased layer handle. The blit shim acquires
 * one layer per slot with z_order == slot, so the layer whose z_order matches
 * the slot is that slot's layer; its handle is rebuilt from index+generation
 * the same way bk_gpu_overlay_layer_acquire() encodes it. Returns
 * BK_GPU_OVERLAY_LAYER_INVALID if the slot has no layer yet. Caller holds
 * overlay->mutex.
 */
static bk_gpu_overlay_layer_handle_t
legacy_slot_handle_locked(bk_gpu_overlay_handle_t overlay, uint8_t slot,
                          bk_gpu_overlay_backing_policy_t *policy_out)
{
    for (uint8_t i = 0U; i < BK_GPU_OVERLAY_LAYER_MAX; i++)
    {
        const bk_gpu_overlay_layer_t *layer = &overlay->layers[i];
        if (layer->allocated && layer->desc.z_order == (int16_t)slot)
        {
            if (policy_out != NULL)
            {
                *policy_out = layer->desc.backing_policy;
            }
            return ((uint32_t)layer->generation << 8) | (uint32_t)(i + 1U);
        }
    }
    return BK_GPU_OVERLAY_LAYER_INVALID;
}

avdk_err_t bk_gpu_blit_set(bk_gpu_ctlr_handle_t handle, void *src_buffer,
                           bk_gpu_blit_config_t *blit_config)
{
    if (handle == NULL || src_buffer == NULL || blit_config == NULL ||
        !overlay_rotation_valid(blit_config->rotate_degree))
    {
        return AVDK_ERR_INVAL;
    }
    uint8_t slot = blit_config->osd_slot;
    if (slot >= BK_GPU_OVERLAY_LAYER_MAX)
    {
        LOGW("blit osd_slot %u out of range, clamp to 0\r\n", (unsigned)slot);
        slot = 0U;
    }

    bk_gpu_overlay_handle_t overlay = NULL;
    avdk_err_t ret = legacy_overlay_get(handle, &overlay);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    rtos_lock_mutex(&overlay->mutex);
    bk_gpu_overlay_layer_handle_t layer =
        legacy_slot_handle_locked(overlay, slot, NULL);
    rtos_unlock_mutex(&overlay->mutex);

    if (layer == BK_GPU_OVERLAY_LAYER_INVALID)
    {
        /*
         * One layer per slot; z_order = slot both keeps the old slot-index draw
         * order and tags the layer so it can be found again on the next blit to
         * this slot. Blit layers use no backing: the layer is re-drawn on every
         * fresh GPU output frame (legacy behavior).
         */
        bk_gpu_overlay_layer_desc_t desc = {
            .z_order = (int16_t)slot,
            .backing_policy = BK_GPU_OVERLAY_BACKING_NONE,
        };
        ret = bk_gpu_overlay_layer_acquire(overlay, &desc, &layer);
        if (ret != AVDK_ERR_OK)
        {
            return ret;
        }
    }

    bool swap = (blit_config->rotate_degree == 90U ||
                 blit_config->rotate_degree == 270U);
    bk_gpu_overlay_layer_update_config_t update = {
        .buffer_width = blit_config->sprite_width
                            ? blit_config->sprite_width
                            : blit_config->src_width,
        .buffer_height = blit_config->sprite_height
                             ? blit_config->sprite_height
                             : blit_config->src_height,
        .src_x = blit_config->src_x,
        .src_y = blit_config->src_y,
        .src_width = blit_config->src_width,
        .src_height = blit_config->src_height,
        .src_format = blit_config->src_format,
        .dst_x = blit_config->dst_x,
        .dst_y = blit_config->dst_y,
        .rotation_degree = blit_config->rotate_degree,
        .enable_alpha_blend = blit_config->alpha_blend ? 1U : 0U,
        .footprint_rect = {
            .x = blit_config->dst_x,
            .y = blit_config->dst_y,
            .width = swap ? blit_config->src_height : blit_config->src_width,
            .height = swap ? blit_config->src_width : blit_config->src_height,
        },
        .release_user_data = blit_config->args,
        .buffer_release_cb = blit_config->free,
    };
    return bk_gpu_overlay_layer_submit_region(overlay, layer, src_buffer,
                                              &update);
}

/* Fetch the controller's shared overlay without creating it, or NULL. */
static bk_gpu_overlay_handle_t legacy_overlay_peek(bk_gpu_ctlr_handle_t handle)
{
    bk_gpu_frame_composer_t composer;
    if (bk_gpu_ioctl(handle, BK_GPU_IOCTL_GET_FRAME_COMPOSER, &composer) !=
        AVDK_ERR_OK)
    {
        return NULL;
    }
    return (bk_gpu_overlay_handle_t)composer.ctx;
}

/* Release the layer leased for one blit slot, if any. */
static void legacy_blit_release_slot(bk_gpu_overlay_handle_t overlay,
                                     uint8_t slot)
{
    rtos_lock_mutex(&overlay->mutex);
    bk_gpu_overlay_layer_handle_t layer =
        legacy_slot_handle_locked(overlay, slot, NULL);
    rtos_unlock_mutex(&overlay->mutex);
    if (layer != BK_GPU_OVERLAY_LAYER_INVALID)
    {
        (void)bk_gpu_overlay_layer_release(overlay, layer);
    }
}

avdk_err_t bk_gpu_blit_clear(bk_gpu_ctlr_handle_t handle)
{
    if (handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    bk_gpu_overlay_handle_t overlay = legacy_overlay_peek(handle);
    if (overlay == NULL)
    {
        return AVDK_ERR_OK; /* blit was never used on this controller */
    }
    /*
     * Release every layer the blit path leased. Each slot tags its layer with
     * z_order == slot, so find and release each by its slot tag.
     */
    for (uint8_t slot = 0U; slot < BK_GPU_OVERLAY_LAYER_MAX; slot++)
    {
        legacy_blit_release_slot(overlay, slot);
    }
    return AVDK_ERR_OK;
}

avdk_err_t bk_gpu_blit_clear_slot(bk_gpu_ctlr_handle_t handle, uint8_t slot)
{
    if (handle == NULL || slot >= BK_GPU_OVERLAY_LAYER_MAX)
    {
        return AVDK_ERR_INVAL;
    }
    bk_gpu_overlay_handle_t overlay = legacy_overlay_peek(handle);
    if (overlay == NULL)
    {
        return AVDK_ERR_OK; /* blit was never used on this controller */
    }
    legacy_blit_release_slot(overlay, slot);
    return AVDK_ERR_OK;
}
