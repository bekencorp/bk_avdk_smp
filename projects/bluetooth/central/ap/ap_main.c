#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

//#include "bt_manager.h"
#include "media_service.h"

#define AUTO_ENABLE_BLUETOOTH_DEMO 1

int main(void)
{
	bk_init();

	media_service_init();

#if AUTO_ENABLE_BLUETOOTH_DEMO
	//bt_manager_init();

#if CONFIG_BT
    extern int cli_a2dp_source_demo_init(void);
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
