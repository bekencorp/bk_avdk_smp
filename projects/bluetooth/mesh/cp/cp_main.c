#include "bk_private/bk_init.h"
#include <components/ate.h>
#include <components/system.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include <os/os.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{
    if (!ate_is_enabled())
    {
        bk_start_ap_system();
    }
}

int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();

    return 0;
}
