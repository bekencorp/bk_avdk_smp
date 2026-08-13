/**
 * @file bk_baf_internal.h
 *
 * bk_baf COMPONENT-INTERNAL concrete type definitions, completing the opaque
 * types declared in the public headers: the decoder backend vtable
 * (bk_baf_decoder_ops_t) and the decoder handle (bk_baf_decoder_t). Owned by the
 * open adapter (bk_baf_adapter.c); NOT a user API -- it shares the component's
 * include/ dir with the public headers but must not be included by users. The
 * closed core provides one vtable implementation via the baf_decoder_* exports
 * (see modules/baf_decoder.h).
 */

#ifndef BK_BAF_INTERNAL_H
#define BK_BAF_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_baf_types.h"   /* bk_baf_decoder_result_t + stdint */

/* Decoder backend vtable layout.
 * required: open, close, poll_frame, get_width, get_height, get_frame_duration,
 *           get_canvas, get_canvas_size
 * optional (may be NULL): rewind, pause, resume, get_alpha_mask,
 *           get_loop_count, set_loop_count */
struct bk_baf_decoder_ops {
    void * (*open)(const void * data);
    void (*close)(void * context);
    bk_baf_decoder_result_t (*poll_frame)(void * context);
    void (*rewind)(void * context);
    void (*pause)(void * context);
    void (*resume)(void * context);
    uint16_t (*get_width)(const void * context);
    uint16_t (*get_height)(const void * context);
    uint32_t (*get_frame_duration)(const void * context);
    uint8_t * (*get_canvas)(void * context);
    size_t (*get_canvas_size)(const void * context);
    uint8_t * (*get_alpha_mask)(void * context);
    int32_t (*get_loop_count)(const void * context);
    void (*set_loop_count)(void * context, int32_t count);
};

/* Frame pacer: schedules display cadence from the per-frame durations over a
 * monotonic millisecond clock. Driven inside bk_baf_poll(); not part of
 * the public API. */
typedef struct {
    uint32_t next_due_ms;
    bool started;
    bool free_run;   /* ignore per-frame durations: always "due" (max speed) */
} bk_baf_pacer_t;

void bk_baf_pacer_reset(bk_baf_pacer_t * pacer);
void bk_baf_pacer_frame_shown(bk_baf_pacer_t * pacer, uint32_t now_ms, uint32_t duration_ms);
int32_t bk_baf_pacer_time_until_due(const bk_baf_pacer_t * pacer, uint32_t now_ms);

/* Decoder handle: a chosen backend vtable, its opaque per-instance context, and
 * the pacer that gates display cadence inside bk_baf_poll(). */
struct _bk_baf_decoder_t {
    const bk_baf_decoder_ops_t * ops;
    void * context;
    bk_baf_pacer_t pacer;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BK_BAF_INTERNAL_H */
