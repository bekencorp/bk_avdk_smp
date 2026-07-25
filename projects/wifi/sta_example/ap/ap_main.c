#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <components/event.h>
#include <components/netif_types.h>
#include <modules/wifi.h>
#include <modules/wifi_types.h>

#define TAG "sta_example"

#ifndef WIFI_STA_EXAMPLE_SSID
#define WIFI_STA_EXAMPLE_SSID     "your_ssid"
#endif
#ifndef WIFI_STA_EXAMPLE_PASSWORD
#define WIFI_STA_EXAMPLE_PASSWORD "your_password"
#endif

static int wifi_sta_event_cb(void *arg, event_module_t module, int event_id, void *event_data)
{
	(void)arg;

	if (module == EVENT_MOD_WIFI) {
		switch (event_id) {
		case EVENT_WIFI_STA_CONNECTED: {
			wifi_event_sta_connected_t *info = (wifi_event_sta_connected_t *)event_data;
			BK_LOGI(TAG, "EVENT_WIFI_STA_CONNECTED ssid=%s\r\n", info->ssid);
			break;
		}
		case EVENT_WIFI_STA_DISCONNECTED: {
			wifi_event_sta_disconnected_t *info = (wifi_event_sta_disconnected_t *)event_data;
			BK_LOGI(TAG, "EVENT_WIFI_STA_DISCONNECTED reason=%d local=%d\r\n",
				info->disconnect_reason, info->local_generated);
			break;
		}
		default:
			break;
		}
	} else if (module == EVENT_MOD_NETIF && event_id == EVENT_NETIF_GOT_IP4) {
		netif_event_got_ip4_t *got_ip = (netif_event_got_ip4_t *)event_data;
		BK_LOGI(TAG, "EVENT_NETIF_GOT_IP4 if=%d\r\n", got_ip->netif_if);
	}

	return BK_OK;
}

static void sta_example_task(void *arg)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

	(void)arg;

	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_CONNECTED, wifi_sta_event_cb, NULL);
	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED, wifi_sta_event_cb, NULL);
	bk_event_register_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4, wifi_sta_event_cb, NULL);

	os_strlcpy(sta_config.ssid, WIFI_STA_EXAMPLE_SSID, sizeof(sta_config.ssid));
	os_strlcpy(sta_config.password, WIFI_STA_EXAMPLE_PASSWORD, sizeof(sta_config.password));

	BK_LOGI(TAG, "connect ssid=%s\r\n", sta_config.ssid);
	bk_wifi_sta_set_config(&sta_config);
	bk_wifi_sta_start();
	rtos_delete_thread(NULL);
}

int main(void)
{
	bk_init();
	rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "sta_example",
			   (beken_thread_function_t)sta_example_task, 2048, 0);
	return 0;
}
