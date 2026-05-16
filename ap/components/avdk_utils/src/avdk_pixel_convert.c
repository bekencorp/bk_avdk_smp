#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include <avdk_check.h>

#include <avdk_pixel_convert.h>

static const char* TAG = "px-convert";

#define LOGI(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW((char*)TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE((char*)TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD((char*)TAG, ##__VA_ARGS__)
#define LOGV(...)

/**
 * Convert BGRA8888 (memory order +0=B +1=G +2=R +3=A) to RGB565.
 *
 * On the little-endian BK platform, reading 4 BGRA bytes as a uint32_t
 * yields 0xAARRGGBB, so R/G/B can be extracted with the shift pattern
 * below. (Note: this is NOT the same as BK_PIXEL_FORMAT_ARGB8888, which
 * has memory order +0=A +1=R +2=G +3=B.)
 */
int bk_pixel_bgra8888_to_rgb565(uint32_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
    if (src == NULL || dst == NULL || width == 0 || height == 0) {
        LOGE("%s, %d src or dst is NULL or width or height is 0\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    for (uint32_t i = 0; i < width * height; ++i) {
        /* mem[+3..+0] = A R G B  ->  uint32_t = 0xAARRGGBB on little-endian */
        uint32_t pixel = src[i];

        uint8_t r8 = (pixel >> 16) & 0xFF;
        uint8_t g8 = (pixel >> 8)  & 0xFF;
        uint8_t b8 = (pixel >> 0)  & 0xFF;

        uint16_t r5 = (uint16_t)(r8 >> 3);
        uint16_t g6 = (uint16_t)(g8 >> 2);
        uint16_t b5 = (uint16_t)(b8 >> 3);

        dst[i] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }

    return BK_OK;
}
/**
 * Convert packed RGB888 (memory order +0=R +1=G +2=B) to RGB565.
 *
 * Source bytes are read directly from byte addresses, so this routine is
 * endian-safe and matches BK_PIXEL_FORMAT_RGB888 exactly.
 */
int bk_pixel_rgb888_to_rgb565(uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height)
{
    if (src == NULL || dst == NULL || width == 0 || height == 0) {
        LOGE("%s, %d src or dst is NULL or width or height is 0\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    const uint64_t pixel_cnt = (uint64_t)width * (uint64_t)height;
    const uint64_t src_bytes_needed = pixel_cnt * 3ULL;

    /* Prevent 32-bit index overflow when computing i*3. */
    if (src_bytes_needed > (uint64_t)UINT32_MAX) {
        LOGE("%s, %d invalid size: width=%u height=%u\r\n",
             __func__, __LINE__, (unsigned)width, (unsigned)height);
        return BK_FAIL;
    }

    for (uint32_t i = 0; i < (uint32_t)pixel_cnt; ++i) {
        const uint32_t base = i * 3U;
        const uint8_t r8 = src[base + 0U];
        const uint8_t g8 = src[base + 1U];
        const uint8_t b8 = src[base + 2U];

        const uint16_t r5 = (uint16_t)(r8 >> 3);
        const uint16_t g6 = (uint16_t)(g8 >> 2);
        const uint16_t b5 = (uint16_t)(b8 >> 3);

        dst[i] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }

    return BK_OK;
}