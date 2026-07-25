/*
 * usb_hset AP main.
 *
 * Minimal USB device build: bring up an HS CDC-ACM device at power-up so a
 * compliance host (USBHSET) can trigger the USB 2.0 HS test modes via
 * SET_FEATURE(TEST_MODE) for eye-diagram / signal-quality measurement.
 * All original usb_example features (udisk host/MSC, MTP, UVC, UAC) and their
 * CLI commands have been removed.
 */
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/log.h>

#define TAG "usb_hset"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#if CONFIG_USB_HSET

extern void bk_usb_hset_cdc_device_init(void);

#define USB_CDC_INIT_TASK_PRIORITY    BEKEN_DEFAULT_WORKER_PRIORITY
#define USB_CDC_INIT_TASK_STACK_SIZE  (1024 * 8)
#define USB_CDC_INIT_TASK_NAME        "usb_hset"

static beken_thread_t s_usb_cdc_init_thread = NULL;

/* Run the device bring-up off a worker thread (same pattern usb_example uses
 * for msc_storage_init) so main() is not blocked by the PHY settle delays. */
static void usb_cdc_init_task(void *arg)
{
	(void)arg;

	bk_usb_hset_cdc_device_init();

	s_usb_cdc_init_thread = NULL;
	rtos_delete_thread(NULL);
}
#endif /* CONFIG_USB_HSET */

int main(void)
{
	bk_init();

	LOGI("M55 main running (USB HSET CDC device)...\r\n");

#if CONFIG_USB_HSET
	bk_err_t ret = rtos_create_thread(&s_usb_cdc_init_thread,
		USB_CDC_INIT_TASK_PRIORITY,
		USB_CDC_INIT_TASK_NAME,
		(beken_thread_function_t)usb_cdc_init_task,
		USB_CDC_INIT_TASK_STACK_SIZE,
		NULL);
	if (ret != BK_OK) {
		LOGE("create %s task failed ret=%d\r\n", USB_CDC_INIT_TASK_NAME, ret);
		return -1;
	}
#else
	LOGE("CONFIG_USB_HSET is not enabled; nothing to do\r\n");
#endif

	return 0;
}
