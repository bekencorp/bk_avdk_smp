#include <stdint.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_err.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_display.h>
#include <components/bk_gpu.h>
#include <components/bk_gpu_ctlr.h>
#include <components/bk_flexa_bond.h>
#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>
#include <common/avdk_pixel_types.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>

#include "jpeg_encode_test.h"
#include "jpeg_encode_display.h"

#define TAG "bk_jpeg_enc_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_DISP_WIDTH            (1920U)
#define JPEG_DISP_HEIGHT           (1080U)
#define JPEG_DISP_ALIGNED_HEIGHT   ((JPEG_DISP_HEIGHT + 15U) & ~15U)
#define JPEG_DISP_FLEXA_LINES      (16U)
#define JPEG_DISP_SEGMENT_CNT      (3U)
#define JPEG_DISP_FRAME_CNT        (5U)
#define JPEG_DISP_MODE_MAX         (3U)
#define JPEG_DISP_STREAM_SIZE      (512U * 1024U)
#define JPEG_DISP_FRAME_TIMEOUT_MS (3000U)
#define JPEG_DISP_HOLD_MS          (3000U)

#define JPEG_DISP_PANEL_RESET_PIN     GPIO_60
#define JPEG_DISP_PANEL_BACKLIGHT_PIN GPIO_7

typedef struct {
	bk_display_ctlr_handle_t dpu;
	bk_display_bus_handle_t bus;
	bk_avdk_lcd_panel_handle_t panel;
	uint8_t opened;
} jpeg_disp_display_t;

typedef struct {
	bk_gpu_ctlr_handle_t handle;
	beken_semaphore_t done_sem;
	volatile uint32_t frames_done;
} jpeg_disp_gpu_t;

typedef struct {
	bk_jpeg_decode_ctlr_handle_t handle;
	uint8_t *ring;
	uint32_t ring_size;
	void *bond;
} jpeg_disp_decode_t;

static jpeg_disp_display_t s_display;
static jpeg_disp_gpu_t s_gpu;
static jpeg_disp_decode_t s_decode;

static void *jpeg_disp_hsram_aligned_alloc(uint32_t alignment, uint32_t size)
{
	void *raw;
	uintptr_t start;
	uintptr_t aligned;
	uint32_t total;

	if (alignment < (uint32_t)sizeof(void *))
		alignment = (uint32_t)sizeof(void *);
	if ((alignment & (alignment - 1U)) != 0U)
		return NULL;

	total = size + alignment - 1U + (uint32_t)sizeof(void *);
	raw = hsram_malloc(total);
	if (raw == NULL)
		return NULL;

	start = (uintptr_t)raw + sizeof(void *);
	aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
	((void **)aligned)[-1] = raw;
	return (void *)aligned;
}

static void jpeg_disp_hsram_aligned_free(void *ptr)
{
	if (ptr == NULL)
		return;

	os_free(((void **)ptr)[-1]);
}

static void jpeg_disp_backlight_on(void)
{
	gpio_dev_unmap(JPEG_DISP_PANEL_BACKLIGHT_PIN);
	BK_LOG_ON_ERR(bk_gpio_enable_output(JPEG_DISP_PANEL_BACKLIGHT_PIN));
	BK_LOG_ON_ERR(bk_gpio_pull_up(JPEG_DISP_PANEL_BACKLIGHT_PIN));
	bk_gpio_set_capacity(JPEG_DISP_PANEL_BACKLIGHT_PIN, GPIO_DRIVER_CAPACITY_3);
	bk_gpio_set_output_high(JPEG_DISP_PANEL_BACKLIGHT_PIN);
}

static avdk_err_t jpeg_disp_display_open(void)
{
	avdk_err_t ret;
	bk_display_dpu_config_t dpu_cfg;
	bk_lcd_panel_config_t panel_cfg;
	const bk_display_dsi_panel_t *panel = &lcd_device_hx8399c_mipi_1080x1920;

	if (s_display.opened)
		return AVDK_ERR_OK;

	os_memset(&dpu_cfg, 0, sizeof(dpu_cfg));
	dpu_cfg.video.enable = true;
	dpu_cfg.video.decompress = true;
	dpu_cfg.video.format = BK_PIXEL_FORMAT_ARGB8888;

	os_memset(&panel_cfg, 0, sizeof(panel_cfg));
	panel_cfg.reset_pin = JPEG_DISP_PANEL_RESET_PIN;

	ret = bk_display_dsi_bus_new(&s_display.bus, NULL);
	if (ret != AVDK_ERR_OK) {
		LOGE("display dsi bus new failed, ret=%d\r\n", ret);
		return ret;
	}

	if (bk_lcd_mipi_panel_new(s_display.bus, &panel_cfg, panel, &s_display.panel) != BK_OK) {
		LOGE("lcd mipi panel new failed\r\n");
		goto error;
	}

	ret = bk_display_dpu_ctlr_new(&s_display.dpu, s_display.panel, &dpu_cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("display dpu ctlr new failed, ret=%d\r\n", ret);
		goto error;
	}

	ret = bk_display_init(s_display.dpu);
	if (ret != AVDK_ERR_OK) {
		LOGE("display init failed, ret=%d\r\n", ret);
		goto error_deinit;
	}

	ret = bk_display_open(s_display.dpu);
	if (ret != AVDK_ERR_OK) {
		LOGE("display open failed, ret=%d\r\n", ret);
		goto error_deinit;
	}

	jpeg_disp_backlight_on();
	s_display.opened = 1U;
	LOGI("lcd ready, panel=%s %ux%u\r\n", panel->name,
	     (unsigned)panel->timing.h_size, (unsigned)panel->timing.v_size);
	return AVDK_ERR_OK;

error_deinit:
	(void)bk_display_deinit(s_display.dpu);
	s_display.dpu = NULL;
error:
	if (s_display.panel != NULL) {
		(void)bk_lcd_panel_delete(s_display.panel);
		s_display.panel = NULL;
	}
	if (s_display.bus != NULL) {
		(void)bk_display_bus_delete(s_display.bus);
		s_display.bus = NULL;
	}
	return AVDK_ERR_GENERIC;
}

static void jpeg_disp_display_close(void)
{
	if (s_display.dpu != NULL) {
		(void)bk_display_close(s_display.dpu);
		(void)bk_display_deinit(s_display.dpu);
		(void)bk_display_delete(s_display.dpu);
		s_display.dpu = NULL;
	}

	if (s_display.panel != NULL) {
		(void)bk_lcd_panel_delete(s_display.panel);
		s_display.panel = NULL;
	}

	if (s_display.bus != NULL) {
		(void)bk_display_bus_delete(s_display.bus);
		s_display.bus = NULL;
	}

	s_display.opened = 0U;
}

static void *jpeg_disp_gpu_frame_malloc(uint32_t size)
{
	void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, size);

	if (frame == NULL)
		LOGE("gpu frame malloc failed, size=%u\r\n", (unsigned)size);

	return frame;
}

static avdk_err_t jpeg_disp_gpu_frame_free(void *ptr)
{
	if (ptr != NULL)
		bk_frame_buffer_free(ptr);

	return AVDK_ERR_OK;
}

/* Releases a displayed frame once the DPU has consumed it. */
static avdk_err_t jpeg_disp_frame_release(void *args)
{
	return jpeg_disp_gpu_frame_free(args);
}

static void jpeg_disp_gpu_frame_done(void *frame, uint32_t frame_size, void *args)
{
	avdk_err_t ret;

	(void)frame_size;
	(void)args;

	s_gpu.frames_done++;

	if (!s_display.opened || s_display.dpu == NULL) {
		(void)jpeg_disp_gpu_frame_free(frame);
		return;
	}

	ret = bk_display_flush(s_display.dpu, frame, jpeg_disp_frame_release);
	if (ret != AVDK_ERR_OK) {
		LOGW("display flush failed, drop frame ret=%d\r\n", ret);
		(void)jpeg_disp_gpu_frame_free(frame);
		return;
	}

	if (s_gpu.done_sem != NULL)
		(void)rtos_set_semaphore(&s_gpu.done_sem);
}

static avdk_err_t jpeg_disp_gpu_open(const uint8_t *src_buffer)
{
	avdk_err_t ret;
	bk_gpu_ctlr_config_t cfg;

	if (s_gpu.handle != NULL)
		return AVDK_ERR_OK;

	if (src_buffer == NULL) {
		LOGE("gpu src buffer is NULL\r\n");
		return AVDK_ERR_INVAL;
	}

	if (rtos_init_semaphore(&s_gpu.done_sem, 1) != BK_OK) {
		LOGE("gpu done sem init failed\r\n");
		return AVDK_ERR_GENERIC;
	}

	os_memset(&cfg, 0, sizeof(cfg));
	cfg.rotate_degree = 90U;
	cfg.src_width = JPEG_DISP_WIDTH;
	cfg.src_height = JPEG_DISP_ALIGNED_HEIGHT;
	cfg.dst_width = JPEG_DISP_WIDTH;
	cfg.dst_height = JPEG_DISP_HEIGHT;
	cfg.src_format = BK_PIXEL_FORMAT_NV12;
	cfg.dst_format = BK_PIXEL_FORMAT_ARGB8888;
	cfg.src_buffer = (uint8_t *)(uintptr_t)src_buffer;
	cfg.compress = true;
	cfg.scale = true;
	cfg.flexa = true;
	cfg.flexa_lines = JPEG_DISP_FLEXA_LINES;
	cfg.flexa_buff_cnt = JPEG_DISP_SEGMENT_CNT;
	cfg.frame_malloc = jpeg_disp_gpu_frame_malloc;
	cfg.frame_free = jpeg_disp_gpu_frame_free;
	cfg.frame_done = jpeg_disp_gpu_frame_done;
	cfg.frame_done_args = NULL;

	ret = bk_gpu_ctlr_new(&s_gpu.handle, &cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu ctlr new failed, ret=%d\r\n", ret);
		goto error;
	}

	ret = bk_gpu_init(s_gpu.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu init failed, ret=%d\r\n", ret);
		goto error_close;
	}

	ret = bk_gpu_open(s_gpu.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("gpu open failed, ret=%d\r\n", ret);
		goto error_close;
	}

	s_gpu.frames_done = 0U;
	LOGI("gpu ready, %ux%u NV12 -> %ux%u ARGB8888 rotate=90\r\n",
	     (unsigned)JPEG_DISP_WIDTH, (unsigned)JPEG_DISP_ALIGNED_HEIGHT,
	     (unsigned)JPEG_DISP_WIDTH, (unsigned)JPEG_DISP_HEIGHT);
	return AVDK_ERR_OK;

error_close:
	(void)bk_gpu_close(s_gpu.handle);
	(void)bk_gpu_deinit(s_gpu.handle);
	(void)bk_gpu_delete(s_gpu.handle);
	s_gpu.handle = NULL;
error:
	if (s_gpu.done_sem != NULL) {
		(void)rtos_deinit_semaphore(&s_gpu.done_sem);
		s_gpu.done_sem = NULL;
	}
	return AVDK_ERR_GENERIC;
}

static void jpeg_disp_gpu_close(void)
{
	if (s_gpu.handle != NULL) {
		(void)bk_gpu_close(s_gpu.handle);
		(void)bk_gpu_deinit(s_gpu.handle);
		(void)bk_gpu_delete(s_gpu.handle);
		s_gpu.handle = NULL;
	}

	if (s_gpu.done_sem != NULL) {
		(void)rtos_deinit_semaphore(&s_gpu.done_sem);
		s_gpu.done_sem = NULL;
	}
}

static void jpeg_disp_decode_close(void)
{
	if (s_decode.bond != NULL) {
		bk_flexa_mjpegd_gpu_bond_stop(s_decode.bond);
		s_decode.bond = NULL;
	}

	if (s_decode.handle != NULL) {
		(void)bk_jpeg_decode_close(s_decode.handle);
		(void)bk_jpeg_decode_deinit(s_decode.handle);
		(void)bk_jpeg_decode_delete(s_decode.handle);
		s_decode.handle = NULL;
	}

	if (s_decode.ring != NULL) {
		jpeg_disp_hsram_aligned_free(s_decode.ring);
		s_decode.ring = NULL;
	}
	s_decode.ring_size = 0U;
}

static avdk_err_t jpeg_disp_decode_open(void)
{
	avdk_err_t ret;
	bk_jpeg_decode_flexa_config_t cfg = DEFAULT_JPEG_DECODE_FLEXA_CONFIG;

	s_decode.ring_size = bk_image_size_get(JPEG_DISP_WIDTH,
					       JPEG_DISP_FLEXA_LINES * JPEG_DISP_SEGMENT_CNT,
					       BK_PIXEL_FORMAT_NV12);
	s_decode.ring = (uint8_t *)jpeg_disp_hsram_aligned_alloc(64U, s_decode.ring_size);
	if (s_decode.ring == NULL) {
		LOGE("jpeg decode ring alloc failed, size=%u\r\n", (unsigned)s_decode.ring_size);
		return AVDK_ERR_NOMEM;
	}

	cfg.out_width = JPEG_DISP_WIDTH;
	cfg.out_height = JPEG_DISP_HEIGHT;
	cfg.out_format = BK_PIXEL_FORMAT_NV12;
	cfg.segment_height = JPEG_DISP_FLEXA_LINES / 16U;
	cfg.segment_number = JPEG_DISP_SEGMENT_CNT;
	cfg.timeout_ms = JPEG_DISP_FRAME_TIMEOUT_MS;

	ret = bk_jpeg_decode_flexa_ctlr_new(&s_decode.handle, &cfg);
	if (ret != AVDK_ERR_OK) {
		LOGE("jpeg decode flexa new failed, ret=%d\r\n", ret);
		goto error;
	}

	ret = bk_jpeg_decode_init(s_decode.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("jpeg decode init failed, ret=%d\r\n", ret);
		goto error;
	}

	ret = bk_jpeg_decode_open(s_decode.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("jpeg decode open failed, ret=%d\r\n", ret);
		goto error;
	}

	LOGI("jpeg flexa decoder ready, ring=%p size=%u segments=%u\r\n",
	     s_decode.ring, (unsigned)s_decode.ring_size, (unsigned)JPEG_DISP_SEGMENT_CNT);
	return AVDK_ERR_OK;

error:
	jpeg_disp_decode_close();
	return AVDK_ERR_GENERIC;
}

static avdk_err_t jpeg_disp_bond_start(void)
{
	if (s_decode.bond != NULL)
		return AVDK_ERR_OK;

	avdk_err_t ret = bk_flexa_mjpegd_gpu_bond_start(&s_decode.bond,
							s_decode.handle, s_gpu.handle);
	if (ret != AVDK_ERR_OK) {
		LOGE("mjpegd-gpu bond start failed, ret=%d\r\n", ret);
		s_decode.bond = NULL;
	}

	return ret;
}

static avdk_err_t jpeg_disp_decode_one(const uint8_t *stream, uint32_t stream_len)
{
	bk_jpeg_decode_input_t in;

	/* Drop a stale completion before starting the decode. */
	if (s_gpu.done_sem != NULL)
		(void)rtos_get_semaphore(&s_gpu.done_sem, 0U);

	os_memset(&in, 0, sizeof(in));
	in.stream = (uint8_t *)(uintptr_t)stream;
	in.stream_len = stream_len;
	in.out_buffer = s_decode.ring;
	in.out_buffer_size = s_decode.ring_size;

	avdk_err_t ret = bk_jpeg_decode_frame(s_decode.handle, &in);
	if (ret != AVDK_ERR_OK) {
		LOGE("jpeg decode frame failed, ret=%d\r\n", ret);
		return ret;
	}

	if (s_gpu.done_sem != NULL &&
	    rtos_get_semaphore(&s_gpu.done_sem, JPEG_DISP_FRAME_TIMEOUT_MS) != BK_OK) {
		LOGE("wait gpu frame done timeout\r\n");
		return AVDK_ERR_TIMEOUT;
	}

	return AVDK_ERR_OK;
}

typedef struct {
	jpeg_encode_osd_display_mode_t mode;
	const char *name;
	const char *osd_text;
} jpeg_disp_mode_desc_t;

static const jpeg_disp_mode_desc_t s_display_modes[JPEG_DISP_MODE_MAX] = {
	{ JPEG_ENCODE_OSD_DISPLAY_FRAME,    "frame",    "HW FRAME" },
	{ JPEG_ENCODE_OSD_DISPLAY_FLEXA,    "flexa",    "SW FLEXA" },
	{ JPEG_ENCODE_OSD_DISPLAY_SW_FRAME, "sw_frame", "SW FRAME" },
};

static const char *jpeg_disp_mode_name(jpeg_encode_osd_display_mode_t mode)
{
	switch (mode) {
	case JPEG_ENCODE_OSD_DISPLAY_FRAME:
		return "frame";
	case JPEG_ENCODE_OSD_DISPLAY_FLEXA:
		return "flexa";
	case JPEG_ENCODE_OSD_DISPLAY_SW_FRAME:
		return "sw_frame";
	case JPEG_ENCODE_OSD_DISPLAY_ALL:
		return "all";
	default:
		return "unknown";
	}
}

static uint32_t jpeg_disp_select_modes(jpeg_encode_osd_display_mode_t mode, uint32_t *selected)
{
	uint32_t count = 0U;

	if (mode == JPEG_ENCODE_OSD_DISPLAY_ALL) {
		for (uint32_t i = 0U; i < JPEG_DISP_MODE_MAX; i++)
			selected[count++] = i;
	} else if ((uint32_t)mode < JPEG_DISP_MODE_MAX) {
		selected[count++] = (uint32_t)mode;
	}

	return count;
}

static int jpeg_disp_encode_once(jpeg_encode_osd_display_mode_t mode, const char *osd_text,
				 uint8_t *stream, uint32_t stream_size, uint32_t *jpeg_len)
{
	LOGE("%s %d %d\n", __func__, __LINE__, mode);
	switch (mode) {
	case JPEG_ENCODE_OSD_DISPLAY_FRAME:
		return bk_jpeg_encode_osd_frame_once(stream, stream_size, jpeg_len, osd_text);
	case JPEG_ENCODE_OSD_DISPLAY_FLEXA:
		return bk_jpeg_encode_osd_flexa_once(stream, stream_size, jpeg_len, osd_text);
	case JPEG_ENCODE_OSD_DISPLAY_SW_FRAME:
		return bk_jpeg_encode_osd_sw_frame_once(stream, stream_size, jpeg_len, osd_text);
	default:
		return BK_FAIL;
	}
}

static void jpeg_disp_log_result(uint8_t pass, jpeg_encode_osd_display_mode_t mode,
				 const char *stage, int ret, uint32_t done_frames,
				 uint32_t total_frames)
{
	if (pass)
		LOGI("[RESULT][PASS] bk_jpeg_encode_osd_display_test success, mode=%s frames=%u/%u\r\n",
		     jpeg_disp_mode_name(mode), done_frames, total_frames);
	else
		LOGE("[RESULT][FAIL] bk_jpeg_encode_osd_display_test failed at %s ret=%d mode=%s frames=%u/%u\r\n",
		     stage, ret, jpeg_disp_mode_name(mode), done_frames, total_frames);
}

int bk_jpeg_encode_osd_display_test(jpeg_encode_osd_display_mode_t mode)
{
	int ret = BK_FAIL;
	avdk_err_t avdk_ret;
	uint8_t *stream = NULL;
	uint32_t done_frames = 0U;
	uint32_t total_frames = 0U;
	uint32_t selected[JPEG_DISP_MODE_MAX];
	uint32_t mode_cnt;
	uint8_t pass = 0U;
	const char *fail_stage = "start";

	mode_cnt = jpeg_disp_select_modes(mode, selected);
	if (mode_cnt == 0U) {
		LOGE("invalid osd display mode %d\r\n", (int)mode);
		jpeg_disp_log_result(0U, mode, "invalid_mode", BK_FAIL, 0U, 0U);
		return BK_FAIL;
	}
	total_frames = mode_cnt * JPEG_DISP_FRAME_CNT;

	LOGI("osd display test start, mode=%s modes=%u rounds=%u %ux%u\r\n",
	     jpeg_disp_mode_name(mode), mode_cnt, total_frames,
	     (unsigned)JPEG_DISP_WIDTH, (unsigned)JPEG_DISP_HEIGHT);

	avdk_ret = jpeg_disp_display_open();
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "display_open";
		ret = (int)avdk_ret;
		goto exit;
	}

	avdk_ret = jpeg_disp_decode_open();
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "decode_open";
		ret = (int)avdk_ret;
		goto exit;
	}

	avdk_ret = jpeg_disp_gpu_open(s_decode.ring);
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "gpu_open";
		ret = (int)avdk_ret;
		goto exit;
	}

	avdk_ret = jpeg_disp_bond_start();
	if (avdk_ret != AVDK_ERR_OK) {
		fail_stage = "bond_start";
		ret = (int)avdk_ret;
		goto exit;
	}

	stream = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, JPEG_DISP_STREAM_SIZE);
	if (stream == NULL) {
		fail_stage = "alloc_stream";
		ret = BK_FAIL;
		LOGE("jpeg stream buffer alloc failed, size=%u\r\n", (unsigned)JPEG_DISP_STREAM_SIZE);
		goto exit;
	}

	for (uint32_t m = 0U; m < mode_cnt; m++) {
		const jpeg_disp_mode_desc_t *desc = &s_display_modes[selected[m]];

		for (uint32_t i = 0U; i < JPEG_DISP_FRAME_CNT; i++) {
			uint32_t jpeg_len = 0U;

			ret = jpeg_disp_encode_once(desc->mode, desc->osd_text, stream,
						    JPEG_DISP_STREAM_SIZE, &jpeg_len);
			if (ret != BK_OK) {
				fail_stage = "encode_osd";
				LOGE("%s encode round %u failed, ret=%d\r\n",
				     desc->name, (unsigned)(i + 1U), ret);
				goto exit;
			}

			avdk_ret = jpeg_disp_decode_one(stream, jpeg_len);
			if (avdk_ret != AVDK_ERR_OK) {
				fail_stage = "decode_display";
				ret = (int)avdk_ret;
				LOGE("%s decode/display round %u failed, jpeg_len=%u ret=%d\r\n",
				     desc->name, (unsigned)(i + 1U), (unsigned)jpeg_len, ret);
				goto exit;
			}

			done_frames++;
			LOGI("%s round %u/%u osd=%s encoded=%u displayed, gpu_frames=%u\r\n",
			     desc->name, (unsigned)(i + 1U), (unsigned)JPEG_DISP_FRAME_CNT,
			     desc->osd_text, (unsigned)jpeg_len, (unsigned)s_gpu.frames_done);
		}

		/* Drop the cached encode input before the next mode allocates its own. */
		bk_jpeg_encode_osd_input_release();
	}

	pass = 1U;
	ret = BK_OK;

exit:
	if (pass) {
		/* Keep the last frame on screen long enough to be observed. */
		rtos_delay_milliseconds(JPEG_DISP_HOLD_MS);
	}

	if (stream != NULL)
		bk_frame_buffer_free(stream);

	jpeg_disp_decode_close();
	jpeg_disp_gpu_close();
	jpeg_disp_display_close();
	bk_jpeg_encode_osd_input_release();

	jpeg_disp_log_result(pass, mode, fail_stage, ret, done_frames, total_frames);
	return pass ? BK_OK : BK_FAIL;
}
