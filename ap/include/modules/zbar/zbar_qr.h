/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef MODULES_ZBAR_ZBAR_QR_H
#define MODULES_ZBAR_ZBAR_QR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zbar_qr_context zbar_qr_context_t;

typedef struct {
    int x;
    int y;
} zbar_qr_point_t;

typedef struct {
    uint8_t *payload;
    size_t payload_capacity;
    size_t payload_len;
    zbar_qr_point_t corners[4];
} zbar_qr_result_t;

enum {
    ZBAR_QR_ERROR_INVALID_ARGUMENT = -1,
    ZBAR_QR_ERROR_NO_MEMORY = -2,
    ZBAR_QR_ERROR_OUTPUT_TOO_SMALL = -3,
    ZBAR_QR_ERROR_SCAN_FAILED = -4
};

/*
 * Backing heap for the per-frame binarization buffer (width*height bytes).
 * Defaults to SRAM (fastest); the other heaps are opt-in.
 */
typedef enum {
    ZBAR_QR_MEM_TYPE_SRAM = 0,      /* Internal SRAM heap (default) */
    ZBAR_QR_MEM_TYPE_PSRAM,         /* psram_malloc() PSRAM heap */
    ZBAR_QR_MEM_TYPE_PSRAM_UNCODED, /* Uncoded PSRAM slab: MEM_SLAB_HEAP_UNCODED */
    ZBAR_QR_MEM_TYPE_PSRAM_CODED,   /* Coded PSRAM slab: MEM_SLAB_HEAP_CODED */
} zbar_qr_mem_type_t;

zbar_qr_context_t *zbar_qr_create(void);
void zbar_qr_destroy(zbar_qr_context_t *context);

/*
 * Select the backing heap for zbar's internal binarization buffer. The setting
 * is module-wide and takes effect on the next scan; call it before scanning.
 * Defaults to ZBAR_QR_MEM_TYPE_SRAM, so existing callers keep the fastest path.
 */
void zbar_qr_set_mem_type(zbar_qr_mem_type_t mem_type);

/*
 * Scan an 8-bit grayscale image. The input remains owned by the caller and is
 * only used for the duration of this call. Payload bytes are copied to each
 * caller-provided result buffer. Returns the result count or a negative error.
 */
int zbar_qr_scan(zbar_qr_context_t *context,
                 const uint8_t *gray,
                 size_t width,
                 size_t height,
                 size_t stride,
                 zbar_qr_result_t *results,
                 size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
