#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "bk_api_ipc_test.h"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

void user_app_main(void) {
    // start smp(cpu1, cpu2)
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
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