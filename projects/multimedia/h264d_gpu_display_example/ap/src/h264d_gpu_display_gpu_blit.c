#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <components/bk_gpu.h>
#include <modules/vg_lite_gpu/vg_lite.h>
#include <driver/hpdma.h>
#include "soc/reg_base.h"   /* SOC_SRAM_PERI_ADDR: HPDMA/GPU 只能访问 0x28 SRAM 别名 */

#include "h264d_gpu_display_config.h"
#include "h264d_gpu_display_gpu_blit.h"

#define TAG "h264d_blit"

#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/*
 * Whole-frame RGB blit acceleration (mirrors bk_gpu_ctlr_default.c flexa path):
 * instead of letting the GPU render the compressed frame straight into PSRAM
 * (slow, because compressed-tile writes to PSRAM are read-modify-write heavy),
 * render one flexa_lines-high strip into an SRAM ping-pong buffer, then let
 * HPDMA burst-copy that strip SRAM->PSRAM. GPU(strip N+1) overlaps HPDMA(strip N).
 */
#define H264D_GPU_BLIT_COMPRESS        1
#define H264D_GPU_BLIT_PIXEL_SIZE      4U   /* ARGB8888/BGRA8888 destination */
#define H264D_GPU_BLIT_FLEXA_LINES     ((uint16_t)H264D_GPU_DISPLAY_GPU_FLEXA_LINES)
#define H264D_GPU_BLIT_OUTPUT_WIDTH    ((uint16_t)H264D_GPU_DISPLAY_GPU_DST_WIDTH)
#define H264D_GPU_BLIT_OUTPUT_HEIGHT   ((uint16_t)((H264D_GPU_DISPLAY_GPU_DST_HEIGHT + 15U) & ~15U))
#if H264D_GPU_BLIT_COMPRESS
#define H264D_GPU_BLIT_STRIP_BYTES     ((uint32_t)H264D_GPU_BLIT_OUTPUT_WIDTH * (uint32_t)H264D_GPU_BLIT_FLEXA_LINES)
#else
#define H264D_GPU_BLIT_STRIP_BYTES     (H264D_GPU_BLIT_PIXEL_SIZE * (uint32_t)H264D_GPU_BLIT_OUTPUT_WIDTH * (uint32_t)H264D_GPU_BLIT_FLEXA_LINES)
#endif
#define H264D_GPU_BLIT_ALIGN           64U
#define H264D_GPU_BLIT_HPDMA_TIMEOUT_MS 3000U

typedef struct {
	uintptr_t pingpong_raw;     /* raw allocation, freed on deinit */
	uintptr_t buffers[2];       /* 64-byte aligned strip buffers (0x2C CPU alias) */
	uint8_t   dst_idx;          /* current ping-pong index */
	hpdma_id_t gdma;            /* SRAM->PSRAM copy channel */
	void      *link_table;      /* HPDMA linked-list descriptor table */
	beken_semaphore_t transfer_sem; /* posted by HPDMA finish ISR */
} h264d_gpu_blit_dma_t;

static uint8_t s_gpu_blit_initialized;
static void *s_gpu_blit_contiguous_buffer;
static h264d_gpu_blit_dma_t s_gpu_blit_dma;

void bk_gpu_driver_init(void);
void bk_gpu_driver_deinit(void);

static inline uintptr_t h264d_gpu_blit_align64(uintptr_t addr)
{
	return (addr + (H264D_GPU_BLIT_ALIGN - 1U)) & ~((uintptr_t)H264D_GPU_BLIT_ALIGN - 1U);
}

static uint32_t h264d_gpu_display_compressed_argb_size(uint32_t width, uint32_t height)
{
	return (uint32_t)bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888) * (width / 4U) * height;
}

static vg_lite_buffer_format_t h264d_gpu_display_rgb_src_format(bk_pixel_format_t src_format)
{
	switch (src_format) {
	case BK_PIXEL_FORMAT_RGB565:
		/* On this GC GPU the BK "RGB565" byte order maps to VG_LITE_BGR565
		 * (see bk_gpu_ctlr_default.c / lv_camera_blend.c). Using VG_LITE_RGB565
		 * swaps R/B and turns e.g. yellow into blue. The PP's RGB565 and RGB888
		 * outputs do NOT share channel order: 565 needs the R/B swap, 888 does
		 * not (verified on-panel). */
		return VG_LITE_BGR565;
	case BK_PIXEL_FORMAT_RGB888:
		/* VCDec PP RGB888 writes one pixel into a 32-bit word: X,R,G,B in
		 * memory; VG_LITE_XRGB8888 displays it correctly (verified on-panel). */
		return VG_LITE_XRGB8888;
	case BK_PIXEL_FORMAT_NV12:
		/* Decoder PP scale-only output: planar Y + interleaved UV (4:2:0). */
		return VG_LITE_NV12;
	default:
		return (vg_lite_buffer_format_t)-1;
	}
}

static void h264d_gpu_blit_dma_finish_cb(hpdma_id_t hpdma_id, void *user_data)
{
	(void)hpdma_id;
	if (user_data != NULL) {
		rtos_set_semaphore((beken_semaphore_t *)user_data);
	}
}

static void h264d_gpu_blit_dma_deinit(void)
{
	if (s_gpu_blit_dma.gdma < HPDMA_ID_MAX) {
		(void)bk_hpdma_disable_finish_interrupt(s_gpu_blit_dma.gdma);
		(void)bk_hpdma_register_isr(s_gpu_blit_dma.gdma, NULL, NULL, NULL, NULL);
		(void)bk_hpdma_free(HPDMA_DEV_DTCM, s_gpu_blit_dma.gdma);
		s_gpu_blit_dma.gdma = HPDMA_ID_MAX;
	}
	if (s_gpu_blit_dma.link_table != NULL) {
		bk_hpdma_link_deinit(s_gpu_blit_dma.link_table);
		s_gpu_blit_dma.link_table = NULL;
	}
	if (s_gpu_blit_dma.transfer_sem != NULL) {
		(void)rtos_deinit_semaphore(&s_gpu_blit_dma.transfer_sem);
		s_gpu_blit_dma.transfer_sem = NULL;
	}
	if (s_gpu_blit_dma.pingpong_raw != 0) {
		hsram_free((void *)s_gpu_blit_dma.pingpong_raw);
		s_gpu_blit_dma.pingpong_raw = 0;
	}
	s_gpu_blit_dma.buffers[0] = 0;
	s_gpu_blit_dma.buffers[1] = 0;
	s_gpu_blit_dma.dst_idx = 0;
}

static avdk_err_t h264d_gpu_blit_dma_init(void)
{
	uint32_t pingpong_size = H264D_GPU_BLIT_STRIP_BYTES * 2U + H264D_GPU_BLIT_ALIGN;
	uintptr_t base;
	bk_err_t bk_ret;

	s_gpu_blit_dma.gdma = HPDMA_ID_MAX;
	s_gpu_blit_dma.link_table = NULL;
	s_gpu_blit_dma.transfer_sem = NULL;
	s_gpu_blit_dma.pingpong_raw = 0;
	s_gpu_blit_dma.dst_idx = 0;

	base = (uintptr_t)bk_get_gpu_output_buffer(pingpong_size);
	if (base == 0) {
		LOGE("gpu blit: alloc SRAM ping-pong failed, size=%u\r\n", (unsigned)pingpong_size);
		return AVDK_ERR_NOMEM;
	}
	s_gpu_blit_dma.pingpong_raw = base;
	os_memset((void *)base, 0, pingpong_size);
	s_gpu_blit_dma.buffers[0] = h264d_gpu_blit_align64(base);
	s_gpu_blit_dma.buffers[1] = h264d_gpu_blit_align64(base + H264D_GPU_BLIT_STRIP_BYTES);

	s_gpu_blit_dma.link_table = bk_hpdma_link_init(1);
	if (s_gpu_blit_dma.link_table == NULL) {
		LOGE("gpu blit: bk_hpdma_link_init failed\r\n");
		goto fail;
	}

	s_gpu_blit_dma.gdma = bk_hpdma_alloc(HPDMA_DEV_DTCM);
	if (s_gpu_blit_dma.gdma >= HPDMA_ID_MAX) {
		LOGE("gpu blit: bk_hpdma_alloc failed\r\n");
		goto fail;
	}
	(void)bk_hpdma_set_dest_burst_len(s_gpu_blit_dma.gdma, HPDMA_BURST_LEN_INC16);
	(void)bk_hpdma_set_src_burst_len(s_gpu_blit_dma.gdma, HPDMA_BURST_LEN_INC16);

	bk_ret = rtos_init_semaphore(&s_gpu_blit_dma.transfer_sem, 1);
	if (bk_ret != BK_OK) {
		LOGE("gpu blit: init transfer_sem failed=%d\r\n", (int)bk_ret);
		goto fail;
	}
	(void)rtos_set_semaphore(&s_gpu_blit_dma.transfer_sem);

	(void)bk_hpdma_register_isr(s_gpu_blit_dma.gdma, NULL, NULL,
				    h264d_gpu_blit_dma_finish_cb, &s_gpu_blit_dma.transfer_sem);
	(void)bk_hpdma_enable_finish_interrupt(s_gpu_blit_dma.gdma);

	LOGI("gpu blit dma ready: strip=%u bytes buf0=0x%x buf1=0x%x gdma=%d\r\n",
	     (unsigned)H264D_GPU_BLIT_STRIP_BYTES,
	     (unsigned)s_gpu_blit_dma.buffers[0],
	     (unsigned)s_gpu_blit_dma.buffers[1],
	     (int)s_gpu_blit_dma.gdma);
	return AVDK_ERR_OK;

fail:
	h264d_gpu_blit_dma_deinit();
	return AVDK_ERR_GENERIC;
}

/* Issue one strip copy SRAM(0x2C src alias)->PSRAM. HPDMA link driver
 * remaps 0x2C->0x28 internally and leaves PSRAM addresses unchanged. */
static void h264d_gpu_blit_dma_transfer(uint32_t src_addr, void *frame, uint32_t offset,
					uint32_t xsize, uint32_t ysize, uint32_t dst_step)
{
	hpdma_link_config_t cfg;

	os_memset(&cfg, 0, sizeof(cfg));
	cfg.src_addr = src_addr;
	cfg.dst_addr = (uint32_t)((uintptr_t)frame + offset);
	cfg.src_xsize = (uint16_t)xsize;
	cfg.dst_xsize = (uint16_t)xsize;
	cfg.src_ysize = (uint16_t)ysize;
	cfg.dst_ysize = (uint16_t)ysize;
	cfg.src_step = 0;
	cfg.dst_step = (uint16_t)dst_step;
	cfg.finish_int_en = 1;
	cfg.half_finish_int_en = 0;
	(void)bk_hpdma_link_set_descs(s_gpu_blit_dma.link_table, &cfg, 1);
	(void)bk_hpdma_link_transfer(s_gpu_blit_dma.gdma, s_gpu_blit_dma.link_table);
}

/* Wait for any in-flight strip DMA, then normalize transfer_sem back to a
 * single available token so the next frame starts from a known state, even
 * after an error path. */
static void h264d_gpu_blit_dma_sync(void)
{
	uint32_t guard = 0;

	if (s_gpu_blit_dma.gdma < HPDMA_ID_MAX) {
		while (bk_hpdma_get_enable_status(s_gpu_blit_dma.gdma) && (guard++ < 1000000U)) {
			;
		}
	}
	if (s_gpu_blit_dma.transfer_sem != NULL) {
		while (rtos_get_semaphore(&s_gpu_blit_dma.transfer_sem, BEKEN_NO_WAIT) == BK_OK) {
			;
		}
		(void)rtos_set_semaphore(&s_gpu_blit_dma.transfer_sem);
	}
}

static avdk_err_t h264d_gpu_display_gpu_blit_init(void)
{
	vg_lite_error_t vg_ret;

	if (s_gpu_blit_initialized != 0U) {
		return AVDK_ERR_OK;
	}

	bk_gpu_driver_init();
	s_gpu_blit_contiguous_buffer = bk_get_gpu_flexa_buffer(CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
	if (s_gpu_blit_contiguous_buffer == NULL) {
		LOGE("alloc VG-Lite contiguous buffer failed, size=%u\r\n",
		     (unsigned)CONFIG_VG_LITE_GPU_CONTIGUOUS_MEM_SZ);
		bk_gpu_driver_deinit();
		return AVDK_ERR_NOMEM;
	}

	vg_ret = vg_lite_set_buffer((uint8_t *)s_gpu_blit_contiguous_buffer);
	if (vg_ret == VG_LITE_SUCCESS) {
		vg_ret = vg_lite_init(0, 0);
	}
	if (vg_ret != VG_LITE_SUCCESS) {
		LOGE("VG-Lite init failed, ret=%d\r\n", (int)vg_ret);
		hsram_free(s_gpu_blit_contiguous_buffer);
		s_gpu_blit_contiguous_buffer = NULL;
		bk_gpu_driver_deinit();
		return AVDK_ERR_GENERIC;
	}

	if (h264d_gpu_blit_dma_init() != AVDK_ERR_OK) {
		(void)vg_lite_close();
		hsram_free(s_gpu_blit_contiguous_buffer);
		s_gpu_blit_contiguous_buffer = NULL;
		bk_gpu_driver_deinit();
		return AVDK_ERR_GENERIC;
	}

	s_gpu_blit_initialized = 1U;
	return AVDK_ERR_OK;
}

void h264d_gpu_display_gpu_blit_deinit(void)
{
	if (s_gpu_blit_initialized == 0U) {
		return;
	}

	h264d_gpu_blit_dma_deinit();
	(void)vg_lite_close();
	bk_gpu_driver_deinit();
	if (s_gpu_blit_contiguous_buffer != NULL) {
		hsram_free(s_gpu_blit_contiguous_buffer);
		s_gpu_blit_contiguous_buffer = NULL;
	}
	s_gpu_blit_initialized = 0U;
}

avdk_err_t h264d_gpu_display_gpu_blit_rgb_frame(const uint8_t *src_buffer,
						uint16_t src_width,
						uint16_t src_height,
						bk_pixel_format_t src_format,
						void **out_frame,
						uint32_t *out_frame_size)
{
	avdk_err_t ret;
	vg_lite_error_t vg_ret;
	vg_lite_buffer_t src_buf;
	vg_lite_buffer_t dst_buf;
	vg_lite_matrix_t matrix;
	void *frame;
	uint32_t frame_size;
	vg_lite_buffer_format_t vg_src_format;
	uint32_t start_ms;
	uint32_t cost_ms;
	const uint16_t rotate = (uint16_t)H264D_GPU_DISPLAY_GPU_ROTATE_DEGREE;
	const uint16_t flexa_lines = H264D_GPU_BLIT_FLEXA_LINES;
	const uint16_t output_width = H264D_GPU_BLIT_OUTPUT_WIDTH;
	const uint16_t output_height = H264D_GPU_BLIT_OUTPUT_HEIGHT;
	const uint32_t strip_count = (uint32_t)output_height / (uint32_t)flexa_lines;
	uint32_t strip_index;
	bool frame_failed = false;

	if (src_buffer == NULL || out_frame == NULL || out_frame_size == NULL ||
	    src_width == 0U || src_height == 0U) {
		return AVDK_ERR_INVAL;
	}

	vg_src_format = h264d_gpu_display_rgb_src_format(src_format);
	if (vg_src_format == (vg_lite_buffer_format_t)-1) {
		LOGE("unsupported rgb source format=%u\r\n", (unsigned)src_format);
		return AVDK_ERR_UNSUPPORTED;
	}

	ret = h264d_gpu_display_gpu_blit_init();
	if (ret != AVDK_ERR_OK) {
		return ret;
	}

	frame_size = h264d_gpu_display_compressed_argb_size(H264D_GPU_DISPLAY_GPU_DISPLAY_WIDTH,
							   H264D_GPU_DISPLAY_GPU_DISPLAY_HEIGHT);
	frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
	if (frame == NULL) {
		LOGE("alloc rgb display frame failed, size=%u\r\n", (unsigned)frame_size);
		return AVDK_ERR_NOMEM;
	}

	os_memset(&src_buf, 0, sizeof(src_buf));
	os_memset(&dst_buf, 0, sizeof(dst_buf));
	os_memset(&matrix, 0, sizeof(matrix));

	/* Wrap the full source frame (PSRAM) as GPU source; the GPU reads only the
	 * band that maps to the strip currently being rendered. RGB565/RGB888 are
	 * single-plane; NV12 is planar Y followed by an interleaved UV plane. The
	 * UV plane offset MUST match the decoder PP's write layout: the Y plane is
	 * exactly (even-aligned) src_height rows tall and the UV plane starts right
	 * after it. Using a 16-row-aligned offset here would read the UV plane 8
	 * rows too far, showing up as green stripes / wrong chroma. */
	src_buf.width = src_width;
	src_buf.height = src_height;
	src_buf.format = vg_src_format;
	src_buf.compress_mode = VG_LITE_DEC_DISABLE;
	src_buf.tiled = VG_LITE_LINEAR;
	if (src_format == BK_PIXEL_FORMAT_NV12) {
		uint32_t y_plane_height = ((uint32_t)src_height + 1U) & ~1U;
		void *uv_plane = (void *)(src_buffer + (uint32_t)src_width * y_plane_height);

		src_buf.stride = (vg_lite_int32_t)src_width;
		src_buf.yuv.uv_stride = (vg_lite_uint32_t)src_width;
		src_buf.yuv.uv_height = (vg_lite_uint32_t)(y_plane_height / 2U);
		vg_ret = vg_lite_allocate_with_data(&src_buf, (void *)src_buffer, uv_plane, NULL, NULL);
	} else {
		vg_ret = vg_lite_allocate_with_data(&src_buf, (void *)src_buffer, NULL, NULL, NULL);
	}
	if (vg_ret != VG_LITE_SUCCESS) {
		LOGE("wrap source failed, fmt=%u ret=%d\r\n", (unsigned)src_format, (int)vg_ret);
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	/* Base transform = rotate then scale (input -> full pre-rotation output),
	 * identical to the flexa path in bk_gpu_ctlr_default.c. A per-strip
	 * translation added below selects the output band for each strip. */
	{
		float scale_x = (float)output_width / (float)src_width;
		float scale_y = (float)output_height / (float)src_height;

		vg_lite_identity(&matrix);
		if (rotate != 0U) {
			vg_lite_rotate((float)rotate, &matrix);
		}
		vg_lite_scale(scale_x, scale_y, &matrix);
	}

	/* Destination is one flexa_lines-high strip rendered into SRAM. */
	if (rotate == 90U || rotate == 270U) {
		dst_buf.width = flexa_lines;
		dst_buf.height = output_width;
	} else {
		dst_buf.width = output_width;
		dst_buf.height = flexa_lines;
	}
	dst_buf.format = VG_LITE_BGRA8888;
#if H264D_GPU_BLIT_COMPRESS
	dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
	dst_buf.tiled = VG_LITE_TILED;
#else
	dst_buf.compress_mode = VG_LITE_DEC_DISABLE;
	dst_buf.tiled = VG_LITE_LINEAR;
#endif
	s_gpu_blit_dma.dst_idx = 0U;
	/* vg_lite_allocate_with_data sets .memory to the 0x2C CPU alias and
	 * .address to the 0x28 peripheral alias the GPU renders to. */
	vg_ret = vg_lite_allocate_with_data(&dst_buf,
					    (void *)(uintptr_t)s_gpu_blit_dma.buffers[0],
					    NULL, NULL, NULL);
	if (vg_ret != VG_LITE_SUCCESS) {
		LOGE("wrap rgb strip dst failed, ret=%d\r\n", (int)vg_ret);
		(void)vg_lite_free_without_free_data(&src_buf);
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	start_ms = rtos_get_time();
	for (strip_index = 1U; strip_index <= strip_count; strip_index++) {
		/* Per-strip translation (mirrors gpu_flex_update_matrix). */
		if (rotate == 0U) {
			matrix.m[1][2] = -((float)(strip_index - 1U) * (float)flexa_lines);
		} else if (rotate == 90U) {
			matrix.m[0][2] = ((float)strip_index * (float)flexa_lines);
		} else { /* 270 */
			matrix.m[0][2] = -((float)(strip_index - 1U) * (float)flexa_lines);
			matrix.m[1][2] = (float)output_width;
		}

		vg_ret = vg_lite_blit(&dst_buf, &src_buf, &matrix,
				      VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
		if (vg_ret == VG_LITE_SUCCESS) {
			vg_ret = vg_lite_finish();
		}
		if (vg_ret != VG_LITE_SUCCESS) {
			LOGE("rgb strip blit failed, strip=%u ret=%d\r\n",
			     (unsigned)strip_index, (int)vg_ret);
			frame_failed = true;
			break;
		}

		/* Wait for the previous strip's DMA before reusing the channel. */
		if (rtos_get_semaphore(&s_gpu_blit_dma.transfer_sem,
				       H264D_GPU_BLIT_HPDMA_TIMEOUT_MS) != BK_OK) {
			LOGE("rgb strip dma wait timeout, strip=%u\r\n", (unsigned)strip_index);
			frame_failed = true;
			break;
		}

		/* Burst-copy the finished strip SRAM -> PSRAM frame. */
		{
			uint32_t src_addr = (uint32_t)(uintptr_t)dst_buf.memory;

			if (rotate == 90U || rotate == 270U) {
				uint32_t stride = (uint32_t)(output_height - flexa_lines) *
						  H264D_GPU_BLIT_PIXEL_SIZE;
#if H264D_GPU_BLIT_COMPRESS
				uint32_t ysize = (uint32_t)output_width / 4U;
#else
				uint32_t ysize = (uint32_t)output_width;
#endif
				uint32_t xsize = (uint32_t)flexa_lines * H264D_GPU_BLIT_PIXEL_SIZE;
				uint32_t offset = (rotate == 90U) ?
					((uint32_t)(output_height - strip_index * flexa_lines) *
					 H264D_GPU_BLIT_PIXEL_SIZE) :
					((strip_index - 1U) * (uint32_t)flexa_lines *
					 H264D_GPU_BLIT_PIXEL_SIZE);

				h264d_gpu_blit_dma_transfer(src_addr, frame, offset,
							    xsize, ysize, stride);
			} else {
				uint32_t offset = (strip_index - 1U) * H264D_GPU_BLIT_STRIP_BYTES;
				uint32_t xsize = (uint32_t)output_width * H264D_GPU_BLIT_PIXEL_SIZE;
#if H264D_GPU_BLIT_COMPRESS
				uint32_t ysize = (uint32_t)flexa_lines / 4U;
#else
				uint32_t ysize = (uint32_t)flexa_lines;
#endif
				h264d_gpu_blit_dma_transfer(src_addr, frame, offset,
							    xsize, ysize, 0);
			}
		}

		/* Switch to the other ping-pong buffer for the next strip. */
		s_gpu_blit_dma.dst_idx = 1U - s_gpu_blit_dma.dst_idx;
		dst_buf.memory = (vg_lite_pointer)(uintptr_t)s_gpu_blit_dma.buffers[s_gpu_blit_dma.dst_idx];
		dst_buf.address = (uint32_t)SOC_SRAM_PERI_ADDR(s_gpu_blit_dma.buffers[s_gpu_blit_dma.dst_idx]);
	}

	/* Ensure the last strip's DMA has landed before the frame is used, and
	 * leave transfer_sem balanced for the next call. */
	h264d_gpu_blit_dma_sync();
	cost_ms = rtos_get_time() - start_ms;

	(void)vg_lite_free_without_free_data(&dst_buf);
	(void)vg_lite_free_without_free_data(&src_buf);

	if (frame_failed) {
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	*out_frame = frame;
	*out_frame_size = frame_size;
	LOGV("rgb strip blit done: src=%ux%u fmt=%u dst=%ux%u rotate=%u strips=%u cost=%u ms frame=%p size=%u\r\n",
	     (unsigned)src_width,
	     (unsigned)src_height,
	     (unsigned)src_format,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DISPLAY_WIDTH,
	     (unsigned)H264D_GPU_DISPLAY_GPU_DISPLAY_HEIGHT,
	     (unsigned)rotate,
	     (unsigned)strip_count,
	     (unsigned)cost_ms,
	     frame,
	     (unsigned)frame_size);
	return AVDK_ERR_OK;
}
