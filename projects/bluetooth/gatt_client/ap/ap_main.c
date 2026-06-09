#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "gatt_client_demo.h"

int main(void)
{
	bk_init();

	rtos_delay_milliseconds(500);

	gatt_client_demo_init();
	
	return 0;
}
