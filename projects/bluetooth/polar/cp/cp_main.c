#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

static void user_app_main(void)
{
    /*
     * DUT mode is selected by the ATE strap. Unlike normal Bluetooth demos,
     * the AP must still boot so its CLI can send DUT commands over IPC.
     */
    bk_start_ap_system();
}

int main(void)
{
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    bk_init();
    return 0;
}
