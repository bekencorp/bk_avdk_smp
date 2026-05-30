

/*
 * Implement MJPEG encoding stress test:
 * - CLI: mjpeg_encode_stress <start|stop>
 * - Start: allocate YUV input/output buffers, start PSRAM DMA stress, create encode thread
 * - Stop: set stop flag; thread deinit encoder, stop DMA and free buffers
 */

#include <stdint.h>
#include <string.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include "h264_encoder_api.h"
#include "psram_dma_stress.h"
#include "dwt.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#define TAG "mjpeg_enc_stress"

#define MJPEG_ENCODE_WIDTH     1920
#define MJPEG_ENCODE_HEIGHT    1080
#define MJPEG_ENCODE_Y_SIZE   ((uint32_t)(MJPEG_ENCODE_WIDTH) * (uint32_t)(MJPEG_ENCODE_HEIGHT))
#define MJPEG_ENCODE_YUV_SIZE ((uint32_t)(MJPEG_ENCODE_WIDTH) * (uint32_t)(MJPEG_ENCODE_HEIGHT) * 3U / 2U)

//#define MJPEG_FRAME_ENCODE_DEBUG    // open gpio debug for frame encode

#ifdef MJPEG_FRAME_ENCODE_DEBUG

#define MJPEG_FRAME_ENCODE_DEBUG_GPIO_INIT(id)  do { gpio_dev_unmap(id); bk_gpio_enable_output(id); bk_gpio_set_output_low(id);} while (0)

#define MJPEG_FRAME_ENCODE_DEBUG_INIT()  do { MJPEG_FRAME_ENCODE_DEBUG_GPIO_INIT(GPIO_32); } while (0)

#define MJPEG_FRAME_ENCODE_START()                            do { bk_gpio_set_output_low(GPIO_32); bk_gpio_set_output_high(GPIO_32);} while (0)
#define MJPEG_FRAME_ENCODE_END()                              do { bk_gpio_set_output_low(GPIO_32); } while (0)

#else

#define MJPEG_FRAME_ENCODE_DEBUG_INIT()

#define MJPEG_FRAME_ENCODE_START()
#define MJPEG_FRAME_ENCODE_END()

#endif


//#define MJPEG_FLEXA_ENCODE_DEBUG    // open gpio debug for flexa encode

#ifdef MJPEG_FLEXA_ENCODE_DEBUG

#define MJPEG_FLEXA_ENCODE_DEBUG_GPIO_INIT(id)  do { gpio_dev_unmap(id); bk_gpio_enable_output(id); bk_gpio_set_output_low(id);} while (0)

#define MJPEG_FLEXA_ENCODE_DEBUG_INIT()  do { MJPEG_FLEXA_ENCODE_DEBUG_GPIO_INIT(GPIO_32); MJPEG_FLEXA_ENCODE_DEBUG_GPIO_INIT(GPIO_33); } while (0)

#define MJPEG_FLEXA_ENCODE_START()                            do { bk_gpio_set_output_low(GPIO_32); bk_gpio_set_output_high(GPIO_32);} while (0)
#define MJPEG_FLEXA_ENCODE_END()                              do { bk_gpio_set_output_low(GPIO_32); } while (0)

#define MJPEG_FLEXA_ENCODE_LINE_START()                  do { bk_gpio_set_output_low(GPIO_33); bk_gpio_set_output_high(GPIO_33);} while (0)
#define MJPEG_FLEXA_ENCODE_LINE_DOWN()                    do { bk_gpio_set_output_low(GPIO_33); } while (0)

#else

#define MJPEG_FLEXA_ENCODE_DEBUG_INIT()

#define MJPEG_FLEXA_ENCODE_START()
#define MJPEG_FLEXA_ENCODE_END()

#define MJPEG_FLEXA_ENCODE_LINE_START()
#define MJPEG_FLEXA_ENCODE_LINE_DOWN()

#endif


#define CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO

/* Conservative output buffer size in bytes. */
#define MJPEG_ENCODE_OUT_BUF_SIZE ((uint32_t)(MJPEG_ENCODE_WIDTH) * (uint32_t)(MJPEG_ENCODE_HEIGHT))

/* NV12 input source selection.
 * 0: Use auto-generated NV12 image (default).
 * 1: Use flash-stored NV12 image, then copy into PSRAM input buffer before stress.
 */
#ifndef MJPEG_ENCODE_NV12_FROM_FLASH
#define MJPEG_ENCODE_NV12_FROM_FLASH 1
#endif

#if MJPEG_ENCODE_NV12_FROM_FLASH
/* NV12 flash image data (global const array).
 * The actual data can be filled in the header file.
 */
#include "mjpeg_encode_nv12_data.h"

static bk_err_t mjpeg_load_nv12_from_flash_to_psram(uint8_t *dst_psram)
{
	if (dst_psram == NULL) {
		bk_printf("%s: dst_psram is NULL\n", __func__);
		return BK_FAIL;
	}

	/* Copy full NV12 frame to PSRAM for encoder input. */
	memcpy(dst_psram, g_nv12_1920x1080_data, MJPEG_ENCODE_YUV_SIZE);
	return BK_OK;
}
#endif

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

static void *s_mjpeg_encode_context = NULL;
static beken_thread_t s_mjpeg_encode_stress_thread = NULL;
static volatile uint8_t s_mjpeg_encode_stress_stop = 0;

static uint8_t *s_yuv_buffer = NULL;
static uint8_t *s_output_buffer = NULL;

typedef enum {
	MJPEG_ENCODE_MODE_FRAME = 0,
	MJPEG_ENCODE_MODE_SW_FLEXA,
} mjpeg_encode_mode_t;

static mjpeg_encode_mode_t s_mjpeg_encode_mode = MJPEG_ENCODE_MODE_FRAME;

static uint8_t *s_dma_src_buffer = NULL;
static uint8_t *s_dma_dst_buffer = NULL;
static psram_dma_stress_handle_t s_psram_dma_handle = NULL;

static volatile uint8_t s_mjpeg_encode_error = 0;

#ifndef MEM_CACHABLE_MASK
#define MEM_CACHABLE_MASK ((uint32_t)0)
#endif

#define MJPEG_FLEXA_BLOCK_LINES   (16U)
#define MJPEG_FLEXA_RING_BLOCKS   (3U)

static uint8_t *s_flexa_src_yuv_aligned = NULL;
static uint32_t s_flexa_src_height_aligned = 0;
static uint8_t *s_flexa_ring_raw = NULL;
static uint8_t *s_flexa_ring_aligned = NULL;
static volatile uint32_t s_flexa_linebuf_count = 0;

/* stack_mem_dump is implemented in platform misc stack_base code. */
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

/* One-shot MJPEG flexa dump command context (keep isolated from stress globals). */
static volatile uint8_t s_mjpeg_one_shot_dump_mode = 0;
static beken_semaphore_t s_mjpeg_one_shot_dump_sem = NULL;
static uint8_t *s_mjpeg_one_shot_src_yuv_aligned = NULL;
static uint32_t s_mjpeg_one_shot_src_height_aligned = 0;
static uint8_t *s_mjpeg_one_shot_ring_raw = NULL;
static uint8_t *s_mjpeg_one_shot_ring_aligned = NULL;
static volatile uint32_t s_mjpeg_one_shot_linebuf_count = 0;


/**
 * 计算RGB到YUV的转换（BT.601标准，范围0-255）
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @param y 输出Y分量
 * @param u 输出U分量
 * @param v 输出V分量
 */
void rgb_to_yuv(uint8_t r, uint8_t g, uint8_t b, uint8_t *y, uint8_t *u, uint8_t *v)
{
    // 使用整数运算避免浮点误差
    int y_val = (66 * r + 129 * g + 25 * b + 128) >> 8;
    int u_val = (-38 * r - 74 * g + 112 * b + 128) >> 8;
    int v_val = (112 * r - 94 * g - 18 * b + 128) >> 8;
    
    // 添加偏移并限制范围
    *y = (uint8_t)((y_val + 16) & 0xFF);
    *u = (uint8_t)((u_val + 128) & 0xFF);
    *v = (uint8_t)((v_val + 128) & 0xFF);
}

/**
 * 生成NV12格式的纯红色图像（精确版本）
 * @param buffer 输出缓冲区，需要提前分配足够空间
 * @param width 图像宽度
 * @param height 图像高度
 * @return 0表示成功，-1表示参数无效
 */
int generate_nv12_red_image_precise(uint8_t *buffer, int width, int height)
{
    // 参数检查
    if (buffer == NULL || width <= 0 || height <= 0) {
        return -1;
    }
    
    // NV12格式的内存布局
    int y_size = width * height;
    int uv_size = y_size / 2;
    
    // 计算红色(255,0,0)对应的YUV值
    uint8_t y_red, u_red, v_red;
    rgb_to_yuv(255, 0, 0, &y_red, &u_red, &v_red);
    
    // 填充Y平面
    memset(buffer, y_red, y_size);
    
    // 填充UV平面（交错排列）
    uint8_t *uv_plane = buffer + y_size;
    
    for (int i = 0; i < uv_size; i += 2) {
        uv_plane[i] = u_red;     // U值
        uv_plane[i + 1] = v_red; // V值
    }
    
    return 0;
}

static void mjpeg_encode_out_callback(uint8_t *buffer, uint32_t size, uint32_t type)
{
	bk_printf("%s(%p, %d, %d)\n", __func__, buffer, size, type);
	// extern void bk_mem_dump_ex(const char *title, unsigned char *data, uint32_t data_len);
	// bk_mem_dump_ex("buffer", buffer, size);
}

static void mjpeg_one_shot_dump_bitstream(uint8_t *buffer, uint32_t size)
{
	uint32_t dump_aligned = (size + 3U) & ~3U;
	uint32_t irq_flags;

	if (buffer == NULL || size == 0) {
		return;
	}

	irq_flags = rtos_enter_critical();
	bk_printf("mjpeg_frame_dump size=%u buf=%p aligned=%u\n",
			  (unsigned)size,
			  buffer,
			  (unsigned)dump_aligned);
	stack_mem_dump((uint32_t)(uintptr_t)buffer,
				   (uint32_t)(uintptr_t)(buffer + dump_aligned));
	rtos_exit_critical(irq_flags);
}

static void mjpeg_one_shot_out_callback(uint8_t *buffer, uint32_t size, uint32_t type)
{
	(void)type;

	if (!s_mjpeg_one_shot_dump_mode) {
		return;
	}

	mjpeg_one_shot_dump_bitstream(buffer, size);

	if (s_mjpeg_one_shot_dump_sem != NULL) {
		(void)rtos_set_semaphore(&s_mjpeg_one_shot_dump_sem);
	}
}

static uint32_t mjpeg_one_shot_flexa_linebuf_done_callback(uint8_t *yDst, uint8_t *uDst, uint8_t *vDst)
{
	(void)vDst;
	/* NV12 (YUV420SP): copy one block (16 lines Y + 8 lines UV) into flexa ring buffer. */
	const uint32_t width = (uint32_t)MJPEG_ENCODE_WIDTH;
	const uint32_t height = s_mjpeg_one_shot_src_height_aligned;
	const uint32_t y_plane_size = width * height;
	const uint32_t block_y_bytes = width * MJPEG_FLEXA_BLOCK_LINES;
	const uint32_t block_uv_bytes = width * (MJPEG_FLEXA_BLOCK_LINES / 2U);

	if (s_mjpeg_one_shot_src_yuv_aligned == NULL || yDst == NULL || uDst == NULL) {
		return 0;
	}

	const uint32_t dst_slot = (uint32_t)(s_mjpeg_one_shot_linebuf_count % MJPEG_FLEXA_RING_BLOCKS);
	yDst += dst_slot * block_y_bytes;
	uDst += dst_slot * block_uv_bytes;

	const uint32_t start_y_line = (uint32_t)s_mjpeg_one_shot_linebuf_count * MJPEG_FLEXA_BLOCK_LINES;
	if (start_y_line >= height) {
		os_memset(yDst, 0, block_y_bytes);
		os_memset(uDst, 0x80, block_uv_bytes);
		s_mjpeg_one_shot_linebuf_count++;
		return 1;
	}

	const uint32_t src_y_off = start_y_line * width;
	const uint32_t src_uv_off = (start_y_line / 2U) * width;
	const uint8_t *src_y = s_mjpeg_one_shot_src_yuv_aligned + src_y_off;
	const uint8_t *src_uv = s_mjpeg_one_shot_src_yuv_aligned + y_plane_size + src_uv_off;

	os_memcpy(yDst, src_y, block_y_bytes);
	os_memcpy(uDst, src_uv, block_uv_bytes);

	s_mjpeg_one_shot_linebuf_count++;
	return 1;
}

static uint32_t mjpeg_encode_flexa_linebuf_done_callback(uint8_t *yDst, uint8_t *uDst, uint8_t *vDst)
{
	(void)vDst;
	MJPEG_FLEXA_ENCODE_LINE_DOWN();
#if 1
	/* NV12 (YUV420SP): copy one block (16 lines Y + 8 lines UV) into flexa ring buffer.
	 * The encoder provides a 3-block ring; select destination slot by (linebuf_count % 3).
	 */
	const uint32_t width = (uint32_t)MJPEG_ENCODE_WIDTH;
	const uint32_t height = s_flexa_src_height_aligned;
	const uint32_t y_plane_size = width * height;
	const uint32_t block_y_bytes = width * MJPEG_FLEXA_BLOCK_LINES;
	const uint32_t block_uv_bytes = width * (MJPEG_FLEXA_BLOCK_LINES / 2U);

	if (s_flexa_src_yuv_aligned == NULL || yDst == NULL || uDst == NULL) {
		return 0;
	}


	const uint32_t dst_slot = (uint32_t)(s_flexa_linebuf_count % MJPEG_FLEXA_RING_BLOCKS);
	yDst += dst_slot * block_y_bytes;
	uDst += dst_slot * block_uv_bytes;

	const uint32_t start_y_line = (uint32_t)s_flexa_linebuf_count * MJPEG_FLEXA_BLOCK_LINES;
	if (start_y_line >= height) {
		os_memset(yDst, 0, block_y_bytes);
		os_memset(uDst, 0x80, block_uv_bytes);
		s_flexa_linebuf_count++;
		MJPEG_FLEXA_ENCODE_LINE_START();
		return 1;
	}

	const uint32_t src_y_off = start_y_line * width;
	const uint32_t src_uv_off = (start_y_line / 2U) * width;
	const uint8_t *src_y = s_flexa_src_yuv_aligned + src_y_off;
	const uint8_t *src_uv = s_flexa_src_yuv_aligned + y_plane_size + src_uv_off;

	os_memcpy(yDst, src_y, block_y_bytes);
	os_memcpy(uDst, src_uv, block_uv_bytes);

	s_flexa_linebuf_count++;
#endif

	MJPEG_FLEXA_ENCODE_LINE_START();

	return 1;
}

static void mjpeg_encode_dma_stress_close(void)
{
	if (s_psram_dma_handle != NULL) {
		(void)psram_dma_stress_stop(s_psram_dma_handle);
		(void)psram_dma_stress_deinit(s_psram_dma_handle);
		s_psram_dma_handle = NULL;
	}

	if (s_dma_src_buffer != NULL) {
		bk_frame_buffer_free(s_dma_src_buffer);
		s_dma_src_buffer = NULL;
	}

	if (s_dma_dst_buffer != NULL) {
		bk_frame_buffer_free(s_dma_dst_buffer);
		s_dma_dst_buffer = NULL;
	}
}

static bk_err_t mjpeg_encode_dma_stress_start(void)
{
	bk_err_t ret;
	if (s_psram_dma_handle != NULL) {
		return BK_OK;
	}

	ret = psram_dma_stress_create(&s_psram_dma_handle);
	if (ret != BK_OK || s_psram_dma_handle == NULL) {
		bk_printf("%s: psram_dma_stress_create failed, ret=%d handle=%p\n", __func__, (int)ret, s_psram_dma_handle);
		s_psram_dma_handle = NULL;
		return BK_FAIL;
	}

	ret = psram_dma_stress_start(s_psram_dma_handle,
								  s_dma_src_buffer,
								  s_dma_dst_buffer,
								  MJPEG_ENCODE_YUV_SIZE);
	if (ret != BK_OK) {
		bk_printf("%s: psram_dma_stress_start failed, ret=%d\n", __func__, (int)ret);
		(void)psram_dma_stress_deinit(s_psram_dma_handle);
		s_psram_dma_handle = NULL;
		return BK_FAIL;
	}

	return BK_OK;
}

static void mjpeg_encode_stress_thread_entry(void *args)
{
	bk_err_t ret;
	uint32_t frame_index = 0;

	(void)args;

	s_mjpeg_encode_stress_stop = 0;
	s_mjpeg_encode_error = 0;

	if (s_mjpeg_encode_mode == MJPEG_ENCODE_MODE_SW_FLEXA) {
		MJPEG_FLEXA_ENCODE_DEBUG_INIT();
		ret = jpeg_encoder_init(&s_mjpeg_encode_context,
								 MJPEG_ENCODE_WIDTH,
								 MJPEG_ENCODE_HEIGHT,
								 VCENC_FLEXA_MODE_SOFTWARE,
								 mjpeg_encode_flexa_linebuf_done_callback,
								 mjpeg_encode_out_callback);
	} else {
		MJPEG_FRAME_ENCODE_DEBUG_INIT();
		ret = jpeg_encoder_init(&s_mjpeg_encode_context,
								 MJPEG_ENCODE_WIDTH,
								 MJPEG_ENCODE_HEIGHT,
								 VCENC_FLEXA_MODE_NONE,
								 NULL,
								 mjpeg_encode_out_callback);
	}
	if (ret != 0 || s_mjpeg_encode_context == NULL) {
		bk_printf("%s: jpeg_encoder_init failed, ret=%d handle=%p\n", __func__, (int)ret, s_mjpeg_encode_context);
		s_mjpeg_encode_stress_stop = 1;
		goto exit;
	}

	bk_printf("%s thread started, %ux%u\n", TAG, MJPEG_ENCODE_WIDTH, MJPEG_ENCODE_HEIGHT);

#ifdef CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO
	dwt_init_cycle_counter();
#endif

	while (!s_mjpeg_encode_stress_stop) {
#ifdef CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO
		uint32_t start_cycles = dwt_get_cycle_counter_val();
#endif

		if (s_mjpeg_encode_mode == MJPEG_ENCODE_MODE_SW_FLEXA) {
			if (s_flexa_ring_aligned == NULL || s_flexa_src_yuv_aligned == NULL) {
				bk_printf("%s: flexa buffers not ready\n", __func__);
				ret = BK_FAIL;
			} else {
				/* Pre-fill 3 blocks for the ring buffer (48 lines) before encoding. */
				const uint32_t width = (uint32_t)MJPEG_ENCODE_WIDTH;
				const uint32_t height = s_flexa_src_height_aligned;
				const uint32_t y_plane_size = width * height;
				const uint32_t y_prefill_bytes = width * MJPEG_FLEXA_BLOCK_LINES * MJPEG_FLEXA_RING_BLOCKS;
				const uint32_t uv_prefill_bytes = width * (MJPEG_FLEXA_BLOCK_LINES / 2U) * MJPEG_FLEXA_RING_BLOCKS;

				os_memcpy(s_flexa_ring_aligned, s_flexa_src_yuv_aligned, y_prefill_bytes);
				os_memcpy(s_flexa_ring_aligned + y_prefill_bytes, s_flexa_src_yuv_aligned + y_plane_size, uv_prefill_bytes);

				s_flexa_linebuf_count = 1;
				MJPEG_FLEXA_ENCODE_START();
				MJPEG_FLEXA_ENCODE_LINE_START();
				ret = jpeg_encoder_encode(s_mjpeg_encode_context,
										   (uint32_t)(uintptr_t)s_flexa_ring_aligned,
										   MJPEG_FLEXA_BLOCK_LINES * 1,
										   (uint32_t)s_output_buffer,
										   MJPEG_ENCODE_OUT_BUF_SIZE);
				MJPEG_FLEXA_ENCODE_END();
			}
		} else {
			MJPEG_FRAME_ENCODE_START();
			ret = jpeg_encoder_encode(s_mjpeg_encode_context,
									   (uint32_t)s_yuv_buffer,
									   0,
									   (uint32_t)s_output_buffer,
									   MJPEG_ENCODE_OUT_BUF_SIZE);
			MJPEG_FRAME_ENCODE_END();
		}

#ifdef CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO
		uint32_t end_cycles = dwt_get_cycle_counter_val();

		/* DWT cycles -> time (CPU frequency fixed as 480MHz). */
		uint32_t cycles = end_cycles - start_cycles;
		uint32_t us = cycles / 480U;
		if (us < 1000U) {
			bk_printf("%s: frame_encode_time frame_index=%u, %u us\n",
					  TAG, (unsigned)frame_index, (unsigned)us);
		} else {
			uint32_t ms = us / 1000U;
			bk_printf("%s: frame_encode_time frame_index=%u, %u ms\n",
					  TAG, (unsigned)frame_index, (unsigned)ms);
		}
#endif

		if (ret < 0 || s_mjpeg_encode_error) {
			bk_printf("%s: jpeg_encoder_encode failed, ret=%d frame_index=%u err=%u\n",
					  __func__,
					  (int)ret,
					  frame_index,
					  (unsigned)s_mjpeg_encode_error);
			s_mjpeg_encode_stress_stop = 1;
			break;
		}

		frame_index++;
		rtos_delay_milliseconds(20);
	}

	bk_printf("%s thread exit, frame_index=%u stop_flag=%u err=%u\n",
			  __func__,
			  frame_index,
			  (unsigned)s_mjpeg_encode_stress_stop,
			  (unsigned)s_mjpeg_encode_error);

exit:
	if (s_mjpeg_encode_context != NULL) {
		(void)jpeg_encoder_deinit(s_mjpeg_encode_context);
		s_mjpeg_encode_context = NULL;
	}

	mjpeg_encode_dma_stress_close();

	if (s_yuv_buffer != NULL) {
		bk_frame_buffer_free(s_yuv_buffer);
		s_yuv_buffer = NULL;
	}

	if (s_output_buffer != NULL) {
		bk_frame_buffer_free(s_output_buffer);
		s_output_buffer = NULL;
	}

	if (s_flexa_src_yuv_aligned != NULL) {
		bk_frame_buffer_free(s_flexa_src_yuv_aligned);
		s_flexa_src_yuv_aligned = NULL;
		s_flexa_src_height_aligned = 0;
	}

	if (s_flexa_ring_raw != NULL) {
		os_free(s_flexa_ring_raw);
		s_flexa_ring_raw = NULL;
		s_flexa_ring_aligned = NULL;
	}

#ifdef CONFIG_MJPEG_ENCODE_FRAME_ENCODE_TIME_AUTO
	dwt_disable_cycle_counter();
#endif

	s_mjpeg_encode_stress_thread = NULL;
	rtos_delete_thread(NULL);
}

void cli_mjpeg_encode_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	const char *msg = CLI_CMD_RSP_SUCCEED;

	if (argc < 3) {
		bk_printf("usage: mjpeg_encode_stress <start|stop> <frame|sw_flexa>\n");
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	if (os_strcmp(argv[2], "frame") == 0) {
		s_mjpeg_encode_mode = MJPEG_ENCODE_MODE_FRAME;
	} else if (os_strcmp(argv[2], "sw_flexa") == 0) {
		s_mjpeg_encode_mode = MJPEG_ENCODE_MODE_SW_FLEXA;
	} else {
		bk_printf("usage: mjpeg_encode_stress <start|stop> <frame|sw_flexa>\n");
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (s_mjpeg_encode_stress_thread != NULL) {
			bk_printf("%s: mjpeg encode stress thread already running\n", TAG);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		s_yuv_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_YUV_SIZE);
		if (s_yuv_buffer == NULL) {
			bk_printf("%s: s_yuv_buffer malloc failed\n", TAG);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		s_output_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_OUT_BUF_SIZE);
		if (s_output_buffer == NULL) {
			bk_printf("%s: s_output_buffer malloc failed\n", TAG);
			bk_frame_buffer_free(s_yuv_buffer);
			s_yuv_buffer = NULL;
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		s_dma_src_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_YUV_SIZE);
		if (s_dma_src_buffer == NULL) {
			bk_printf("%s: s_dma_src_buffer malloc failed\n", TAG);
			bk_frame_buffer_free(s_output_buffer);
			s_output_buffer = NULL;
			bk_frame_buffer_free(s_yuv_buffer);
			s_yuv_buffer = NULL;
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		s_dma_dst_buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_YUV_SIZE);
		if (s_dma_dst_buffer == NULL) {
			bk_printf("%s: s_dma_dst_buffer malloc failed\n", TAG);
			bk_frame_buffer_free(s_dma_src_buffer);
			s_dma_src_buffer = NULL;
			bk_frame_buffer_free(s_output_buffer);
			s_output_buffer = NULL;
			bk_frame_buffer_free(s_yuv_buffer);
			s_yuv_buffer = NULL;
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		bk_printf("%s: s_yuv_buffer=%p, s_output_buffer=%p\n", __func__, s_yuv_buffer, s_output_buffer);

#if MJPEG_ENCODE_NV12_FROM_FLASH
		/* Load NV12 from flash to PSRAM before starting stress. */
		ret = mjpeg_load_nv12_from_flash_to_psram(s_yuv_buffer);
		if (ret != BK_OK) {
			bk_printf("%s: mjpeg_load_nv12_from_flash_to_psram failed, ret=%d\n", __func__, (int)ret);
			mjpeg_encode_dma_stress_close();
			if (s_yuv_buffer != NULL) {
				bk_frame_buffer_free(s_yuv_buffer);
				s_yuv_buffer = NULL;
			}
			if (s_output_buffer != NULL) {
				bk_frame_buffer_free(s_output_buffer);
				s_output_buffer = NULL;
			}
			if (s_dma_src_buffer != NULL) {
				bk_frame_buffer_free(s_dma_src_buffer);
				s_dma_src_buffer = NULL;
			}
			if (s_dma_dst_buffer != NULL) {
				bk_frame_buffer_free(s_dma_dst_buffer);
				s_dma_dst_buffer = NULL;
			}
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
#else
		ret = generate_nv12_red_image_precise(s_yuv_buffer, MJPEG_ENCODE_WIDTH, MJPEG_ENCODE_HEIGHT);
		if (ret != 0) {
			bk_printf("%s: generate_nv12_red_image_precise failed, ret=%d\n", __func__, (int)ret);
			mjpeg_encode_dma_stress_close();
			if (s_yuv_buffer != NULL) {
				bk_frame_buffer_free(s_yuv_buffer);
				s_yuv_buffer = NULL;
			}
			if (s_output_buffer != NULL) {
				bk_frame_buffer_free(s_output_buffer);
				s_output_buffer = NULL;
			}
			if (s_dma_src_buffer != NULL) {
				bk_frame_buffer_free(s_dma_src_buffer);
				s_dma_src_buffer = NULL;
			}
			if (s_dma_dst_buffer != NULL) {
				bk_frame_buffer_free(s_dma_dst_buffer);
				s_dma_dst_buffer = NULL;
			}
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
#endif

		if (s_mjpeg_encode_mode == MJPEG_ENCODE_MODE_SW_FLEXA) {
			/* Flexa software mode requires height aligned to 16 lines. Prepare an aligned NV12 frame buffer. */
			const uint32_t aligned_h = (MJPEG_ENCODE_HEIGHT + (MJPEG_FLEXA_BLOCK_LINES - 1U)) & ~(MJPEG_FLEXA_BLOCK_LINES - 1U);
			const uint32_t width = (uint32_t)MJPEG_ENCODE_WIDTH;
			const uint32_t y_size_aligned = width * aligned_h;
			const uint32_t yuv_size_aligned = bk_image_size_get((uint16_t)width, (uint16_t)aligned_h, BK_PIXEL_FORMAT_NV12);

			s_flexa_src_height_aligned = aligned_h;
			s_flexa_src_yuv_aligned = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, yuv_size_aligned);
			if (s_flexa_src_yuv_aligned == NULL) {
				bk_printf("%s: flexa aligned yuv malloc failed, size=%u\n", TAG, (unsigned)yuv_size_aligned);
				ret = BK_FAIL;
				msg = CLI_CMD_RSP_ERROR;
				goto exit;
			}

			/* Copy original frame, then pad extra lines (if any) with the last valid line. */
			{
				const uint32_t orig_h = (uint32_t)MJPEG_ENCODE_HEIGHT;
				const uint32_t orig_y_size = width * orig_h;
				const uint32_t orig_uv_size = orig_y_size / 2U;
				const uint32_t aligned_uv_size = y_size_aligned / 2U;

				os_memcpy(s_flexa_src_yuv_aligned, s_yuv_buffer, orig_y_size + orig_uv_size);

				if (aligned_h > orig_h) {
					const uint32_t pad_lines = aligned_h - orig_h;
					const uint8_t *last_y = s_flexa_src_yuv_aligned + (orig_h - 1U) * width;
					uint8_t *pad_y = s_flexa_src_yuv_aligned + orig_h * width;
					for (uint32_t i = 0; i < pad_lines; i++) {
						os_memcpy(pad_y + i * width, last_y, width);
					}

					const uint32_t orig_uv_h = orig_h / 2U;
					const uint32_t aligned_uv_h = aligned_h / 2U;
					const uint32_t pad_uv_lines = aligned_uv_h - orig_uv_h;
					const uint8_t *last_uv = s_flexa_src_yuv_aligned + orig_y_size + (orig_uv_h - 1U) * width;
					uint8_t *pad_uv = s_flexa_src_yuv_aligned + y_size_aligned + orig_uv_h * width;
					for (uint32_t i = 0; i < pad_uv_lines; i++) {
						os_memcpy(pad_uv + i * width, last_uv, width);
					}

					(void)aligned_uv_size;
				}
			}

			/* Allocate 3-block ring buffer for flexa input (Y + UV), 64-byte aligned. */
			{
				const uint32_t ring_bytes = bk_image_size_get((uint16_t)width,
				                                              MJPEG_FLEXA_BLOCK_LINES * MJPEG_FLEXA_RING_BLOCKS,
				                                              BK_PIXEL_FORMAT_NV12);
				s_flexa_ring_raw = (uint8_t *)os_malloc(ring_bytes + 64U);
				if (s_flexa_ring_raw == NULL) {
					bk_printf("%s: flexa ring malloc failed, size=%u\n", TAG, (unsigned)(ring_bytes + 64U));
					ret = BK_FAIL;
					msg = CLI_CMD_RSP_ERROR;
					goto exit;
				}
				s_flexa_ring_aligned = (uint8_t *)((((uint32_t)(uintptr_t)s_flexa_ring_raw + (64U - 1U)) & ~(64U - 1U)) | MEM_CACHABLE_MASK);
			}
		}

		ret = mjpeg_encode_dma_stress_start();
		if (ret != BK_OK) {
			bk_printf("%s: mjpeg_encode_dma_stress_start failed\n", TAG);
			mjpeg_encode_dma_stress_close();
			if (s_yuv_buffer != NULL) {
				bk_frame_buffer_free(s_yuv_buffer);
				s_yuv_buffer = NULL;
			}
			if (s_output_buffer != NULL) {
				bk_frame_buffer_free(s_output_buffer);
				s_output_buffer = NULL;
			}
			if (s_dma_src_buffer != NULL) {
				bk_frame_buffer_free(s_dma_src_buffer);
				s_dma_src_buffer = NULL;
			}
			if (s_dma_dst_buffer != NULL) {
				bk_frame_buffer_free(s_dma_dst_buffer);
				s_dma_dst_buffer = NULL;
			}
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

#if MJPEG_ENCODE_NV12_FROM_FLASH
		bk_printf("%s: s_yuv_buffer=%p, s_output_buffer=%p\n", __func__, s_yuv_buffer, s_output_buffer);
		mjpeg_load_nv12_from_flash_to_psram(s_yuv_buffer);
#else
		bk_printf("%s: s_yuv_buffer=%p, s_output_buffer=%p\n", __func__, s_yuv_buffer, s_output_buffer);
		generate_nv12_red_image_precise(s_yuv_buffer, MJPEG_ENCODE_WIDTH, MJPEG_ENCODE_HEIGHT);
#endif

		s_mjpeg_encode_stress_stop = 0;

		ret = rtos_create_thread(&s_mjpeg_encode_stress_thread,
								  BEKEN_DEFAULT_WORKER_PRIORITY,
								  "mjpeg_encode_stress",
								  (beken_thread_function_t)mjpeg_encode_stress_thread_entry,
								  10 * 1024,
								  NULL);
		if (ret != BK_OK) {
			bk_printf("%s: create mjpeg encode stress thread failed, ret=%d\n", TAG, (int)ret);
			s_mjpeg_encode_stress_thread = NULL;
			mjpeg_encode_dma_stress_close();
			if (s_yuv_buffer != NULL) {
				bk_frame_buffer_free(s_yuv_buffer);
				s_yuv_buffer = NULL;
			}
			if (s_output_buffer != NULL) {
				bk_frame_buffer_free(s_output_buffer);
				s_output_buffer = NULL;
			}
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (s_mjpeg_encode_stress_thread == NULL) {
			bk_printf("%s: mjpeg encode stress thread not running\n", TAG);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		s_mjpeg_encode_stress_stop = 1;
	} else {
		bk_printf("usage: mjpeg_encode_stress <start|stop> <frame|sw_flexa>\n");
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

exit:
	/* If start failed before thread creation, release any flexa-only buffers allocated in CLI. */
	if (ret != BK_OK && s_mjpeg_encode_stress_thread == NULL) {
		if (s_flexa_src_yuv_aligned != NULL) {
			bk_frame_buffer_free(s_flexa_src_yuv_aligned);
			s_flexa_src_yuv_aligned = NULL;
			s_flexa_src_height_aligned = 0;
		}
		if (s_flexa_ring_raw != NULL) {
			os_free(s_flexa_ring_raw);
			s_flexa_ring_raw = NULL;
			s_flexa_ring_aligned = NULL;
		}
	}
	if (pcWriteBuffer && xWriteBufferLen > 0) {
		int len = os_strlen(msg);
		if (len >= xWriteBufferLen) {
			len = xWriteBufferLen - 1;
		}
		os_memcpy(pcWriteBuffer, msg, len);
		pcWriteBuffer[len] = '\0';
	}
}

void cli_mjpeg_flexa_dump_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	const char *msg = CLI_CMD_RSP_SUCCEED;
	void *encoder = NULL;
	uint8_t *yuv = NULL;
	uint8_t *out_buf = NULL;

	(void)argc;
	(void)argv;

	if (s_mjpeg_encode_stress_thread != NULL) {
		bk_printf("%s: stress thread running, stop it first\n", TAG);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	if (s_mjpeg_one_shot_dump_sem != NULL) {
		bk_printf("%s: one_shot dump already running\n", TAG);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	ret = rtos_init_semaphore(&s_mjpeg_one_shot_dump_sem, 1);
	if (ret != BK_OK) {
		bk_printf("%s: init semaphore failed, ret=%d\n", TAG, (int)ret);
		s_mjpeg_one_shot_dump_sem = NULL;
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	yuv = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_YUV_SIZE);
	if (yuv == NULL) {
		bk_printf("%s: yuv malloc failed\n", TAG);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

	out_buf = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, MJPEG_ENCODE_OUT_BUF_SIZE);
	if (out_buf == NULL) {
		bk_printf("%s: out_buf malloc failed\n", TAG);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

#if MJPEG_ENCODE_NV12_FROM_FLASH
	ret = mjpeg_load_nv12_from_flash_to_psram(yuv);
	if (ret != BK_OK) {
		bk_printf("%s: load nv12 failed, ret=%d\n", TAG, (int)ret);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}
#else
	ret = generate_nv12_red_image_precise(yuv, MJPEG_ENCODE_WIDTH, MJPEG_ENCODE_HEIGHT);
	if (ret != 0) {
		bk_printf("%s: generate nv12 failed, ret=%d\n", TAG, (int)ret);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}
#endif

	/* Prepare aligned NV12 source (height must be 16-line aligned for flexa). */
	{
		const uint32_t aligned_h = (MJPEG_ENCODE_HEIGHT + (MJPEG_FLEXA_BLOCK_LINES - 1U)) & ~(MJPEG_FLEXA_BLOCK_LINES - 1U);
		const uint32_t width = (uint32_t)MJPEG_ENCODE_WIDTH;
		const uint32_t y_size_aligned = width * aligned_h;
		const uint32_t yuv_size_aligned = bk_image_size_get((uint16_t)width, (uint16_t)aligned_h, BK_PIXEL_FORMAT_NV12);
		const uint32_t orig_h = (uint32_t)MJPEG_ENCODE_HEIGHT;
		const uint32_t orig_y_size = width * orig_h;
		const uint32_t orig_uv_size = orig_y_size / 2U;

		s_mjpeg_one_shot_src_height_aligned = aligned_h;
		s_mjpeg_one_shot_src_yuv_aligned = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, yuv_size_aligned);
		if (s_mjpeg_one_shot_src_yuv_aligned == NULL) {
			bk_printf("%s: aligned yuv malloc failed, size=%u\n", TAG, (unsigned)yuv_size_aligned);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}

		{
			uint8_t *dst_y = s_mjpeg_one_shot_src_yuv_aligned;
			uint8_t *dst_uv = s_mjpeg_one_shot_src_yuv_aligned + y_size_aligned;
			const uint8_t *src_y = yuv;
			const uint8_t *src_uv = yuv + orig_y_size;

			/* Match flexa callback addressing: UV must start at (aligned Y size). */
			os_memcpy(dst_y, src_y, orig_y_size);
			os_memcpy(dst_uv, src_uv, orig_uv_size);
		}

		if (aligned_h > orig_h) {
			const uint32_t pad_lines = aligned_h - orig_h;
			uint8_t *dst_y = s_mjpeg_one_shot_src_yuv_aligned;
			uint8_t *dst_uv = s_mjpeg_one_shot_src_yuv_aligned + y_size_aligned;
			const uint8_t *last_y = dst_y + (orig_h - 1U) * width;
			uint8_t *pad_y = dst_y + orig_h * width;
			for (uint32_t i = 0; i < pad_lines; i++) {
				os_memcpy(pad_y + i * width, last_y, width);
			}

			const uint32_t orig_uv_h = orig_h / 2U;
			const uint32_t aligned_uv_h = aligned_h / 2U;
			const uint32_t pad_uv_lines = aligned_uv_h - orig_uv_h;
			const uint8_t *last_uv = dst_uv + (orig_uv_h - 1U) * width;
			uint8_t *pad_uv = dst_uv + orig_uv_h * width;
			for (uint32_t i = 0; i < pad_uv_lines; i++) {
				os_memcpy(pad_uv + i * width, last_uv, width);
			}
		}

		/* Allocate 3-block ring buffer, 64-byte aligned. */
		{
			const uint32_t ring_bytes = bk_image_size_get((uint16_t)width,
			                                              MJPEG_FLEXA_BLOCK_LINES * MJPEG_FLEXA_RING_BLOCKS,
			                                              BK_PIXEL_FORMAT_NV12);
			s_mjpeg_one_shot_ring_raw = (uint8_t *)os_malloc(ring_bytes + 64U);
			if (s_mjpeg_one_shot_ring_raw == NULL) {
				bk_printf("%s: ring malloc failed, size=%u\n", TAG, (unsigned)(ring_bytes + 64U));
				ret = BK_FAIL;
				msg = CLI_CMD_RSP_ERROR;
				goto exit;
			}
			s_mjpeg_one_shot_ring_aligned = (uint8_t *)((((uint32_t)(uintptr_t)s_mjpeg_one_shot_ring_raw + (64U - 1U)) & ~(64U - 1U)) | MEM_CACHABLE_MASK);

			/* Prefill 3 blocks (48 lines) for flexa ring startup stability. */
			const uint32_t y_plane_size = width * aligned_h;
			const uint32_t y_prefill_bytes = width * MJPEG_FLEXA_BLOCK_LINES * MJPEG_FLEXA_RING_BLOCKS;
			const uint32_t uv_prefill_bytes = width * (MJPEG_FLEXA_BLOCK_LINES / 2U) * MJPEG_FLEXA_RING_BLOCKS;
			os_memcpy(s_mjpeg_one_shot_ring_aligned, s_mjpeg_one_shot_src_yuv_aligned, y_prefill_bytes);
			os_memcpy(s_mjpeg_one_shot_ring_aligned + y_prefill_bytes, s_mjpeg_one_shot_src_yuv_aligned + y_plane_size, uv_prefill_bytes);
			s_mjpeg_one_shot_linebuf_count = MJPEG_FLEXA_RING_BLOCKS;
		}
	}

	{
		int32_t init_ret = jpeg_encoder_init(&encoder,
											 MJPEG_ENCODE_WIDTH,
											 MJPEG_ENCODE_HEIGHT,
											 VCENC_FLEXA_MODE_SOFTWARE,
											 mjpeg_one_shot_flexa_linebuf_done_callback,
											 mjpeg_one_shot_out_callback);
		if (init_ret != 0 || encoder == NULL) {
			bk_printf("%s: jpeg_encoder_init failed, ret=%d encoder=%p\n", TAG, (int)init_ret, encoder);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
	}

	s_mjpeg_one_shot_dump_mode = 1;

	{
		int32_t enc_ret = jpeg_encoder_encode(encoder,
											  (uint32_t)(uintptr_t)s_mjpeg_one_shot_ring_aligned,
											  MJPEG_FLEXA_BLOCK_LINES * MJPEG_FLEXA_RING_BLOCKS,
											  (uint32_t)(uintptr_t)out_buf,
											  MJPEG_ENCODE_OUT_BUF_SIZE);
		if (enc_ret < 0) {
			bk_printf("%s: jpeg_encoder_encode failed, ret=%d\n", TAG, (int)enc_ret);
			ret = BK_FAIL;
			msg = CLI_CMD_RSP_ERROR;
			goto exit;
		}
	}

	ret = rtos_get_semaphore(&s_mjpeg_one_shot_dump_sem, 5000);
	if (ret != BK_OK) {
		bk_printf("%s: wait dump timeout, ret=%d\n", TAG, (int)ret);
		ret = BK_FAIL;
		msg = CLI_CMD_RSP_ERROR;
		goto exit;
	}

exit:
	s_mjpeg_one_shot_dump_mode = 0;
	if (encoder != NULL) {
		(void)jpeg_encoder_deinit(encoder);
		encoder = NULL;
	}
	if (out_buf != NULL) {
		bk_frame_buffer_free(out_buf);
		out_buf = NULL;
	}
	if (yuv != NULL) {
		bk_frame_buffer_free(yuv);
		yuv = NULL;
	}
	if (s_mjpeg_one_shot_src_yuv_aligned != NULL) {
		bk_frame_buffer_free(s_mjpeg_one_shot_src_yuv_aligned);
		s_mjpeg_one_shot_src_yuv_aligned = NULL;
		s_mjpeg_one_shot_src_height_aligned = 0;
	}
	if (s_mjpeg_one_shot_ring_raw != NULL) {
		os_free(s_mjpeg_one_shot_ring_raw);
		s_mjpeg_one_shot_ring_raw = NULL;
		s_mjpeg_one_shot_ring_aligned = NULL;
	}
	if (s_mjpeg_one_shot_dump_sem != NULL) {
		(void)rtos_deinit_semaphore(&s_mjpeg_one_shot_dump_sem);
		s_mjpeg_one_shot_dump_sem = NULL;
	}

	if (pcWriteBuffer && xWriteBufferLen > 0) {
		int len = os_strlen(msg);
		if (len >= xWriteBufferLen) {
			len = xWriteBufferLen - 1;
		}
		os_memcpy(pcWriteBuffer, msg, len);
		pcWriteBuffer[len] = '\0';
	}
}