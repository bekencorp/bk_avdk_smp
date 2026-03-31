#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include "avdk_monitor.h"
#include <components/media_types.h>
#include <avdk_check.h>

#define TAG "MONITOR"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define AVDK_MONITOR_THREAD_STACK_SIZE (1024 * 2)
#define AVDK_MONITOR_TASK_DELAY_SECONDS (1000 * 2)

avdk_monitor_info_t *avdk_monitor_info = NULL;

void avdk_monitor_init(void)
{
    avdk_monitor_info = os_malloc(sizeof(avdk_monitor_info_t));
    AVDK_RETURN_VOID_ON_FALSE(avdk_monitor_info, TAG, "avdk_monitor_info is NULL");
    os_memset(avdk_monitor_info, 0, sizeof(avdk_monitor_info_t));
}

static void avdk_monitor_task_entry(void* arg)
{
    AVDK_RETURN_VOID_ON_FALSE(avdk_monitor_info, TAG, "avdk_monitor_info is NULL");

    while (avdk_monitor_info->enable) {
        AVDK_GOTO_VOID_ON_FALSE(avdk_monitor_info, error, TAG, "avdk_monitor_info is NULL");

        LOGI("%d-MP[F:%d, L: %d], %d-SP[F:%d, L: %d], %d-GPU[F: %d, L: %d], %d-DPU[F: %d, I: %d]\n", 
            avdk_monitor_info->mp, avdk_monitor_info->isp_mp_frame_count, avdk_monitor_info->isp_mp_line_count,
            avdk_monitor_info->sp, avdk_monitor_info->isp_sp_frame_count, avdk_monitor_info->isp_sp_line_count,
            avdk_monitor_info->gpu, avdk_monitor_info->gpu_frame_count, avdk_monitor_info->gpu_line_count,
            avdk_monitor_info->dpu, avdk_monitor_info->dpu_fps_count, avdk_monitor_info->dpu_isr_count);
        rtos_delay_milliseconds(AVDK_MONITOR_TASK_DELAY_SECONDS);
    }

error:

    LOGI("%s exit\n", __func__);
    rtos_delete_thread(NULL);
}

void avdk_monitor_start(void)
{
    AVDK_RETURN_VOID_ON_FALSE(avdk_monitor_info, TAG, "avdk_monitor_info is NULL");

    avdk_monitor_info->enable = true;

    AVDK_RETURN_VOID_ON_ERROR(rtos_create_thread(&avdk_monitor_info->thread,
                      BEKEN_DEFAULT_WORKER_PRIORITY,
                      "avdk_monitor",
                      (beken_thread_function_t)avdk_monitor_task_entry,
                      AVDK_MONITOR_THREAD_STACK_SIZE,
                      avdk_monitor_info), TAG, "create monitor task error");
}

void avdk_monitor_stop(void)
{
    //TODO
}

void avdk_monitor_deinit(void)
{
    os_free(avdk_monitor_info);
}
