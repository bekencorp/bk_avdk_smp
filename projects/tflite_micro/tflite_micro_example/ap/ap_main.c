#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include "media_service.h"

#include "tflm_person_detection_demo.h"
#include "tflm_multi_model_demo.h"

#define TAG "tflite_demo"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

// 1: dual-interpreter parallel demo (person on core0 + pet on core1, shared NPU).
// 0: original single-interpreter person detection demo (baseline / fallback).
#define TFLM_MULTI_INTERPRETER_DEMO  0

// Task configuration for running TensorFlow Lite Micro demo
// Notes:
// - Run the demo in a dedicated RTOS task to avoid blocking the main thread.
// - Stack size may need to be increased depending on model/runtime changes.
#define TFLM_DEMO_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define TFLM_DEMO_TASK_STACK_SIZE  (1024 * 8)

#if !TFLM_MULTI_INTERPRETER_DEMO
static beken_thread_t s_tflm_demo_thread = NULL;

static void tflm_demo_task_entry(void *arg)
{
    (void)arg;

    LOGI("===========>start person_detection demo\r\n");
    while(1) {
        bk_err_t ret = tflm_person_detection_run_demo();
		if (ret != BK_OK) {
			LOGE("===========>person_detection demo failed, ret=%d\r\n", ret);
		} else {
			LOGI("===========>person_detection demo finished successfully\r\n");
		}
	}

    // Self-delete to release task resources.
    rtos_delete_thread(NULL);
}
#endif

int main(void)
{
    bk_init();
    media_service_init();
    // Initialize frame buffer heaps required by media modules.
    bk_frame_buffer_init();

#if TFLM_MULTI_INTERPRETER_DEMO
    // Single-threaded init phase (NPU + both models), then two core-pinned tasks.
    if (tflm_multi_demo_init() != BK_OK) {
        LOGE("multi interpreter demo init failed\r\n");
        return 0;
    }
    if (tflm_multi_demo_start() != BK_OK) {
        LOGE("multi interpreter demo start failed\r\n");
        return 0;
    }
#else
    bk_err_t ret = rtos_create_thread(&s_tflm_demo_thread,
                                     TFLM_DEMO_TASK_PRIORITY,
                                     "tflm_demo",
                                     (beken_thread_function_t)tflm_demo_task_entry,
                                     TFLM_DEMO_TASK_STACK_SIZE,
                                     NULL);
    if (ret != BK_OK) {
        LOGE("create tflm_demo task failed, ret=%d\r\n", ret);
    }
#endif

    bk_printf("m55 running...\r\n");

    return 0;
}


