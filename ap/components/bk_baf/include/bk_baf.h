/**
 * @file bk_baf.h
 *
 * bk_baf public playback API. Getters/get_frame_desc are valid only after
 * poll() returned FRAME and must be called from the polling thread. Asset data
 * model: bk_baf_types.h.
 */

#ifndef BK_BAF_H
#define BK_BAF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_baf_types.h"
#include <avdk_error.h>          /* avdk_err_t */

typedef struct _bk_baf_decoder_t bk_baf_decoder_t;

/* ---- Lifecycle ----
 * Open a player from a config (see bk_baf_config_t): backend, optional GPU
 * bring-up + shared handle, source, loop/free-run. Returns NULL on failure.
 * bk_baf_close() closes it and tears down any GPU that bk_baf itself created. */
bk_baf_decoder_t * bk_baf_open(const bk_baf_config_t * cfg);
void bk_baf_close(bk_baf_decoder_t * decoder);

/* ---- Per-frame render loop: poll then compose ----
 * Non-blocking poll of the decode pipeline. Also paces playback to the source's
 * per-frame durations, so just call it in a loop and show every FRAME returned:
 *   FRAME - a new frame is ready (read it with bk_baf_get_frame_desc()),
 *   WAIT  - nothing ready yet, or the next frame isn't due (paced); retry soon,
 *   END   - playback finished (loop count exhausted),
 *   <0    - error (see bk_baf_result_is_error()). */
bk_baf_decoder_result_t bk_baf_poll(bk_baf_decoder_t * decoder);

/* Composite @p canvas (XRGB8888) modulated by @p alpha (A8; NULL = opaque) over
 * @p clear_argb into @p dst (ARGB8888): dst.RGB = canvas*a + clear*(1-a), dst.A = a.
 * clear_argb = 0 for a transparent frame (frontend blends), or opaque for direct
 * scanout. Runs on the backend chosen in bk_baf_open() (GPU = VG-Lite, CPU =
 * Helium); the GPU path serialises against bk_baf_config_t.gpu_handle. */
avdk_err_t bk_baf_compose(const bk_baf_frame_desc_t * dst,
                          const bk_baf_frame_desc_t * canvas,
                          const bk_baf_frame_desc_t * alpha,
                          uint32_t clear_argb);

/* ---- Playback control ----
 * Issue a control command (bk_baf_ioctl_cmd_t). @p arg per cmd, NULL if none.
 * Returns AVDK_ERR_OK / AVDK_ERR_INVAL (NULL decoder) / AVDK_ERR_UNSUPPORTED. */
avdk_err_t bk_baf_ioctl(bk_baf_decoder_t * decoder, bk_baf_ioctl_cmd_t cmd, void * arg);

/* ---- Queries / settings ---- */

/* Current frame's pixel descriptors: @p rgb (XRGB8888 canvas) and @p alpha (A8;
 * alpha->data == NULL if none). Either may be NULL. Valid after poll() == FRAME. */
void bk_baf_get_frame_desc(bk_baf_decoder_t * decoder,
                           bk_baf_frame_desc_t * rgb,
                           bk_baf_frame_desc_t * alpha);

/* Asset geometry (constant for the stream), e.g. to size buffers up front. */
uint16_t bk_baf_get_width(const bk_baf_decoder_t * decoder);
uint16_t bk_baf_get_height(const bk_baf_decoder_t * decoder);

/* Loop count: 0 = infinite, 1 = play once then END, >1 = play N times. */
int32_t bk_baf_get_loop_count(const bk_baf_decoder_t * decoder);
void bk_baf_set_loop_count(bk_baf_decoder_t * decoder, int32_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BK_BAF_H */
