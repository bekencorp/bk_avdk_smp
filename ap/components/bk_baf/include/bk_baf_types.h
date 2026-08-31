/**
 * @file bk_baf_types.h
 *
 * bk_baf public data model: the animation-asset description (media/stream/AU),
 * the decode result code, a per-frame pixel descriptor, and the asset source
 * struct. These types are usually produced by the asset generator (to_baf.py)
 * and consumed read-only by the application. No platform/LVGL/vg_lite deps.
 */

#ifndef BK_BAF_TYPES_H
#define BK_BAF_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Marks a valid bk_baf_source_t ("BAF"). */
#define BK_BAF_SOURCE_MAGIC 0x424B4146UL

/* ---- Decode result ---- */

typedef enum {
    BK_BAF_DECODER_RESULT_ALPHA_ERROR = -3,
    BK_BAF_DECODER_RESULT_RGB_ERROR   = -2,
    BK_BAF_DECODER_RESULT_ERROR       = -1,
    BK_BAF_DECODER_RESULT_END         = 0,
    BK_BAF_DECODER_RESULT_FRAME       = 1,
    BK_BAF_DECODER_RESULT_WAIT        = 2,
} bk_baf_decoder_result_t;

/* True for any error result (all negative values are errors). */
static inline bool bk_baf_result_is_error(bk_baf_decoder_result_t result)
{
    return result < BK_BAF_DECODER_RESULT_END;
}

/* ---- Playback control commands (for bk_baf_ioctl) ---- */

typedef enum {
    BK_BAF_IOCTL_REWIND,       /* restart from the first frame;      arg: NULL */
    BK_BAF_IOCTL_PAUSE,        /* pause decoding;                    arg: NULL */
    BK_BAF_IOCTL_RESUME,       /* resume decoding;                   arg: NULL */
    /* Free-run (max-speed): poll() ignores the source's per-frame durations and
     * returns a new FRAME as fast as the pipeline produces one. Off by default;
     * persists across rewind/resume.                    arg: const bool * */
    BK_BAF_IOCTL_SET_FREERUN,
} bk_baf_ioctl_cmd_t;

/* ---- Render backend ----
 * GPU (VG-Lite) or CPU (Helium) compositor, selected via
 * bk_baf_config_t.backend (bk_baf_open). Default GPU; the CPU path needs no GPU. */
typedef enum {
    BK_BAF_RENDER_GPU = 0,
    BK_BAF_RENDER_CPU,
} bk_baf_render_backend_t;

/* ---- Asset format (compiled-in animation description) ---- */

typedef struct {
    uint32_t offset;
    uint32_t size;
} bk_baf_au_t;

typedef struct {
    const uint8_t * data;
    size_t data_size;
    const bk_baf_au_t * aus;
    uint32_t au_count;
} bk_baf_stream_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    /* Alpha resolution; 0 means "same as width / height". */
    uint16_t alpha_width;
    uint16_t alpha_height;
    uint32_t frame_count;
    bk_baf_stream_t rgb;
    bk_baf_stream_t alpha;
    const uint32_t * durations_ms;
} bk_baf_media_t;

/* ---- Per-frame pixel descriptor ---- */

typedef enum {
    BK_BAF_PIXEL_XRGB8888 = 0,  /* decoded canvas: BGRA byte order, X ignored */
    BK_BAF_PIXEL_ARGB8888,      /* composed straight-alpha frame */
    BK_BAF_PIXEL_A8,            /* alpha mask (one byte per pixel) */
} bk_baf_pixel_format_t;

typedef struct {
    void * data;
    bk_baf_pixel_format_t format;
    uint16_t width;
    uint16_t height;
    uint32_t stride;            /* bytes per row */
} bk_baf_frame_desc_t;

/* ---- Backend binding ----
 * The decoder backend vtable is OPAQUE to users: an asset only references the
 * built-in backend instance by address. The concrete layout is defined in the
 * component-internal header (bk_baf_internal.h) and must not be relied upon here. */
typedef struct bk_baf_decoder_ops bk_baf_decoder_ops_t;

/* Built-in decoder backend, provided by the bk_baf adapter. Reference it from a
 * source's .ops field (see the generated asset). */
extern const bk_baf_decoder_ops_t bk_baf_decoder_ops;

typedef struct {
    uint32_t magic;                        /* BK_BAF_SOURCE_MAGIC */
    const bk_baf_decoder_ops_t * ops;      /* &bk_baf_decoder_ops */
    const void * data;                     /* const bk_baf_media_t * */
} bk_baf_source_t;

/* ---- One-time hardware setup (for bk_baf_init) ----
 * Render backend + GPU lifecycle. Set once and kept across many open/close
 * cycles, so switching sources never re-inits the GPU. */
typedef struct {
    bk_baf_render_backend_t backend;     /* GPU (VG-Lite, default) or CPU (Helium) */
    bool     init_gpu;                   /* true  = bk_baf brings the GPU up in
                                          *         bk_baf_init() and tears it down in
                                          *         bk_baf_deinit() (RAW/standalone:
                                          *         creator destroys);
                                          * false = the GPU is already owned elsewhere
                                          *         (LVGL/flexa); bk_baf won't touch its
                                          *         lifetime. Ignored for the CPU backend. */
    void *   gpu_handle;                 /* shared bk_gpu_ctlr handle for compose
                                          * serialisation; NULL = no shared lock */
} bk_baf_hw_config_t;

/* ---- Per-source playback setup (for bk_baf_open) ----
 * Zero-initialise, then set EITHER .source (a pre-parsed asset) OR .data+.data_len
 * (a BAF v1 container image, parsed internally). Hardware must already be up via
 * bk_baf_init(). */
typedef struct {
    const bk_baf_source_t * source;      /* option 1: a pre-parsed asset to play */
    const uint8_t *         data;        /* option 2: a BAF v1 container image (loaded .baf
                                          * bytes or a compiled-in C array). Parsed in place;
                                          * the decoder owns the parse and ALIASES these
                                          * bytes -- keep them alive until bk_baf_close(). */
    uint32_t                data_len;    /* byte length of .data (used only when .source == NULL) */
    int32_t  loop_count;                 /* 0 = infinite, 1 = once, >1 = N times */
    bool     free_run;                   /* true = max-speed (ignore durations) */
} bk_baf_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* BK_BAF_TYPES_H */
