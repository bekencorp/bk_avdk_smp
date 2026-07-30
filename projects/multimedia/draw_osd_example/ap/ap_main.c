#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include "media_service.h"
#include "draw_osd_test.h"
#include "osd_mipi.h"

#define TAG "draw_osd"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Boot IT case: start flexa OSD and keep displaying (pipeline stays open; normal runtime).
 * Pre-boot delay must be long enough: AP pipeline open logs heavily and holds HSPL UART_LOG lock;
 * starting too early makes CP UART ISR wait on lock, SysTick misses WWDT feed -> dump.
 * Watchdog dump happens during pipeline bringup; osd_mipi_show() OK means danger window passed;
 * log [RESULT][PASS] immediately, no extra hold needed. */
#define DRAW_OSD_BOOT_DELAY_MS  500

static void draw_osd_boot_entry(beken_thread_arg_t arg)
{
    const char *case_name = "draw_osd_mipi_flexa_boot";
    avdk_err_t ret;

    (void)arg;

    rtos_delay_milliseconds(DRAW_OSD_BOOT_DELAY_MS);

    ret = osd_mipi_show(OSD_BLEND_PER_FLEXA);
    draw_osd_log_result(case_name, (ret == AVDK_ERR_OK), "show");

    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();
    media_service_init();

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif

    LOGI("draw_osd_example m55 running...\r\n");

    cli_draw_osd_test_init();

    if (rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "osd_boot",
                           (beken_thread_function_t)draw_osd_boot_entry,
                           1024 * 8, NULL) != kNoErr) {
        LOGE("create osd_boot thread failed\r\n");
    }

    return 0;
}
