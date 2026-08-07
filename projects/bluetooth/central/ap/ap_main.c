#include <stdio.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#include "bt_manager.h"
#include "media_service.h"
#include "a2dp_source_demo.h"

#define AUTO_ENABLE_BLUETOOTH_DEMO 1

int main(void)
{
    bk_init();

    media_service_init();

#if AUTO_ENABLE_BLUETOOTH_DEMO
    uint8_t bt_mac[6] = {0};
    char local_name[30] = {0};

    if (bk_bluetooth_get_address(bt_mac) == BK_OK)
    {
        snprintf(local_name, sizeof(local_name), "a2dp_source_%02x%02x%02x", bt_mac[2], bt_mac[1], bt_mac[0]);
    }
    else
    {
        snprintf(local_name, sizeof(local_name), "a2dp_source");
    }

    /* bt_manager owns the single GAP callback: link keys are persisted through
     * bluetooth_storage; the demo chains discovery via bt_manager_register_callback. */
    bt_manager_cfg_t bt_manager_cfg =
    {
        .local_name = local_name,
        .device_class = COD_PHONE,
        .page_scan_interval = 0x0800,  // 1.28s
        .page_scan_window = 0x00B4,  // 11.25ms
        .page_timeout = 16000,   // unit of 0.625ms
        .io_capability = BK_BT_IO_CAP_NONE,
        .role = 1, /* 1 = master; a2dp source becomes master after the link is up */
    };
    bt_manager_init(&bt_manager_cfg);

#if CONFIG_BT
    /* eager profile init so a peer (speaker) can initiate the connection */
    bt_a2dp_source_demo_init();
    cli_a2dp_source_demo_init();
#endif
#if 0//CONFIG_BLE
    extern int cli_ble_gatt_demo_init(void);
    cli_ble_gatt_demo_init();

    extern int cli_ble_hogpd_demo_init(void);
    cli_ble_hogpd_demo_init();
#endif


#endif
    return 0;
}
