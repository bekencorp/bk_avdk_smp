#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 3DNR temporal denoise blend for one YUV420 (semi-planar) frame.
 *
 * In-place first-order IIR temporal blend:
 *     out = alpha/256 * cur + (256 - alpha)/256 * out_prev
 *
 * The history plane (dst_y/dst_uv) is both the previous output and the new
 * output. The current frame (src) is read-only. No memory is allocated and no
 * frame data is copied beyond the blend itself.
 *
 * Caller responsibilities (out of scope for this library):
 *   - Provide/manage the history buffer and seed it with the first frame.
 *   - Decide the fixed global alpha (0..256).
 *   - Perform any cache maintenance / DMA coherency around the buffers.
 *
 * @param dst_y   Y-plane history buffer (in/out), size = width * height bytes.
 * @param dst_uv  UV-plane history buffer (in/out), size = width * height / 2 bytes.
 * @param src     Current frame, YUV420 planar: [Y (w*h)][UV (w*h/2)].
 * @param width   Frame width in pixels.
 * @param height  Frame height in pixels.
 * @param alpha   Blend weight of the current frame in [1, 256].
 *                alpha == 256 copies src into dst (no temporal filtering);
 *                alpha == 0 is a no-op.
 *
 * @note The MVE kernel processes data in 64-byte blocks; any remainder
 *       (count % 64) is left untouched, matching the reference implementation.
 */
void bk_dnr_blend_planes(uint8_t *dst_y, uint8_t *dst_uv, const uint8_t *src,
                         uint32_t width, uint32_t height, uint32_t alpha);

/**
 * @brief Blend current NV12 frame with a separate history frame into dst.
 *
 * Unlike bk_dnr_blend_planes(), this API does not overwrite history, allowing
 * the previous filtered frame to be encoded while the next one is generated.
 */
void bk_dnr_blend_planes_to(uint8_t *dst_y, uint8_t *dst_uv,
			    const uint8_t *history, const uint8_t *src,
			    uint32_t width, uint32_t height, uint32_t alpha);

#ifdef __cplusplus
}
#endif
