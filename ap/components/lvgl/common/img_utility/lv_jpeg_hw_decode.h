#ifndef __LV_JPEG_HW_DECODE_H_
#define __LV_JPEG_HW_DECODE_H_

#include "lvgl.h"
#include <stdint.h>

bk_err_t lv_jpeg_hw_decode_start(uint8_t *jpeg_data, uint32_t jpeg_size, lv_img_dsc_t *img_dst, bool byte_swap);

#endif
