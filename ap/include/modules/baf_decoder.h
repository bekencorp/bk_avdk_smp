/**
 * @file baf_decoder.h
 *
 * Closed BAF core (libbaf.a) decoder module interface -- the module's own header.
 * These are the baf_decoder_* decoder functions and the GPU compositor that the
 * open bk_baf adapter (bk_baf_adapter.c) binds into its bk_baf_decoder_ops
 * vtable. This is the module <-> adapter contract, not the end-user API (that is
 * the bk_baf component's bk_baf.h + bk_baf_types.h).
 *
 * Naming convention: closed-core symbols use the plain "baf_" prefix (no "bk_");
 * the "bk_baf_" prefix is reserved for the adapter's public API.
 */

#ifndef BAF_DECODER_H
#define BAF_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_baf_types.h"   /* bk_baf_decoder_result_t, bk_baf_frame_desc_t */
#include <avdk_error.h>                       /* avdk_err_t */

/* ---- Closed-core (libbaf.a) decoder backend exports ----
 * The adapter assembles these into a bk_baf_decoder_ops vtable instance. The
 * closed core provides all of them; open/close/poll_frame/get_width/get_height/
 * get_frame_duration/get_canvas/get_canvas_size are mandatory for the adapter. */
void * baf_decoder_open(const void * data);
void baf_decoder_close(void * context);
bk_baf_decoder_result_t baf_decoder_poll_frame(void * context);
void baf_decoder_rewind(void * context);
void baf_decoder_pause(void * context);
void baf_decoder_resume(void * context);
uint16_t baf_decoder_get_width(const void * context);
uint16_t baf_decoder_get_height(const void * context);
uint32_t baf_decoder_get_duration(const void * context);
uint8_t * baf_decoder_get_canvas(void * context);
size_t baf_decoder_get_canvas_size(const void * context);
uint8_t * baf_decoder_get_alpha_mask(void * context);
int32_t baf_decoder_get_loop_count(const void * context);
void baf_decoder_set_loop_count(void * context, int32_t count);

/* Closed-core compositors. Composite @p canvas (XRGB8888) modulated by @p alpha
 * (A8; NULL = fully opaque) into @p dst (ARGB8888). Two modes:
 *   @p is_new_layer = false : base -- clear @p dst to @p clear_argb, then write
 *                             dst.RGB = canvas.RGB*a + clear.RGB*(1-a), dst.A = a.
 *   @p is_new_layer = true  : stacked layer -- src-over onto @p dst's existing
 *                             pixels, dst.RGB = canvas.RGB*a + dst.RGB*(1-a)
 *                             (@p clear_argb ignored). Stacks layers front over back.
 * Two backends with identical output: _gpu_ uses VG-Lite, _cpu_ uses Helium/MVE.
 * The adapter re-exports these behind bk_baf_compose(..., is_new_layer).
 * Return AVDK_ERR_OK on success. */
avdk_err_t baf_gpu_compose_frame(const bk_baf_frame_desc_t * dst,
                                 const bk_baf_frame_desc_t * canvas,
                                 const bk_baf_frame_desc_t * alpha,
                                 uint32_t clear_argb,
                                 bool is_new_layer);
avdk_err_t baf_cpu_compose_frame(const bk_baf_frame_desc_t * dst,
                                 const bk_baf_frame_desc_t * canvas,
                                 const bk_baf_frame_desc_t * alpha,
                                 uint32_t clear_argb,
                                 bool is_new_layer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BAF_DECODER_H */
