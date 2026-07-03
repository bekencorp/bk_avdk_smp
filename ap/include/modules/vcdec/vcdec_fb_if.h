#pragma once

/**
 * @file vcdec_fb_if.h
 * @brief Abstract frame-buffer interface between the vcdec H.264 decoder
 *        (precompiled INTERNAL_LIB) and an external, runtime-injected frame
 *        pool implementation.
 *
 * The decoder library only ever sees this pure header (a vtable of function
 * pointers + the internal frame descriptor). The concrete pool lives outside
 * the vendor library (see ap/components/media_service/h264d_fbpool) and is
 * injected at runtime via vcdec_h264_register_fb_if(), so the decoder has zero
 * compile-time dependency on the pool. This is the buffer-layer decoupling
 * mandated by docs/vcdec_h264_fbpool_zerocopy_design.md.
 *
 * A single frame buffer plays three roles -- decode target / reference store /
 * display output -- coordinated by a reference count, hence "zero copy".
 *
 * Memory layout of one slot (4:2:0 progressive, matches native h264d):
 *
 *   +------------------ one frame buffer ------------------+
 *   |  Y (w*h)  |  UV (w*h/2)  |  colocated MV  |  sync32  |
 *   +-----------+--------------+----------------+----------+
 *   ^ y_bus (DEC_OUT_BASE / REFERx_BASE)        ^ mv_bus (DIR_MV_BASE)
 *
 * mv region offset == yuv_size (== picSizeInMbs*384 for 4:2:0), so direct-mode
 * colocated MVs of each reference live inside that reference's own buffer and
 * stay alive exactly as long as the reference does.
 */

#include <stdint.h>
#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Pixel format of a decoded frame buffer. */
typedef enum {
	VCDEC_PIX_NV12  = 0,    /* 4:2:0 semi-planar: Y plane then interleaved UV */
	VCDEC_PIX_GRAY8 = 1,    /* monochrome: Y plane only (chroma_format_idc==0) */
} vcdec_pix_fmt_e;

/** Slot lifecycle state. */
typedef enum {
	VCDEC_FB_FREE = 0,
	VCDEC_FB_DECODING,
	VCDEC_FB_READY,
	VCDEC_FB_INUSE,
} vcdec_fb_state_e;

/**
 * @brief Internal frame descriptor -- visible to the decoder and the pool only.
 *
 * Carries both the physical buffer info (pointers/bus addresses/sizes) and the
 * DPB metadata the decoder needs to manage references and display order. The
 * application never sees this type; it only sees the display-view subset.
 */
typedef struct vcdec_fb {
	/* physical buffer (filled by the pool at reconfigure) */
	uint8_t  *y_ptr;        /* pixel plane start (CPU addr) */
	uint32_t  y_bus;        /* pixel plane bus addr (DEC_OUT_BASE / REFERx_BASE) */
	uint8_t  *mv_ptr;       /* colocated/direct MV region start, NULL if mv_size==0 */
	uint32_t  mv_bus;       /* MV region bus addr (DIR_MV_BASE) */
	uint32_t  mv_size;      /* MV region size in bytes */
	uint32_t  capacity;     /* physical YUV capacity of this slot (>= data_len) */

	/* picture geometry / format (filled by the decoder per frame) */
	uint16_t  width;        /* 16-aligned coded width (== row pitch) */
	uint16_t  height;       /* 16-aligned coded height */
	uint32_t  data_len;     /* valid pixel bytes: NV12=w*h*3/2, GRAY8=w*h */
	uint8_t   format;       /* vcdec_pix_fmt_e */
	uint8_t   frame_type;   /* vcdec_h264_frame_type_t */

	/* DPB / reorder metadata (owned by the decoder) */
	int32_t   poc;          /* picture order count (display order key) */
	int32_t   poc_field[2]; /* top/bottom field POC (frame: both == poc) */
	uint16_t  frame_num;
	int32_t   pic_num;      /* short-term picNum / long-term longTermPicNum */
	int32_t   long_term_frame_idx;
	uint8_t   is_reference; /* currently a DPB reference */
	uint8_t   is_long_term; /* long-term reference */

	/* pool bookkeeping */
	int16_t   refcount;     /* DPB hold + display/app hold */
	uint8_t   index;        /* slot index, also used as HW pic id */
	uint8_t   state;        /* vcdec_fb_state_e */
	uint8_t   in_disp_q;    /* already queued for display (publish guard) */
} vcdec_fb_t;

/**
 * @brief Decode-side frame-pool interface (vtable). Implemented by the pool,
 *        injected into the decoder. The application must NOT call these.
 */
typedef struct vcdec_fb_if {
	void *ctx;              /* pool instance, passed back to every callback */

	/*
	 * (Re)allocate physical slots after SPS / geometry is known. Reports only
	 * codec-side needs: dpb_count = max_num_ref_frames + 1 (decode target),
	 * per-slot yuv_size and mv_size (mv_size==0 => no MV region, pure I/P).
	 * The pool adds its own display depth on top. Idempotent when geometry is
	 * unchanged. Returns VCDEC_OK on success.
	 */
	vcdec_ret_e (*reconfigure)(void *ctx, uint16_t dpb_count,
	                           uint32_t yuv_size, uint32_t mv_size);
	vcdec_fb_t *(*acquire)(void *ctx);                 /* free slot as decode target, refcount=1 */
	void        (*ref)(void *ctx, vcdec_fb_t *fb);     /* DPB hold +1 */
	void        (*unref)(void *ctx, vcdec_fb_t *fb);   /* hold -1, 0 => back to FREE */
	void        (*publish)(void *ctx, vcdec_fb_t *fb); /* enqueue for display (display order) */
	void        (*flush)(void *ctx);                   /* drop all DPB/display holds (IDR/reset) */
} vcdec_fb_if_t;

#ifdef __cplusplus
}
#endif
