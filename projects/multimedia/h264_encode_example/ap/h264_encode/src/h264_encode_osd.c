#include <stdint.h>
#include <stddef.h>

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>
#include "cache.h"
#include "h264_encode_osd.h"
#include "h264_osd_draw.h"
#include "../common/h264_encode_stream_256x128.h"

#define TAG "h264_encode_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264_ENC_TEST_WIDTH          256U
#define H264_ENC_TEST_HEIGHT         128U
#define H264_ENC_TEST_GOP            15U
#define H264_ENC_TEST_TIMEOUT_MS     3000U
#define H264_OSD_FORMAT_TEST_COUNT   3U
#define H264_OSD_CHANNEL_TEST_COUNT  H264_OSD_SLOT_COUNT
#define H264_OSD_TEST_FRAME_COUNT    1U

#define H264_ENC_OSD_LEFT_X          4U
#define H264_ENC_OSD_ROW0_Y          0U
#define H264_ENC_OSD_ROW1_Y          16U
#define H264_ENC_OSD_TEST_ROW0_Y     H264_ENC_OSD_ROW0_Y
#define H264_ENC_OSD_TEST_ROW1_Y     H264_ENC_OSD_ROW1_Y
#define H264_ENC_OSD_STRIP_W         32U
#define H264_ENC_OSD_STRIP_H         16U
#define H264_ENC_OSD_TEST_STRIP_W    H264_ENC_OSD_STRIP_W
#define H264_ENC_OSD_TEST_STRIP_H    H264_ENC_OSD_STRIP_H
#define H264_ENC_OSD_COL_STEP        64U
#define H264_ENC_OSD_TIME_W          48U
#define H264_ENC_OSD_TIME_H          16U

#define H264_ENC_OSD_ARGB_YELLOW     (0xFFFFFF00U)
#define H264_ENC_OSD_ARGB_CYAN       (0xFF00FFFFU)
#define H264_ENC_OSD_ARGB_GREEN      (0xFF00FF00U)
#define H264_ENC_OSD_ARGB_ORANGE     (0xFFFFA500U)
#define H264_ENC_OSD_ARGB_MAGENTA    (0xFFFF00FFU)
#define H264_ENC_OSD_ARGB_WHITE      (0xFFFFFFFFU)

/*
 * H.264 CTB = 64x16: one overlay per CTB cell (4 columns x 2 rows).
 * Row0: slot0..3, Row1: slot4..7.
 */
static const h264_encode_osd_layout_t s_osd_layouts[H264_OSD_SLOT_COUNT] = {
    { H264_ENC_OSD_LEFT_X + 0U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW0_Y,
      H264_ENC_OSD_TIME_W, H264_ENC_OSD_TIME_H, H264_OSD_TIME_FONT_SCALE, H264_ENC_OSD_ARGB_YELLOW },
    { H264_ENC_OSD_LEFT_X + 1U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW0_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_CYAN },
    { H264_ENC_OSD_LEFT_X + 2U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW0_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_GREEN },
    { H264_ENC_OSD_LEFT_X + 3U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW0_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_ORANGE },
    { H264_ENC_OSD_LEFT_X + 0U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW1_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_MAGENTA },
    { H264_ENC_OSD_LEFT_X + 1U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW1_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_WHITE },
    { H264_ENC_OSD_LEFT_X + 2U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW1_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_CYAN },
    { H264_ENC_OSD_LEFT_X + 3U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_ROW1_Y,
      H264_ENC_OSD_STRIP_W, H264_ENC_OSD_STRIP_H, H264_OSD_LABEL_FONT_SCALE, H264_ENC_OSD_ARGB_YELLOW },
};

typedef struct {
    beken_semaphore_t done_sem;
    uint32_t result;
    uint32_t frame_size;
    uint32_t frame_type;
} h264_encode_osd_encode_ctx_t;

static void h264_encode_osd_buffer_free(void *buffer, void *free_arg)
{
    (void)free_arg;

    if (buffer != NULL) {
        bk_frame_buffer_free(buffer);
    }
}

const h264_encode_osd_layout_t *h264_encode_osd_get_layout(uint32_t slot_index)
{
    if (slot_index >= H264_OSD_SLOT_COUNT) {
        return NULL;
    }

    return &s_osd_layouts[slot_index];
}

static void h264_encode_osd_format_u32_2(char *text, size_t text_len, uint32_t value)
{
    if (text == NULL || text_len < 3U) {
        return;
    }

    value %= 100U;
    text[0] = (char)('0' + (value / 10U));
    text[1] = (char)('0' + (value % 10U));
    text[2] = '\0';
}

static void h264_encode_osd_update_time_string(char *text, size_t text_len, uint32_t elapsed_ms)
{
    uint32_t total_sec = elapsed_ms / 1000U;
    uint32_t hour = (total_sec / 3600U) % 100U;
    uint32_t min = (total_sec / 60U) % 60U;
    uint32_t sec = total_sec % 60U;

    if (text == NULL || text_len < 9U) {
        return;
    }

    text[0] = (char)('0' + (hour / 10U));
    text[1] = (char)('0' + (hour % 10U));
    text[2] = ':';
    text[3] = (char)('0' + (min / 10U));
    text[4] = (char)('0' + (min % 10U));
    text[5] = ':';
    text[6] = (char)('0' + (sec / 10U));
    text[7] = (char)('0' + (sec % 10U));
    text[8] = '\0';
}

static void h264_encode_osd_fill_texts(char texts[H264_OSD_SLOT_COUNT][16],
                                      uint32_t elapsed_ms,
                                      uint32_t frame_index)
{
    uint32_t total_sec = elapsed_ms / 1000U;
    uint32_t hour = (total_sec / 3600U) % 100U;
    uint32_t min = (total_sec / 60U) % 60U;
    uint32_t sec = total_sec % 60U;

    h264_encode_osd_update_time_string(texts[0], sizeof(texts[0]), elapsed_ms);
    h264_encode_osd_format_u32_2(texts[1], sizeof(texts[1]), frame_index + 1U);
    h264_encode_osd_format_u32_2(texts[2], sizeof(texts[2]), frame_index % H264_ENC_TEST_GOP);
    h264_encode_osd_format_u32_2(texts[3], sizeof(texts[3]), sec);
    h264_encode_osd_format_u32_2(texts[4], sizeof(texts[4]), min);
    h264_encode_osd_format_u32_2(texts[5], sizeof(texts[5]), hour);
    h264_encode_osd_format_u32_2(texts[6], sizeof(texts[6]), (elapsed_ms / 100U) % 100U);
    os_snprintf(texts[7], sizeof(texts[7]), "S7");
}

static void *h264_encode_osd_outbuf_malloc(uint32_t size, void *args)
{
    (void)args;

    uint32_t frame_size = ((sizeof(frame_buffer_t) + size + 63U) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                                     frame_size);
    if (frame == NULL) {
        return NULL;
    }

    frame->frame = ((uint8_t *)frame) + (((sizeof(frame_buffer_t) + 63U) >> 6) << 6);
    frame->size = size;
    return frame->frame;
}

static uint32_t h264_encode_osd_outbuf_complete(bk_h264_encode_outbuf_info_t *info)
{
    h264_encode_osd_encode_ctx_t *ctx;

    if (info == NULL) {
        return BK_FAIL;
    }

    ctx = (h264_encode_osd_encode_ctx_t *)info->args;
    if (ctx != NULL) {
        ctx->result = info->status;
        ctx->frame_size = info->length;
        ctx->frame_type = info->type;
        if (ctx->done_sem != NULL) {
            rtos_set_semaphore(&ctx->done_sem);
        }
    }

    if (info->outbuf != NULL) {
        uint32_t frame_size = ((sizeof(frame_buffer_t) + 63U) >> 6) << 6;
        frame_buffer_t *frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
        bk_frame_buffer_free(frame);
    }

    return BK_OK;
}

avdk_err_t h264_encode_osd_submit_slot(bk_h264_encode_ctlr_handle_t handle,
                                       uint32_t slot_index,
                                       const char *text)
{
    bk_h264_encode_osd_t osd_cfg = {0};
    const h264_encode_osd_layout_t *layout;
    uint8_t *osd_buf;
    avdk_err_t ret;

    if (handle == NULL || slot_index >= H264_OSD_SLOT_COUNT || text == NULL) {
        return AVDK_ERR_INVAL;
    }

    layout = &s_osd_layouts[slot_index];
    osd_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                 layout->width * layout->height * 4U);
    if (osd_buf == NULL) {
        return AVDK_ERR_NOMEM;
    }

    if (slot_index == 0U) {
        h264_osd_draw_time_argb8888(osd_buf, layout->width, layout->height, text);
    } else {
        h264_osd_draw_text_argb8888(osd_buf, layout->width, layout->height,
                                    text, layout->scale, layout->argb);
    }
    flush_dcache(osd_buf, (long)(layout->width * layout->height * 4U));

    osd_cfg.index = slot_index;
    osd_cfg.buffer = osd_buf;
    osd_cfg.format = BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888;
    osd_cfg.alpha = 255U;
    osd_cfg.x = layout->x;
    osd_cfg.y = layout->y;
    osd_cfg.width = layout->width;
    osd_cfg.height = layout->height;
    osd_cfg.buffer_free = h264_encode_osd_buffer_free;
    osd_cfg.free_arg = NULL;
    osd_cfg.bitmap_y = 0U;
    osd_cfg.bitmap_u = 0U;
    osd_cfg.bitmap_v = 0U;

    ret = bk_h264_encode_set_osd(handle, &osd_cfg);
    if (ret != AVDK_ERR_OK) {
        h264_encode_osd_buffer_free(osd_buf, NULL);
        LOGE("set osd slot %u failed, ret=%d\r\n", slot_index, ret);
    }

    return ret;
}

avdk_err_t h264_encode_osd_clear_slot(bk_h264_encode_ctlr_handle_t handle,
                                      uint32_t slot_index)
{
    bk_h264_encode_osd_t osd_cfg = {0};

    if (handle == NULL || slot_index >= H264_OSD_SLOT_COUNT) {
        return AVDK_ERR_INVAL;
    }

    osd_cfg.index = slot_index;
    osd_cfg.buffer = NULL;
    osd_cfg.format = BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888;
    return bk_h264_encode_set_osd(handle, &osd_cfg);
}

avdk_err_t h264_encode_osd_submit_all(bk_h264_encode_ctlr_handle_t handle,
                                      uint32_t elapsed_ms,
                                      uint32_t frame_index)
{
    char texts[H264_OSD_SLOT_COUNT][16];
    avdk_err_t ret;

    h264_encode_osd_fill_texts(texts, elapsed_ms, frame_index);

    for (uint32_t i = 0U; i < H264_OSD_SLOT_COUNT; i++) {
        ret = h264_encode_osd_submit_slot(handle, i, texts[i]);
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
    }

    return AVDK_ERR_OK;
}

static void h264_encode_osd_log_test_result(uint8_t pass, const char *stage, int ret,
                                            uint32_t done_frames, uint32_t frame_size,
                                            uint32_t frame_type)
{
    if (pass) {
        LOGI("[RESULT][PASS] h264_encode_osd_test success, slots=%u/%u formats=%u/%u "
             "frames=%u/%u encoded_size=%u frame_type=%u\r\n",
             H264_OSD_CHANNEL_TEST_COUNT, H264_OSD_CHANNEL_TEST_COUNT,
             H264_OSD_FORMAT_TEST_COUNT, H264_OSD_FORMAT_TEST_COUNT,
             done_frames, H264_OSD_TEST_FRAME_COUNT, frame_size, frame_type);
    } else {
        LOGE("[RESULT][FAIL] h264_encode_osd_test failed at %s, ret=%d, slots=%u/%u "
             "formats=%u frames=%u/%u encoded_size=%u frame_type=%u\r\n",
             stage, ret, H264_OSD_CHANNEL_TEST_COUNT, H264_OSD_CHANNEL_TEST_COUNT,
             H264_OSD_FORMAT_TEST_COUNT, done_frames, H264_OSD_TEST_FRAME_COUNT,
             frame_size, frame_type);
    }
}

typedef struct {
    const char *name;
    uint32_t slot;
    uint32_t format;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t alpha;
    uint8_t bitmap_y;
    uint8_t bitmap_u;
    uint8_t bitmap_v;
    uint32_t argb;
    uint32_t font_scale;
    uint8_t is_time;
    const char *label;
} h264_encode_osd_channel_case_t;

/*
 * CTB = 64x16: one overlay per CTB cell (4 columns x 2 rows).
 * Row0: slot0 ARGB8888 time + slot1 NV12 + slot2 Bitmap + slot3 ARGB8888
 * Row1: slot4..7 ARGB8888 channel labels
 */
static const h264_encode_osd_channel_case_t s_channel_cases[H264_OSD_CHANNEL_TEST_COUNT] = {
    {
        "ARGB8888/time", 0U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 0U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW0_Y,
        H264_ENC_OSD_TIME_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_YELLOW, H264_OSD_TIME_FONT_SCALE, 1U, "00:00:00",
    },
    {
        "NV12", 1U, BK_H264_ENCODE_OVERLAY_FORMAT_NV12,
        H264_ENC_OSD_LEFT_X + 1U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW0_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 220U, 0U, 0U, 0U,
        0U, H264_OSD_LABEL_FONT_SCALE, 0U, "01",
    },
    {
        "Bitmap", 2U, BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP,
        H264_ENC_OSD_LEFT_X + 2U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW0_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 210U, 16U, 146U,
        0U, H264_OSD_LABEL_FONT_SCALE, 0U, "02",
    },
    {
        "ARGB8888", 3U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 3U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW0_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_CYAN, H264_OSD_LABEL_FONT_SCALE, 0U, "03",
    },
    {
        "ARGB8888", 4U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 0U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW1_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_MAGENTA, H264_OSD_LABEL_FONT_SCALE, 0U, "04",
    },
    {
        "ARGB8888", 5U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 1U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW1_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_WHITE, H264_OSD_LABEL_FONT_SCALE, 0U, "05",
    },
    {
        "ARGB8888", 6U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 2U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW1_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_GREEN, H264_OSD_LABEL_FONT_SCALE, 0U, "06",
    },
    {
        "ARGB8888", 7U, BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888,
        H264_ENC_OSD_LEFT_X + 3U * H264_ENC_OSD_COL_STEP, H264_ENC_OSD_TEST_ROW1_Y,
        H264_ENC_OSD_TEST_STRIP_W, H264_ENC_OSD_TEST_STRIP_H, 255U, 0U, 0U, 0U,
        H264_ENC_OSD_ARGB_ORANGE, H264_OSD_LABEL_FONT_SCALE, 0U, "07",
    },
};

static uint32_t h264_encode_osd_buffer_bytes(uint32_t format, uint32_t width, uint32_t height)
{
    switch (format) {
    case BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888:
        return width * height * 4U;
    case BK_H264_ENCODE_OVERLAY_FORMAT_NV12:
        return width * height * 3U / 2U;
    case BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP:
        return (width / 8U) * height;
    default:
        return 0U;
    }
}

static avdk_err_t h264_encode_osd_draw_channel_buffer(const h264_encode_osd_channel_case_t *test_case,
                                                     uint8_t *buffer)
{
    if (test_case == NULL || buffer == NULL) {
        return AVDK_ERR_INVAL;
    }

    switch (test_case->format) {
    case BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888:
        if (test_case->is_time) {
            h264_osd_draw_time_argb8888(buffer, test_case->width, test_case->height,
                                        test_case->label);
        } else {
            h264_osd_draw_text_argb8888(buffer, test_case->width, test_case->height,
                                        test_case->label, test_case->font_scale,
                                        test_case->argb);
        }
        flush_dcache(buffer, (long)(test_case->width * test_case->height * 4U));
        return AVDK_ERR_OK;
    case BK_H264_ENCODE_OVERLAY_FORMAT_NV12:
        h264_osd_draw_text_nv12(buffer, test_case->width, test_case->height,
                                test_case->label, test_case->font_scale,
                                210U, 16U, 146U);
        flush_dcache(buffer, (long)(test_case->width * test_case->height * 3U / 2U));
        return AVDK_ERR_OK;
    case BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP:
        h264_osd_draw_text_bitmap(buffer, test_case->width, test_case->height,
                                  test_case->label, test_case->font_scale);
        flush_dcache(buffer, (long)((test_case->width / 8U) * test_case->height));
        return AVDK_ERR_OK;
    default:
        return AVDK_ERR_INVAL;
    }
}

static avdk_err_t h264_encode_osd_submit_channel_case(bk_h264_encode_ctlr_handle_t handle,
                                                      const h264_encode_osd_channel_case_t *test_case)
{
    bk_h264_encode_osd_t osd_cfg = {0};
    uint8_t *osd_buf;
    uint32_t buf_bytes;
    avdk_err_t ret;

    if (handle == NULL || test_case == NULL) {
        return AVDK_ERR_INVAL;
    }

    buf_bytes = h264_encode_osd_buffer_bytes(test_case->format,
                                             test_case->width, test_case->height);
    if (buf_bytes == 0U) {
        return AVDK_ERR_INVAL;
    }

    osd_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buf_bytes);
    if (osd_buf == NULL) {
        return AVDK_ERR_NOMEM;
    }

    ret = h264_encode_osd_draw_channel_buffer(test_case, osd_buf);
    if (ret != AVDK_ERR_OK) {
        h264_encode_osd_buffer_free(osd_buf, NULL);
        return ret;
    }

    osd_cfg.index = test_case->slot;
    osd_cfg.buffer = osd_buf;
    osd_cfg.format = test_case->format;
    osd_cfg.alpha = test_case->alpha;
    osd_cfg.x = test_case->x;
    osd_cfg.y = test_case->y;
    osd_cfg.width = test_case->width;
    osd_cfg.height = test_case->height;
    osd_cfg.bitmap_y = test_case->bitmap_y;
    osd_cfg.bitmap_u = test_case->bitmap_u;
    osd_cfg.bitmap_v = test_case->bitmap_v;
    osd_cfg.buffer_free = h264_encode_osd_buffer_free;
    osd_cfg.free_arg = NULL;

    ret = bk_h264_encode_set_osd(handle, &osd_cfg);
    if (ret != AVDK_ERR_OK) {
        h264_encode_osd_buffer_free(osd_buf, NULL);
        LOGE("set osd %s slot %u failed, ret=%d\r\n",
             test_case->name, test_case->slot, ret);
    }

    return ret;
}

static avdk_err_t h264_encode_osd_submit_all_channels(bk_h264_encode_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_OK;
    avdk_err_t slot_ret;

    if (handle == NULL) {
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0U; i < H264_OSD_SLOT_COUNT; i++) {
        slot_ret = h264_encode_osd_clear_slot(handle, i);
        if (slot_ret != AVDK_ERR_OK) {
            ret = slot_ret;
        }
    }

    for (uint32_t idx = 0U; idx < H264_OSD_CHANNEL_TEST_COUNT; idx++) {
        const h264_encode_osd_channel_case_t *test_case = &s_channel_cases[idx];

        LOGI("osd channel %u/%u: %s slot=%u fmt=%u x=%u y=%u w=%u h=%u label=%s\r\n",
             idx + 1U, H264_OSD_CHANNEL_TEST_COUNT, test_case->name, test_case->slot,
             test_case->format, test_case->x, test_case->y,
             test_case->width, test_case->height, test_case->label);

        slot_ret = h264_encode_osd_submit_channel_case(handle, test_case);
        if (slot_ret != AVDK_ERR_OK) {
            ret = slot_ret;
        }
    }

    return ret;
}

static int h264_encode_osd_wait_done(h264_encode_osd_encode_ctx_t *ctx)
{
    bk_err_t ret;

    if (ctx == NULL || ctx->done_sem == NULL) {
        return BK_FAIL;
    }

    ret = rtos_get_semaphore(&ctx->done_sem, H264_ENC_TEST_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("encode wait timeout, ret=%d\r\n", ret);
        return BK_FAIL;
    }
    if (ctx->result != BK_OK) {
        LOGE("encode failed, result=%u\r\n", ctx->result);
        return BK_FAIL;
    }

    return BK_OK;
}

int h264_encode_osd_test(void)
{
    int ret = BK_FAIL;
    avdk_err_t avdk_ret;
    uint8_t *input = NULL;
    bk_h264_encode_ctlr_handle_t handle = NULL;
    h264_encode_osd_encode_ctx_t ctx;
    bk_h264_encode_frame_config_t config;
    const char *fail_stage = "start";
    uint8_t test_pass = 0U;
    uint32_t done_frames = 0U;

    LOGI("osd test start, slots=%u formats=%u frame=%ux%u\r\n",
         H264_OSD_CHANNEL_TEST_COUNT, H264_OSD_FORMAT_TEST_COUNT,
         H264_ENC_TEST_WIDTH, H264_ENC_TEST_HEIGHT);
    os_memset(&ctx, 0, sizeof(ctx));
    os_memset(&config, 0, sizeof(config));

    if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
        fail_stage = "init_done_sem";
        goto exit;
    }

    input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                              h264_encode_stream_256x128_bytes + 32U);
    if (input == NULL) {
        fail_stage = "alloc_input";
        goto exit;
    }
    os_memcpy(input, h264_encode_stream_256x128, h264_encode_stream_256x128_bytes);

    config.width = H264_ENC_TEST_WIDTH;
    config.height = H264_ENC_TEST_HEIGHT;
    config.gop_frame_count = H264_ENC_TEST_GOP;
    config.input_format = BK_PIXEL_FORMAT_NV12;
    config.input_flexa_cnt = 1;
    config.input_buf = (uint32_t)input;
    config.input_size = h264_encode_stream_256x128_bytes;
    config.outbuf_malloc = h264_encode_osd_outbuf_malloc;
    config.outbuf_complete = h264_encode_osd_outbuf_complete;
    config.outbuf_complete_args = &ctx;

    avdk_ret = bk_h264_encode_frame_new(&handle, &config);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "frame_ctlr_new";
        ret = avdk_ret;
        goto exit;
    }
    avdk_ret = bk_h264_encode_init(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_init";
        ret = avdk_ret;
        goto exit;
    }
    avdk_ret = bk_h264_encode_open(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_open";
        ret = avdk_ret;
        goto exit;
    }

    avdk_ret = h264_encode_osd_submit_all_channels(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "set_osd";
        ret = avdk_ret;
        goto exit;
    }

    ctx.result = BK_FAIL;
    ctx.frame_size = 0U;
    ctx.frame_type = 0U;

    avdk_ret = bk_h264_encode_start(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encode_start";
        ret = avdk_ret;
        goto exit;
    }

    fail_stage = "wait_done";
    ret = h264_encode_osd_wait_done(&ctx);
    if (ret != BK_OK) {
        goto exit;
    }

    done_frames = 1U;
    LOGI("osd 8-channel encode done, size=%u type=%u\r\n",
         ctx.frame_size, ctx.frame_type);

    test_pass = 1U;
    ret = BK_OK;

exit:
    if (handle != NULL) {
        bk_h264_encode_close(handle);
        bk_h264_encode_deinit(handle);
        bk_h264_encode_delete(handle);
    }
    if (input != NULL) {
        bk_frame_buffer_free(input);
    }
    if (ctx.done_sem != NULL) {
        rtos_deinit_semaphore(&ctx.done_sem);
    }
    h264_encode_osd_log_test_result(test_pass, fail_stage, ret, done_frames,
                                    ctx.frame_size, ctx.frame_type);
    return ret;
}
