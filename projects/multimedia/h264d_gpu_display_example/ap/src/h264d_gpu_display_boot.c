#include <os/os.h>
#include <components/log.h>
#include <common/bk_err.h>
#include <components/avdk_utils/avdk_error.h>

#include "h264d_gpu_display_boot.h"
#include "h264d_gpu_display_demo.h"

#define TAG "h264d_gpu_boot"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define H264D_GPU_DISPLAY_BOOT_TASK_PRIORITY   3
#define H264D_GPU_DISPLAY_BOOT_TASK_STACK_SIZE (1024 * 4)
#define H264D_GPU_DISPLAY_BOOT_DELAY_MS        1000U
#define H264D_GPU_DISPLAY_BOOT_LOOPS           1U

static beken_thread_t s_h264d_gpu_display_boot_thread = NULL;
static volatile uint8_t s_h264d_gpu_display_boot_running = 0U;

static void h264d_gpu_display_boot_task_entry(void *arg)
{
	avdk_err_t ret;

	(void)arg;

	rtos_delay_milliseconds(H264D_GPU_DISPLAY_BOOT_DELAY_MS);

	LOGI("boot case start: h264 decode gpu display, loops=%u\r\n",
	     (unsigned)H264D_GPU_DISPLAY_BOOT_LOOPS);

	ret = h264d_gpu_display_start(H264D_GPU_DISPLAY_BOOT_LOOPS);
	if (ret != AVDK_ERR_OK) {
		LOGE("boot case start failed, ret=%d\r\n", (int)ret);
	} else {
		LOGI("boot case task created\r\n");
	}

	s_h264d_gpu_display_boot_running = 0U;
	s_h264d_gpu_display_boot_thread = NULL;
	rtos_delete_thread(NULL);
}

void h264d_gpu_display_run_boot_case(void)
{
	bk_err_t ret;

	if (s_h264d_gpu_display_boot_running != 0U) {
		LOGE("boot case is already running\r\n");
		return;
	}

	s_h264d_gpu_display_boot_running = 1U;
	ret = rtos_create_thread(&s_h264d_gpu_display_boot_thread,
				 H264D_GPU_DISPLAY_BOOT_TASK_PRIORITY,
				 "h264d_gpu_boot",
				 (beken_thread_function_t)h264d_gpu_display_boot_task_entry,
				 H264D_GPU_DISPLAY_BOOT_TASK_STACK_SIZE,
				 NULL);
	if (ret != BK_OK) {
		LOGE("create boot case task failed, ret=%d\r\n", (int)ret);
		s_h264d_gpu_display_boot_running = 0U;
		s_h264d_gpu_display_boot_thread = NULL;
		return;
	}

	LOGI("boot case task created\r\n");
}
