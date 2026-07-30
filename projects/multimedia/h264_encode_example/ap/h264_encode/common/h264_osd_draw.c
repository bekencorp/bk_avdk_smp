#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>

#include "h264_osd_draw.h"

#define OSD_ARGB(a, r, g, b) \
    ((uint32_t)(((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

static void osd_put_pixel(uint8_t *buffer, uint32_t width, uint32_t x, uint32_t y, uint32_t argb)
{
    if (buffer == NULL || x >= width) {
        return;
    }

    uint32_t *pixel = (uint32_t *)(buffer + (y * width + x) * 4U);
    *pixel = argb;
}

static void osd_fill_rect(uint8_t *buffer, uint32_t width, uint32_t height,
                          uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t argb)
{
    for (uint32_t row = 0; row < h; row++) {
        if ((y + row) >= height) {
            break;
        }
        for (uint32_t col = 0; col < w; col++) {
            if ((x + col) >= width) {
                break;
            }
            osd_put_pixel(buffer, width, x + col, y + row, argb);
        }
    }
}

static const uint8_t s_digit_font[11][7] = {
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

static const uint8_t *osd_font_row_for_char(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return s_digit_font[ch - '0'];
    }
    if (ch == ':') {
        return s_digit_font[10];
    }
    return NULL;
}

static void osd_draw_char(uint8_t *buffer, uint32_t width, uint32_t height,
                          uint32_t x, uint32_t y, char ch, uint32_t scale, uint32_t argb)
{
    const uint8_t *rows = osd_font_row_for_char(ch);
    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            osd_fill_rect(buffer, width, height,
                          x + col * scale, y + row * scale, scale, scale, argb);
        }
    }
}

void h264_osd_draw_text_argb8888(uint8_t *buffer, uint32_t width, uint32_t height,
                                 const char *text, uint32_t scale, uint32_t argb)
{
    const uint32_t glyph_w = 5U * scale;
    const uint32_t glyph_h = 7U * scale;
    const uint32_t char_w = glyph_w + 4U;
    const uint32_t text_len = (text != NULL) ? os_strlen(text) : 0U;
    uint32_t total_w = (text_len > 0U) ? (text_len * char_w - 4U) : 0U;
    uint32_t x;
    uint32_t y;

    if (buffer == NULL || width == 0 || height == 0) {
        return;
    }

    os_memset(buffer, 0, (size_t)width * (size_t)height * 4U);

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
        osd_draw_char(buffer, width, height, x, y, text[i], scale, argb);
        x += char_w;
    }
}

void h264_osd_draw_time_argb8888(uint8_t *buffer, uint32_t width, uint32_t height,
                                 const char *text)
{
    h264_osd_draw_text_argb8888(buffer, width, height, text,
                               H264_OSD_TIME_FONT_SCALE,
                               OSD_ARGB(255, 255, 255, 0));
}

static void osd_put_pixel_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
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

static void osd_fill_rect_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
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
            osd_put_pixel_nv12(buffer, width, height, x + col, y + row,
                               y_val, u_val, v_val);
        }
    }
}

static void osd_draw_char_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
                               uint32_t x, uint32_t y, char ch, uint32_t scale,
                               uint8_t y_val, uint8_t u_val, uint8_t v_val)
{
    const uint8_t *rows = osd_font_row_for_char(ch);
    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            osd_fill_rect_nv12(buffer, width, height,
                               x + col * scale, y + row * scale, scale, scale,
                               y_val, u_val, v_val);
        }
    }
}

void h264_osd_draw_text_nv12(uint8_t *buffer, uint32_t width, uint32_t height,
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
    uint32_t y_size = width * height;

    if (buffer == NULL || width == 0U || height == 0U) {
        return;
    }

    os_memset(buffer, 0, (size_t)y_size * 3U / 2U);
    for (uint32_t i = y_size; i < y_size + (width * height / 2U); i++) {
        buffer[i] = 128U;
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
        osd_draw_char_nv12(buffer, width, height, x, y, text[i], scale,
                           fg_y, fg_u, fg_v);
        x += char_w;
    }
}

static void osd_set_bitmap_bit(uint8_t *buffer, uint32_t width, uint32_t x, uint32_t y)
{
    uint32_t row_bytes = width / 8U;

    if (buffer == NULL || x >= width) {
        return;
    }

    buffer[y * row_bytes + (x / 8U)] |= (uint8_t)(0x80U >> (x & 7U));
}

static void osd_fill_rect_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
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
            osd_set_bitmap_bit(buffer, width, x + col, y + row);
        }
    }
}

static void osd_draw_char_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                                 uint32_t x, uint32_t y, char ch, uint32_t scale)
{
    const uint8_t *rows = osd_font_row_for_char(ch);
    if (rows == NULL) {
        return;
    }

    for (uint32_t row = 0; row < 7U; row++) {
        uint8_t bits = rows[row];
        for (uint32_t col = 0; col < 5U; col++) {
            if ((bits & (1U << (4U - col))) == 0U) {
                continue;
            }
            osd_fill_rect_bitmap(buffer, width, height,
                                 x + col * scale, y + row * scale, scale, scale);
        }
    }
}

void h264_osd_draw_text_bitmap(uint8_t *buffer, uint32_t width, uint32_t height,
                               const char *text, uint32_t scale)
{
    const uint32_t glyph_w = 5U * scale;
    const uint32_t glyph_h = 7U * scale;
    const uint32_t char_w = glyph_w + 4U;
    const uint32_t text_len = (text != NULL) ? os_strlen(text) : 0U;
    uint32_t total_w = (text_len > 0U) ? (text_len * char_w - 4U) : 0U;
    uint32_t x;
    uint32_t y;
    uint32_t buf_bytes = (width / 8U) * height;

    if (buffer == NULL || width == 0U || height == 0U || ((width & 7U) != 0U)) {
        return;
    }

    os_memset(buffer, 0, buf_bytes);

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
        osd_draw_char_bitmap(buffer, width, height, x, y, text[i], scale);
        x += char_w;
    }
}
