#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>


extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_set_swd_mode(void);

void user_app_main(void) {
    // start smp(cpu1, cpu2)
    bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
}



int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

	return 0;
}