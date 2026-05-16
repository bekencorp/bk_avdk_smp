#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "bk_api_ipc_test.h"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);


void user_app_main(void) {
#if (CONFIG_SOC_BK7236XX)
    // start smp(cpu1, cpu2)
    bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
#endif

#if 0//(CONFIG_SUPPORT_MULTICORE)
    rtos_delay_milliseconds(3000);
    bk_printf("M55 start multicore\r\n");
	BK_LOG_ON_ERR(bk_multicore_start(2));
    bk_printf("M55 multicore started\r\n");
#endif
}

int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

#if CONFIG_AUDIO
    extern int cli_aud_init(void);
    cli_aud_init();
#endif
    os_printf("main exit!\n");
    return 0;
}