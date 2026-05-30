/**
 * @file lv_hpdma.h
 *
 */

#pragma once

#include "lv_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_hpdma_memcpy_init(lv_vnd_data_t *vnd_data);
void lv_hpdma_memcpy_deinit(lv_vnd_data_t *vnd_data);
bk_err_t lv_hpdma_memcpy_start(void *src_buf, void *dst_buf, uint16_t src_xsize, uint16_t src_ysize,
                               uint16_t dst_xsize, uint16_t dst_ysize, uint16_t src_step, uint16_t dst_step);
bk_err_t lv_hpdma_memcpy_wait_finish(uint32_t timeout_ms);
bk_err_t lv_hpdma_memcpy_stop(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif
