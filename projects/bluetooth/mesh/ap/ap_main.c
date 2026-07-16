#include "bk_private/bk_init.h"
#include <components/bluetooth/bk_dm_bluetooth.h>
#include <components/log.h>
#include <components/system.h>
#include <driver/gpio.h>
#include <os/os.h>

#define TAG "mesh_app"

extern int cli_ble_mesh_init(void);

void ble_gpio_debug(uint32_t pin, uint8_t up)
{
	gpio_id_t gpio_id = (gpio_id_t)pin;

	bk_gpio_enable_output(gpio_id);
	if (up)
	{
		bk_gpio_set_output_high(gpio_id);
	}
	else
	{
		bk_gpio_set_output_low(gpio_id);
	}
}

int main(void)
{
	bk_init();

	if (bk_bluetooth_init() != BK_OK)
	{
		BK_LOGE(TAG, "bk_bluetooth_init failed\n");
		return -1;
	}

	cli_ble_mesh_init();
	BK_LOGI(TAG, "ble_mesh cli ready\n");

	return 0;
}
