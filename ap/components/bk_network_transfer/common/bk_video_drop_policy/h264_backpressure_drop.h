#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct frame_buffer_t frame_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

#define NTWK_H264_DROP_MIN_AVAILABLE_BUFFER_COUNT   (5)
#define NTWK_H264_DROP_START_AVAILABLE_BUFFER_COUNT (2)
#define NTWK_H264_DROP_STOP_AVAILABLE_BUFFER_MARGIN (4)

void ntwk_h264_backpressure_drop_init(uint8_t max_available_buffer_count);
void ntwk_h264_backpressure_drop_reset(void);
bool ntwk_h264_backpressure_drop_is_h264_frame(const frame_buffer_t *frame);
bool ntwk_h264_backpressure_drop_check(uint8_t available_buffer_count, const frame_buffer_t *frame);
bool ntwk_h264_backpressure_drop_on_recycle(uint8_t available_buffer_count, const frame_buffer_t *frame);
bool ntwk_h264_backpressure_drop_consume_force_idr(void);

#ifdef __cplusplus
}
#endif
