#pragma once

#include <avdk_error.h>
#include <common/bk_include.h>
#include <stdbool.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DECODE_BUFFER_CNT       (3)
#define DECODE_FLEXA_LINES      (16)
#define DECODE_FLEXA_ALIGN_SIZE (16)

avdk_err_t decode_test_open(uint16_t width, uint16_t height, bk_image_format_t format, uint8_t flexa_mode);
avdk_err_t decode_test_close(void);
avdk_err_t decode_test_get_flexa_context(uint8_t **decode_buffer, uint8_t *ring_buffer_cnt);
avdk_err_t decode_test_get_handle(bk_jpeg_decode_ctlr_handle_t *handle);
bool decode_test_is_open(void);

#ifdef __cplusplus
}
#endif
