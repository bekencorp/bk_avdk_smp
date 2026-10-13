#include "bk_private/bk_init.h"
#include <common/bk_err.h>
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "media_service.h"
#include "fisheye_cli.h"

#define TAG "fisheye"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define FISHEYE_BOOT_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define FISHEYE_BOOT_TASK_STACK_SIZE  (1024 * 8)
#define FISHEYE_BOOT_DELAY_MS         2000

static beken_thread_t s_fisheye_boot_thread = NULL;

static void fisheye_boot_task_entry(void *arg)
{
    (void)arg;

    rtos_delay_milliseconds(FISHEYE_BOOT_DELAY_MS);
    LOGI("run default fisheye calibration after boot delay\r\n");

    int ret = fisheye_run_default_it_test();
    if (ret != BK_OK)
    {
        LOGE("default fisheye calibration failed, ret=%d\r\n", ret);
    }

    s_fisheye_boot_thread = NULL;
    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_err_t ret;

    bk_init();
    media_service_init();

#ifdef CONFIG_FRAME_BUFFER
    bk_frame_buffer_init();
#endif
    fisheye_cli_init();
    ret = rtos_create_thread(&s_fisheye_boot_thread,
                             FISHEYE_BOOT_TASK_PRIORITY,
                             "fisheye_boot",
                             (beken_thread_function_t)fisheye_boot_task_entry,
                             FISHEYE_BOOT_TASK_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("create fisheye_boot task failed, ret=%d\r\n", ret);
    }

    return 0;
}
