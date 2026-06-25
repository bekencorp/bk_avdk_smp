#include <stdint.h>
#include "os/mem.h"
#include <components/log.h>

#include "h264_decode_h264_parser.h"

#define TAG "h264_parser"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

typedef struct {
	const uint8_t *data;
	uint32_t size;
	uint32_t byte_pos;
	uint8_t curr_byte;
	uint8_t bit_pos;
	uint8_t zero_count;
} h264_decode_bs_t;

static int h264_decode_h264_find_start_code(const uint8_t *buf, uint32_t len,
					    uint32_t offset, uint32_t *sc_off,
					    uint32_t *sc_len)
{
	uint32_t i;

	for (i = offset; i + 3U < len; i++) {
		if (buf[i] == 0U && buf[i + 1U] == 0U) {
			if (buf[i + 2U] == 0x01U) {
				*sc_off = i;
				*sc_len = 3U;
				return 0;
			}
			if (i + 4U < len && buf[i + 2U] == 0U && buf[i + 3U] == 0x01U) {
				*sc_off = i;
				*sc_len = 4U;
				return 0;
			}
		}
	}

	return -1;
}

static int h264_decode_h264_bs_next_byte(h264_decode_bs_t *bs, uint8_t *value)
{
	while (bs->byte_pos < bs->size) {
		uint8_t byte = bs->data[bs->byte_pos++];

		if (bs->zero_count == 2U && byte == 0x03U) {
			bs->zero_count = 0U;
			continue;
		}

		if (byte == 0U) {
			bs->zero_count++;
		} else {
			bs->zero_count = 0U;
		}

		*value = byte;
		return 0;
	}

	return -1;
}

static int h264_decode_h264_bs_read_bit(h264_decode_bs_t *bs, uint32_t *value)
{
	if (bs->bit_pos == 0U) {
		if (h264_decode_h264_bs_next_byte(bs, &bs->curr_byte) != 0) {
			return -1;
		}
		bs->bit_pos = 8U;
	}

	*value = (bs->curr_byte >> (bs->bit_pos - 1U)) & 0x1U;
	bs->bit_pos--;
	return 0;
}

static int h264_decode_h264_bs_read_ue(h264_decode_bs_t *bs, uint32_t *value)
{
	uint32_t zeros = 0U;
	uint32_t bit = 0U;
	uint32_t suffix = 0U;
	uint32_t i;

	while (1) {
		if (h264_decode_h264_bs_read_bit(bs, &bit) != 0) {
			return -1;
		}
		if (bit != 0U) {
			break;
		}
		zeros++;
		if (zeros > 31U) {
			return -1;
		}
	}

	for (i = 0U; i < zeros; i++) {
		if (h264_decode_h264_bs_read_bit(bs, &bit) != 0) {
			return -1;
		}
		suffix = (suffix << 1) | bit;
	}

	*value = ((1U << zeros) - 1U) + suffix;
	return 0;
}

static void h264_decode_h264_bs_init(h264_decode_bs_t *bs,
				     const uint8_t *nal_payload,
				     uint32_t nal_payload_size)
{
	bs->data = nal_payload + 1U;
	bs->size = nal_payload_size - 1U;
	bs->byte_pos = 0U;
	bs->curr_byte = 0U;
	bs->bit_pos = 0U;
	bs->zero_count = 0U;
}

static int h264_decode_h264_parse_first_mb(const uint8_t *nal_payload,
					   uint32_t nal_payload_size,
					   uint32_t *first_mb_in_slice)
{
	h264_decode_bs_t bs;

	if (nal_payload == NULL || nal_payload_size <= 1U || first_mb_in_slice == NULL) {
		return -1;
	}

	h264_decode_h264_bs_init(&bs, nal_payload, nal_payload_size);
	return h264_decode_h264_bs_read_ue(&bs, first_mb_in_slice);
}

static int h264_decode_h264_parse_slice_type(const uint8_t *nal_payload,
					     uint32_t nal_payload_size,
					     uint32_t *slice_type)
{
	h264_decode_bs_t bs;
	uint32_t first_mb;

	if (nal_payload == NULL || nal_payload_size <= 1U || slice_type == NULL) {
		return -1;
	}

	h264_decode_h264_bs_init(&bs, nal_payload, nal_payload_size);
	if (h264_decode_h264_bs_read_ue(&bs, &first_mb) != 0) {
		return -1;
	}

	return h264_decode_h264_bs_read_ue(&bs, slice_type);
}

static int h264_decode_h264_is_vcl(uint8_t nal_type)
{
	return (nal_type == 1U || nal_type == 2U || nal_type == 5U);
}

static int h264_decode_h264_is_boundary_nal(uint8_t nal_type)
{
	return (nal_type == 6U || nal_type == 7U || nal_type == 8U ||
		nal_type == 9U || nal_type == 10U || nal_type == 11U ||
		nal_type == 12U);
}

avdk_err_t h264_decode_h264_next_au(const uint8_t *stream,
				    uint32_t stream_size,
				    uint32_t *offset,
				    const uint8_t **au_ptr,
				    uint32_t *au_size)
{
	uint32_t au_start;
	uint32_t tmp_sc_len;
	uint32_t pos;
	uint8_t seen_vcl = 0U;

	if (stream == NULL || offset == NULL || au_ptr == NULL || au_size == NULL ||
	    stream_size == 0U) {
		return AVDK_ERR_INVAL;
	}
	if (*offset >= stream_size) {
		return AVDK_ERR_EOF;
	}

	if (h264_decode_h264_find_start_code(stream, stream_size, *offset, &au_start, &tmp_sc_len) != 0) {
		return AVDK_ERR_GENERIC;
	}
	(void)tmp_sc_len;

	pos = au_start;
	while (pos < stream_size) {
		uint32_t sc_off;
		uint32_t sc_len;
		uint32_t next_sc_off;
		uint32_t next_sc_len;
		uint32_t payload_off;
		uint8_t nal_type;
		uint8_t boundary = 0U;

		if (h264_decode_h264_find_start_code(stream, stream_size, pos, &sc_off, &sc_len) != 0) {
			break;
		}

		payload_off = sc_off + sc_len;
		if (payload_off >= stream_size) {
			break;
		}

		nal_type = stream[payload_off] & 0x1FU;
		if (h264_decode_h264_find_start_code(stream, stream_size, payload_off,
						     &next_sc_off, &next_sc_len) != 0) {
			next_sc_off = stream_size;
		}
		(void)next_sc_len;

		if (sc_off != au_start && seen_vcl) {
			if (h264_decode_h264_is_vcl(nal_type)) {
				uint32_t first_mb_in_slice = 0U;

				if (h264_decode_h264_parse_first_mb(&stream[payload_off],
								    next_sc_off - payload_off,
								    &first_mb_in_slice) != 0) {
					boundary = 1U;
				} else if (first_mb_in_slice == 0U) {
					boundary = 1U;
				}
			} else if (h264_decode_h264_is_boundary_nal(nal_type)) {
				boundary = 1U;
			}
		}

		if (boundary) {
			*au_ptr = &stream[au_start];
			*au_size = sc_off - au_start;
			*offset = sc_off;
			return AVDK_ERR_OK;
		}

		if (h264_decode_h264_is_vcl(nal_type)) {
			seen_vcl = 1U;
		}

		if (next_sc_off >= stream_size) {
			break;
		}
		pos = next_sc_off;
	}

	*au_ptr = &stream[au_start];
	*au_size = stream_size - au_start;
	*offset = stream_size;
	return AVDK_ERR_OK;
}

vcdec_h264_frame_type_t h264_decode_h264_peek_au_frame_type(const uint8_t *au_ptr,
							    uint32_t au_size)
{
	uint32_t pos = 0U;
	uint32_t sc_off;
	uint32_t sc_len;

	if (au_ptr == NULL || au_size == 0U) {
		return VCDEC_H264_FRAME_P;
	}

	while (h264_decode_h264_find_start_code(au_ptr, au_size, pos, &sc_off, &sc_len) == 0) {
		uint32_t payload_off = sc_off + sc_len;
		uint32_t next_sc_off;
		uint32_t next_sc_len;
		uint8_t nal_type;

		if (payload_off >= au_size) {
			break;
		}

		nal_type = au_ptr[payload_off] & 0x1FU;
		if (h264_decode_h264_is_vcl(nal_type)) {
			uint32_t slice_type;

			if (nal_type == 5U) {
				return VCDEC_H264_FRAME_IDR;
			}

			if (h264_decode_h264_find_start_code(au_ptr, au_size, payload_off,
							     &next_sc_off, &next_sc_len) != 0) {
				next_sc_off = au_size;
			}
			(void)next_sc_len;

			if (h264_decode_h264_parse_slice_type(&au_ptr[payload_off],
							      next_sc_off - payload_off,
							      &slice_type) == 0) {
				return ((slice_type % 5U) == 2U) ? VCDEC_H264_FRAME_I : VCDEC_H264_FRAME_P;
			}
			break;
		}

		pos = payload_off;
	}

	return VCDEC_H264_FRAME_P;
}

avdk_err_t h264_decode_h264_build_frame_table(const uint8_t *stream,
					      uint32_t stream_size,
					      h264_decode_h264_frame_t **table,
					      uint32_t *count)
{
	h264_decode_h264_frame_t *frames = NULL;
	uint32_t offset = 0U;
	uint32_t frame_count = 0U;
	uint32_t i;

	if (stream == NULL || stream_size == 0U || table == NULL || count == NULL) {
		return AVDK_ERR_INVAL;
	}
	*table = NULL;
	*count = 0U;

	while (offset < stream_size) {
		const uint8_t *frame_ptr = NULL;
		uint32_t frame_size = 0U;

		if (h264_decode_h264_next_au(stream, stream_size, &offset, &frame_ptr, &frame_size) != AVDK_ERR_OK) {
			break;
		}
		if (frame_ptr != NULL && frame_size != 0U) {
			frame_count++;
		}
	}

	if (frame_count == 0U) {
		LOGE("no h264 frame found in stream\r\n");
		return AVDK_ERR_GENERIC;
	}

	frames = (h264_decode_h264_frame_t *)os_malloc(sizeof(h264_decode_h264_frame_t) * frame_count);
	if (frames == NULL) {
		LOGE("alloc h264 frame table failed, count=%u\r\n", (unsigned)frame_count);
		return AVDK_ERR_NOMEM;
	}

	offset = 0U;
	for (i = 0U; i < frame_count && offset < stream_size;) {
		const uint8_t *frame_ptr = NULL;
		uint32_t frame_size = 0U;

		if (h264_decode_h264_next_au(stream, stream_size, &offset, &frame_ptr, &frame_size) != AVDK_ERR_OK) {
			break;
		}
		if (frame_ptr == NULL || frame_size == 0U) {
			continue;
		}

		frames[i].offset = (uint32_t)(frame_ptr - stream);
		frames[i].size = frame_size;
		i++;
	}

	if (i != frame_count) {
		os_free(frames);
		LOGE("h264 frame table changed while parsing: counted=%u filled=%u\r\n",
		     (unsigned)frame_count, (unsigned)i);
		return AVDK_ERR_GENERIC;
	}

	*table = frames;
	*count = frame_count;
	LOGI("h264 frame table ready, count=%u first=%u/%u last=%u/%u\r\n",
	     (unsigned)frame_count,
	     (unsigned)frames[0].offset,
	     (unsigned)frames[0].size,
	     (unsigned)frames[frame_count - 1U].offset,
	     (unsigned)frames[frame_count - 1U].size);

	return AVDK_ERR_OK;
}

void h264_decode_h264_frame_table_free(h264_decode_h264_frame_t *table)
{
	if (table != NULL) {
		os_free(table);
	}
}
