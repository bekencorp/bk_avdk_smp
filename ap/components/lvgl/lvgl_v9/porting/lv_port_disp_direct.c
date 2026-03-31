/**
 * @file lv_port_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp.h"
#include <modules/image_scale.h>
#include "lv_vendor.h"
#include <driver/lcd_types.h>
#include "../src/misc/lv_area_private.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>

#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

//TODO