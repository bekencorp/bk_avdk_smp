#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <stdint.h>
#if CONFIG_BK_NETWORK_PROVISIONING_BLE_EXAMPLE
#include "bk_network_provisioning.h"
#endif

#include "bk_api_ipc_test.h"

int32_t bk_sys_uart_write_string(uint32_t uart_id, const char *string);

int main(void)
{
    bk_init();

#if (BK_IPC_UT_TEST)
    bk_ipc_test_init();
#endif

    BK_LOGI(NULL, "AP main running...\r\n");


    return 0;
}
