/**
 * @file lv_port_disp.h
 *
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "lv_vendor.h"

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void bk_lv_port_disp_init(lv_vnd_data_t *vnd_data);

void lv_port_disp_deinit(lv_vnd_data_t *vnd_data);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif
