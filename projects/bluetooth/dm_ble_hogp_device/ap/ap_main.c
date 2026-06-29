#include <stdio.h>
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#if CONFIG_BLE
#include "dm_gatt.h"
#include "dm_gatts.h"
#include "hogpd.h"
#include "ble_demo.h"
#endif

int main(void)
{
	bk_init();

	/* Bluetooth auto-enables at startup (CONFIG_BLUETOOTH_AUTO_ENABLE=y);
	 * wait a moment so the controller/host is ready before issuing BLE APIs. */
	rtos_delay_milliseconds(500);

#if CONFIG_BLE
	cli_gatt_param_t param = {.rpa = 0, .p_rpa = &param.rpa, .pa = 0, .p_pa = &param.pa};

	/* Bring up the public dm BLE GAP + GATT-server framework. bk_dm_prf_gatts_main()
	 * also configures the advertising payload (HID service UUID + appearance)
	 * and starts advertising. */
	bk_dm_prf_gap_main(&param);
	bk_dm_prf_gatts_main(&param);

	/* Register the HID-over-GATT (keyboard) service database. */
	bk_dm_prf_hogpd_init();
	ble_demo_init();
	ble_demo_adv_enable(1);
#if CONFIG_CLI
	/* Optional CLI helpers: "hogpd init" and the generic gatt demo cmds. */
	extern int cli_ble_hogpd_init(void);
	cli_ble_hogpd_init();
	extern int cli_ble_gatt_demo_init(void);
	cli_ble_gatt_demo_init();
#endif
#endif

	return 0;
}
