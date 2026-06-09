#pragma once

/**
 * @file vcenc_common.h
 * @brief Lightweight public header that exposes the unified VCENC ISR.
 *
 * The full VCENC public type set is split across vcenc_types.h /
 * vcenc_jpeg_types.h / vcenc_h264_types.h. This header is intentionally
 * minimal: it only exists to publish the single ISR entry that the
 * hardware-encoder controller registers against the H.26x interrupt.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Unified vcenc encoder ISR entry.
 *
 * Registered against the H.26x encoder interrupt; internally dispatches to
 * the active vcenc instance (JPEG or H.264) via the common active-instance
 * pointer.
 */
void vcenc_isr(void);

#ifdef __cplusplus
}
#endif
