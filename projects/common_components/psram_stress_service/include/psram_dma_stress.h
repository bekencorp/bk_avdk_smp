#pragma once

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psram_dma_stress *psram_dma_stress_handle_t;

bk_err_t psram_dma_stress_create(psram_dma_stress_handle_t *out_handle);

bk_err_t psram_dma_stress_start(psram_dma_stress_handle_t handle,
                                uint8_t *src_addr, uint8_t *dst_addr, uint32_t size);

bk_err_t psram_dma_stress_stop(psram_dma_stress_handle_t handle);

bk_err_t psram_dma_stress_deinit(psram_dma_stress_handle_t handle);

#ifdef __cplusplus
}
#endif

