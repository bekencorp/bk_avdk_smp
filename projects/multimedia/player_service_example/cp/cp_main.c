#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "customer_msg.h"
#include <components/ate.h>
#include "bk_api_ipc_test.h"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_set_swd_mode(void);

void user_app_main(void) {
    bk_start_ap_system();

#if 0
    // start smp(cpu1, cpu2)
    bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);

#if !CONFIG_BTDM_CONTROLLER_ONLY
    if (!ate_is_enabled())
    {
        cifd_cust_msg_init();
    }
#endif

#endif
}



int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif
	return 0;
}