#pragma once

#include <stdint.h>
#include <components/avdk_utils/avdk_error.h>
#include "modules/vcdec/vcdec_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t offset;
	uint32_t size;
} h264_decode_h264_frame_t;

/* Return one Annex-B H.264 access unit from stream and advance offset. */
avdk_err_t h264_decode_h264_next_au(const uint8_t *stream,
				    uint32_t stream_size,
				    uint32_t *offset,
				    const uint8_t **au_ptr,
				    uint32_t *au_size);

vcdec_h264_frame_type_t h264_decode_h264_peek_au_frame_type(const uint8_t *au_ptr,
							    uint32_t au_size);

avdk_err_t h264_decode_h264_build_frame_table(const uint8_t *stream,
					      uint32_t stream_size,
					      h264_decode_h264_frame_t **table,
					      uint32_t *count);

void h264_decode_h264_frame_table_free(h264_decode_h264_frame_t *table);

#ifdef __cplusplus
}
#endif
