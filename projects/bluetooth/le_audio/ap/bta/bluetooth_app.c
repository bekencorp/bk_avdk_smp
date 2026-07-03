#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"

#include "bluetooth_app.h"
#include "bta_manager.h"
#include "bluetooth_config.h"
#include "bta_event.h"
#include "bta_cli.h"
#include "bta_auracast.h"
#include "bta_audio.h"

#define TAG "bluetooth_app"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

static beken_thread_t s_bt_task;

static void bluetooth_app_task_entry(void *arg)
{
	(void)arg;

	bta_event_init();
	bta_audio_init();
	bta_auracast_init();
	bta_manager_init();
	bta_cli_init();

	rtos_delete_thread(NULL);
}

void bluetooth_app_pairing(void)
{
	bta_manager_pairing_mode();
}

void bluetooth_app_broadcast(void)
{
	bta_manager_broadcast();
}

void bluetooth_app_abs_volume(uint8_t per)
{
	bta_audio_set_abs_volume(per);
}

void bluetooth_app_volume_up(void)
{
	bta_manager_volume_up();
}

void bluetooth_app_volume_down(void)
{
	bta_manager_volume_down();
}

void bluetooth_app_play(void)
{
	bta_manager_play();
}

void bluetooth_app_on(void)
{
	bta_manager_on();
}

void bluetooth_app_off(void)
{
	bta_manager_off();
}

uint8_t bluetooth_app_is_on(void)
{
	return bta_manager_is_turn_on();
}

void bluetooth_gat_address(uint8_t *address)
{
	bta_manager_get_address(address);
}

void bluetooth_set_name(char *name)
{
	bta_manager_set_name(name);
}

void bluetooth_app_init(void)
{
	bk_err_t ret;

	ret = rtos_create_thread(&s_bt_task,
	                         BEKEN_DEFAULT_WORKER_PRIORITY,
	                         "bta_task",
	                         (beken_thread_function_t)bluetooth_app_task_entry,
	                         4096,
	                         NULL);
	(void)ret;
}
