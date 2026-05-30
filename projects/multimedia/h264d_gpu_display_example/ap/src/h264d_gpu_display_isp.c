#include "h264d_gpu_display_isp.h"

#if H264D_GPU_DISPLAY_ENABLE_ISP_PIP

#include <os/os.h>
#include <os/mem.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_camera_bus.h>
#include <components/bk_camera_sensor.h>
#include <components/bk_camera_configs.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_gpu.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <driver/isp_base.h>

#include "h264d_gpu_display_gpu.h"

#define TAG "h264d_isp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define ISP_TASK_PRIORITY      5U
#define ISP_TASK_STACK_SIZE    (1024U * 4U)
#define ISP_READ_TIMEOUT_MS    200U

/* Preallocated ISP (PIP) frame buffer pool. */
#define ISP_FRAME_POOL_COUNT   3U
#define ISP_FRAME_POOL_HEAP    MEM_SLAB_HEAP_UNCODED

#define ISP_PIXEL_FORMAT      BK_PIXEL_FORMAT_NV12
#define ISP_GPU_BLIT_SRC_FORMAT   BK_PIXEL_FORMAT_NV12

static inline uint16_t isp_visible_width(uint16_t isp_w, uint16_t isp_h)
{
#if H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE == 90U || H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE == 270U
	(void)isp_w;
	return isp_h;
#else
	(void)isp_h;
	return isp_w;
#endif
}

static inline uint16_t isp_visible_height(uint16_t isp_w, uint16_t isp_h)
{
#if H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE == 90U || H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE == 270U
	(void)isp_h;
	return isp_w;
#else
	(void)isp_w;
	return isp_h;
#endif
}

#define ISP_SENSOR_PIN_RESET   GPIO_71
#define ISP_SENSOR_PIN_PWDN    0xFF      /* no PWDN line wired */
#define ISP_SENSOR_PIN_XCLK    GPIO_59

typedef struct {
	beken_semaphore_t done_sem;
	beken_semaphore_t blit_release_sem;
	beken_thread_t task;
	volatile uint8_t running;
	volatile uint8_t opened;

	bk_camera_bus_t *bus;
	bk_camera_sensor_handle_t sensor;
	bk_isp_camera_ctlr_handle_t ctlr;

	uint16_t sensor_w;
	uint16_t sensor_h;
	uint16_t fps;
	uint16_t isp_w;
	uint16_t isp_h;
	uint16_t dst_x;
	uint16_t dst_y;
	uint32_t isp_frame_size;
} h264d_gpu_display_isp_ctx_t;

typedef struct {
	void *buf;
	uint8_t in_use;
} isp_frame_pool_entry_t;

static isp_frame_pool_entry_t s_isp_frame_pool[ISP_FRAME_POOL_COUNT];
static uint32_t s_isp_frame_pool_buf_size;
static uint32_t s_isp_frame_pool_init_count;

#define ISP_RELEASE_WAIT_MS  200U
#define ISP_FREE_PRIME_COUNT  2U

static h264d_gpu_display_isp_ctx_t s_isp_ctx = {0};

static void *isp_frame_alloc(uint32_t size)
{
	void *ptr = NULL;
	uint32_t flags;
	uint32_t i;

	flags = rtos_enter_critical();
	if (size == s_isp_frame_pool_buf_size && s_isp_frame_pool_init_count != 0U) {
		for (i = 0U; i < s_isp_frame_pool_init_count; i++) {
			if (s_isp_frame_pool[i].buf != NULL && s_isp_frame_pool[i].in_use == 0U) {
				s_isp_frame_pool[i].in_use = 1U;
				ptr = s_isp_frame_pool[i].buf;
				break;
			}
		}
	}
	rtos_exit_critical(flags);

	if (ptr == NULL) {
		ptr = bk_frame_buffer_malloc(ISP_FRAME_POOL_HEAP, size);
		if (ptr == NULL) {
			/* Last-ditch: try the other heap before giving up. */
			ptr = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);
		}
	}
	return ptr;
}

static void isp_frame_free(void *ptr)
{
	uint32_t flags;
	uint8_t from_pool = 0U;
	uint32_t i;

	if (ptr == NULL) {
		return;
	}

	flags = rtos_enter_critical();
	for (i = 0U; i < s_isp_frame_pool_init_count; i++) {
		if (s_isp_frame_pool[i].buf == ptr) {
			s_isp_frame_pool[i].in_use = 0U;
			from_pool = 1U;
			break;
		}
	}
	rtos_exit_critical(flags);

	if (from_pool == 0U) {
		bk_frame_buffer_free(ptr);
	}
}

static avdk_err_t isp_frame_pool_init(uint32_t buf_size)
{
	uint32_t i;
	uint32_t flags;

	if (buf_size == 0U) {
		return AVDK_ERR_INVAL;
	}

	if (s_isp_frame_pool_buf_size == buf_size && s_isp_frame_pool_init_count == ISP_FRAME_POOL_COUNT) {
		flags = rtos_enter_critical();
		for (i = 0U; i < s_isp_frame_pool_init_count; i++) {
			if (s_isp_frame_pool[i].in_use != 0U) {
				LOGW("isp pool slot %u still in use during reinit\r\n", (unsigned)i);
			}
			s_isp_frame_pool[i].in_use = 0U;
		}
		rtos_exit_critical(flags);
		LOGI("isp pool reused: %u slots x %u bytes\r\n", (unsigned)s_isp_frame_pool_init_count, (unsigned)buf_size);
		return AVDK_ERR_OK;
	}

	for (i = 0U; i < s_isp_frame_pool_init_count; i++) {
		if (s_isp_frame_pool[i].buf != NULL) {
			if (s_isp_frame_pool[i].in_use != 0U) {
				LOGW("isp pool slot %u still in use during free, leaking\r\n", (unsigned)i);
			} else {
				bk_frame_buffer_free(s_isp_frame_pool[i].buf);
			}
			s_isp_frame_pool[i].buf = NULL;
			s_isp_frame_pool[i].in_use = 0U;
		}
	}
	s_isp_frame_pool_init_count = 0U;
	s_isp_frame_pool_buf_size = 0U;

	for (i = 0U; i < ISP_FRAME_POOL_COUNT; i++) {
		s_isp_frame_pool[i].buf = bk_frame_buffer_malloc(ISP_FRAME_POOL_HEAP, buf_size);
		s_isp_frame_pool[i].in_use = 0U;
		if (s_isp_frame_pool[i].buf == NULL) {
			LOGE("isp pool init: alloc slot %u (size=%u) failed\r\n",
			     (unsigned)i, (unsigned)buf_size);
			for (uint32_t j = 0U; j < i; j++) {
				bk_frame_buffer_free(s_isp_frame_pool[j].buf);
				s_isp_frame_pool[j].buf = NULL;
			}
			return AVDK_ERR_NOMEM;
		}
	}
	s_isp_frame_pool_init_count = ISP_FRAME_POOL_COUNT;
	s_isp_frame_pool_buf_size = buf_size;
	LOGI("isp pool ready: %u slots x %u bytes = %u total\r\n",
	     (unsigned)ISP_FRAME_POOL_COUNT,
	     (unsigned)buf_size,
	     (unsigned)(ISP_FRAME_POOL_COUNT * buf_size));
	return AVDK_ERR_OK;
}

static void isp_frame_pool_deinit(void)
{
	uint32_t i;

	for (i = 0U; i < s_isp_frame_pool_init_count; i++) {
		if (s_isp_frame_pool[i].buf != NULL) {
			if (s_isp_frame_pool[i].in_use != 0U) {
				LOGW("isp pool slot %u still in use during deinit, leaking\r\n",
				     (unsigned)i);
			} else {
				bk_frame_buffer_free(s_isp_frame_pool[i].buf);
			}
			s_isp_frame_pool[i].buf = NULL;
			s_isp_frame_pool[i].in_use = 0U;
		}
	}
	s_isp_frame_pool_init_count = 0U;
	s_isp_frame_pool_buf_size = 0U;
}

static void isp_blit_free_cb(void *frame, void *args)
{
	(void)args;
	if (frame != NULL) {
		isp_frame_free(frame);
	}
	if (s_isp_ctx.blit_release_sem != NULL) {
		(void)rtos_set_semaphore(&s_isp_ctx.blit_release_sem);
	}
}

static bk_isp_camera_channel_config_t isp_channel_config_build(uint16_t isp_w, uint16_t isp_h)
{
	bk_isp_camera_channel_config_t cfg = CAM_MP_NV12_RB_INSTANCE_CONFIG(isp_w, isp_h);

	cfg.port_id = ISP_MIPI_PORT_ID;
	cfg.enable_flexa = 0U;
	cfg.work_mode = 0U;       /* frame mode */
	cfg.buf_cnt = 2U;
	cfg.width = isp_w;
	cfg.height = isp_h;
	cfg.format = ISP_PIXEL_FORMAT;
	return cfg;
}

static avdk_err_t isp_validate_sensor_resolution(uint16_t sensor_w, uint16_t sensor_h, uint16_t fps)
{
	bk_camera_sensor_format_array_t fmts = {0};
	uint16_t max_w = 0U;
	uint16_t max_h = 0U;
	uint8_t exact = 0U;

	if (s_isp_ctx.sensor == NULL) {
		return AVDK_ERR_INVAL;
	}

	if (bk_camera_sensor_query_support_formats(s_isp_ctx.sensor, &fmts) != AVDK_ERR_OK) {
		LOGW("query sensor formats failed, skip validation\r\n");
		return AVDK_ERR_OK;
	}

	for (uint32_t i = 0U; i < fmts.size; i++) {
		uint16_t fw = fmts.format_array[i].width;
		uint16_t fh = fmts.format_array[i].height;
		uint16_t ff = fmts.format_array[i].fps;

		if (fw > max_w) {
			max_w = fw;
		}
		if (fh > max_h) {
			max_h = fh;
		}
		if (fw == sensor_w && fh == sensor_h && ff == fps) {
			exact = 1U;
			break;
		}
	}

	if (!exact) {
		LOGE("sensor does not support %ux%u@%ufps\r\n", (unsigned)sensor_w, (unsigned)sensor_h, (unsigned)fps);
		for (uint32_t i = 0U; i < fmts.size; i++) {
			LOGE("  supported: %ux%u@%ufps\r\n",
			     (unsigned)fmts.format_array[i].width,
			     (unsigned)fmts.format_array[i].height,
			     (unsigned)fmts.format_array[i].fps);
		}
		return AVDK_ERR_INVAL;
	}

	if (max_w != 0U && max_h != 0U) {
		LOGI("sensor max=%ux%u, requested sensor input=%ux%u\r\n",
		     (unsigned)max_w, (unsigned)max_h,
		     (unsigned)sensor_w, (unsigned)sensor_h);
	}

	return AVDK_ERR_OK;
}

static void isp_overlay_task_entry(void *arg)
{
	uint32_t push_count = 0U;

	(void)arg;
	LOGI("isp_overlay_task started, isp=%ux%u visible=%ux%u rotate=%u dst=(%u,%u) frame_size=%u\r\n",
	     (unsigned)s_isp_ctx.isp_w, (unsigned)s_isp_ctx.isp_h,
	     (unsigned)isp_visible_width(s_isp_ctx.isp_w, s_isp_ctx.isp_h),
	     (unsigned)isp_visible_height(s_isp_ctx.isp_w, s_isp_ctx.isp_h),
	     (unsigned)H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE,
	     (unsigned)s_isp_ctx.dst_x, (unsigned)s_isp_ctx.dst_y,
	     (unsigned)s_isp_ctx.isp_frame_size);

	while (s_isp_ctx.running != 0U) {
		uint32_t frame_size;
		uint16_t cur_w;
		uint16_t cur_h;
		uint16_t cur_dst_x;
		uint16_t cur_dst_y;
		bk_isp_camera_ctlr_handle_t ctlr_local;
		uint8_t *frame_buf;
		bk_gpu_blit_config_t blit;
		bk_gpu_ctlr_handle_t gpu;
		avdk_err_t ret;

		frame_size = s_isp_ctx.isp_frame_size;
		cur_w = s_isp_ctx.isp_w;
		cur_h = s_isp_ctx.isp_h;
		cur_dst_x = s_isp_ctx.dst_x;
		cur_dst_y = s_isp_ctx.dst_y;
		ctlr_local = s_isp_ctx.ctlr;

		if (frame_size == 0U || ctlr_local == NULL) {
			rtos_delay_milliseconds(20U);
			continue;
		}

		if (push_count >= ISP_FREE_PRIME_COUNT &&
		    s_isp_ctx.blit_release_sem != NULL) {
			(void)rtos_get_semaphore(&s_isp_ctx.blit_release_sem,
						 ISP_RELEASE_WAIT_MS);
		}

		frame_buf = (uint8_t *)isp_frame_alloc(frame_size);
		if (frame_buf == NULL) {
			rtos_delay_milliseconds(20U);
			continue;
		}

		ret = bk_isp_camera_read(ctlr_local, ISP_SP_CHN_ID, frame_buf, frame_size, ISP_READ_TIMEOUT_MS);
		if (ret != AVDK_ERR_OK) {
			isp_frame_free(frame_buf);
			continue;
		}

		gpu = h264d_gpu_display_gpu_handle_get();
		if (gpu == NULL) {
			/* H.264 pipeline not running yet, drop frame and wait. */
			isp_frame_free(frame_buf);
			rtos_delay_milliseconds(20U);
			continue;
		}
		os_memset(&blit, 0, sizeof(blit));
		blit.src_x = 0U;
		blit.src_y = 0U;
		blit.src_width = cur_w;
		blit.src_height = cur_h;
		blit.src_format = ISP_GPU_BLIT_SRC_FORMAT;
		blit.dst_x = cur_dst_x;
		blit.dst_y = cur_dst_y;
		blit.rotate_degree = H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE;
		blit.args = NULL;
		blit.free = isp_blit_free_cb;

		ret = bk_gpu_blit_set(gpu, frame_buf, &blit);
		if (ret != AVDK_ERR_OK) {
			LOGW("isp_overlay_task: bk_gpu_blit_set failed=%d, drop frame\r\n", (int)ret);
			isp_frame_free(frame_buf);
			rtos_delay_milliseconds(5U);
		} else {
			push_count++;
		}
	}

	LOGI("isp_overlay_task exiting\r\n");

	if (s_isp_ctx.done_sem != NULL) {
		(void)rtos_set_semaphore(&s_isp_ctx.done_sem);
	}
	s_isp_ctx.task = NULL;
	rtos_delete_thread(NULL);
}

static avdk_err_t isp_resources_setup(const h264d_gpu_display_isp_params_t *params)
{
	avdk_err_t ret;
	bk_camera_bus_config_t bus_config = (bk_camera_bus_config_t)CSI_CAM_BUS_I2C1_8BIT_2000TIMEOUT();
	bk_camera_sensor_config_t sensor_config = {
		.pin_reset = ISP_SENSOR_PIN_RESET,
		.pin_pwdn  = ISP_SENSOR_PIN_PWDN,
		.pin_xclk  = ISP_SENSOR_PIN_XCLK,
		.bus       = NULL,
	};

	s_isp_ctx.bus = bk_camera_bus_new(&bus_config);
	if (s_isp_ctx.bus == NULL) {
		LOGE("bk_camera_bus_new failed\r\n");
		return AVDK_ERR_GENERIC;
	}

	ret = bk_camera_bus_enable(s_isp_ctx.bus);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_camera_bus_enable failed=%d\r\n", (int)ret);
		return ret;
	}

	sensor_config.bus = s_isp_ctx.bus;
	s_isp_ctx.sensor = bk_camera_sensor_auto_detect(&sensor_config, CSI_CAMERA_PORT);
	if (s_isp_ctx.sensor == NULL) {
		LOGE("no CSI sensor detected\r\n");
		return AVDK_ERR_NODEV;
	}

	ret = isp_validate_sensor_resolution(params->sensor_w, params->sensor_h, params->fps);
	if (ret != AVDK_ERR_OK) {
		return ret;
	}

	bk_isp_camera_ctlr_config_t port_cfg =
		CAM_CSI_DEFAULT_RAW10_CONFIG(params->sensor_w, params->sensor_h, params->fps);
	port_cfg.sensor_object = bk_camera_sensor_get_sensor_object(s_isp_ctx.sensor);
	if (port_cfg.sensor_object == NULL) {
		LOGE("get sensor object failed\r\n");
		return AVDK_ERR_GENERIC;
	}

	bk_camera_sensor_format_array_t fmt_arr = {0};
	avdk_err_t qret = bk_camera_sensor_query_support_formats(s_isp_ctx.sensor, &fmt_arr);

	if (qret != AVDK_ERR_OK || fmt_arr.size == 0U) {
		LOGE("sensor query_support_formats failed=%d size=%u\r\n",
				(int)qret, (unsigned)fmt_arr.size);
		return (qret != AVDK_ERR_OK) ? qret : AVDK_ERR_GENERIC;
	}
	port_cfg.input_pixel_fmt = fmt_arr.format_array[0].output_pixel_fmt;
	for (uint32_t i = 0U; i < fmt_arr.size; i++) {
		if (fmt_arr.format_array[i].width == params->sensor_w
			&& fmt_arr.format_array[i].height == params->sensor_h
			&& fmt_arr.format_array[i].fps == params->fps) {
			port_cfg.input_pixel_fmt = fmt_arr.format_array[i].output_pixel_fmt;
			break;
		}
	}

	ret = bk_camera_isp_ctlr_new(&s_isp_ctx.ctlr);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_camera_isp_ctlr_new failed=%d\r\n", (int)ret);
		return ret;
	}

	ret = bk_isp_camera_dev_init(s_isp_ctx.ctlr);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_isp_camera_dev_init failed=%d\r\n", (int)ret);
		return ret;
	}

	ret = bk_isp_camera_port_init(s_isp_ctx.ctlr, &port_cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_isp_camera_port_init failed=%d\r\n", (int)ret);
		return ret;
	}

	bk_isp_camera_channel_config_t isp = isp_channel_config_build(params->isp_w, params->isp_h);

	ret = bk_isp_camera_channel_open(s_isp_ctx.ctlr, ISP_SP_CHN_ID, &isp);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_isp_camera_channel_open(ISP %ux%u) failed=%d\r\n",
		     (unsigned)params->isp_w, (unsigned)params->isp_h, (int)ret);
		return ret;
	}

	ret = bk_camera_sensor_init(s_isp_ctx.sensor);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_camera_sensor_init failed=%d\r\n", (int)ret);
		return ret;
	}

	bk_camera_sensor_format_t fmt = {
		.width  = params->sensor_w,
		.height = params->sensor_h,
		.fps    = params->fps,
		.xclk   = 0U,
	};
	ret = bk_camera_sensor_set_format(s_isp_ctx.sensor, &fmt);
	if (ret != AVDK_ERR_OK) {
		LOGE("bk_camera_sensor_set_format failed=%d\r\n", (int)ret);
		return ret;
	}

	s_isp_ctx.sensor_w = params->sensor_w;
	s_isp_ctx.sensor_h = params->sensor_h;
	s_isp_ctx.fps      = params->fps;
	s_isp_ctx.isp_w     = params->isp_w;
	s_isp_ctx.isp_h     = params->isp_h;
	s_isp_ctx.dst_x    = params->dst_x;
	s_isp_ctx.dst_y    = params->dst_y;
	s_isp_ctx.isp_frame_size = (uint32_t)params->isp_w * (uint32_t)params->isp_h * 3U / 2U;

	LOGI("isp resources ready: sensor=%ux%u@%ufps isp=%ux%u visible=%ux%u rotate=%u dst=(%u,%u) frame_size=%u\r\n",
	     (unsigned)params->sensor_w, (unsigned)params->sensor_h, (unsigned)params->fps,
	     (unsigned)params->isp_w, (unsigned)params->isp_h,
	     (unsigned)isp_visible_width(params->isp_w, params->isp_h),
	     (unsigned)isp_visible_height(params->isp_w, params->isp_h),
	     (unsigned)H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE,
	     (unsigned)params->dst_x, (unsigned)params->dst_y,
	     (unsigned)s_isp_ctx.isp_frame_size);

	return AVDK_ERR_OK;
}

static void isp_resources_teardown(void)
{
	if (s_isp_ctx.ctlr != NULL) {
		(void)bk_isp_camera_channel_close(s_isp_ctx.ctlr, ISP_SP_CHN_ID);
		(void)bk_isp_camera_deinit(s_isp_ctx.ctlr);
		(void)bk_isp_camera_delete(s_isp_ctx.ctlr);
		s_isp_ctx.ctlr = NULL;
	}

	if (s_isp_ctx.sensor != NULL) {
		bk_camera_sensor_destroy(s_isp_ctx.sensor);
		s_isp_ctx.sensor = NULL;
	}

	if (s_isp_ctx.bus != NULL) {
		(void)bk_camera_bus_disable(s_isp_ctx.bus);
		(void)bk_camera_bus_delete(s_isp_ctx.bus);
		s_isp_ctx.bus = NULL;
	}

	s_isp_ctx.sensor_w = 0U;
	s_isp_ctx.sensor_h = 0U;
	s_isp_ctx.fps = 0U;
	s_isp_ctx.isp_w = 0U;
	s_isp_ctx.isp_h = 0U;
	s_isp_ctx.isp_frame_size = 0U;
}

avdk_err_t h264d_gpu_display_isp_open(const h264d_gpu_display_isp_params_t *params)
{
	avdk_err_t ret;
	bk_err_t bk_ret;

	if (params == NULL) {
		return AVDK_ERR_INVAL;
	}
	if (params->sensor_w == 0U || params->sensor_h == 0U || params->fps == 0U ||
	    params->isp_w == 0U || params->isp_h == 0U) {
		return AVDK_ERR_INVAL;
	}
	if ((params->isp_w & 1U) != 0U || (params->isp_h & 1U) != 0U) {
		LOGE("ISP %ux%u is invalid for NV12 (width/height must be even)\r\n",
		     (unsigned)params->isp_w, (unsigned)params->isp_h);
		return AVDK_ERR_INVAL;
	}
	if (params->isp_w > params->sensor_w || params->isp_h > params->sensor_h) {
		LOGE("ISP %ux%u exceeds sensor %ux%u (no upscale)\r\n",
		     (unsigned)params->isp_w, (unsigned)params->isp_h,
		     (unsigned)params->sensor_w, (unsigned)params->sensor_h);
		return AVDK_ERR_INVAL;
	}

	if (s_isp_ctx.opened != 0U) {
		LOGW("isp already open, call isp_close first\r\n");
		return AVDK_ERR_GENERIC;
	}

	if (s_isp_ctx.done_sem == NULL) {
		bk_ret = rtos_init_semaphore(&s_isp_ctx.done_sem, 1);
		if (bk_ret != BK_OK) {
			LOGE("init done_sem failed=%d\r\n", (int)bk_ret);
			return AVDK_ERR_GENERIC;
		}
	}
	if (s_isp_ctx.blit_release_sem == NULL) {
		bk_ret = rtos_init_semaphore(&s_isp_ctx.blit_release_sem, 1);
		if (bk_ret != BK_OK) {
			LOGE("init blit_release_sem failed=%d\r\n", (int)bk_ret);
			return AVDK_ERR_GENERIC;
		}
	} else {
		/* Drain stale signal from a previous isp_open/isp_close cycle. */
		(void)rtos_get_semaphore(&s_isp_ctx.blit_release_sem, 0U);
	}

	ret = isp_resources_setup(params);
	if (ret != AVDK_ERR_OK) {
		LOGE("isp resources setup failed=%d, rolling back\r\n", (int)ret);
		isp_resources_teardown();
		return ret;
	}

	/* Pool init must precede task creation so the first alloc hits the pool. */
	ret = isp_frame_pool_init(s_isp_ctx.isp_frame_size);
	if (ret != AVDK_ERR_OK) {
		LOGE("isp pool init failed=%d (size=%u), rolling back\r\n",
		     (int)ret, (unsigned)s_isp_ctx.isp_frame_size);
		isp_resources_teardown();
		return ret;
	}

	s_isp_ctx.running = 1U;

	bk_ret = rtos_create_thread(&s_isp_ctx.task,
				    ISP_TASK_PRIORITY,
				    "isp_overlay",
				    (beken_thread_function_t)isp_overlay_task_entry,
				    ISP_TASK_STACK_SIZE,
				    NULL);
	if (bk_ret != BK_OK) {
		LOGE("create isp_overlay task failed=%d\r\n", (int)bk_ret);
		s_isp_ctx.running = 0U;
		isp_resources_teardown();
		return AVDK_ERR_GENERIC;
	}

	s_isp_ctx.opened = 1U;
	LOGI("isp_open OK, rotate=%u visible=%ux%u\r\n",
	     (unsigned)H264D_GPU_DISPLAY_ISP_PIP_ROTATE_DEGREE,
	     (unsigned)isp_visible_width(s_isp_ctx.isp_w, s_isp_ctx.isp_h),
	     (unsigned)isp_visible_height(s_isp_ctx.isp_w, s_isp_ctx.isp_h));
	return AVDK_ERR_OK;
}

bool h264d_gpu_display_isp_is_open(void)
{
	return (s_isp_ctx.opened != 0U);
}

void h264d_gpu_display_isp_close(void)
{
	bk_gpu_ctlr_handle_t gpu;

	if (s_isp_ctx.opened == 0U) {
		return;
	}

	LOGI("isp_close start\r\n");

	s_isp_ctx.running = 0U;
	if (s_isp_ctx.done_sem != NULL) {
		(void)rtos_get_semaphore(&s_isp_ctx.done_sem, BEKEN_WAIT_FOREVER);
	}

	gpu = h264d_gpu_display_gpu_handle_get();
	if (gpu != NULL) {
		(void)bk_gpu_blit_clear(gpu);
	}

	isp_resources_teardown();
	isp_frame_pool_deinit();

	s_isp_ctx.opened = 0U;
	LOGI("isp_close done\r\n");
}

#endif /* H264D_GPU_DISPLAY_ENABLE_ISP_PIP */
