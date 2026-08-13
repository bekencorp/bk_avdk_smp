/**
 * @file bk_baf_pacer.c
 *
 * Component-internal BAF frame pacer (see bk_baf_internal.h). Pure state machine
 * over a caller-supplied millisecond clock; no LVGL, OS or GPU dependency. Driven
 * inside bk_baf_poll() to gate display cadence.
 */

#include "bk_baf_internal.h"

void bk_baf_pacer_reset(bk_baf_pacer_t * pacer)
{
    pacer->started = false;
}

void bk_baf_pacer_frame_shown(bk_baf_pacer_t * pacer, uint32_t now_ms, uint32_t duration_ms)
{
    if(!pacer->started) {
        pacer->started = true;
        pacer->next_due_ms = now_ms + duration_ms;
        return;
    }
    pacer->next_due_ms += duration_ms;
    /* More than a whole frame behind: re-baseline so we don't burst-catch-up. */
    if((int32_t)(now_ms - pacer->next_due_ms) > (int32_t)duration_ms) {
        pacer->next_due_ms = now_ms + duration_ms;
    }
}

int32_t bk_baf_pacer_time_until_due(const bk_baf_pacer_t * pacer, uint32_t now_ms)
{
    /* "Not yet started" and free-run both mean show now; a blocking caller then
     * sleeps 0. When paced again, frame_shown()'s catch-up clamp re-baselines. */
    if(!pacer->started || pacer->free_run) return 0;
    return (int32_t)(pacer->next_due_ms - now_ms);
}
