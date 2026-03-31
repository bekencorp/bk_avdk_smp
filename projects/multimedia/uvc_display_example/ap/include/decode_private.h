#pragma once

#include <avdk_error.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DECODE_BUFFER_CNT (2)
#define DECODE_FLEXA_LINES (16)
#define DECODE_FLEXA_ALIGN_SIZE (16)
#define DECODE_DUMP_FRAME_ENABLE (0)

typedef void (*decode_test_nv12_frame_cb_t)(void *arg, uint8_t *nv12, uint32_t w, uint32_t h);

typedef enum {
	DECODE_TEST_PORT_GPU = 0,
	DECODE_TEST_PORT_H264E = 1,
	DECODE_TEST_PORT_MAX,
} decode_test_port_t;

avdk_err_t decode_test_set_port_rd_cnt(decode_test_port_t port, uint32_t rd_cnt);
avdk_err_t decode_test_register_nv12_frame_callback(decode_test_nv12_frame_cb_t cb, void *arg);
avdk_err_t decode_test_release_nv12_frame_buffer(uint8_t buffer_index);
avdk_err_t decode_test_open(uint16_t width, uint16_t height, bk_image_format_t format,
			    uint8_t flexa_mode, uint8_t enable_nv12_output);
avdk_err_t decode_test_close(void);
avdk_err_t decode_test_get_decode_context(uint8_t **decode_buffer, uint8_t *ring_buffer_cnt);
avdk_err_t decode_test_register_isr_callback(void (*callback)(uint32_t wr_cnt, void *arg), void *arg);
avdk_err_t decode_test_set_wr_cnt(uint32_t wr_cnt);

#ifdef __cplusplus
}
#endif
