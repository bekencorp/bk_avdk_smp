#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Unified vcdec decoder ISR entry.
 *
 * Registered against the H264D interrupt; internally dispatches to the active
 * vcdec instance (JPEG or H.264) via the common active-instance pointer.
 */
void vcdec_isr(void);

/**
 * @brief Unified vcdec post-processor ISR entry.
 *
 * Registered against the H264D_PP interrupt; handles the PP ring buffer
 * pointer/cb update for the active vcdec instance (JPEG or H.264).
 */
void vcdec_pp_isr(void);

#ifdef __cplusplus
}
#endif
