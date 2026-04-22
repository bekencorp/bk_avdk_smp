#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>

#if CONFIG_FREERTOS_SMP
static beken_semaphore_t app_semaphore;

static void smp_test_task1(void *arg)
{
    BK_LOGD(NULL, "===smp_test_task1===:\r\n");
    for(;;) {
        rtos_get_semaphore(&app_semaphore, BEKEN_WAIT_FOREVER);
        BK_LOGD(NULL, "smp_test_task1 run core: %d\r\n", rtos_get_core_id());
    }
}

static void smp_test_task2(void *arg)
{
    BK_LOGD(NULL, "smp_test_task2 run core: %d\r\n", rtos_get_core_id());
  
    for(;;) {
        rtos_set_semaphore(&app_semaphore);
        BK_LOGD(NULL, "smp_test_task2 run core: %d\r\n", rtos_get_core_id());
        rtos_delay_milliseconds(1000);
    }
}

void app_test_smp_core0(void)
{
    int ret;
    beken_thread_t cpu1_thread;

    /* create a semaphore */
    ret = rtos_init_semaphore(&app_semaphore, 5);
    if (ret != kNoErr) {
        BK_LOGD(NULL, "Error: Failed to init app_semaphore: %d\r\n",ret);
    }

    /* create a thread on core 0 */
    ret = rtos_core0_create_thread(&cpu1_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "smp_test_task1",
                             (beken_thread_function_t)smp_test_task1,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create smp_test_task1: %d\r\n",ret);
    }
}

void app_test_smp_core1(void)
{
    int ret;
    beken_thread_t cpu2_thread;
    
    /* create a thread on core 1 */
    ret = rtos_core1_create_thread(&cpu2_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "smp_test_task2",
                             (beken_thread_function_t)smp_test_task2 ,
                             2048,
                             0);
    if (ret != kNoErr) {
        BK_LOGE(NULL, "Error: Failed to create smp_test_task2: %d\r\n",ret);
    }
}
#endif

extern void rtos_set_user_app_entry(beken_thread_function_t entry);


void user_app_main(void) {
    bk_start_ap_system();
}

int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();

#if CONFIG_FREERTOS_SMP_TEST
    app_test_smp_core0();
    app_test_smp_core1();
#endif

    return 0;
}
