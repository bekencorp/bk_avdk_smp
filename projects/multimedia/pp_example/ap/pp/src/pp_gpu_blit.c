#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>
#include <components/bk_gpu.h>
#include <modules/vg_lite_gpu/vg_lite.h>
#include <driver/hpdma.h>
#include "soc/reg_base.h"

#include "pp_config.h"
#include "pp_gpu_blit.h"

#define TAG "pp_blit"

#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define PP_GPU_BLIT_COMPRESS        1
#define PP_GPU_BLIT_PIXEL_SIZE      4U
#define PP_GPU_BLIT_FLEXA_LINES     ((uint16_t)PP_EXAMPLE_GPU_FLEXA_LINES)
#define PP_GPU_BLIT_OUTPUT_WIDTH    ((uint16_t)PP_EXAMPLE_GPU_DST_WIDTH)
#define PP_GPU_BLIT_OUTPUT_HEIGHT   ((uint16_t)((PP_EXAMPLE_GPU_DST_HEIGHT + 15U) & ~15U))
#if PP_GPU_BLIT_COMPRESS
#define PP_GPU_BLIT_STRIP_BYTES     ((uint32_t)PP_GPU_BLIT_OUTPUT_WIDTH * (uint32_t)PP_GPU_BLIT_FLEXA_LINES)
#else
#define PP_GPU_BLIT_STRIP_BYTES     (PP_GPU_BLIT_PIXEL_SIZE * (uint32_t)PP_GPU_BLIT_OUTPUT_WIDTH * (uint32_t)PP_GPU_BLIT_FLEXA_LINES)
#endif
#define PP_GPU_BLIT_ALIGN           64U
#define PP_GPU_BLIT_HPDMA_TIMEOUT_MS 3000U

typedef struct {
	uintptr_t pingpong_raw;
	uintptr_t buffers[2];
	uint8_t   dst_idx;
	hpdma_id_t gdma;
	void      *link_table;
	beken_semaphore_t transfer_sem;
} pp_gpu_blit_dma_t;

static uint8_t s_gpu_blit_initialized;
static void *s_gpu_blit_contiguous_buffer;
static pp_gpu_blit_dma_t s_gpu_blit_dma;

void bk_gpu_driver_init(void);
void bk_gpu_driver_deinit(void);

static inline uintptr_t pp_gpu_blit_align64(uintptr_t addr)
{
	return (addr + (PP_GPU_BLIT_ALIGN - 1U)) & ~((uintptr_t)PP_GPU_BLIT_ALIGN - 1U);
}

static uint32_t pp_gpu_display_compressed_argb_size(uint32_t width, uint32_t height)
{
	return (uint32_t)bk_pixel_size_get(BK_PIXEL_FORMAT_ARGB8888) * (width / 4U) * height;
}

static vg_lite_buffer_format_t pp_gpu_rgb_src_format(bk_pixel_format_t src_format)
{
	switch (src_format) {
	case BK_PIXEL_FORMAT_RGB565:
		return VG_LITE_BGR565;
	case BK_PIXEL_FORMAT_RGB888:
		return VG_LITE_XRGB8888;
	case BK_PIXEL_FORMAT_NV12:
		return VG_LITE_NV12;
	default:
		return (vg_lite_buffer_format_t)-1;
	}
}

static void pp_gpu_blit_dma_finish_cb(hpdma_id_t hpdma_id, void *user_data)
{
	(void)hpdma_id;
	if (user_data != NULL) {
		rtos_set_semaphore((beken_semaphore_t *)user_data);
	}
}

static void pp_gpu_blit_dma_deinit(void)
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

static avdk_err_t pp_gpu_blit_dma_init(void)
{
	uint32_t pingpong_size = PP_GPU_BLIT_STRIP_BYTES * 2U + PP_GPU_BLIT_ALIGN;
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
	s_gpu_blit_dma.buffers[0] = pp_gpu_blit_align64(base);
	s_gpu_blit_dma.buffers[1] = pp_gpu_blit_align64(base + PP_GPU_BLIT_STRIP_BYTES);

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
				    pp_gpu_blit_dma_finish_cb, &s_gpu_blit_dma.transfer_sem);
	(void)bk_hpdma_enable_finish_interrupt(s_gpu_blit_dma.gdma);

	LOGI("gpu blit dma ready: strip=%u bytes gdma=%d\r\n",
	     (unsigned)PP_GPU_BLIT_STRIP_BYTES, (int)s_gpu_blit_dma.gdma);
	return AVDK_ERR_OK;

fail:
	pp_gpu_blit_dma_deinit();
	return AVDK_ERR_GENERIC;
}

static void pp_gpu_blit_dma_transfer(uint32_t src_addr, void *frame, uint32_t offset,
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

static void pp_gpu_blit_dma_sync(void)
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

static avdk_err_t pp_gpu_display_gpu_blit_init(void)
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

	if (pp_gpu_blit_dma_init() != AVDK_ERR_OK) {
		(void)vg_lite_close();
		hsram_free(s_gpu_blit_contiguous_buffer);
		s_gpu_blit_contiguous_buffer = NULL;
		bk_gpu_driver_deinit();
		return AVDK_ERR_GENERIC;
	}

	s_gpu_blit_initialized = 1U;
	return AVDK_ERR_OK;
}

void pp_gpu_blit_deinit(void)
{
	if (s_gpu_blit_initialized == 0U) {
		return;
	}

	pp_gpu_blit_dma_deinit();
	(void)vg_lite_close();
	bk_gpu_driver_deinit();
	if (s_gpu_blit_contiguous_buffer != NULL) {
		hsram_free(s_gpu_blit_contiguous_buffer);
		s_gpu_blit_contiguous_buffer = NULL;
	}
	s_gpu_blit_initialized = 0U;
}

avdk_err_t pp_gpu_blit_rgb_frame(const uint8_t *src_buffer,
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
	const uint16_t rotate = (uint16_t)PP_EXAMPLE_GPU_ROTATE_DEGREE;
	const uint16_t flexa_lines = PP_GPU_BLIT_FLEXA_LINES;
	const uint16_t output_width = PP_GPU_BLIT_OUTPUT_WIDTH;
	const uint16_t output_height = PP_GPU_BLIT_OUTPUT_HEIGHT;
	const uint32_t strip_count = (uint32_t)output_height / (uint32_t)flexa_lines;
	uint32_t strip_index;
	bool frame_failed = false;

	if (src_buffer == NULL || out_frame == NULL || out_frame_size == NULL ||
	    src_width == 0U || src_height == 0U) {
		return AVDK_ERR_INVAL;
	}

	vg_src_format = pp_gpu_rgb_src_format(src_format);
	if (vg_src_format == (vg_lite_buffer_format_t)-1) {
		LOGE("unsupported source format=%u\r\n", (unsigned)src_format);
		return AVDK_ERR_UNSUPPORTED;
	}

	ret = pp_gpu_display_gpu_blit_init();
	if (ret != AVDK_ERR_OK) {
		return ret;
	}

	frame_size = pp_gpu_display_compressed_argb_size(PP_EXAMPLE_GPU_DISPLAY_WIDTH,
							 PP_EXAMPLE_GPU_DISPLAY_HEIGHT);
	frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
	if (frame == NULL) {
		LOGE("alloc display frame failed, size=%u\r\n", (unsigned)frame_size);
		return AVDK_ERR_NOMEM;
	}

	os_memset(&src_buf, 0, sizeof(src_buf));
	os_memset(&dst_buf, 0, sizeof(dst_buf));
	os_memset(&matrix, 0, sizeof(matrix));

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
		if (src_format == BK_PIXEL_FORMAT_RGB565) {
			src_buf.stride = (vg_lite_int32_t)src_width * 2;
		} else if (src_format == BK_PIXEL_FORMAT_RGB888) {
			src_buf.stride = (vg_lite_int32_t)src_width * 4;
		}
		vg_ret = vg_lite_allocate_with_data(&src_buf, (void *)src_buffer, NULL, NULL, NULL);
	}
	if (vg_ret != VG_LITE_SUCCESS) {
		LOGE("wrap source failed, fmt=%u ret=%d\r\n", (unsigned)src_format, (int)vg_ret);
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	{
		float scale_x = (float)output_width / (float)src_width;
		float scale_y = (float)output_height / (float)src_height;

		vg_lite_identity(&matrix);
		if (rotate != 0U) {
			vg_lite_rotate((float)rotate, &matrix);
		}
		vg_lite_scale(scale_x, scale_y, &matrix);
	}

	if (rotate == 90U || rotate == 270U) {
		dst_buf.width = flexa_lines;
		dst_buf.height = output_width;
	} else {
		dst_buf.width = output_width;
		dst_buf.height = flexa_lines;
	}
	dst_buf.format = VG_LITE_BGRA8888;
#if PP_GPU_BLIT_COMPRESS
	dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
	dst_buf.tiled = VG_LITE_TILED;
#else
	dst_buf.compress_mode = VG_LITE_DEC_DISABLE;
	dst_buf.tiled = VG_LITE_LINEAR;
#endif
	s_gpu_blit_dma.dst_idx = 0U;
	vg_ret = vg_lite_allocate_with_data(&dst_buf,
					    (void *)(uintptr_t)s_gpu_blit_dma.buffers[0],
					    NULL, NULL, NULL);
	if (vg_ret != VG_LITE_SUCCESS) {
		LOGE("wrap strip dst failed, ret=%d\r\n", (int)vg_ret);
		(void)vg_lite_free_without_free_data(&src_buf);
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	start_ms = rtos_get_time();
	for (strip_index = 1U; strip_index <= strip_count; strip_index++) {
		if (rotate == 0U) {
			matrix.m[1][2] = -((float)(strip_index - 1U) * (float)flexa_lines);
		} else if (rotate == 90U) {
			matrix.m[0][2] = ((float)strip_index * (float)flexa_lines);
		} else {
			matrix.m[0][2] = -((float)(strip_index - 1U) * (float)flexa_lines);
			matrix.m[1][2] = (float)output_width;
		}

		vg_ret = vg_lite_blit(&dst_buf, &src_buf, &matrix,
				      VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
		if (vg_ret == VG_LITE_SUCCESS) {
			vg_ret = vg_lite_finish();
		}
		if (vg_ret != VG_LITE_SUCCESS) {
			LOGE("strip blit failed, strip=%u ret=%d\r\n",
			     (unsigned)strip_index, (int)vg_ret);
			frame_failed = true;
			break;
		}

		if (rtos_get_semaphore(&s_gpu_blit_dma.transfer_sem,
				       PP_GPU_BLIT_HPDMA_TIMEOUT_MS) != BK_OK) {
			LOGE("strip dma wait timeout, strip=%u\r\n", (unsigned)strip_index);
			frame_failed = true;
			break;
		}

		{
			uint32_t src_addr = (uint32_t)(uintptr_t)dst_buf.memory;

			if (rotate == 90U || rotate == 270U) {
				uint32_t stride = (uint32_t)(output_height - flexa_lines) *
						  PP_GPU_BLIT_PIXEL_SIZE;
#if PP_GPU_BLIT_COMPRESS
				uint32_t ysize = (uint32_t)output_width / 4U;
#else
				uint32_t ysize = (uint32_t)output_width;
#endif
				uint32_t xsize = (uint32_t)flexa_lines * PP_GPU_BLIT_PIXEL_SIZE;
				uint32_t offset = (rotate == 90U) ?
					((uint32_t)(output_height - strip_index * flexa_lines) *
					 PP_GPU_BLIT_PIXEL_SIZE) :
					((strip_index - 1U) * (uint32_t)flexa_lines *
					 PP_GPU_BLIT_PIXEL_SIZE);

				pp_gpu_blit_dma_transfer(src_addr, frame, offset,
							 xsize, ysize, stride);
			} else {
				uint32_t offset = (strip_index - 1U) * PP_GPU_BLIT_STRIP_BYTES;
				uint32_t xsize = (uint32_t)output_width * PP_GPU_BLIT_PIXEL_SIZE;
#if PP_GPU_BLIT_COMPRESS
				uint32_t ysize = (uint32_t)flexa_lines / 4U;
#else
				uint32_t ysize = (uint32_t)flexa_lines;
#endif
				pp_gpu_blit_dma_transfer(src_addr, frame, offset,
							 xsize, ysize, 0);
			}
		}

		s_gpu_blit_dma.dst_idx = 1U - s_gpu_blit_dma.dst_idx;
		dst_buf.memory = (vg_lite_pointer)(uintptr_t)s_gpu_blit_dma.buffers[s_gpu_blit_dma.dst_idx];
		dst_buf.address = (uint32_t)SOC_SRAM_PERI_ADDR(s_gpu_blit_dma.buffers[s_gpu_blit_dma.dst_idx]);
	}

	pp_gpu_blit_dma_sync();
	cost_ms = rtos_get_time() - start_ms;

	(void)vg_lite_free_without_free_data(&dst_buf);
	(void)vg_lite_free_without_free_data(&src_buf);

	if (frame_failed) {
		bk_frame_buffer_free(frame);
		return AVDK_ERR_GENERIC;
	}

	*out_frame = frame;
	*out_frame_size = frame_size;
	LOGI("gpu blit done: src=%ux%u fmt=%u dst=%ux%u rotate=%u cost=%u ms\r\n",
	     (unsigned)src_width, (unsigned)src_height, (unsigned)src_format,
	     (unsigned)PP_EXAMPLE_GPU_DISPLAY_WIDTH, (unsigned)PP_EXAMPLE_GPU_DISPLAY_HEIGHT,
	     (unsigned)rotate, (unsigned)cost_ms);
	return AVDK_ERR_OK;
}
