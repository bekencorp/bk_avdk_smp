#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a BGRA8888 frame to RGB565.
 *
 * Source byte order in memory (per BK_PIXEL_FORMAT_BGRA8888):
 *     +0=B  +1=G  +2=R  +3=A
 *
 * Destination is little-endian RGB565: bits [15:11]=R5, [10:5]=G6, [4:0]=B5.
 *
 * NOTE: BGRA8888 is what the BK display/GPU pipeline normally produces; it
 * is NOT the same byte layout as BK_PIXEL_FORMAT_ARGB8888 (+0=A +1=R +2=G
 * +3=B). Always check the upstream pixel format before calling.
 *
 * @param src     Source frame, 4 bytes per pixel.
 * @param dst     Destination buffer, 2 bytes per pixel.
 * @param width   Image width  in pixels.
 * @param height  Image height in pixels.
 * @return BK_OK on success, BK_FAIL on bad arguments.
 */
int bk_pixel_bgra8888_to_rgb565(uint32_t *src, uint16_t *dst, uint32_t width, uint32_t height);

/**
 * Convert a packed RGB888 frame to RGB565.
 *
 * Source byte order in memory (per BK_PIXEL_FORMAT_RGB888):
 *     +0=R  +1=G  +2=B
 *
 * Destination is little-endian RGB565: bits [15:11]=R5, [10:5]=G6, [4:0]=B5.
 *
 * @param src     Source frame, 3 bytes per pixel.
 * @param dst     Destination buffer, 2 bytes per pixel.
 * @param width   Image width  in pixels.
 * @param height  Image height in pixels.
 * @return BK_OK on success, BK_FAIL on bad arguments or size overflow.
 */
int bk_pixel_rgb888_to_rgb565(uint8_t *src, uint16_t *dst, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
