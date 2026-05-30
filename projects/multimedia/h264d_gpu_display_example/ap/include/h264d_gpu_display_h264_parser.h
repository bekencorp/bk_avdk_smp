#pragma once

#include <stdint.h>
#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t offset;
	uint32_t size;
} h264d_gpu_display_h264_frame_t;

/* Return one decodable H.264 access unit from stream and advance offset. */
avdk_err_t h264d_gpu_display_h264_next_frame(const uint8_t *stream,
					     uint32_t stream_size,
					     uint32_t *offset,
					     const uint8_t **frame_ptr,
					     uint32_t *frame_size);
avdk_err_t h264d_gpu_display_h264_build_frame_table(const uint8_t *stream,
						    uint32_t stream_size,
						    h264d_gpu_display_h264_frame_t **table,
						    uint32_t *count);
void h264d_gpu_display_h264_frame_table_free(h264d_gpu_display_h264_frame_t *table);

#ifdef __cplusplus
}
#endif
