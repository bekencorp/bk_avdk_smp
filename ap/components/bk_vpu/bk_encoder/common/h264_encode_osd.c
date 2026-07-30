#include <components/log.h>
#include <components/avdk_utils/avdk_error.h>
#include <cache.h>
#include <os/os.h>
#include <stdint.h>

#include "h264_encode_osd_priv.h"
#include "modules/vcenc/vcenc_h264_api.h"

#define TAG "h264_encode_osd"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264_OSD_CTB_W           64U
#define H264_OSD_CTB_H           16U
#define H264_OSD_XY_ALIGN        2U
#define H264_OSD_BITMAP_ALIGN    8U
#define H264_OSD_ADDR_ALIGN      4U

static beken_mutex_t s_osd_slots_mutex;
static bool s_osd_mutex_ready;

void h264_encode_osd_module_init(void)
{
    if (s_osd_mutex_ready) {
        return;
    }

    if (rtos_init_mutex(&s_osd_slots_mutex) == BK_OK) {
        s_osd_mutex_ready = true;
    }
}

static void h264_encode_osd_slots_lock(void)
{
    if (!s_osd_mutex_ready) {
        h264_encode_osd_module_init();
    }
    if (s_osd_mutex_ready) {
        rtos_lock_mutex(&s_osd_slots_mutex);
    }
}

static void h264_encode_osd_slots_unlock(void)
{
    if (s_osd_mutex_ready) {
        rtos_unlock_mutex(&s_osd_slots_mutex);
    }
}

static void h264_encode_osd_invoke_buffer_free(void *buffer,
                                               bk_h264_encode_osd_buffer_free_cb_t buffer_free,
                                               void *free_arg)
{
    if (buffer != NULL && buffer_free != NULL) {
        buffer_free(buffer, free_arg);
    }
}

static bool h264_encode_osd_region_hits_ctb(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                            uint32_t ctb_col, uint32_t ctb_row)
{
    uint32_t ctb_x0 = ctb_col * H264_OSD_CTB_W;
    uint32_t ctb_y0 = ctb_row * H264_OSD_CTB_H;
    uint32_t ctb_x1 = ctb_x0 + H264_OSD_CTB_W;
    uint32_t ctb_y1 = ctb_y0 + H264_OSD_CTB_H;

    return (x < ctb_x1 && (x + w) > ctb_x0 && y < ctb_y1 && (y + h) > ctb_y0);
}

static bool h264_encode_osd_regions_share_ctb(uint32_t x0, uint32_t y0, uint32_t w0, uint32_t h0,
                                              uint32_t x1, uint32_t y1, uint32_t w1, uint32_t h1)
{
    uint32_t c0_min = x0 / H264_OSD_CTB_W;
    uint32_t c0_max = (x0 + w0 - 1U) / H264_OSD_CTB_W;
    uint32_t r0_min = y0 / H264_OSD_CTB_H;
    uint32_t r0_max = (y0 + h0 - 1U) / H264_OSD_CTB_H;

    for (uint32_t c = c0_min; c <= c0_max; c++) {
        for (uint32_t r = r0_min; r <= r0_max; r++) {
            if (h264_encode_osd_region_hits_ctb(x1, y1, w1, h1, c, r)) {
                return true;
            }
        }
    }

    return false;
}

static bool h264_encode_osd_slot_enabled(const h264_encode_osd_slot_state_t *slot)
{
    return (slot != NULL && (slot->enabled || slot->active_enabled) &&
            (slot->buffer != NULL || slot->active_buffer != NULL));
}

static uint32_t h264_encode_osd_slot_x(const h264_encode_osd_slot_state_t *slot)
{
    return h264_encode_osd_slot_enabled(slot) ? slot->x : 0U;
}

static uint32_t h264_encode_osd_slot_y(const h264_encode_osd_slot_state_t *slot)
{
    return h264_encode_osd_slot_enabled(slot) ? slot->y : 0U;
}

static uint32_t h264_encode_osd_slot_w(const h264_encode_osd_slot_state_t *slot)
{
    return h264_encode_osd_slot_enabled(slot) ? slot->width : 0U;
}

static uint32_t h264_encode_osd_slot_h(const h264_encode_osd_slot_state_t *slot)
{
    return h264_encode_osd_slot_enabled(slot) ? slot->height : 0U;
}

static avdk_err_t h264_encode_osd_validate_config(h264_enc_param_t *enc_param,
                                                  h264_encode_osd_slot_state_t *slots,
                                                  const bk_h264_encode_osd_t *osd)
{
    uint32_t frame_w;
    uint32_t frame_h;
    uintptr_t buf_addr;

    if (enc_param == NULL || osd == NULL) {
        LOGE("invalid arg: enc_param or osd is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    if (osd->buffer == NULL) {
        return AVDK_ERR_OK;
    }

    frame_w = enc_param->width;
    frame_h = enc_param->height;
    buf_addr = (uintptr_t)osd->buffer;

    if (buf_addr == 0U) {
        LOGE("slot %u invalid buffer address 0\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    if ((buf_addr & (H264_OSD_ADDR_ALIGN - 1U)) != 0U) {
        LOGE("slot %u buffer %p not %u-byte aligned\r\n",
             osd->index, osd->buffer, H264_OSD_ADDR_ALIGN);
        return AVDK_ERR_INVAL;
    }

    if (osd->buffer_free == NULL) {
        LOGE("slot %u buffer_free is NULL\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    if (osd->format != BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888 &&
        osd->format != BK_H264_ENCODE_OVERLAY_FORMAT_NV12 &&
        osd->format != BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP) {
        LOGE("slot %u unsupported format %u\r\n", osd->index, osd->format);
        return AVDK_ERR_INVAL;
    }

    if (osd->format == BK_H264_ENCODE_OVERLAY_FORMAT_NV12 &&
        (((osd->width | osd->height) & 1U) != 0U)) {
        LOGE("slot %u NV12 size must be even: w=%u h=%u\r\n",
             osd->index, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if (osd->format == BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP &&
        (((osd->width | osd->height) & (H264_OSD_BITMAP_ALIGN - 1U)) != 0U)) {
        LOGE("slot %u bitmap size must be %u-pixel aligned: w=%u h=%u\r\n",
             osd->index, H264_OSD_BITMAP_ALIGN, osd->width, osd->height);
        return AVDK_ERR_INVAL;
    }

    if (osd->format == BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888) {
        /* alpha is ignored for ARGB8888 */
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

    if ((osd->x & (H264_OSD_XY_ALIGN - 1U)) != 0U ||
        (osd->y & (H264_OSD_XY_ALIGN - 1U)) != 0U) {
        LOGE("slot %u offset not %u-pixel aligned: x=%u y=%u\r\n",
             osd->index, H264_OSD_XY_ALIGN, osd->x, osd->y);
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

    if (slots != NULL) {
        for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
            const h264_encode_osd_slot_state_t *other = &slots[i];
            uint32_t ox;
            uint32_t oy;
            uint32_t ow;
            uint32_t oh;

            if (i == osd->index || !h264_encode_osd_slot_enabled(other)) {
                continue;
            }

            ox = h264_encode_osd_slot_x(other);
            oy = h264_encode_osd_slot_y(other);
            ow = h264_encode_osd_slot_w(other);
            oh = h264_encode_osd_slot_h(other);

            if (ow == 0U || oh == 0U) {
                continue;
            }

            if (h264_encode_osd_regions_share_ctb(osd->x, osd->y, osd->width, osd->height,
                                                    ox, oy, ow, oh)) {
                LOGE("slot %u CTB overlap with slot %u: "
                     "new(%u,%u,%u,%u) vs exist(%u,%u,%u,%u)\r\n",
                     osd->index, i, osd->x, osd->y, osd->width, osd->height,
                     ox, oy, ow, oh);
                return AVDK_ERR_INVAL;
            }
        }
    }

    return AVDK_ERR_OK;
}

static void h264_encode_osd_apply_slot_config(h264_encode_osd_slot_state_t *slot,
                                              const bk_h264_encode_osd_t *osd)
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

static void h264_encode_osd_slot_clear_pending(h264_encode_osd_slot_state_t *slot)
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

static void h264_encode_osd_slot_clear_active(h264_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    slot->active_buffer = NULL;
    slot->active_buffer_free = NULL;
    slot->active_free_arg = NULL;
    slot->active_enabled = false;
}

static void h264_encode_osd_slot_clear_retire(h264_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    slot->retire_buffer = NULL;
    slot->retire_buffer_free = NULL;
    slot->retire_free_arg = NULL;
}

static void h264_encode_osd_slot_release_retire(h264_encode_osd_slot_state_t *slot)
{
    if (slot == NULL) {
        return;
    }

    h264_encode_osd_invoke_buffer_free(slot->retire_buffer,
                                       slot->retire_buffer_free,
                                       slot->retire_free_arg);
    h264_encode_osd_slot_clear_retire(slot);
}

static void h264_encode_osd_slot_release_pending(h264_encode_osd_slot_state_t *slot)
{
    void *buffer;
    bk_h264_encode_osd_buffer_free_cb_t buffer_free;
    void *free_arg;

    if (slot == NULL || slot->buffer == NULL) {
        h264_encode_osd_slot_clear_pending(slot);
        return;
    }

    buffer = slot->buffer;
    buffer_free = slot->buffer_free;
    free_arg = slot->free_arg;
    h264_encode_osd_slot_clear_pending(slot);
    h264_encode_osd_invoke_buffer_free(buffer, buffer_free, free_arg);
}

static void h264_encode_osd_slot_drop_uncommitted_pending(h264_encode_osd_slot_state_t *slot)
{
    if (slot == NULL || slot->buffer == NULL) {
        return;
    }

    if (slot->buffer == slot->active_buffer) {
        return;
    }

    h264_encode_osd_slot_release_pending(slot);
}

static void h264_encode_osd_slot_retire_buffer(h264_encode_osd_slot_state_t *slot,
                                               void *buffer,
                                               bk_h264_encode_osd_buffer_free_cb_t buffer_free,
                                               void *free_arg)
{
    if (slot == NULL || buffer == NULL) {
        return;
    }

    h264_encode_osd_slot_release_retire(slot);
    slot->retire_buffer = buffer;
    slot->retire_buffer_free = buffer_free;
    slot->retire_free_arg = free_arg;
}

static void h264_encode_osd_flush_slot(const h264_encode_osd_slot_state_t *slot)
{
    if (slot == NULL || !slot->enabled || slot->buffer == NULL) {
        return;
    }

    if (slot->format == BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888 &&
        slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)(slot->width * slot->height * 4U));
    } else if (slot->format == BK_H264_ENCODE_OVERLAY_FORMAT_NV12 &&
               slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)(slot->width * slot->height * 3U / 2U));
    } else if (slot->format == BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP &&
               slot->width > 0U && slot->height > 0U) {
        flush_dcache(slot->buffer, (long)((slot->width / 8U) * slot->height));
    }
}

static void h264_encode_osd_push_slot_to_vcenc(h264_enc_param_t *enc_param,
                                               const h264_encode_osd_slot_state_t *slot,
                                               uint32_t index)
{
    vcenc_ret_e ret;

    if (enc_param == NULL || slot == NULL || index >= H264_ENCODE_OSD_SLOT_COUNT) {
        return;
    }

    if (slot->enabled && slot->buffer != NULL) {
        ret = vcenc_h264_set_osd(enc_param, index, slot->buffer, slot->format, slot->alpha,
                                 slot->x, slot->y, slot->width, slot->height,
                                 slot->bitmap_y, slot->bitmap_u, slot->bitmap_v);
    } else {
        ret = vcenc_h264_set_osd(enc_param, index, NULL, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
                                  0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U);
    }

    if (ret != VCENC_OK) {
        LOGE("vcenc_h264_set_osd failed, index=%u ret=%d, buf=%p x=%u y=%u w=%u h=%u\r\n",
             index, ret, slot->buffer, slot->x, slot->y, slot->width, slot->height);
    }
}

void h264_encode_osd_sync_to_vcenc(h264_enc_param_t *enc_param,
                                   h264_encode_osd_slot_state_t *slots)
{
    if (enc_param == NULL || slots == NULL) {
        return;
    }

    h264_encode_osd_slots_lock();

    for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
        h264_encode_osd_slot_state_t *slot = &slots[i];

        h264_encode_osd_flush_slot(slot);
        h264_encode_osd_push_slot_to_vcenc(enc_param, slot, i);

        if (slot->enabled && slot->buffer != NULL) {
            if (slot->active_buffer != NULL && slot->active_buffer != slot->buffer) {
                h264_encode_osd_slot_retire_buffer(slot,
                                                   slot->active_buffer,
                                                   slot->active_buffer_free,
                                                   slot->active_free_arg);
            }
            slot->active_enabled = true;
            slot->active_buffer = slot->buffer;
            slot->active_buffer_free = slot->buffer_free;
            slot->active_free_arg = slot->free_arg;
        } else if (slot->active_buffer != NULL) {
            h264_encode_osd_slot_retire_buffer(slot,
                                               slot->active_buffer,
                                               slot->active_buffer_free,
                                               slot->active_free_arg);
            h264_encode_osd_slot_clear_active(slot);
        } else {
            slot->active_enabled = false;
        }
    }

    h264_encode_osd_slots_unlock();
}

void h264_encode_osd_finish_frame(h264_encode_osd_slot_state_t *slots)
{
    if (slots == NULL) {
        return;
    }

    h264_encode_osd_slots_lock();
    for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
        h264_encode_osd_slot_release_retire(&slots[i]);
    }
    h264_encode_osd_slots_unlock();
}

void h264_encode_osd_release_all(h264_enc_param_t *enc_param,
                                 h264_encode_osd_slot_state_t *slots)
{
    if (slots == NULL) {
        return;
    }

    h264_encode_osd_slots_lock();

    for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
        h264_encode_osd_slot_state_t *slot = &slots[i];

        h264_encode_osd_slot_drop_uncommitted_pending(slot);
        h264_encode_osd_slot_clear_pending(slot);
    }

    if (enc_param != NULL) {
        for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
            h264_encode_osd_slot_state_t *slot = &slots[i];

            h264_encode_osd_flush_slot(slot);
            h264_encode_osd_push_slot_to_vcenc(enc_param, slot, i);
            h264_encode_osd_slot_clear_active(slot);
        }
    }

    for (uint32_t i = 0U; i < H264_ENCODE_OSD_SLOT_COUNT; i++) {
        h264_encode_osd_slot_state_t *slot = &slots[i];

        h264_encode_osd_invoke_buffer_free(slot->active_buffer,
                                           slot->active_buffer_free,
                                           slot->active_free_arg);
        h264_encode_osd_slot_clear_active(slot);
        h264_encode_osd_slot_release_retire(slot);
    }

    h264_encode_osd_slots_unlock();
}

avdk_err_t h264_encode_set_osd_common(h264_enc_param_t *enc_param,
                                      bool encoder_inited,
                                      h264_encode_osd_slot_state_t *slots,
                                      bk_h264_encode_osd_t *osd)
{
    h264_encode_osd_slot_state_t *slot;
    avdk_err_t ret;

    if (osd == NULL || enc_param == NULL || slots == NULL) {
        LOGE("osd arg is NULL\r\n");
        return AVDK_ERR_INVAL;
    }
    if (!encoder_inited) {
        LOGE("h264 encoder is not opened\r\n");
        return AVDK_ERR_INVAL;
    }
    if (osd->index >= H264_ENCODE_OSD_SLOT_COUNT) {
        LOGE("invalid osd index: %u\r\n", osd->index);
        return AVDK_ERR_INVAL;
    }

    h264_encode_osd_slots_lock();
    slot = &slots[osd->index];

    if (osd->buffer == NULL) {
        h264_encode_osd_slot_drop_uncommitted_pending(slot);
        h264_encode_osd_slot_clear_pending(slot);
        h264_encode_osd_slots_unlock();
        return AVDK_ERR_OK;
    }

    ret = h264_encode_osd_validate_config(enc_param, slots, osd);
    if (ret != AVDK_ERR_OK) {
        h264_encode_osd_slots_unlock();
        return ret;
    }

    if (osd->buffer == slot->buffer) {
        h264_encode_osd_apply_slot_config(slot, osd);
        h264_encode_osd_flush_slot(slot);
        h264_encode_osd_slots_unlock();
        return AVDK_ERR_OK;
    }

    h264_encode_osd_slot_drop_uncommitted_pending(slot);
    h264_encode_osd_apply_slot_config(slot, osd);
    h264_encode_osd_flush_slot(slot);

    h264_encode_osd_slots_unlock();
    return AVDK_ERR_OK;
}
