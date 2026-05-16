/**
 * @file lv_gpu_rotate.h
 *
 */

#pragma once

#include "lv_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_gpu_rotate_init(lv_vnd_data_t *vnd_data);
void lv_gpu_rotate_deinit(lv_vnd_data_t *vnd_data);
void lv_gpu_rotate_process(lv_vnd_data_t *vnd_data, uint8_t *src_buf, lv_coord_t src_width, lv_coord_t src_height);

#ifdef __cplusplus
} /*extern "C"*/
#endif
