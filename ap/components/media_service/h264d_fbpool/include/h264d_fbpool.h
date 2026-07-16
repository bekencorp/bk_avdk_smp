#pragma once

/**
 * @file h264d_fbpool.h
 * @brief Zero-copy frame pool for the vcdec H.264 decoder (frame mode).
 *
 * Independent SDK component (lives outside the precompiled vcdec vendor lib).
 * Implements the decode-side vtable vcdec_fb_if_t (declared in the library-
 * internal header modules/vcdec/vcdec_fb_if.h) and injects it into the decoder
 * via vcdec_h264_register_fb_if(). One physical buffer simultaneously serves as
 * decode target / DPB reference / display output, coordinated by a reference
 * count, so reference and output copies are eliminated.
 *
 * Design: docs/vcdec_h264_fbpool_zerocopy_design.md
 *
 * Threading: the decoder thread calls the vtable (acquire/ref/unref/publish/
 * flush); the application thread calls dequeue/release. Both are serialized by
 * an internal mutex; dequeue blocks on a display semaphore.
 */

#include <stdint.h>
#include <components/avdk_utils/avdk_error.h>
#include "modules/vcdec/vcdec_fb_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque pool instance. */
typedef struct h264d_fbpool h264d_fbpool_t;

/**
 * @brief Application-facing display view of one decoded frame.
 *
 * The app only ever sees this subset (plus the opaque return token). Frames are
 * delivered by the pool already in display (POC) order, so sequential dequeue
 * yields correct playback order with no app-side reordering.
 */
typedef struct {
	uint8_t  *data;        /* pixel data first address (render/copy) */
	uint32_t  data_len;    /* valid pixel bytes (NV12=w*h*3/2, GRAY8=w*h) */
	uint32_t  capacity;    /* physical slot capacity (>= data_len) */
	uint16_t  width;       /* 16-aligned coded width (== row pitch) */
	uint16_t  height;      /* 16-aligned coded height */
	uint8_t   format;      /* vcdec_pix_fmt_e */
	uint8_t   frame_type;  /* vcdec_h264_frame_type_t */
	int32_t   poc;         /* picture order count (exposed for validation) */
	void     *token;       /* opaque return token (= internal fb), do not deref */
} vcdec_frame_t;

/** DMA-able memory allocator injected by the application (NULL => internal). */
typedef void *(*h264d_fbpool_alloc_cb)(uint32_t size, uint32_t align);
typedef void  (*h264d_fbpool_free_cb)(void *ptr);

/**
 * @brief Create an empty frame pool.
 *
 * Allocates the pool object and its synchronization primitives (mutex +
 * display semaphore) but NOT the per-frame physical buffers. The physical
 * slots are allocated lazily on the first reconfigure() call from the decoder
 * once the picture geometry (SPS) is known. After create, export the vtable
 * with h264d_fbpool_get_if() and inject it via vcdec_h264_register_fb_if().
 *
 * @param[out] pool       Receives the created pool handle. Must not be NULL.
 * @param[in]  align      Physical slot start-address alignment in bytes; must
 *                        be a power of two. 0 (or a non-power-of-two) selects
 *                        the default 64.
 * @param[in]  disp_depth Number of extra slots reserved on top of the codec
 *                        DPB so the application can hold decoded frames for
 *                        display concurrently. Range 1..4 (FBPOOL_DISP_DEPTH_MAX).
 * @param[in]  alloc_cb   DMA-able buffer allocator. NULL selects the internal
 *                        os_malloc fallback (NOTE: not guaranteed DMA-able).
 * @param[in]  free_cb    Matching free for @p alloc_cb. NULL selects os_free.
 *                        @p alloc_cb and @p free_cb must both be NULL or both
 *                        be non-NULL.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL if @p pool is NULL, @p disp_depth is out of range, or
 *         only one of @p alloc_cb / @p free_cb is NULL;
 *         AVDK_ERR_NOMEM if the pool object, mutex or semaphore allocation
 *         fails.
 */
avdk_err_t h264d_fbpool_create(h264d_fbpool_t **pool, uint32_t align,
                               uint16_t disp_depth,
                               h264d_fbpool_alloc_cb alloc_cb,
                               h264d_fbpool_free_cb free_cb);

/**
 * @brief Destroy the pool, freeing all physical slot buffers and the mutex /
 *        semaphore.
 *
 * The caller must ensure the decoder is no longer using the pool (deregistered
 * / deinitialized) and that no application thread is blocked in dequeue.
 *
 * @param[in] pool Pool handle from h264d_fbpool_create(). NULL is a no-op.
 *
 * @return None.
 */
void h264d_fbpool_destroy(h264d_fbpool_t *pool);

/**
 * @brief Export the decode-side interface (vtable) of the pool.
 *
 * Fills @p out with the function pointers (acquire/ref/unref/publish/flush/
 * reconfigure) and sets out->ctx to the pool instance. Call once after create
 * and pass @p out to vcdec_h264_register_fb_if(). The decoder owns the calls
 * in this vtable; the application must not invoke them directly.
 *
 * @param[in]  pool Pool handle from h264d_fbpool_create(). Must not be NULL.
 * @param[out] out  Receives the decode-side vtable. Must not be NULL.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL if @p pool or @p out is NULL.
 */
avdk_err_t h264d_fbpool_get_if(h264d_fbpool_t *pool, vcdec_fb_if_t *out);

/**
 * @brief Set the decode-target acquire wait budget (milliseconds).
 *
 * By default acquire() is non-blocking: when no slot is free it fails at once
 * (returns NULL) and the caller must drop the frame. With a non-zero budget the
 * pool instead waits up to @p timeout_ms for a slot to be released by the
 * application (dequeue/release runs on a different thread) before giving up.
 * This trades frame rate for a bounded peak memory footprint (fewer display
 * slots) without hard decode failures. When a slot is already free acquire()
 * still returns immediately, so a non-zero budget is behavior-neutral under no
 * contention.
 *
 * @param[in] pool       Pool handle from h264d_fbpool_create(). NULL is a no-op.
 * @param[in] timeout_ms Max wait per acquire in ms; 0 restores non-blocking.
 *
 * @return None.
 */
void h264d_fbpool_set_acquire_timeout(h264d_fbpool_t *pool, uint32_t timeout_ms);

/**
 * @brief Application: dequeue the next decoded frame in display (POC) order.
 *
 * Blocks on the display semaphore until a frame is available or @p timeout_ms
 * elapses. The returned frame keeps a hold on its slot until it is returned
 * with h264d_fbpool_release(); the buffer is safe to render/copy until then.
 *
 * @param[in]  pool       Pool handle from h264d_fbpool_create(). Must not be NULL.
 * @param[out] out        Receives the frame's display view (pixels, geometry,
 *                        format, POC and the opaque return token). Must not be NULL.
 * @param[in]  timeout_ms Max wait in milliseconds; 0 performs a non-blocking poll.
 *
 * @return AVDK_ERR_OK and fills @p out on success;
 *         AVDK_ERR_TIMEOUT if no frame became available within @p timeout_ms;
 *         AVDK_ERR_INVAL if @p pool or @p out is NULL.
 */
avdk_err_t h264d_fbpool_dequeue(h264d_fbpool_t *pool, vcdec_frame_t *out,
                                uint32_t timeout_ms);

/**
 * @brief Application: return a previously dequeued frame to the pool.
 *
 * Drops the application's display hold on the slot. When no decoder (DPB)
 * reference remains either, the slot returns to the FREE list for reuse.
 * Each frame obtained from h264d_fbpool_dequeue() must be released exactly once.
 *
 * @param[in] pool  Pool handle from h264d_fbpool_create(). Must not be NULL.
 * @param[in] frame Frame previously returned by h264d_fbpool_dequeue(); its
 *                  @c token must be valid. Must not be NULL.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL if @p pool, @p frame or frame->token is NULL.
 */
avdk_err_t h264d_fbpool_release(h264d_fbpool_t *pool, const vcdec_frame_t *frame);

/**
 * @brief Diagnostics: number of slots currently on the FREE list.
 *
 * Mainly used to detect leaks (after a full drain the free count should equal
 * the total allocated slot count).
 *
 * @param[in] pool Pool handle from h264d_fbpool_create().
 *
 * @return Count of currently free slots; 0 if @p pool is NULL.
 */
uint16_t h264d_fbpool_free_count(h264d_fbpool_t *pool);

#ifdef __cplusplus
}
#endif
