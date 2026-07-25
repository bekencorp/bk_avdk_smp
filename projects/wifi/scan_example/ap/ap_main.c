#include "bk_private/bk_init.h"
#include <os/os.h>
#include <components/event.h>
#include <modules/wifi.h>
#include <modules/wifi_types.h>

#define TAG "scan_example"

static int wifi_scan_event_cb(void *arg, event_module_t module, int event_id, void *event_data)
{
	(void)arg;
	(void)event_data;

	if (module != EVENT_MOD_WIFI || event_id != EVENT_WIFI_SCAN_DONE) {
		return BK_OK;
	}

	wifi_scan_result_t scan_result = {0};

	BK_LOGI(TAG, "EVENT_WIFI_SCAN_DONE\r\n");
	if (bk_wifi_scan_get_result(&scan_result) == BK_OK) {
		bk_wifi_scan_dump_result(&scan_result);
		bk_wifi_scan_free_result(&scan_result);
	} else {
		BK_LOGE(TAG, "bk_wifi_scan_get_result failed\r\n");
	}

	return BK_OK;
}

static void scan_example_task(void *arg)
{
	(void)arg;

	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE, wifi_scan_event_cb, NULL);
	BK_LOGI(TAG, "start full channel scan\r\n");
	bk_wifi_scan_start(NULL);
	rtos_delete_thread(NULL);
}

int main(void)
{
	bk_init();
	rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY, "scan_example",
			   (beken_thread_function_t)scan_example_task, 2048, 0);
	return 0;
}
