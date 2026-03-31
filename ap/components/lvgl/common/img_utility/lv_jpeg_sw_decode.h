#ifndef __LV_JPEG_SW_DECODE_H_
#define __LV_JPEG_SW_DECODE_H_

#include "lvgl.h"
#include "components/media_types.h"

bk_err_t lv_jpeg_sw_decode_init(void);

bk_err_t lv_jpeg_sw_decode_deinit(void);

bk_err_t lv_jpeg_sw_decode_start(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst, bool byte_swap);

#endif
