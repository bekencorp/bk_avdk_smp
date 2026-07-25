#include "bk_private/bk_init.h"
#include <os/os.h>
#include <os/str.h>
#include <components/event.h>
#include <modules/wifi.h>
#include <modules/wifi_types.h>

#define TAG "softap_example"

#ifndef WIFI_SOFTAP_EXAMPLE_SSID
#define WIFI_SOFTAP_EXAMPLE_SSID     "beken_softap"
#endif
#ifndef WIFI_SOFTAP_EXAMPLE_PASSWORD
#define WIFI_SOFTAP_EXAMPLE_PASSWORD "12345678"
#endif

static int wifi_softap_event_cb(void *arg, event_module_t module, int event_id, void *event_data)
{
	(void)arg;

	if (module != EVENT_MOD_WIFI) {
		return BK_OK;
	}

	switch (event_id) {
	case EVENT_WIFI_AP_CONNECTED: {
		wifi_event_ap_connected_t *info = (wifi_event_ap_connected_t *)event_data;
		BK_LOGI(TAG, "EVENT_WIFI_AP_CONNECTED sta=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
			info->mac[0], info->mac[1], info->mac[2],
			info->mac[3], info->mac[4], info->mac[5]);
		break;
	}
	case EVENT_WIFI_AP_DISCONNECTED: {
		wifi_event_ap_disconnected_t *info = (wifi_event_ap_disconnected_t *)event_data;
		BK_LOGI(TAG, "EVENT_WIFI_AP_DISCONNECTED sta=%02x:%02x:%02x:%02x:%02x:%02x\r\n",
			info->mac[0], info->mac[1], info->mac[2],
			info->mac[3], info->mac[4], info->mac[5]);
		break;
	}
	default:
		break;
	}

	return BK_OK;
}

static void softap_example_task(void *arg)
{
	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();

	(void)arg;

	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_AP_CONNECTED, wifi_softap_event_cb, NULL);
	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_AP_DISCONNECTED, wifi_softap_event_cb, NULL);

	os_strlcpy(ap_config.ssid, WIFI_SOFTAP_EXAMPLE_SSID, sizeof(ap_config.ssid));
	os_strlcpy(ap_config.password, WIFI_SOFTAP_EXAMPLE_PASSWORD, sizeof(ap_config.password));

	BK_LOGI(TAG, "start softap ssid=%s\r\n", ap_config.ssid);
	bk_wifi_ap_set_config(&ap_config);
	bk_wifi_ap_start();
	rtos_delete_thread(NULL);
}

int main(void)
{
	bk_init();
	rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "softap_example",
			   (beken_thread_function_t)softap_example_task, 2048, 0);
	return 0;
}
