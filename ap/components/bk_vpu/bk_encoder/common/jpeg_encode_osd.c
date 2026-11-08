#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_frame_buffer.h>
#include <cache.h>
#include <os/os.h>
#include <os/mem.h>
#include <stdint.h>

#include "jpeg_encode_osd_priv.h"
#include "modules/vcenc/vcenc_jpeg_api.h"

#define TAG "jpeg_encode_osd"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#define JPEG_OSD_XY_ALIGN        2U
#define JPEG_OSD_BITMAP_ALIGN    8U
#define JPEG_OSD_ADDR_ALIGN      4U
#define JPEG_OSD_BOX_MAX_COUNT   64U

static beken_mutex_t s_osd_slots_mutex;
static bool s_osd_mutex_ready;

void jpeg_encode_osd_module_init(void)
{
    if (s_osd_mutex_ready) {
        return;
    }

    if (rtos_init_mutex(&s_osd_slots_mutex) == BK_OK) {
        s_osd_mutex_ready = true;
    }
}

static void jpeg_encode_osd_slots_lock(void)
{
    if (!s_osd_mutex_ready) {
        jpeg_encode_osd_module_init();
    }
    if (s_osd_mutex_ready) {
        rtos_lock_mutex(&s_osd_slots_mutex);
    }
}

static void jpeg_encode_osd_slots_unlock(void)
{
    if (s_osd_mutex_ready) {
        rtos_unlock_mutex(&s_osd_slots_mutex);
    }
}

static void jpeg_encode_osd_invoke_buffer_free(void *buffer,
                                               bk_jpeg_encode_osd_buffer_free_cb_t buffer_free,
                                               void *free_arg)
{
    if (buffer != NULL && buffer_free != NULL) {
        buffer_free(buffer, free_arg);
    }
}

avdk_err_t jpeg_encode_osd_validate_common(uint32_t frame_w,
                                           uint32_t frame_h,
                                           const bk_jpeg_encode_osd_t *osd)
{
    uintptr_t buf_addr;

    if (osd == NULL) {
        LOGE("invalid arg: osd is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    if (osd->buffer == NULL) {
        return AVDK_ERR_OK;
    }

    buf_addr = (uintptr_t)osd->buffer;

    if (buf_addr == 0U) {
        LOGE("slot %u invalid buffer address 0\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    if ((buf_addr & (JPEG_OSD_ADDR_ALIGN - 1U)) != 0U) {
        LOGE("slot %u buffer %p not %u-byte aligned\r\n",
             osd->index, osd->buffer, JPEG_OSD_ADDR_ALIGN);
        return AVDK_ERR_INVAL;
    }

    if (osd->buffer_free == NULL) {
        LOGE("slot %u buffer_free is NULL\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    if (osd->format != BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888 &&
        osd->format != BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12 &&
        osd->format != BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP) {
        LOGE("slot %u unsupported format %u\r\n", osd->index, osd->format);
        return AVDK_ERR_INVAL;
    }

    if (osd->format == BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12 &&
        (((osd->width | osd->height) & 1U) != 0U)) {
        LOGE("slot %u NV12 size must be even: w=%u h=%u\r\n",
             osd->index, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if (osd->format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP &&
        (((osd->width | osd->height) & (JPEG_OSD_BITMAP_ALIGN - 1U)) != 0U)) {
        LOGE("slot %u bitmap size must be %u-pixel aligned: w=%u h=%u\r\n",
             osd->index, JPEG_OSD_BITMAP_ALIGN, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if (frame_w == 0U || frame_h == 0U) {
        LOGE("slot %u encoder frame size invalid: %ux%u\r\n",
             osd->index, frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }

    if (osd->width == 0U || osd->height == 0U) {
        LOGE("slot %u invalid size: w=%u h=%u\r\n",
             osd->index, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if ((osd->x & (JPEG_OSD_XY_ALIGN - 1U)) != 0U ||
        (osd->y & (JPEG_OSD_XY_ALIGN - 1U)) != 0U) {
        LOGE("slot %u offset not %u-pixel aligned: x=%u y=%u\r\n",
             osd->index, JPEG_OSD_XY_ALIGN, osd->x, osd->y);
        return AVDK_ERR_INVAL;
    }

    if (osd->x > UINT16_MAX || osd->y > UINT16_MAX ||
        osd->width > UINT16_MAX || osd->height > UINT16_MAX) {
        LOGE("slot %u geometry exceeds HW limit 65535: x=%u y=%u w=%u h=%u\r\n",
             osd->index, osd->x, osd->y, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if ((osd->x + osd->width) > frame_w || (osd->y + osd->height) > frame_h) {
        LOGE("slot %u out of frame %ux%u: x=%u y=%u w=%u h=%u\r\n",
             osd->index, frame_w, frame_h, osd->x, osd->y, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    return AVDK_ERR_OK;
}

static void jpeg_encode_osd_apply_slot_config(jpeg_encode_osd_slot_state_t *slot,
                                              const bk_jpeg_encode_osd_t *osd)
{
    slot->enabled = true;
    slot->buffer = osd->buffer;
    slot->buffer_free = osd->buffer_free;
    slot->free_arg = osd->free_arg;
    slot->format = osd->format;
    slot->alpha = osd->alpha;
    slot->x = osd->x;
    slot->y = osd->y;
    slot->width = osd->width;
    slot->height = osd->height;
    slot->bitmap_y = osd->bitmap_y;
    slot->bitmap_u = osd->bitmap_u;
    slot->bitmap_v = osd->bitmap_v;
}

static void jpeg_encode_osd_slot_clear_pending(jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    slot->enabled = false;
    slot->buffer = NULL;
    slot->buffer_free = NULL;
    slot->free_arg = NULL;
    slot->format = 0U;
    slot->alpha = 0U;
    slot->x = 0U;
    slot->y = 0U;
    slot->width = 0U;
    slot->height = 0U;
}

static void jpeg_encode_osd_slot_clear_active(jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    slot->active_buffer = NULL;
    slot->active_buffer_free = NULL;
    slot->active_free_arg = NULL;
    slot->active_enabled = false;
}

static void jpeg_encode_osd_slot_clear_retire(jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    slot->retire_buffer = NULL;
    slot->retire_buffer_free = NULL;
    slot->retire_free_arg = NULL;
}

static void jpeg_encode_osd_slot_release_retire(jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    jpeg_encode_osd_invoke_buffer_free(slot->retire_buffer,
                                       slot->retire_buffer_free,
                                       slot->retire_free_arg);
    jpeg_encode_osd_slot_clear_retire(slot);
}

static void jpeg_encode_osd_slot_release_pending(jpeg_encode_osd_slot_state_t *slot)
{
    void *buffer;
    bk_jpeg_encode_osd_buffer_free_cb_t buffer_free;
    void *free_arg;

    if (slot == NULL || slot->buffer == NULL) {
        jpeg_encode_osd_slot_clear_pending(slot);
        return;
    }

    buffer = slot->buffer;
    buffer_free = slot->buffer_free;
    free_arg = slot->free_arg;
    jpeg_encode_osd_slot_clear_pending(slot);
    jpeg_encode_osd_invoke_buffer_free(buffer, buffer_free, free_arg);
}

static void jpeg_encode_osd_slot_drop_uncommitted_pending(jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL || slot->buffer == NULL) {
        return;
    }

    if (slot->buffer == slot->active_buffer) {
        return;
    }

    jpeg_encode_osd_slot_release_pending(slot);
}

static void jpeg_encode_osd_slot_retire_buffer(jpeg_encode_osd_slot_state_t *slot,
                                               void *buffer,
                                               bk_jpeg_encode_osd_buffer_free_cb_t buffer_free,
                                               void *free_arg)
{
    if (slot == NULL || buffer == NULL) {
        return;
    }

    jpeg_encode_osd_slot_release_retire(slot);
    slot->retire_buffer = buffer;
    slot->retire_buffer_free = buffer_free;
    slot->retire_free_arg = free_arg;
}

static void jpeg_encode_osd_flush_slot(const jpeg_encode_osd_slot_state_t *slot)
{
    if (slot == NULL || !slot->enabled || slot->buffer == NULL) {
        return;
    }

    if (slot->format == BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888 &&
        slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)(slot->width * slot->height * 4U));
    } else if (slot->format == BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12 &&
               slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)(slot->width * slot->height * 3U / 2U));
    } else if (slot->format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP &&
               slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)((slot->width / 8U) * slot->height));
    }
}

static void jpeg_encode_osd_push_slot_to_vcenc(jpeg_enc_param_t *enc_param,
                                               const jpeg_encode_osd_slot_state_t *slot,
                                               uint32_t index)
{
    vcenc_ret_e ret;

    if (enc_param == NULL || slot == NULL || index >= JPEG_ENCODE_OSD_SLOT_COUNT) {
        return;
    }

    if (slot->enabled && slot->buffer != NULL) {
        ret = vcenc_jpeg_set_osd(enc_param, index, slot->buffer, slot->format, slot->alpha,
                                 slot->x, slot->y, slot->width, slot->height,
                                 slot->bitmap_y, slot->bitmap_u, slot->bitmap_v);
    } else {
        ret = vcenc_jpeg_set_osd(enc_param, index, NULL, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
                                  0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U);
    }

    if (ret != VCENC_OK) {
        LOGE("vcenc_jpeg_set_osd failed, index=%u ret=%d, buf=%p x=%u y=%u w=%u h=%u\r\n",
             index, ret, slot->buffer, slot->x, slot->y, slot->width, slot->height);
    }
}

void jpeg_encode_osd_sync_to_vcenc(jpeg_enc_param_t *enc_param,
                                   jpeg_encode_osd_slot_state_t *slots)
{
    if (enc_param == NULL || slots == NULL) {
        return;
    }

    jpeg_encode_osd_slots_lock();

    for (uint32_t i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
        jpeg_encode_osd_slot_state_t *slot = &slots[i];

        jpeg_encode_osd_flush_slot(slot);
        jpeg_encode_osd_push_slot_to_vcenc(enc_param, slot, i);

        if (slot->enabled && slot->buffer != NULL) {
            if (slot->active_buffer != NULL && slot->active_buffer != slot->buffer) {
                jpeg_encode_osd_slot_retire_buffer(slot,
                                                   slot->active_buffer,
                                                   slot->active_buffer_free,
                                                   slot->active_free_arg);
            }
            slot->active_enabled = true;
            slot->active_buffer = slot->buffer;
            slot->active_buffer_free = slot->buffer_free;
            slot->active_free_arg = slot->free_arg;
        } else if (slot->active_buffer != NULL) {
            jpeg_encode_osd_slot_retire_buffer(slot,
                                               slot->active_buffer,
                                               slot->active_buffer_free,
                                               slot->active_free_arg);
            jpeg_encode_osd_slot_clear_active(slot);
        } else {
            slot->active_enabled = false;
        }
    }

    jpeg_encode_osd_slots_unlock();
}

void jpeg_encode_osd_finish_frame(jpeg_encode_osd_slot_state_t *slots)
{
    if (slots == NULL) {
        return;
    }

    jpeg_encode_osd_slots_lock();
    for (uint32_t i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
        jpeg_encode_osd_slot_release_retire(&slots[i]);
    }
    jpeg_encode_osd_slots_unlock();
}

void jpeg_encode_osd_release_all(jpeg_enc_param_t *enc_param,
                                 jpeg_encode_osd_slot_state_t *slots)
{
    if (slots == NULL) {
        return;
    }

    jpeg_encode_osd_slots_lock();

    for (uint32_t i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
        jpeg_encode_osd_slot_state_t *slot = &slots[i];

        jpeg_encode_osd_slot_drop_uncommitted_pending(slot);
        jpeg_encode_osd_slot_clear_pending(slot);
    }

    if (enc_param != NULL) {
        for (uint32_t i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
            jpeg_encode_osd_slot_state_t *slot = &slots[i];

            /* Pending was cleared above, so this only disables the hardware slot.
             * Keep active_buffer/active_buffer_free intact here: the loop below
             * frees them, and clearing them here would leak every OSD buffer. */
            jpeg_encode_osd_flush_slot(slot);
            jpeg_encode_osd_push_slot_to_vcenc(enc_param, slot, i);
        }
    }

    for (uint32_t i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
        jpeg_encode_osd_slot_state_t *slot = &slots[i];

        jpeg_encode_osd_invoke_buffer_free(slot->active_buffer,
                                           slot->active_buffer_free,
                                           slot->active_free_arg);
        jpeg_encode_osd_slot_clear_active(slot);
        jpeg_encode_osd_slot_release_retire(slot);
    }

    jpeg_encode_osd_slots_unlock();
}

avdk_err_t jpeg_encode_set_osd_common(jpeg_enc_param_t *enc_param,
                                      bool encoder_inited,
                                      jpeg_encode_osd_slot_state_t *slots,
                                      bk_jpeg_encode_osd_t *osd)
{
    jpeg_encode_osd_slot_state_t *slot;
    avdk_err_t ret;

    if (osd == NULL || enc_param == NULL || slots == NULL) {
        LOGE("osd arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("jpeg encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }
    if (osd->index >= JPEG_ENCODE_OSD_SLOT_COUNT) {
        LOGE("invalid osd index: %u\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    jpeg_encode_osd_slots_lock();
    slot = &slots[osd->index];

    if (osd->buffer == NULL) {
        jpeg_encode_osd_slot_drop_uncommitted_pending(slot);
        jpeg_encode_osd_slot_clear_pending(slot);
        jpeg_encode_osd_slots_unlock();
        return AVDK_ERR_OK;
    }

    ret = jpeg_encode_osd_validate_common(enc_param->width, enc_param->height, osd);
    if (ret != AVDK_ERR_OK) {
        jpeg_encode_osd_slots_unlock();
        return ret;
    }

    if (osd->buffer == slot->buffer) {
        jpeg_encode_osd_apply_slot_config(slot, osd);
        jpeg_encode_osd_flush_slot(slot);
        jpeg_encode_osd_slots_unlock();
        return AVDK_ERR_OK;
    }

    jpeg_encode_osd_slot_drop_uncommitted_pending(slot);
    jpeg_encode_osd_apply_slot_config(slot, osd);
    jpeg_encode_osd_flush_slot(slot);

    jpeg_encode_osd_slots_unlock();
    return AVDK_ERR_OK;
}

static uint32_t jpeg_encode_osd_align_down(uint32_t value, uint32_t align)
{
    return value & ~(align - 1U);
}

static uint32_t jpeg_encode_osd_align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1U) & ~(align - 1U);
}

static uint8_t jpeg_encode_osd_clamp_u8(int value)
{
    return (uint8_t)((value < 0) ? 0 : ((value > 255) ? 255 : value));
}

/* BT.601 full-range luma of an RGB triplet, Y in 0..255 */
static uint8_t jpeg_encode_osd_rgb_luma(int r, int g, int b)
{
    return jpeg_encode_osd_clamp_u8((77 * r + 150 * g + 29 * b + 128) >> 8);
}

/* BT.601 full-range Cb of an RGB triplet, 128 = neutral */
static uint8_t jpeg_encode_osd_rgb_cb(int r, int g, int b)
{
    return jpeg_encode_osd_clamp_u8(((-43 * r - 85 * g + 128 * b + 128) >> 8) + 128);
}

/* BT.601 full-range Cr of an RGB triplet, 128 = neutral */
static uint8_t jpeg_encode_osd_rgb_cr(int r, int g, int b)
{
    return jpeg_encode_osd_clamp_u8(((128 * r - 107 * g - 21 * b + 128) >> 8) + 128);
}

static void jpeg_encode_osd_rgb_to_yuv(uint32_t color, uint8_t *y, uint8_t *u, uint8_t *v)
{
    int r = (int)((color >> 16) & 0xFFU);
    int g = (int)((color >> 8) & 0xFFU);
    int b = (int)(color & 0xFFU);

    *y = jpeg_encode_osd_rgb_luma(r, g, b);
    *u = jpeg_encode_osd_rgb_cb(r, g, b);
    *v = jpeg_encode_osd_rgb_cr(r, g, b);
}

static void jpeg_encode_osd_bitmap_set_bit(uint8_t *buffer, uint32_t width,
                                           uint32_t x, uint32_t y)
{
    uint32_t row_bytes = width / 8U;

    buffer[y * row_bytes + (x / 8U)] |= (uint8_t)(0x80U >> (x & 7U));
}

static void jpeg_encode_osd_bitmap_fill_rect(uint8_t *buffer, uint32_t canvas_w,
                                              uint32_t canvas_h, uint32_t x,
                                              uint32_t y, uint32_t w, uint32_t h)
{
    uint32_t row;
    uint32_t col;

    if (buffer == NULL || w == 0U || h == 0U || x >= canvas_w || y >= canvas_h) {
        return;
    }

    if ((x + w) > canvas_w) {
        w = canvas_w - x;
    }
    if ((y + h) > canvas_h) {
        h = canvas_h - y;
    }

    for (row = 0U; row < h; row++) {
        for (col = 0U; col < w; col++) {
            jpeg_encode_osd_bitmap_set_bit(buffer, canvas_w, x + col, y + row);
        }
    }
}

static void jpeg_encode_osd_bitmap_draw_box(uint8_t *buffer, uint32_t canvas_w,
                                            uint32_t canvas_h, uint32_t x,
                                            uint32_t y, uint32_t w, uint32_t h,
                                            uint32_t thickness)
{
    uint32_t t = thickness;

    if (w == 0U || h == 0U || t == 0U) {
        return;
    }

    if (t > (w / 2U)) {
        t = (w > 1U) ? (w / 2U) : 1U;
    }
    if (t > (h / 2U)) {
        t = (h > 1U) ? (h / 2U) : 1U;
    }

    /* top / bottom / left / right borders */
    jpeg_encode_osd_bitmap_fill_rect(buffer, canvas_w, canvas_h, x, y, w, t);
    jpeg_encode_osd_bitmap_fill_rect(buffer, canvas_w, canvas_h, x, y + h - t, w, t);
    jpeg_encode_osd_bitmap_fill_rect(buffer, canvas_w, canvas_h, x, y, t, h);
    jpeg_encode_osd_bitmap_fill_rect(buffer, canvas_w, canvas_h, x + w - t, y, t, h);
}

static void jpeg_encode_osd_bitmap_boxes_buffer_free(void *buffer, void *free_arg)
{
    (void)free_arg;
    if (buffer != NULL) {
        bk_frame_buffer_free(buffer);
    }
}

avdk_err_t jpeg_encode_build_osd_bitmap_boxes(uint32_t frame_w,
                                              uint32_t frame_h,
                                              bk_jpeg_encode_osd_bitmap_boxes_t *boxes,
                                              bk_jpeg_encode_osd_t *osd)
{
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t canvas_x;
    uint32_t canvas_y;
    uint32_t canvas_w;
    uint32_t canvas_h;
    uint32_t buf_size;
    uint8_t *bitmap = NULL;
    uint8_t yuv_y;
    uint8_t yuv_u;
    uint8_t yuv_v;
    uint32_t i;

    if (boxes == NULL || osd == NULL) {
        LOGE("osd boxes arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    os_memset(osd, 0, sizeof(*osd));

    if (boxes->index >= JPEG_ENCODE_OSD_SLOT_COUNT) {
        LOGE("invalid osd boxes index: %u\r\n", boxes->index);
        return AVDK_ERR_INVAL;
    }

    /* Clear slot when no boxes are provided. */
    if (boxes->count == 0U || boxes->boxes == NULL) {
        osd->index = boxes->index;
        osd->buffer = NULL;
        osd->format = BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP;
        return AVDK_ERR_OK;
    }

    if (boxes->count > JPEG_OSD_BOX_MAX_COUNT) {
        LOGE("osd boxes count %u exceeds max %u\r\n", boxes->count, JPEG_OSD_BOX_MAX_COUNT);
        return AVDK_ERR_INVAL;
    }

    if (frame_w == 0U || frame_h == 0U) {
        LOGE("encoder frame size invalid: %ux%u\r\n", frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }

    min_x = frame_w;
    min_y = frame_h;
    max_x = 0U;
    max_y = 0U;

    for (i = 0U; i < boxes->count; i++) {
        const bk_jpeg_encode_osd_bitmap_box_t *box = &boxes->boxes[i];
        uint32_t x1;
        uint32_t y1;

        if (box->width == 0U || box->height == 0U || box->thickness == 0U) {
            LOGE("box[%u] invalid geometry w=%u h=%u t=%u\r\n",
                 i, box->width, box->height, box->thickness);
            return AVDK_ERR_INVAL;
        }
        if ((box->x + box->width) > frame_w || (box->y + box->height) > frame_h) {
            LOGE("box[%u] out of frame %ux%u: x=%u y=%u w=%u h=%u\r\n",
                 i, frame_w, frame_h, box->x, box->y, box->width, box->height);
            return AVDK_ERR_INVAL;
        }

        x1 = box->x + box->width;
        y1 = box->y + box->height;
        if (box->x < min_x) {
            min_x = box->x;
        }
        if (box->y < min_y) {
            min_y = box->y;
        }
        if (x1 > max_x) {
            max_x = x1;
        }
        if (y1 > max_y) {
            max_y = y1;
        }

        if (i > 0U && box->color != boxes->boxes[0].color) {
            LOGW("box[%u] color 0x%06x differs from boxes[0]; BITMAP uses first color\r\n",
                 i, (unsigned)(box->color & 0xFFFFFFU));
        }
    }

    canvas_x = jpeg_encode_osd_align_down(min_x, JPEG_OSD_XY_ALIGN);
    canvas_y = jpeg_encode_osd_align_down(min_y, JPEG_OSD_XY_ALIGN);
    canvas_w = jpeg_encode_osd_align_up(max_x - canvas_x, JPEG_OSD_BITMAP_ALIGN);
    canvas_h = jpeg_encode_osd_align_up(max_y - canvas_y, JPEG_OSD_BITMAP_ALIGN);

    if (canvas_w > frame_w || canvas_h > frame_h) {
        LOGE("bitmap canvas %ux%u exceeds frame %ux%u\r\n",
             canvas_w, canvas_h, frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }
    if ((canvas_x + canvas_w) > frame_w) {
        canvas_x = jpeg_encode_osd_align_down(frame_w - canvas_w, JPEG_OSD_XY_ALIGN);
    }
    if ((canvas_y + canvas_h) > frame_h) {
        canvas_y = jpeg_encode_osd_align_down(frame_h - canvas_h, JPEG_OSD_XY_ALIGN);
    }
    if ((canvas_x + canvas_w) > frame_w || (canvas_y + canvas_h) > frame_h ||
        canvas_w == 0U || canvas_h == 0U) {
        LOGE("aligned bitmap canvas invalid: %ux%u at (%u,%u) frame %ux%u\r\n",
             canvas_w, canvas_h, canvas_x, canvas_y, frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }

    buf_size = (canvas_w / 8U) * canvas_h;
    bitmap = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buf_size);
    if (bitmap == NULL) {
        LOGE("alloc osd bitmap %u bytes failed\r\n", buf_size);
        return AVDK_ERR_NOMEM;
    }
    os_memset(bitmap, 0, buf_size);

    for (i = 0U; i < boxes->count; i++) {
        const bk_jpeg_encode_osd_bitmap_box_t *box = &boxes->boxes[i];

        jpeg_encode_osd_bitmap_draw_box(bitmap, canvas_w, canvas_h,
                                        box->x - canvas_x, box->y - canvas_y,
                                        box->width, box->height, box->thickness);
    }

    jpeg_encode_osd_rgb_to_yuv(boxes->boxes[0].color, &yuv_y, &yuv_u, &yuv_v);

    osd->index = boxes->index;
    osd->buffer = bitmap;
    osd->format = BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP;
    osd->alpha = (boxes->alpha == 0U) ? 255U : boxes->alpha;
    osd->x = canvas_x;
    osd->y = canvas_y;
    osd->width = canvas_w;
    osd->height = canvas_h;
    osd->bitmap_y = yuv_y;
    osd->bitmap_u = yuv_u;
    osd->bitmap_v = yuv_v;
    osd->buffer_free = jpeg_encode_osd_bitmap_boxes_buffer_free;
    osd->free_arg = NULL;
    return AVDK_ERR_OK;
}

avdk_err_t jpeg_encode_set_osd_bitmap_boxes_common(jpeg_enc_param_t *enc_param,
                                                   bool encoder_inited,
                                                   jpeg_encode_osd_slot_state_t *slots,
                                                   bk_jpeg_encode_osd_bitmap_boxes_t *boxes)
{
    bk_jpeg_encode_osd_t osd = {0};
    avdk_err_t ret;

    if (enc_param == NULL || slots == NULL || boxes == NULL) {
        LOGE("osd boxes arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("jpeg encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }

    ret = jpeg_encode_build_osd_bitmap_boxes(enc_param->width, enc_param->height, boxes, &osd);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    ret = jpeg_encode_set_osd_common(enc_param, encoder_inited, slots, &osd);
    if (ret != AVDK_ERR_OK) {
        jpeg_encode_osd_invoke_buffer_free(osd.buffer, osd.buffer_free, osd.free_arg);
    }
    return ret;
}

static uint32_t jpeg_encode_osd_argb_normalize(uint32_t color)
{
    /* A==0 treated as opaque so RGB888-only callers still draw visible borders. */
    if ((color & 0xFF000000U) == 0U) {
        return color | 0xFF000000U;
    }
    return color;
}

static void jpeg_encode_osd_argb_fill_rect(uint32_t *buffer, uint32_t canvas_w,
                                            uint32_t canvas_h, uint32_t x,
                                            uint32_t y, uint32_t w, uint32_t h,
                                            uint32_t color)
{
    uint32_t row;
    uint32_t col;

    if (buffer == NULL || w == 0U || h == 0U || x >= canvas_w || y >= canvas_h) {
        return;
    }

    if ((x + w) > canvas_w) {
        w = canvas_w - x;
    }
    if ((y + h) > canvas_h) {
        h = canvas_h - y;
    }

    for (row = 0U; row < h; row++) {
        uint32_t *line = &buffer[(y + row) * canvas_w + x];

        for (col = 0U; col < w; col++) {
            line[col] = color;
        }
    }
}

static void jpeg_encode_osd_argb_draw_box(uint32_t *buffer, uint32_t canvas_w,
                                          uint32_t canvas_h, uint32_t x,
                                          uint32_t y, uint32_t w, uint32_t h,
                                          uint32_t thickness, uint32_t color)
{
    uint32_t t = thickness;

    if (w == 0U || h == 0U || t == 0U) {
        return;
    }

    if (t > (w / 2U)) {
        t = (w > 1U) ? (w / 2U) : 1U;
    }
    if (t > (h / 2U)) {
        t = (h > 1U) ? (h / 2U) : 1U;
    }

    jpeg_encode_osd_argb_fill_rect(buffer, canvas_w, canvas_h, x, y, w, t, color);
    jpeg_encode_osd_argb_fill_rect(buffer, canvas_w, canvas_h, x, y + h - t, w, t, color);
    jpeg_encode_osd_argb_fill_rect(buffer, canvas_w, canvas_h, x, y, t, h, color);
    jpeg_encode_osd_argb_fill_rect(buffer, canvas_w, canvas_h, x + w - t, y, t, h, color);
}

static void jpeg_encode_osd_argb8888_boxes_buffer_free(void *buffer, void *free_arg)
{
    (void)free_arg;
    if (buffer != NULL) {
        bk_frame_buffer_free(buffer);
    }
}

avdk_err_t jpeg_encode_build_osd_argb8888_boxes(uint32_t frame_w,
                                                uint32_t frame_h,
                                                bk_jpeg_encode_osd_argb8888_boxes_t *boxes,
                                                bk_jpeg_encode_osd_t *osd)
{
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t canvas_x;
    uint32_t canvas_y;
    uint32_t canvas_w;
    uint32_t canvas_h;
    uint32_t buf_size;
    uint32_t *argb = NULL;
    uint32_t i;

    if (boxes == NULL || osd == NULL) {
        LOGE("osd argb boxes arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    os_memset(osd, 0, sizeof(*osd));

    if (boxes->index >= JPEG_ENCODE_OSD_SLOT_COUNT) {
        LOGE("invalid osd argb boxes index: %u\r\n", boxes->index);
        return AVDK_ERR_INVAL;
    }

    if (boxes->count == 0U || boxes->boxes == NULL) {
        osd->index = boxes->index;
        osd->buffer = NULL;
        osd->format = BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888;
        return AVDK_ERR_OK;
    }

    if (boxes->count > JPEG_OSD_BOX_MAX_COUNT) {
        LOGE("osd argb boxes count %u exceeds max %u\r\n",
             boxes->count, JPEG_OSD_BOX_MAX_COUNT);
        return AVDK_ERR_INVAL;
    }

    if (frame_w == 0U || frame_h == 0U) {
        LOGE("encoder frame size invalid: %ux%u\r\n", frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }

    min_x = frame_w;
    min_y = frame_h;
    max_x = 0U;
    max_y = 0U;

    for (i = 0U; i < boxes->count; i++) {
        const bk_jpeg_encode_osd_argb8888_box_t *box = &boxes->boxes[i];
        uint32_t x1;
        uint32_t y1;

        if (box->width == 0U || box->height == 0U || box->thickness == 0U) {
            LOGE("argb box[%u] invalid geometry w=%u h=%u t=%u\r\n",
                 i, box->width, box->height, box->thickness);
            return AVDK_ERR_INVAL;
        }
        if ((box->x + box->width) > frame_w || (box->y + box->height) > frame_h) {
            LOGE("argb box[%u] out of frame %ux%u: x=%u y=%u w=%u h=%u\r\n",
                 i, frame_w, frame_h, box->x, box->y, box->width, box->height);
            return AVDK_ERR_INVAL;
        }

        x1 = box->x + box->width;
        y1 = box->y + box->height;
        if (box->x < min_x) {
            min_x = box->x;
        }
        if (box->y < min_y) {
            min_y = box->y;
        }
        if (x1 > max_x) {
            max_x = x1;
        }
        if (y1 > max_y) {
            max_y = y1;
        }
    }

    /* ARGB8888 only requires 2-pixel XY alignment. */
    canvas_x = jpeg_encode_osd_align_down(min_x, JPEG_OSD_XY_ALIGN);
    canvas_y = jpeg_encode_osd_align_down(min_y, JPEG_OSD_XY_ALIGN);
    canvas_w = jpeg_encode_osd_align_up(max_x - canvas_x, JPEG_OSD_XY_ALIGN);
    canvas_h = jpeg_encode_osd_align_up(max_y - canvas_y, JPEG_OSD_XY_ALIGN);

    if (canvas_w > frame_w || canvas_h > frame_h) {
        LOGE("argb canvas %ux%u exceeds frame %ux%u\r\n",
             canvas_w, canvas_h, frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }
    if ((canvas_x + canvas_w) > frame_w) {
        canvas_x = jpeg_encode_osd_align_down(frame_w - canvas_w, JPEG_OSD_XY_ALIGN);
    }
    if ((canvas_y + canvas_h) > frame_h) {
        canvas_y = jpeg_encode_osd_align_down(frame_h - canvas_h, JPEG_OSD_XY_ALIGN);
    }
    if ((canvas_x + canvas_w) > frame_w || (canvas_y + canvas_h) > frame_h ||
        canvas_w == 0U || canvas_h == 0U) {
        LOGE("aligned argb canvas invalid: %ux%u at (%u,%u) frame %ux%u\r\n",
             canvas_w, canvas_h, canvas_x, canvas_y, frame_w, frame_h);
        return AVDK_ERR_INVAL;
    }

    buf_size = canvas_w * canvas_h * 4U;
    argb = (uint32_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buf_size);
    if (argb == NULL) {
        LOGE("alloc osd argb %u bytes failed\r\n", buf_size);
        return AVDK_ERR_NOMEM;
    }
    /* Transparent background; only borders carry alpha. */
    os_memset(argb, 0, buf_size);

    for (i = 0U; i < boxes->count; i++) {
        const bk_jpeg_encode_osd_argb8888_box_t *box = &boxes->boxes[i];

        jpeg_encode_osd_argb_draw_box(argb, canvas_w, canvas_h,
                                      box->x - canvas_x, box->y - canvas_y,
                                      box->width, box->height, box->thickness,
                                      jpeg_encode_osd_argb_normalize(box->color));
    }

    osd->index = boxes->index;
    osd->buffer = argb;
    osd->format = BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888;
    osd->alpha = 0U; /* ignored for ARGB8888; per-pixel A is used */
    osd->x = canvas_x;
    osd->y = canvas_y;
    osd->width = canvas_w;
    osd->height = canvas_h;
    osd->buffer_free = jpeg_encode_osd_argb8888_boxes_buffer_free;
    osd->free_arg = NULL;
    return AVDK_ERR_OK;
}

avdk_err_t jpeg_encode_set_osd_argb8888_boxes_common(jpeg_enc_param_t *enc_param,
                                                     bool encoder_inited,
                                                     jpeg_encode_osd_slot_state_t *slots,
                                                     bk_jpeg_encode_osd_argb8888_boxes_t *boxes)
{
    bk_jpeg_encode_osd_t osd = {0};
    avdk_err_t ret;

    if (enc_param == NULL || slots == NULL || boxes == NULL) {
        LOGE("osd argb boxes arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("jpeg encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }

    ret = jpeg_encode_build_osd_argb8888_boxes(enc_param->width, enc_param->height, boxes, &osd);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    ret = jpeg_encode_set_osd_common(enc_param, encoder_inited, slots, &osd);
    if (ret != AVDK_ERR_OK) {
        jpeg_encode_osd_invoke_buffer_free(osd.buffer, osd.buffer_free, osd.free_arg);
    }
    return ret;
}

static uint8_t jpeg_encode_osd_blend_u8(uint8_t dst, uint8_t src, uint8_t alpha)
{
    if (alpha == 0U) {
        return dst;
    }
    if (alpha == 255U) {
        return src;
    }
    return (uint8_t)(((uint32_t)src * alpha + (uint32_t)dst * (255U - alpha) + 127U) / 255U);
}

/*
 * Write one pixel into a packed 4:2:2 frame stored as Y-Cr-Y-Cb pairs
 * (byte order Y V Y U), which is the layout the software JPEG encoder
 * consumes (see bk_jpeg_test_copy_yuyv_line() in the jpeg_encode_example).
 * Luma always, chroma only for the even pixel of a pair so both pixels keep a
 * single consistent U/V value.
 */
static void jpeg_encode_osd_yuyv_write_pixel(uint8_t *frame, uint32_t frame_w,
                                             uint32_t x, uint32_t y,
                                             uint8_t luma, uint8_t cb, uint8_t cr,
                                             uint8_t alpha)
{
    uint32_t pair_x = x & ~1U;
    uint8_t *pixel = frame + ((y * frame_w) + x) * 2U;
    uint8_t *pair = frame + ((y * frame_w) + pair_x) * 2U;

    pixel[0] = jpeg_encode_osd_blend_u8(pixel[0], luma, alpha);
    if (((x & 1U) == 0U) && ((pair_x + 1U) < frame_w)) {
        pair[1] = jpeg_encode_osd_blend_u8(pair[1], cr, alpha);
        pair[3] = jpeg_encode_osd_blend_u8(pair[3], cb, alpha);
    }
}

/*
 * Software OSD compositing strip: JPEG_OSD_STRIP_LINES YUYV lines staged in
 * HSRAM. PSRAM slows down as soon as several blocks hit it at the same time
 * (ISP MP output, the JPEG encoder walking the input frame, the display ring),
 * so the overlay is composited in HSRAM and only the finished lines are copied
 * back with whole-line moves instead of pixel-granular PSRAM accesses. Neither
 * the RGB conversion nor the staging buffer ever needs a frame sized
 * allocation.
 *
 * When HSRAM cannot provide the strip the compositor converts and writes every
 * pixel straight back into the frame instead; that fallback is slower exactly
 * because it goes back to pixel-granular PSRAM access.
 */
#define JPEG_OSD_STRIP_LINES   (16U)

static uint8_t *jpeg_encode_osd_strip_alloc(uint32_t bytes)
{
    if (bytes == 0U) {
        return NULL;
    }

    return (uint8_t *)hsram_malloc(bytes);
}

static void jpeg_encode_osd_strip_free(uint8_t *strip)
{
    if (strip != NULL) {
        hsram_free(strip);
    }
}

static uint32_t jpeg_encode_osd_bitmap_bit(const uint8_t *line, uint32_t col)
{
    return ((uint32_t)line[col >> 3] >> (7U - (col & 7U))) & 1U;
}

/*
 * 1 bpp overlay: a set bit paints (bitmap_y, bitmap_u, bitmap_v) blended by
 * slot->alpha, a clear bit leaves the frame untouched. Chroma is written by the
 * even pixel of a pair only, which keeps the pair consistent and preserves the
 * historical behaviour of the format.
 *
 * Uses the same HSRAM strip as the ARGB8888 path: two neighbouring pixels share
 * one chroma pair, so the window is kept on a pixel-pair boundary and only whole
 * lines are copied back into the frame. A pair whose two bits are set with a
 * fully opaque colour is written as one 4-byte group with no blending at all.
 */
static void jpeg_encode_osd_blend_bitmap_slot(uint8_t *frame, uint32_t frame_w,
                                              uint32_t frame_h,
                                              const jpeg_encode_osd_slot_state_t *slot)
{
    const uint32_t row_bytes = slot->width / 8U;
    uint8_t alpha;
    uint32_t x_start;
    uint32_t cols;
    uint32_t line_bytes;
    uint32_t osd_first;
    uint32_t osd_last;
    uint32_t total_rows;
    uint32_t max_rows;
    uint32_t row = 0U;
    uint8_t *strip;

    if ((slot->x >= frame_w) || (slot->y >= frame_h) ||
        (slot->width == 0U) || (slot->height == 0U)) {
        return;
    }

    alpha = slot->alpha;
    if (alpha == 0U) {
        return;
    }

    x_start = slot->x & ~1U;
    cols = slot->x + slot->width;
    if (cols > frame_w) {
        cols = frame_w;
    }
    cols = (cols + 1U) & ~1U;
    if (cols > frame_w) {
        cols = frame_w;
    }
    cols -= x_start;
    cols &= ~1U;
    if (cols == 0U) {
        return;
    }
    line_bytes = cols * 2U;

    osd_first = slot->x - x_start;
    osd_last = slot->x + slot->width - x_start;
    if (osd_last > cols) {
        osd_last = cols;
    }

    total_rows = frame_h - slot->y;
    if (total_rows > slot->height) {
        total_rows = slot->height;
    }
    max_rows = total_rows;
    if (max_rows > JPEG_OSD_STRIP_LINES) {
        max_rows = JPEG_OSD_STRIP_LINES;
    }

    /* No HSRAM left: composite each line directly in the frame buffer. */
    strip = jpeg_encode_osd_strip_alloc(line_bytes * max_rows);

    while (row < total_rows) {
        uint32_t rows = total_rows - row;

        if (rows > max_rows) {
            rows = max_rows;
        }

        for (uint32_t r = 0U; r < rows; r++) {
            uint32_t frame_row_y = slot->y + row + r;
            uint8_t *frame_row = frame + (((frame_row_y * frame_w) + x_start) * 2U);
            const uint8_t *bits = (const uint8_t *)slot->buffer + ((row + r) * row_bytes);
            uint8_t *dst_row = frame_row;

            if (strip != NULL) {
                dst_row = strip + (r * line_bytes);
                os_memcpy(dst_row, frame_row, line_bytes);
            }

            /* Packed pair: Y0, Cr, Y1, Cb (see the software jpeg input layout). */
            for (uint32_t pair = 0U; pair < (cols / 2U); pair++) {
                uint8_t *pair_buf = dst_row + (pair * 4U);
                uint32_t w0 = pair * 2U;
                uint32_t w1 = w0 + 1U;
                uint32_t b0 = ((w0 >= osd_first) && (w0 < osd_last)) ?
                              jpeg_encode_osd_bitmap_bit(bits, w0 - osd_first) : 0U;
                uint32_t b1 = ((w1 >= osd_first) && (w1 < osd_last)) ?
                              jpeg_encode_osd_bitmap_bit(bits, w1 - osd_first) : 0U;

                if ((b0 | b1) == 0U) {
                    continue;
                }

                if ((b0 & b1) != 0U && alpha == 0xFFU) {
                    pair_buf[0] = slot->bitmap_y;
                    pair_buf[1] = slot->bitmap_v;
                    pair_buf[2] = slot->bitmap_y;
                    pair_buf[3] = slot->bitmap_u;
                    continue;
                }

                if (b0 != 0U) {
                    pair_buf[0] = jpeg_encode_osd_blend_u8(pair_buf[0], slot->bitmap_y, alpha);
                    pair_buf[1] = jpeg_encode_osd_blend_u8(pair_buf[1], slot->bitmap_v, alpha);
                    pair_buf[3] = jpeg_encode_osd_blend_u8(pair_buf[3], slot->bitmap_u, alpha);
                }
                if (b1 != 0U) {
                    pair_buf[2] = jpeg_encode_osd_blend_u8(pair_buf[2], slot->bitmap_y, alpha);
                }
            }

            if (strip != NULL) {
                os_memcpy(frame_row, dst_row, line_bytes);
            }
        }

        row += rows;
    }

    jpeg_encode_osd_strip_free(strip);
}

static void jpeg_encode_osd_blend_argb8888_slot(uint8_t *frame, uint32_t frame_w,
                                                uint32_t frame_h,
                                                const jpeg_encode_osd_slot_state_t *slot)
{
    const uint32_t *argb = (const uint32_t *)slot->buffer;
    uint32_t x_start;
    uint32_t cols;
    uint32_t line_bytes;
    uint32_t osd_first;
    uint32_t osd_last;
    uint32_t total_rows;
    uint32_t max_rows;
    uint32_t row = 0U;
    uint8_t *strip;

    if ((slot->x >= frame_w) || (slot->y >= frame_h) || (slot->width == 0U)) {
        return;
    }

    /*
     * Two neighbouring pixels share one chroma pair, so keep the staged window
     * on a pixel-pair boundary: a pair is never split between the frame and the
     * strip and the write-back cannot clobber a neighbouring pixel.
     */
    x_start = slot->x & ~1U;
    cols = slot->x + slot->width;
    if (cols > frame_w) {
        cols = frame_w;
    }
    cols = (cols + 1U) & ~1U;
    if (cols > frame_w) {
        cols = frame_w;
    }
    cols -= x_start;
    cols &= ~1U;
    if (cols == 0U) {
        return;
    }
    line_bytes = cols * 2U;

    /* Window columns covered by the overlay, in [osd_first, osd_last). */
    osd_first = slot->x - x_start;
    osd_last = slot->x + slot->width - x_start;
    if (osd_last > cols) {
        osd_last = cols;
    }

    max_rows = frame_h - slot->y;
    if (max_rows > slot->height) {
        max_rows = slot->height;
    }
    total_rows = max_rows;
    if (max_rows > JPEG_OSD_STRIP_LINES) {
        max_rows = JPEG_OSD_STRIP_LINES;
    }

    /* No HSRAM left: composite each line directly in the frame buffer. */
    strip = jpeg_encode_osd_strip_alloc(line_bytes * max_rows);

    while (row < total_rows) {
        uint32_t rows = total_rows - row;

        if (rows > max_rows) {
            rows = max_rows;
        }

        for (uint32_t r = 0U; r < rows; r++) {
            uint32_t frame_row_y = slot->y + row + r;
            uint8_t *frame_row = frame + (((frame_row_y * frame_w) + x_start) * 2U);
            const uint32_t *src = argb + ((row + r) * slot->width);
            uint8_t *dst_row = frame_row;
            uint32_t cached_color = 0U;
            uint32_t cache_valid = 0U;
            uint8_t cached_y = 0U;
            uint8_t cached_cb = 0U;
            uint8_t cached_cr = 0U;

            if (strip != NULL) {
                dst_row = strip + (r * line_bytes);
                os_memcpy(dst_row, frame_row, line_bytes);
            }

            /* Packed pair: Y0, Cr, Y1, Cb (see the software jpeg input layout). */
            for (uint32_t pair = 0U; pair < (cols / 2U); pair++) {
                uint8_t *pair_buf = dst_row + (pair * 4U);
                uint32_t w0 = pair * 2U;
                uint32_t w1 = w0 + 1U;
                uint32_t a0 = ((w0 >= osd_first) && (w0 < osd_last)) ?
                              src[w0 - osd_first] : 0U;
                uint32_t a1 = ((w1 >= osd_first) && (w1 < osd_last)) ?
                              src[w1 - osd_first] : 0U;
                uint8_t y0;
                uint8_t y1;
                uint8_t cb;
                uint8_t cr;

                /* Nothing to blend: keep the original YUYV bytes untouched. */
                if (((a0 | a1) & 0xFF000000U) == 0U) {
                    continue;
                }

                y0 = pair_buf[0];
                cr = pair_buf[1];   /* byte 1 carries Cr */
                y1 = pair_buf[2];
                cb = pair_buf[3];   /* byte 3 carries Cb */

                /* Fully opaque pair of one colour (a solid overlay block is the
                 * common case): the result is the OSD colour itself. */
                if ((a0 == a1) && ((a0 >> 24) == 0xFFU)) {
                    if ((cache_valid == 0U) || (a0 != cached_color)) {
                        jpeg_encode_osd_rgb_to_yuv(a0, &cached_y, &cached_cb, &cached_cr);
                        cached_color = a0;
                        cache_valid = 1U;
                    }
                    pair_buf[0] = cached_y;
                    pair_buf[1] = cached_cr;
                    pair_buf[2] = cached_y;
                    pair_buf[3] = cached_cb;
                    continue;
                }

                {
                    /* Blend in YUV space: the overlay alpha is applied to luma
                     * and chroma directly, so no pixel has to go through the
                     * lossy YUV -> RGB -> YUV round trip. Only the OSD colour
                     * needs the RGB -> YUV conversion and that one is cached. */
                    uint8_t cb0 = cb;
                    uint8_t cr0 = cr;
                    uint8_t cb1 = cb;
                    uint8_t cr1 = cr;
                    uint8_t alpha;

                    alpha = (uint8_t)(a0 >> 24);
                    if (alpha != 0U) {
                        if ((cache_valid == 0U) || (a0 != cached_color)) {
                            jpeg_encode_osd_rgb_to_yuv(a0, &cached_y, &cached_cb, &cached_cr);
                            cached_color = a0;
                            cache_valid = 1U;
                        }
                        y0 = jpeg_encode_osd_blend_u8(y0, cached_y, alpha);
                        cr0 = jpeg_encode_osd_blend_u8(cr, cached_cr, alpha);
                        cb0 = jpeg_encode_osd_blend_u8(cb, cached_cb, alpha);
                    }

                    alpha = (uint8_t)(a1 >> 24);
                    if (alpha != 0U) {
                        if ((cache_valid == 0U) || (a1 != cached_color)) {
                            jpeg_encode_osd_rgb_to_yuv(a1, &cached_y, &cached_cb, &cached_cr);
                            cached_color = a1;
                            cache_valid = 1U;
                        }
                        y1 = jpeg_encode_osd_blend_u8(y1, cached_y, alpha);
                        cr1 = jpeg_encode_osd_blend_u8(cr, cached_cr, alpha);
                        cb1 = jpeg_encode_osd_blend_u8(cb, cached_cb, alpha);
                    }

                    /* 4:2:2 shares one chroma sample per pixel pair. */
                    pair_buf[0] = y0;
                    pair_buf[1] = (uint8_t)(((uint32_t)cr0 + (uint32_t)cr1 + 1U) >> 1);
                    pair_buf[2] = y1;
                    pair_buf[3] = (uint8_t)(((uint32_t)cb0 + (uint32_t)cb1 + 1U) >> 1);
                }
            }
        }

        if (strip != NULL) {
            for (uint32_t r = 0U; r < rows; r++) {
                os_memcpy(frame + ((((slot->y + row + r) * frame_w) + x_start) * 2U),
                          strip + (r * line_bytes), line_bytes);
            }
        }

        row += rows;
    }

    jpeg_encode_osd_strip_free(strip);
}

static void jpeg_encode_osd_blend_nv12_slot(uint8_t *frame, uint32_t frame_w,
                                            uint32_t frame_h,
                                            const jpeg_encode_osd_slot_state_t *slot)
{
    const uint8_t *y_plane = (const uint8_t *)slot->buffer;
    const uint8_t *uv_plane = y_plane + (slot->width * slot->height);
    uint32_t row;
    uint32_t col;

    for (row = 0U; row < slot->height; row++) {
        const uint8_t *uv_line;
        uint32_t uv_row;

        if ((slot->y + row) >= frame_h) {
            break;
        }
        uv_row = row / 2U;
        uv_line = uv_plane + (uv_row * slot->width);
        for (col = 0U; col < slot->width; col++) {
            const uint8_t *uv;

            if ((slot->x + col) >= frame_w) {
                break;
            }
            uv = uv_line + ((col / 2U) * 2U);
            jpeg_encode_osd_yuyv_write_pixel(frame, frame_w, slot->x + col, slot->y + row,
                                             y_plane[(row * slot->width) + col],
                                             uv[0], uv[1], slot->alpha);
        }
    }
}

void jpeg_encode_osd_draw_yuyv(uint8_t *frame,
                               uint32_t frame_w,
                               uint32_t frame_h,
                               jpeg_encode_osd_slot_state_t *slots)
{
    uint32_t i;

    if (frame == NULL || slots == NULL || frame_w == 0U || frame_h == 0U) {
        return;
    }

    for (i = 0U; i < JPEG_ENCODE_OSD_SLOT_COUNT; i++) {
        const jpeg_encode_osd_slot_state_t *slot = &slots[i];

        if (!slot->enabled || slot->buffer == NULL) {
            continue;
        }
        if (slot->width == 0U || slot->height == 0U) {
            continue;
        }

        switch (slot->format) {
        case BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP:
            jpeg_encode_osd_blend_bitmap_slot(frame, frame_w, frame_h, slot);
            break;
        case BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888:
            jpeg_encode_osd_blend_argb8888_slot(frame, frame_w, frame_h, slot);
            break;
        case BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12:
            jpeg_encode_osd_blend_nv12_slot(frame, frame_w, frame_h, slot);
            break;
        default:
            break;
        }
    }
}