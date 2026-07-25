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

zbar_qr_context_t *zbar_qr_create(void);
void zbar_qr_destroy(zbar_qr_context_t *context);

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
