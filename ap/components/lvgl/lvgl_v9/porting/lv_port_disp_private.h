/**
 * @file lv_port_disp_private.h
 *
 */

#pragma once

#include "lv_port_disp.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_port_disp_partial_init(lv_vnd_data_t *vnd_data);

void lv_port_disp_partial_deinit(lv_vnd_data_t *vnd_data);

void lv_disp_flush_for_partial_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map);

void lv_disp_flush_for_direct_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map);

void lv_disp_flush_for_full_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map);

#ifdef __cplusplus
} /*extern "C"*/
#endif
