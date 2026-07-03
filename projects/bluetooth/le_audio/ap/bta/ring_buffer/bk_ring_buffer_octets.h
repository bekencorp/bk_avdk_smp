#pragma once

#include <stdint.h>

#define INT_CEIL(x, size) ((x) / (size) * (size) + ((x) % (size) ? (size) : 0))
#define INT_FLOOR(x, size) ((x) / (size) * (size))

#define INT_CEIL_4(x) ((((size_t)(x)) & (~(size_t)3)) + ((((size_t)(x)) & 0b11) ? 0b100: 0))
#define INT_CEIL_4(x) ((((size_t)(x)) & (~(size_t)3)) + ((((size_t)(x)) & 0b11) ? 0b100: 0))
#define INT_FLOOR_4(x) (((size_t)(x)) & (~(size_t)3))

typedef struct
{
    uint8_t *buffer;
    uint32_t len;
    uint32_t rp;
    uint32_t wp;
} bk_ring_buffer_octets_context_t;

int32_t bk_ring_buffer_octets_init(bk_ring_buffer_octets_context_t *ctx, uint8_t *buffer, uint32_t len);
int32_t bk_ring_buffer_octets_deinit(bk_ring_buffer_octets_context_t *ctx);
bool bk_ring_buffer_octets_is_init(bk_ring_buffer_octets_context_t *ctx);
int32_t bk_ring_buffer_octets_reset(bk_ring_buffer_octets_context_t *ctx);

int32_t bk_ring_buffer_octets_write(bk_ring_buffer_octets_context_t *ctx, uint8_t *data, uint32_t len);
int bk_ring_buffer_octets_read(bk_ring_buffer_octets_context_t *ctx, uint8_t *buff, uint32_t len);
uint32_t bk_ring_buffer_octets_len(bk_ring_buffer_octets_context_t *ctx);

void bk_ring_buffer_octets_debug(bk_ring_buffer_octets_context_t *ctx);
int32_t bk_ring_buffer_octets_test(void);
