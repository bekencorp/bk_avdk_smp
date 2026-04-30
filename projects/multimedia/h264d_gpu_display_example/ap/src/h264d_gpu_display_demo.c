#include <stdint.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <components/bk_decode/bk_h264_decode_ctlr.h>
#include <components/bk_flexa_bond.h>

#include "cli.h"
#include "bk_private/bk_cli.h"
#include "h264d_gpu_display_demo.h"
#include "h264d_gpu_display_gpu.h"
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
#include "h264d_gpu_display_display.h"
#endif
#include "h264_decode_stream_1280x720.h"

#define TAG "h264d_gpu_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

#define H264D_GPU_DISPLAY_TASK_PRIORITY   3
#define H264D_GPU_DISPLAY_TASK_STACK_SIZE (1024 * 16)
#define H264D_GPU_DISPLAY_TIMEOUT_MS      1000U
#define H264D_GPU_DISPLAY_SEG_HEIGHT_MB   1U
#define H264D_GPU_DISPLAY_SEG_NUM         3U

typedef struct {
	volatile uint32_t dec_frame_done_count;
	volatile int dec_last_status;
	volatile uint32_t dec_flexa_done_count;
	volatile uint32_t dec_last_wr_ptr;
	volatile uint32_t gpu_line_done_count;
	volatile uint32_t gpu_last_done_lines;
	volatile uint32_t gpu_frame_done_count;
	volatile uint32_t decoded_aus;
} h264d_gpu_display_stats_t;

typedef struct {
	const uint8_t *data;
	uint32_t size;
	uint32_t byte_pos;
	uint8_t curr_byte;
	uint8_t bit_pos;
	uint8_t zero_count;
} h264d_gpu_display_bs_t;

static beken_thread_t s_h264d_gpu_display_thread = NULL;
static volatile uint8_t s_h264d_gpu_display_running = 0U;

/*
 * Flexa input is consumed by GPU/DMA directly, so keep the ring buffer 64-byte
 * aligned like the doorbell decode path.
 */
static void *h264d_gpu_display_hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
	void *raw;
	uint32_t total;
	uintptr_t start;
	uintptr_t aligned;

	if (alignment < (uint32_t)sizeof(void *)) {
		alignment = (uint32_t)sizeof(void *);
	}
	if ((alignment & (alignment - 1U)) != 0U) {
		return NULL;
	}

	total = size + alignment - 1U + (uint32_t)sizeof(void *);
	raw = hsram_malloc(total);
	if (raw == NULL) {
		return NULL;
	}

	start = (uintptr_t)raw + sizeof(void *);
	aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
	((void **)aligned)[-1] = raw;
	return (void *)aligned;
}

static void h264d_gpu_display_hsram_aligned_free(void *ptr)
{
	void *raw;

	if (ptr == NULL) {
		return;
	}

	raw = ((void **)ptr)[-1];
	os_free(raw);
}

static uint32_t h264d_gpu_display_flexa_buffer_size(uint32_t width)
{
	return width * 16U * H264D_GPU_DISPLAY_SEG_HEIGHT_MB *
	       H264D_GPU_DISPLAY_SEG_NUM * 3U / 2U;
}

static int h264d_gpu_display_find_start_code(const uint8_t *buf, uint32_t len,
						 uint32_t offset, uint32_t *sc_off, uint32_t *sc_len)
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

static int h264d_gpu_display_bs_next_byte(h264d_gpu_display_bs_t *bs, uint8_t *value)
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

static int h264d_gpu_display_bs_read_bit(h264d_gpu_display_bs_t *bs, uint32_t *value)
{
	if (bs->bit_pos == 0U) {
		if (h264d_gpu_display_bs_next_byte(bs, &bs->curr_byte) != 0) {
			return -1;
		}
		bs->bit_pos = 8U;
	}

	*value = (bs->curr_byte >> (bs->bit_pos - 1U)) & 0x1U;
	bs->bit_pos--;
	return 0;
}

static int h264d_gpu_display_bs_read_ue(h264d_gpu_display_bs_t *bs, uint32_t *value)
{
	uint32_t zeros = 0U;
	uint32_t bit = 0U;
	uint32_t suffix = 0U;
	uint32_t i;

	while (1) {
		if (h264d_gpu_display_bs_read_bit(bs, &bit) != 0) {
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
		if (h264d_gpu_display_bs_read_bit(bs, &bit) != 0) {
			return -1;
		}
		suffix = (suffix << 1) | bit;
	}

	*value = ((1U << zeros) - 1U) + suffix;
	return 0;
}

static int h264d_gpu_display_parse_first_mb(const uint8_t *nal_payload,
						 uint32_t nal_payload_size,
						 uint32_t *first_mb_in_slice)
{
	h264d_gpu_display_bs_t bs;

	if (nal_payload == NULL || nal_payload_size <= 1U || first_mb_in_slice == NULL) {
		return -1;
	}

	bs.data = nal_payload + 1U;
	bs.size = nal_payload_size - 1U;
	bs.byte_pos = 0U;
	bs.curr_byte = 0U;
	bs.bit_pos = 0U;
	bs.zero_count = 0U;

	return h264d_gpu_display_bs_read_ue(&bs, first_mb_in_slice);
}

static int h264d_gpu_display_is_vcl(uint8_t nal_type)
{
	return (nal_type == 1U || nal_type == 2U || nal_type == 5U);
}

static int h264d_gpu_display_next_au(const uint8_t *stream, uint32_t stream_size,
					 uint32_t *offset, const uint8_t **au_ptr,
					 uint32_t *au_size)
{
	uint32_t au_start;
	uint32_t tmp_sc_len;
	uint32_t pos;
	uint8_t seen_vcl = 0U;

	if (stream == NULL || offset == NULL || au_ptr == NULL || au_size == NULL ||
	    *offset >= stream_size) {
		return -1;
	}

	if (h264d_gpu_display_find_start_code(stream, stream_size, *offset, &au_start, &tmp_sc_len) != 0) {
		return -1;
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

		if (h264d_gpu_display_find_start_code(stream, stream_size, pos, &sc_off, &sc_len) != 0) {
			break;
		}

		payload_off = sc_off + sc_len;
		if (payload_off >= stream_size) {
			break;
		}

		nal_type = stream[payload_off] & 0x1FU;
		if (h264d_gpu_display_find_start_code(stream, stream_size, payload_off, &next_sc_off, &next_sc_len) != 0) {
			next_sc_off = stream_size;
		}
		(void)next_sc_len;

		if (sc_off != au_start && seen_vcl) {
			if (h264d_gpu_display_is_vcl(nal_type)) {
				uint32_t first_mb_in_slice = 0U;

				if (h264d_gpu_display_parse_first_mb(&stream[payload_off],
									 next_sc_off - payload_off,
									 &first_mb_in_slice) != 0) {
					boundary = 1U;
				} else if (first_mb_in_slice == 0U) {
					boundary = 1U;
				}
			} else if (nal_type == 6U || nal_type == 7U || nal_type == 8U ||
				   nal_type == 9U || nal_type == 10U || nal_type == 11U ||
				   nal_type == 12U) {
				boundary = 1U;
			}
		}

		if (boundary) {
			*au_ptr = &stream[au_start];
			*au_size = sc_off - au_start;
			*offset = sc_off;
			return 0;
		}

		if (h264d_gpu_display_is_vcl(nal_type)) {
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
	return 0;
}

static void h264d_gpu_display_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
	size_t msg_len;

	if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL) {
		return;
	}

	msg_len = os_strlen(msg);
	if (msg_len >= (size_t)xWriteBufferLen) {
		msg_len = (size_t)xWriteBufferLen - 1U;
	}

	os_memcpy(pcWriteBuffer, msg, msg_len);
	pcWriteBuffer[msg_len] = '\0';
}

static void h264d_gpu_display_decoder_frame_done(int status, void *args)
{
	h264d_gpu_display_stats_t *stats = (h264d_gpu_display_stats_t *)args;

	if (stats == NULL) {
		return;
	}

	stats->dec_frame_done_count++;
	stats->dec_last_status = status;
}

static void h264d_gpu_display_decoder_flexa_done(uint32_t wr_ptr, void *args)
{
	h264d_gpu_display_stats_t *stats = (h264d_gpu_display_stats_t *)args;

	if (stats == NULL) {
		return;
	}

	stats->dec_flexa_done_count++;
	stats->dec_last_wr_ptr = wr_ptr;
}

static void h264d_gpu_display_gpu_line_done(uint32_t done_lines, void *args)
{
	h264d_gpu_display_stats_t *stats = (h264d_gpu_display_stats_t *)args;

	if (stats == NULL) {
		return;
	}

	stats->gpu_line_done_count++;
	stats->gpu_last_done_lines = done_lines;
}

static void h264d_gpu_display_gpu_frame_done(void *frame, uint32_t frame_size, void *args)
{
	h264d_gpu_display_stats_t *stats = (h264d_gpu_display_stats_t *)args;

	(void)frame;
	(void)frame_size;

	if (stats == NULL) {
		return;
	}

	stats->gpu_frame_done_count++;
}

static avdk_err_t h264d_gpu_display_run(void)
{
	avdk_err_t ret = AVDK_ERR_OK;
	uint8_t *stream_buf = NULL;
	uint8_t *pp_buf = NULL;
	uint32_t stream_size = h264_decode_stream_1280x720_bytes;
	const uint32_t width = 1280U;
	const uint32_t height = 720U;
	const uint32_t pp_size = h264d_gpu_display_flexa_buffer_size(width);
	uint32_t offset = 0U;
	bk_h264_decode_ctlr_handle_t decoder = NULL;
	void *bond = NULL;
	h264d_gpu_display_stats_t stats;
	bk_h264_decode_flexa_config_t dec_cfg = DEFAULT_H264_DECODE_FLEXA_CONFIG;

	os_memset(&stats, 0, sizeof(stats));
	stats.dec_last_status = BK_OK;

	stream_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, stream_size);
	if (stream_buf == NULL) {
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memcpy(stream_buf, h264_decode_stream_1280x720, stream_size);

	pp_buf = (uint8_t *)h264d_gpu_display_hsram_aligned_malloc(64U, pp_size);
	if (pp_buf == NULL) {
		ret = AVDK_ERR_NOMEM;
		goto cleanup;
	}
	os_memset(pp_buf, 0, pp_size);
	LOGI("flexa pp buffer=%p size=%u aligned64=%u\r\n",
	     pp_buf,
	     (unsigned)pp_size,
	     ((uintptr_t)pp_buf & 0x3FU) == 0U ? 1U : 0U);

	dec_cfg.timeout_ms = H264D_GPU_DISPLAY_TIMEOUT_MS;
	dec_cfg.out_width = (uint16_t)width;
	dec_cfg.out_height = (uint16_t)height;
	dec_cfg.out_format = BK_PIXEL_FORMAT_NV12;
	dec_cfg.segment_height = H264D_GPU_DISPLAY_SEG_HEIGHT_MB;
	dec_cfg.segment_number = H264D_GPU_DISPLAY_SEG_NUM;
	dec_cfg.frame_done_cb = h264d_gpu_display_decoder_frame_done;
	dec_cfg.frame_done_args = &stats;
	dec_cfg.flexa_done_cb = h264d_gpu_display_decoder_flexa_done;
	dec_cfg.flexa_done_args = &stats;

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	LOGI("stage: open display\r\n");
	ret = h264d_gpu_display_display_open();
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
#else
	LOGI("stage: display disabled by H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY\r\n");
#endif

	LOGI("stage: open gpu\r\n");
	ret = h264d_gpu_display_gpu_open(pp_buf,
					 H264D_GPU_DISPLAY_SEG_NUM,
					 (uint16_t)width,
					 (uint16_t)height,
					 h264d_gpu_display_gpu_line_done,
					 &stats,
					 h264d_gpu_display_gpu_frame_done,
					 &stats);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	LOGI("stage: create decoder\r\n");
	ret = bk_h264_decode_flexa_ctlr_new(&decoder, &dec_cfg);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	LOGI("stage: init decoder\r\n");
	ret = bk_h264_decode_init(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}
	LOGI("stage: open decoder\r\n");
	ret = bk_h264_decode_open(decoder);
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

	LOGI("stage: start h264d->gpu bond\r\n");
	ret = bk_flexa_h264d_gpu_bond_start(&bond, decoder, h264d_gpu_display_gpu_handle_get());
	if (ret != AVDK_ERR_OK) {
		goto cleanup;
	}

#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	LOGI("demo start, decode=%ux%u, display=%ux%u gpu dst=%ux%u rotate=90 compress=1 scale=1\r\n",
	     (unsigned)width,
	     (unsigned)height,
	     (unsigned)h264d_gpu_display_display_width(),
	     (unsigned)h264d_gpu_display_display_height(),
	     1920U,
	     1080U);
#else
	LOGI("demo start, decode=%ux%u, display=disabled gpu dst=%ux%u rotate=90 compress=1 scale=1\r\n",
	     (unsigned)width,
	     (unsigned)height,
	     1920U,
	     1080U);
#endif

	while (offset < stream_size) {
		const uint8_t *au_ptr = NULL;
		uint32_t au_size = 0U;
		bk_h264_decode_input_t input = {0};
		bk_h264_decode_info_t info = {0};

		if (h264d_gpu_display_next_au(stream_buf, stream_size, &offset, &au_ptr, &au_size) != 0) {
			break;
		}
		if (au_ptr == NULL || au_size == 0U) {
			continue;
		}

		input.stream = (uint8_t *)(uintptr_t)au_ptr;
		input.stream_len = au_size;
		input.out_buffer = pp_buf;
		input.out_buffer_size = pp_size;

		ret = bk_h264_decode_frame(decoder, &input);
		if (ret != AVDK_ERR_OK) {
			LOGE("decode failed on au=%u ret=%d\r\n",
			     (unsigned)(stats.decoded_aus + 1U), (int)ret);
			goto cleanup;
		}

		ret = bk_h264_decode_get_info(decoder, &info);
		if (ret != AVDK_ERR_OK) {
			LOGE("get info failed on au=%u ret=%d\r\n",
			     (unsigned)(stats.decoded_aus + 1U), (int)ret);
			goto cleanup;
		}

		stats.decoded_aus++;
		LOGI("decoded au=%u size=%u type=%u ref=%u dec_fd=%u dec_fx=%u gpu_line=%u gpu_frame=%u\r\n",
		     (unsigned)stats.decoded_aus,
		     (unsigned)au_size,
		     (unsigned)info.frame_type,
		     (unsigned)info.is_reference,
		     (unsigned)stats.dec_frame_done_count,
		     (unsigned)stats.dec_flexa_done_count,
		     (unsigned)stats.gpu_last_done_lines,
		     (unsigned)stats.gpu_frame_done_count);
	}

	if (stats.decoded_aus == 0U) {
		ret = AVDK_ERR_GENERIC;
		goto cleanup;
	}

	LOGI("demo done, decoded_aus=%u dec_frame_done=%u dec_flexa_done=%u gpu_line_done=%u gpu_frame_done=%u\r\n",
	     (unsigned)stats.decoded_aus,
	     (unsigned)stats.dec_frame_done_count,
	     (unsigned)stats.dec_flexa_done_count,
	     (unsigned)stats.gpu_line_done_count,
	     (unsigned)stats.gpu_frame_done_count);

	ret = AVDK_ERR_OK;

cleanup:
	if (bond != NULL) {
		bk_flexa_h264d_gpu_bond_stop(bond);
	}
	h264d_gpu_display_gpu_close();
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	h264d_gpu_display_display_close();
#endif
	if (decoder != NULL) {
		(void)bk_h264_decode_close(decoder);
		(void)bk_h264_decode_deinit(decoder);
		(void)bk_h264_decode_delete(decoder);
	}
	if (pp_buf != NULL) {
		h264d_gpu_display_hsram_aligned_free(pp_buf);
	}
	if (stream_buf != NULL) {
		bk_frame_buffer_free(stream_buf);
	}

	LOGI("[RESULT][%s] decoded_aus=%u dec_status=%d dec_frame_done=%u dec_flexa_done=%u gpu_line_done=%u gpu_frame_done=%u\r\n",
	     (ret == AVDK_ERR_OK) ? "PASS" : "FAIL",
	     (unsigned)stats.decoded_aus,
	     stats.dec_last_status,
	     (unsigned)stats.dec_frame_done_count,
	     (unsigned)stats.dec_flexa_done_count,
	     (unsigned)stats.gpu_line_done_count,
	     (unsigned)stats.gpu_frame_done_count);
	return ret;
}

static void h264d_gpu_display_task_entry(void *arg)
{
	(void)arg;
	LOGI("task entry\r\n");
	(void)h264d_gpu_display_run();

	s_h264d_gpu_display_running = 0U;
	s_h264d_gpu_display_thread = NULL;
	rtos_delete_thread(NULL);
}

static void h264d_gpu_display_print_usage(void)
{
	bk_printf("Usage:\r\n");
#if H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY
	bk_printf("  h264d_gpu_display start   - run H264D -> GPU -> MIPI display test\r\n");
#else
	bk_printf("  h264d_gpu_display start   - run H264D -> GPU test without display\r\n");
#endif
	bk_printf("  h264d_gpu_display help    - show this help\r\n");
}

static avdk_err_t h264d_gpu_display_start_task(void)
{
	avdk_err_t ret;

	if (s_h264d_gpu_display_running != 0U) {
		return AVDK_ERR_BUSY;
	}

	s_h264d_gpu_display_running = 1U;
	ret = rtos_core0_create_thread(&s_h264d_gpu_display_thread,
				       H264D_GPU_DISPLAY_TASK_PRIORITY,
				       "h264d_gpu_disp",
				       (beken_thread_function_t)h264d_gpu_display_task_entry,
				       H264D_GPU_DISPLAY_TASK_STACK_SIZE,
				       NULL);
	if (ret != BK_OK) {
		s_h264d_gpu_display_running = 0U;
		s_h264d_gpu_display_thread = NULL;
		return AVDK_ERR_GENERIC;
	}

	return AVDK_ERR_OK;
}

static void cli_h264d_gpu_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	avdk_err_t ret = AVDK_ERR_OK;

	if (argc < 2) {
		h264d_gpu_display_print_usage();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
		h264d_gpu_display_print_usage();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
		return;
	}

	if (os_strcmp(argv[1], "start") != 0) {
		h264d_gpu_display_print_usage();
		h264d_gpu_display_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	ret = h264d_gpu_display_start_task();
	h264d_gpu_display_write_rsp(pcWriteBuffer,
				       xWriteBufferLen,
				       (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
}

int cli_h264d_gpu_display_init(void)
{
	static const struct cli_command s_h264d_gpu_display_cmds[] = {
		{"h264d_gpu_display", "h264d_gpu_display help|start", cli_h264d_gpu_display_cmd},
	};

	return cli_register_commands(s_h264d_gpu_display_cmds,
				       sizeof(s_h264d_gpu_display_cmds) / sizeof(s_h264d_gpu_display_cmds[0]));
}
