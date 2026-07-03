#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

#include "dm_ble_gatt_client_demo.h"

int main(void)
{
	bk_init();

	rtos_delay_milliseconds(500);

	dm_ble_gatt_client_demo_init();

	extern int cli_ble_gatt_demo_init(void);
	cli_ble_gatt_demo_init();

	return 0;
}
