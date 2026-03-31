#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "bk_api_ipc_test.h"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);


void user_app_main(void) {
}

int main(void)
{
    bk_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    return 0;
}