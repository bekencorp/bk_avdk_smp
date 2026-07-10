#pragma once

#include "vcdec_types.h"

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

/**
 * @brief Register memory allocator for a vcdec decoder instance.
 *
 * Registers malloc/free callbacks used by JPEG and H.264 internal buffer
 * allocation. Must be called after _init() and before _open().
 *
 * @param handle decoder handle returned by vcdec_jpeg_init / vcdec_h264_init
 * @param pmalloc memory allocation callback
 * @param pfree memory free callback
 *
 * @return VCDEC_OK for success, others for failure
 */
vcdec_ret_e vcdec_register_memalloc(vcdec_handle handle, void* (*pmalloc)(uint32_t), void (*pfree)(void*));

#ifdef __cplusplus
}
#endif
