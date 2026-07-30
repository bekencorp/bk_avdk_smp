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

        const uint32_t period_s = AVDK_MONITOR_TASK_DELAY_SECONDS / 1000;

        /* Embedded: keep this to a single, dense line. Only modules that are
         * open (enable bit set) are reported; per module we show f=fps (frame
         * rate / is it working) and i=irq count over the window (is the module
         * interrupt firing). Counters are per-window: cleared after each print. */
        char line[128];
        int off = 0;

        if (avdk_monitor_info->mp) {
            off += os_snprintf(line + off, sizeof(line) - off, "MP[%uf %ui] ",
                avdk_monitor_info->isp_mp_frame_count / period_s, avdk_monitor_info->isp_mp_line_count);
            avdk_monitor_info->isp_mp_frame_count = 0;
            avdk_monitor_info->isp_mp_line_count = 0;
        }
        if (avdk_monitor_info->sp) {
            off += os_snprintf(line + off, sizeof(line) - off, "SP[%uf %ui] ",
                avdk_monitor_info->isp_sp_frame_count / period_s, avdk_monitor_info->isp_sp_line_count);
            avdk_monitor_info->isp_sp_frame_count = 0;
            avdk_monitor_info->isp_sp_line_count = 0;
        }
        if (avdk_monitor_info->gpu) {
            off += os_snprintf(line + off, sizeof(line) - off, "GPU[%uf %ui] ",
                avdk_monitor_info->gpu_frame_count / period_s, avdk_monitor_info->gpu_line_count);
            avdk_monitor_info->gpu_frame_count = 0;
            avdk_monitor_info->gpu_line_count = 0;
        }
        if (avdk_monitor_info->dpu) {
            uint16_t cur_dpu_isr = avdk_monitor_info->dpu_isr_count;
            uint16_t cur_dpu_fps = avdk_monitor_info->dpu_fps_count;
            off += os_snprintf(line + off, sizeof(line) - off,
                "DPU[fps %u rps %u] ",cur_dpu_fps / period_s, cur_dpu_isr / period_s);
            avdk_monitor_info->dpu_fps_count = 0;
            avdk_monitor_info->dpu_isr_count = 0;
        }

        if (off > 0) {
            LOGI("%s\n", line);
        }

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
