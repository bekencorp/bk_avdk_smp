#include "vcdec_h264_test_common.h"

#define TAG "vcdec_h264_boot"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define VCDEC_BOOT_DEMO_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VCDEC_BOOT_DEMO_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_vcdec_boot_demo_thread = NULL;
static volatile uint8_t s_vcdec_boot_demo_running = 0;

/*
 * Boot-time self-run demo: exercise all three bk_decoder controllers once,
 * covering both embedded 1280x720 streams (baseline 1I30P and B-frame IBBP).
 */
static void vcdec_h264_boot_demo_task_entry(void *arg)
{
	(void)arg;

	rtos_delay_milliseconds(1000);
	LOGI("boot demo: frame test (1i30p) start\r\n");
	vcdec_h264_frame_test(H264_DECODE_TEST_STREAM_1280X720_1I30P);
	LOGI("boot demo: frame test (1i30p) done\r\n");

	rtos_delay_milliseconds(1000);
	LOGI("boot demo: flexa test (1i30p) start\r\n");
	vcdec_h264_flexa_test(H264_DECODE_TEST_STREAM_1280X720_1I30P);
	LOGI("boot demo: flexa test (1i30p) done\r\n");

	rtos_delay_milliseconds(1000);
	LOGI("boot demo: frame-zerocopy test (ibbp) start\r\n");
	vcdec_h264_frame_zerocopy_test(H264_DECODE_TEST_STREAM_1280X720_IBBP);
	LOGI("boot demo: frame-zerocopy test (ibbp) done\r\n");

	s_vcdec_boot_demo_running = 0U;
	s_vcdec_boot_demo_thread = NULL;
	rtos_delete_thread(NULL);
}

void vcdec_h264_run_boot_demo(void)
{
	bk_err_t ret;

	if (s_vcdec_boot_demo_running != 0U) {
		LOGE("vcdec h264 boot demo task is already running\r\n");
		return;
	}

	s_vcdec_boot_demo_running = 1U;
	ret = rtos_create_thread(&s_vcdec_boot_demo_thread,
				 VCDEC_BOOT_DEMO_TASK_PRIORITY,
				 "vcdec_boot_demo",
				 (beken_thread_function_t)vcdec_h264_boot_demo_task_entry,
				 VCDEC_BOOT_DEMO_TASK_STACK_SIZE,
				 NULL);
	if (ret != BK_OK) {
		LOGE("create vcdec h264 boot demo task failed, ret=%d\r\n", (int)ret);
		s_vcdec_boot_demo_running = 0U;
		s_vcdec_boot_demo_thread = NULL;
		return;
	}

	LOGI("vcdec h264 boot demo task created\r\n");
}
