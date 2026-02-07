#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
#include "bk_network_provisioning.h"
#endif

int main(void)
{
    bk_init();

    return 0;
}