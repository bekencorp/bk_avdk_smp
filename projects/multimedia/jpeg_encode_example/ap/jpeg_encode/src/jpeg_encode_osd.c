#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_encode/bk_jpeg_encode_ctlr.h>

#include "cache.h"
#include "jpeg_encode_osd.h"

#define TAG "jpeg_encode_osd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_OSD_SLOT_COUNT          8U

/* 4x2 个大色块居中铺开，避免 OSD 太小/贴边导致上屏后看不清 */
#define JPEG_OSD_BLOCK_W             240U
#define JPEG_OSD_BLOCK_H             120U
#define JPEG_OSD_BLOCK_STEP_X        264U
#define JPEG_OSD_BLOCK_STEP_Y        144U
#define JPEG_OSD_BLOCK_LEFT_X        444U
#define JPEG_OSD_BLOCK_TOP_Y         408U

#define JPEG_OSD_COL_X(idx)          (JPEG_OSD_BLOCK_LEFT_X + (idx) * JPEG_OSD_BLOCK_STEP_X)
#define JPEG_OSD_ROW_Y(idx)          (JPEG_OSD_BLOCK_TOP_Y + (idx) * JPEG_OSD_BLOCK_STEP_Y)

#define JPEG_OSD_LABEL_FONT_SCALE    10U
#define JPEG_OSD_TIME_FONT_SCALE     5U

#define JPEG_OSD_ARGB_YELLOW         (0xFFFFFF00U)
#define JPEG_OSD_ARGB_CYAN           (0xFF00FFFFU)
#define JPEG_OSD_ARGB_GREEN          (0xFF00FF00U)
#define JPEG_OSD_ARGB_ORANGE         (0xFFFFA500U)
#define JPEG_OSD_ARGB_MAGENTA        (0xFFFF00FFU)
#define JPEG_OSD_ARGB_WHITE          (0xFFFFFFFFU)

static const uint8_t s_osd_digit_font[11][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    {0x1F, 0x02, 0x04, 0x06, 0x02, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    {0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00},
};

static const uint8_t *jpeg_osd_font_row_for_char(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return s_osd_digit_font[ch - '0'];
    }
    if (ch == ':') {
        return s_osd_digit_font[10];
    }
    return NULL;
}

static void jpeg_osd_put_pixel_argb(uint8_t *buffer, uint32_t width,
                                    uint32_t x, uint32_t y, uint32_t argb)
{
    if (buffer == NULL || x >= width) {
        return;
    }

    *(uint32_t *)(buffer + (y * width + x) * 4U) = argb;
}

static void jpeg_osd_fill_rect_argb(uint8_t *buffer, uint32_t width, uint32_t height,
                                    uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                    uint32_t argb)
{
    for (uint32_t row = 0; row < h; row++) {
        if ((y + row) >= height) {
            break;
        }
        for (uint32_t col = 0; col < w; col++) {
            if ((x + col) >= width) {
                break;
            }
            jpeg_osd_put_pixel_argb(buffer, width, x + col, y + row, argb);
        }
    }
}

static void jpeg_osd_draw_char_argb(uint8_t *buffer, uint32_t width, uint32_t height,
                                    uint32_t x, uint32_t y, char ch, uint32_t scale,
                                    uint32_t argb)
{
    const uint8_t *rows = jpeg_osd_font_row_for_char(ch);

    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            jpeg_osd_fill_rect_argb(buffer, width, height,
                                    x + col * scale, y + row * scale, scale, scale, argb);
        }
    }
}

static void jpeg_osd_draw_text_argb8888(uint8_t *buffer, uint32_t width, uint32_t height,
                                        const char *text, uint32_t scale, uint32_t argb)
{
    const uint32_t glyph_w = 5U * scale;
    const uint32_t glyph_h = 7U * scale;
    const uint32_t char_w = glyph_w + 4U;
    const uint32_t text_len = (text != NULL) ? os_strlen(text) : 0U;
    uint32_t total_w = (text_len > 0U) ? (text_len * char_w - 4U) : 0U;
    uint32_t x;
    uint32_t y;

    if (buffer == NULL || width == 0U || height == 0U) {
        return;
    }

    if (text == NULL || text_len == 0U || scale == 0U) {
        return;
    }

    if (total_w > width) {
        total_w = width;
    }
    if (glyph_h > height) {
        return;
    }

    x = (width > total_w) ? ((width - total_w) / 2U) : 0U;
    y = (height > glyph_h) ? ((height - glyph_h) / 2U) : 0U;

    for (size_t i = 0; text[i] != '\0' && (x + glyph_w) <= width; i++) {
        jpeg_osd_draw_char_argb(buffer, width, height, x, y, text[i], scale, argb);
        x += char_w;
    }
}

static void jpeg_osd_put_pixel_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                                    uint32_t x, uint32_t y,
                                    uint8_t y_val, uint8_t u_val, uint8_t v_val)
{
    uint32_t y_size;
    uint32_t uv_index;

    if (buffer == NULL || x >= width || y >= height) {
        return;
    }

    y_size = width * height;
    buffer[y * width + x] = y_val;
    uv_index = y_size + (y / 2U) * width + (x / 2U) * 2U;
    buffer[uv_index] = u_val;
    buffer[uv_index + 1U] = v_val;
}

static void jpeg_osd_fill_rect_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                                    uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                    uint8_t y_val, uint8_t u_val, uint8_t v_val)
{
    for (uint32_t row = 0; row < h; row++) {
        if ((y + row) >= height) {
            break;
        }
        for (uint32_t col = 0; col < w; col++) {
            if ((x + col) >= width) {
                break;
            }
            jpeg_osd_put_pixel_nv12(buffer, width, height, x + col, y + row,
                                    y_val, u_val, v_val);
        }
    }
}

static void jpeg_osd_draw_char_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                                    uint32_t x, uint32_t y, char ch, uint32_t scale,
                                    uint8_t y_val, uint8_t u_val, uint8_t v_val)
{
    const uint8_t *rows = jpeg_osd_font_row_for_char(ch);

    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            jpeg_osd_fill_rect_nv12(buffer, width, height,
                                    x + col * scale, y + row * scale, scale, scale,
                                    y_val, u_val, v_val);
        }
    }
}

static void jpeg_osd_draw_text_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                                    const char *text, uint32_t scale,
                                    uint8_t fg_y, uint8_t fg_u, uint8_t fg_v)
{
    const uint32_t glyph_w = 5U * scale;
    const uint32_t glyph_h = 7U * scale;
    const uint32_t char_w = glyph_w + 4U;
    const uint32_t text_len = (text != NULL) ? os_strlen(text) : 0U;
    uint32_t total_w = (text_len > 0U) ? (text_len * char_w - 4U) : 0U;
    uint32_t x;
    uint32_t y;

    if (buffer == NULL || width == 0U || height == 0U) {
        return;
    }

    if (text == NULL || text_len == 0U || scale == 0U) {
        return;
    }

    if (total_w > width) {
        total_w = width;
    }
    if (glyph_h > height) {
        return;
    }

    x = (width > total_w) ? ((width - total_w) / 2U) : 0U;
    y = (height > glyph_h) ? ((height - glyph_h) / 2U) : 0U;

    for (size_t i = 0; text[i] != '\0' && (x + glyph_w) <= width; i++) {
        jpeg_osd_draw_char_nv12(buffer, width, height, x, y, text[i], scale,
                                fg_y, fg_u, fg_v);
        x += char_w;
    }
}

static void jpeg_osd_set_bitmap_bit(uint8_t *buffer, uint32_t width, uint32_t x, uint32_t y)
{
    uint32_t row_bytes = width / 8U;

    if (buffer == NULL || x >= width) {
        return;
    }

    buffer[y * row_bytes + (x / 8U)] |= (uint8_t)(0x80U >> (x & 7U));
}

static void jpeg_osd_fill_rect_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                                      uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    for (uint32_t row = 0; row < h; row++) {
        if ((y + row) >= height) {
            break;
        }
        for (uint32_t col = 0; col < w; col++) {
            if ((x + col) >= width) {
                break;
            }
            jpeg_osd_set_bitmap_bit(buffer, width, x + col, y + row);
        }
    }
}

static void jpeg_osd_clear_bitmap_bit(uint8_t *buffer, uint32_t width, uint32_t x, uint32_t y)
{
    uint32_t row_bytes = width / 8U;

    if (buffer == NULL || x >= width) {
        return;
    }

    buffer[y * row_bytes + (x / 8U)] &= (uint8_t)~(0x80U >> (x & 7U));
}

static void jpeg_osd_clear_rect_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                                       uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    for (uint32_t row = 0; row < h; row++) {
        if ((y + row) >= height) {
            break;
        }
        for (uint32_t col = 0; col < w; col++) {
            if ((x + col) >= width) {
                break;
            }
            jpeg_osd_clear_bitmap_bit(buffer, width, x + col, y + row);
        }
    }
}

static void jpeg_osd_draw_char_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                                      uint32_t x, uint32_t y, char ch, uint32_t scale,
                                      bool invert)
{
    const uint8_t *rows = jpeg_osd_font_row_for_char(ch);

    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            if (invert) {
                jpeg_osd_clear_rect_bitmap(buffer, width, height,
                                           x + col * scale, y + row * scale, scale, scale);
            } else {
                jpeg_osd_fill_rect_bitmap(buffer, width, height,
                                          x + col * scale, y + row * scale, scale, scale);
            }
        }
    }
}

static void jpeg_osd_draw_text_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                                      const char *text, uint32_t scale, bool invert)
{
    const uint32_t glyph_w = 5U * scale;
    const uint32_t glyph_h = 7U * scale;
    const uint32_t char_w = glyph_w + 4U;
    const uint32_t text_len = (text != NULL) ? os_strlen(text) : 0U;
    uint32_t total_w = (text_len > 0U) ? (text_len * char_w - 4U) : 0U;
    uint32_t x;
    uint32_t y;

    if (buffer == NULL || width == 0U || height == 0U || ((width & 7U) != 0U)) {
        return;
    }

    if (text == NULL || text_len == 0U || scale == 0U) {
        return;
    }

    if (total_w > width) {
        total_w = width;
    }
    if (glyph_h > height) {
        return;
    }

    x = (width > total_w) ? ((width - total_w) / 2U) : 0U;
    y = (height > glyph_h) ? ((height - glyph_h) / 2U) : 0U;

    for (size_t i = 0; text[i] != '\0' && (x + glyph_w) <= width; i++) {
        jpeg_osd_draw_char_bitmap(buffer, width, height, x, y, text[i], scale, invert);
        x += char_w;
    }
}

static void jpeg_osd_buffer_free(void *buffer, void *free_arg)
{
    (void)free_arg;

    if (buffer != NULL) {
        bk_frame_buffer_free(buffer);
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
} jpeg_encode_osd_channel_case_t;

/*
 * 8 slots in 2 rows of 4 columns, every slot is a 240x120 solid block so the
 * overlay is clearly visible on the 1920x1080 panel:
 * Row0: ARGB8888 time + NV12 + Bitmap + ARGB8888
 * Row1: ARGB8888 x4
 */
static const jpeg_encode_osd_channel_case_t s_channel_cases[JPEG_OSD_SLOT_COUNT] = {
    {
        "ARGB8888/time", 0U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(0U), JPEG_OSD_ROW_Y(0U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_YELLOW, JPEG_OSD_TIME_FONT_SCALE, 1U, "00:00:00",
    },
    {
        "NV12", 1U, BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12,
        JPEG_OSD_COL_X(1U), JPEG_OSD_ROW_Y(0U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        0U, JPEG_OSD_LABEL_FONT_SCALE, 0U, "01",
    },
    {
        "Bitmap", 2U, BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP,
        JPEG_OSD_COL_X(2U), JPEG_OSD_ROW_Y(0U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 210U, 16U, 146U,
        0U, JPEG_OSD_LABEL_FONT_SCALE, 0U, "02",
    },
    {
        "ARGB8888", 3U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(3U), JPEG_OSD_ROW_Y(0U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_CYAN, JPEG_OSD_LABEL_FONT_SCALE, 0U, "03",
    },
    {
        "ARGB8888", 4U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(0U), JPEG_OSD_ROW_Y(1U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_MAGENTA, JPEG_OSD_LABEL_FONT_SCALE, 0U, "04",
    },
    {
        "ARGB8888", 5U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(1U), JPEG_OSD_ROW_Y(1U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_WHITE, JPEG_OSD_LABEL_FONT_SCALE, 0U, "05",
    },
    {
        "ARGB8888", 6U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(2U), JPEG_OSD_ROW_Y(1U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_GREEN, JPEG_OSD_LABEL_FONT_SCALE, 0U, "06",
    },
    {
        "ARGB8888", 7U, BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888,
        JPEG_OSD_COL_X(3U), JPEG_OSD_ROW_Y(1U),
        JPEG_OSD_BLOCK_W, JPEG_OSD_BLOCK_H, 255U, 0U, 0U, 0U,
        JPEG_OSD_ARGB_ORANGE, JPEG_OSD_LABEL_FONT_SCALE, 0U, "07",
    },
};

static uint32_t jpeg_osd_buffer_bytes(uint32_t format, uint32_t width, uint32_t height)
{
    switch (format) {
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888:
        return width * height * 4U;
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12:
        return width * height * 3U / 2U;
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP:
        return (width / 8U) * height;
    default:
        return 0U;
    }
}

static avdk_err_t jpeg_osd_draw_channel_buffer(const jpeg_encode_osd_channel_case_t *test_case,
                                               uint8_t *buffer)
{
    if (test_case == NULL || buffer == NULL) {
        return AVDK_ERR_INVAL;
    }

    switch (test_case->format) {
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888:
        jpeg_osd_fill_rect_argb(buffer, test_case->width, test_case->height,
                                0U, 0U, test_case->width, test_case->height,
                                test_case->argb);
        jpeg_osd_draw_text_argb8888(buffer, test_case->width, test_case->height,
                                    test_case->label, test_case->font_scale,
                                    0xFF000000U);
        flush_dcache(buffer, (long)(test_case->width * test_case->height * 4U));
        return AVDK_ERR_OK;
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_NV12:
        jpeg_osd_fill_rect_nv12(buffer, test_case->width, test_case->height,
                                0U, 0U, test_case->width, test_case->height,
                                210U, 16U, 146U);
        jpeg_osd_draw_text_nv12(buffer, test_case->width, test_case->height,
                                test_case->label, test_case->font_scale,
                                16U, 128U, 128U);
        flush_dcache(buffer, (long)(test_case->width * test_case->height * 3U / 2U));
        return AVDK_ERR_OK;
    case BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP:
        jpeg_osd_fill_rect_bitmap(buffer, test_case->width, test_case->height,
                                  0U, 0U, test_case->width, test_case->height);
        jpeg_osd_draw_text_bitmap(buffer, test_case->width, test_case->height,
                                  test_case->label, test_case->font_scale, true);
        flush_dcache(buffer, (long)((test_case->width / 8U) * test_case->height));
        return AVDK_ERR_OK;
    default:
        return AVDK_ERR_INVAL;
    }
}

static avdk_err_t jpeg_osd_submit_channel_case(bk_jpeg_encode_ctlr_handle_t handle,
                                               const jpeg_encode_osd_channel_case_t *test_case)
{
    bk_jpeg_encode_osd_t osd_cfg = {0};
    uint8_t *osd_buf;
    uint32_t buf_bytes;
    avdk_err_t ret;

    if (handle == NULL || test_case == NULL) {
        return AVDK_ERR_INVAL;
    }

    buf_bytes = jpeg_osd_buffer_bytes(test_case->format,
                                      test_case->width, test_case->height);
    if (buf_bytes == 0U) {
        return AVDK_ERR_INVAL;
    }

    osd_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, buf_bytes);
    if (osd_buf == NULL) {
        return AVDK_ERR_NOMEM;
    }

    ret = jpeg_osd_draw_channel_buffer(test_case, osd_buf);
    if (ret != AVDK_ERR_OK) {
        jpeg_osd_buffer_free(osd_buf, NULL);
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
    osd_cfg.buffer_free = jpeg_osd_buffer_free;
    osd_cfg.free_arg = NULL;

    ret = bk_jpeg_encode_set_osd(handle, &osd_cfg);
    if (ret != AVDK_ERR_OK) {
        jpeg_osd_buffer_free(osd_buf, NULL);
        LOGE("set osd %s slot %u failed, ret=%d\r\n",
             test_case->name, test_case->slot, ret);
    }

    return ret;
}

static avdk_err_t jpeg_osd_clear_slot(bk_jpeg_encode_ctlr_handle_t handle, uint32_t slot_index)
{
    bk_jpeg_encode_osd_t osd_cfg = {0};

    if (handle == NULL || slot_index >= JPEG_OSD_SLOT_COUNT) {
        return AVDK_ERR_INVAL;
    }

    osd_cfg.index = slot_index;
    osd_cfg.buffer = NULL;
    osd_cfg.format = BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888;
    return bk_jpeg_encode_set_osd(handle, &osd_cfg);
}

static avdk_err_t jpeg_osd_submit_channels(bk_jpeg_encode_ctlr_handle_t handle,
                                           const char *mode_text)
{
    avdk_err_t ret = AVDK_ERR_OK;
    avdk_err_t slot_ret;

    if (handle == NULL) {
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0U; i < JPEG_OSD_SLOT_COUNT; i++) {
        slot_ret = jpeg_osd_clear_slot(handle, i);
        if (slot_ret != AVDK_ERR_OK) {
            ret = slot_ret;
        }
    }

    for (uint32_t idx = 0U; idx < JPEG_OSD_SLOT_COUNT; idx++) {
        jpeg_encode_osd_channel_case_t test_case = s_channel_cases[idx];

        if (idx == 0U && mode_text != NULL) {
            test_case.label = mode_text;
        }

        LOGI("osd channel %u/%u: %s slot=%u fmt=%u x=%u y=%u w=%u h=%u label=%s\r\n",
             idx + 1U, JPEG_OSD_SLOT_COUNT, test_case.name, test_case.slot,
             test_case.format, test_case.x, test_case.y,
             test_case.width, test_case.height, test_case.label);

        slot_ret = jpeg_osd_submit_channel_case(handle, &test_case);
        if (slot_ret != AVDK_ERR_OK) {
            ret = slot_ret;
        }
    }

    return ret;
}

avdk_err_t jpeg_encode_osd_submit_all_channels(bk_jpeg_encode_ctlr_handle_t handle,
                                               const char *mode_text)
{
    return jpeg_osd_submit_channels(handle, mode_text);
}

avdk_err_t jpeg_encode_osd_submit_bench_osd(bk_jpeg_encode_ctlr_handle_t handle,
                                            uint32_t format,
                                            uint32_t width, uint32_t height)
{
    jpeg_encode_osd_channel_case_t bench_case;
    avdk_err_t ret = AVDK_ERR_OK;
    avdk_err_t slot_ret;

    if (handle == NULL) {
        return AVDK_ERR_INVAL;
    }

    if (format != BK_JPEG_ENCODE_OVERLAY_FORMAT_ARGB8888 &&
        format != BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP) {
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0U; i < JPEG_OSD_SLOT_COUNT; i++) {
        slot_ret = jpeg_osd_clear_slot(handle, i);
        if (slot_ret != AVDK_ERR_OK) {
            ret = slot_ret;
        }
    }

    /* width/height == 0 means "no OSD": all slots stay cleared. */
    if (width == 0U || height == 0U) {
        return ret;
    }

    /* BITMAP is 1 bpp, the encoder requires both dimensions to be 8-aligned. */
    if (format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP &&
        (((width | height) & 7U) != 0U)) {
        LOGE("osd bench bitmap size must be 8-aligned: w=%u h=%u\r\n",
             (unsigned)width, (unsigned)height);
        return AVDK_ERR_INVAL;
    }

    /*
     * Exactly one block at (0,0) with the requested size and format, so the OSD
     * compositing cost scales with the covered area alone (one block, not eight
     * small ones) and covers the frame up to 1920x1080.
     */
    bench_case = (jpeg_encode_osd_channel_case_t) {
        (format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP) ?
            "BITMAP/bench" : "ARGB8888/bench",
        0U, format,
        0U, 0U, width, height, 255U, 210U, 16U, 146U,
        JPEG_OSD_ARGB_MAGENTA, JPEG_OSD_LABEL_FONT_SCALE, 0U, "SW OSD",
    };

    LOGI("osd bench overlay: %s slot=0 x=0 y=0 w=%u h=%u bytes=%u\r\n",
         (format == BK_JPEG_ENCODE_OVERLAY_FORMAT_BITMAP) ? "BITMAP" : "ARGB8888",
         (unsigned)width, (unsigned)height,
         (unsigned)jpeg_osd_buffer_bytes(format, width, height));

    slot_ret = jpeg_osd_submit_channel_case(handle, &bench_case);
    if (slot_ret != AVDK_ERR_OK) {
        ret = slot_ret;
    }

    return ret;
}
