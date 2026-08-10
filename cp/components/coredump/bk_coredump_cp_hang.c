#include <stdint.h>
#include <stdbool.h>
#include <components/log.h>
#include <driver/ipi_driver.h>
#include <modules/pm.h>
#include <os/os.h>

#define CP_HANG_TAG "cp_hang"
#define CP_HANG_HEARTBEAT_EVENT 1U
#define CP_HANG_HEARTBEAT_PAUSE_EVENT 2U
#define CP_HANG_HEARTBEAT_RESUME_EVENT 3U
#define CP_HANG_HEARTBEAT_STACK_SIZE 1024U
#define CP_HANG_HEARTBEAT_PRIORITY 0U
#define CP_HANG_TIMEOUT_MARGIN_MS 2000U
#define CP_HANG_TIMEOUT_FALLBACK_MS 6000U

#if CONFIG_INT_AON_WDT
#define CP_HANG_AON_WDT_PERIOD_MS CONFIG_INT_AON_WDT_PERIOD_MS
#elif CONFIG_INT_WDT
#define CP_HANG_AON_WDT_PERIOD_MS CONFIG_INT_WDT_PERIOD_MS
#else
#define CP_HANG_AON_WDT_PERIOD_MS (CP_HANG_TIMEOUT_FALLBACK_MS + CP_HANG_TIMEOUT_MARGIN_MS)
#endif

static volatile uint8_t s_cp_hang_ap_power_off;

static uint16_t cp_hang_get_ap_timeout_ms(void)
{
	uint32_t timeout_ms;
	uint32_t min_timeout_ms = CONFIG_CP_HANG_DUMP_BY_AP_PERIOD_MS + 500U;

	if (CP_HANG_AON_WDT_PERIOD_MS > CP_HANG_TIMEOUT_MARGIN_MS) {
		timeout_ms = CP_HANG_AON_WDT_PERIOD_MS - CP_HANG_TIMEOUT_MARGIN_MS;
	} else {
		timeout_ms = CP_HANG_AON_WDT_PERIOD_MS / 2U;
	}

	if (timeout_ms < min_timeout_ms) {
		timeout_ms = min_timeout_ms;
	}

	if (timeout_ms > UINT16_MAX) {
		timeout_ms = UINT16_MAX;
	}

	return (uint16_t)timeout_ms;
}

static void cp_hang_send_ap_event(uint8_t event)
{
	if (s_cp_hang_ap_power_off != 0U) {
		return;
	}

	if (bk_ipi_send_domain(IPI_AP_CORE0, IPI_DOMAIN_CP_HANG_DEBUG,
		event, cp_hang_get_ap_timeout_ms()) != BK_OK) {
		BK_LOGW(CP_HANG_TAG, "send cp hang event %u failed\r\n", event);
	}
}

static void cp_hang_ap_poweroff_callback(void *arg)
{
	(void)arg;

	s_cp_hang_ap_power_off = 1U;
	BK_LOGI(CP_HANG_TAG, "AP power off, pause CP hang heartbeat\r\n");
}

static void cp_hang_ap_poweron_callback(void *arg)
{
	(void)arg;

	s_cp_hang_ap_power_off = 0U;
	cp_hang_send_ap_event(CP_HANG_HEARTBEAT_RESUME_EVENT);
	BK_LOGI(CP_HANG_TAG, "AP power on, resume CP hang heartbeat\r\n");
}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
void bk_cp_hang_debug_heartbeat_lv_enter(void)
{
	cp_hang_send_ap_event(CP_HANG_HEARTBEAT_PAUSE_EVENT);
}

void bk_cp_hang_debug_heartbeat_lv_exit(void)
{
	cp_hang_send_ap_event(CP_HANG_HEARTBEAT_RESUME_EVENT);
}
#endif

static void cp_hang_register_ap_power_callbacks(void)
{
	bk_err_t ret;

	ret = bk_pm_ap_ctrl_callback_register(cp_hang_ap_poweroff_callback, NULL,
		PM_AP_CTRL_CB_TYPE_POWER_OFF);
	if (ret != BK_OK) {
		BK_LOGW(CP_HANG_TAG, "register AP power off callback failed: %d\r\n", ret);
	}

	ret = bk_pm_ap_ctrl_callback_register(cp_hang_ap_poweron_callback, NULL,
		PM_AP_CTRL_CB_TYPE_POWER_ON);
	if (ret != BK_OK) {
		BK_LOGW(CP_HANG_TAG, "register AP power on callback failed: %d\r\n", ret);
	}
}

static void cp_hang_debug_heartbeat_task(void *param)
{
	(void)param;

	while (1) {
		rtos_delay_milliseconds(CONFIG_CP_HANG_DUMP_BY_AP_PERIOD_MS);
		if (s_cp_hang_ap_power_off != 0U) {
			continue;
		}
		cp_hang_send_ap_event(CP_HANG_HEARTBEAT_EVENT);
	}
}

bk_err_t bk_cp_hang_debug_heartbeat_init(void)
{
	bk_err_t ret;

	cp_hang_register_ap_power_callbacks();

	ret = rtos_create_thread(NULL, CP_HANG_HEARTBEAT_PRIORITY,
		"cp_hang_hb", cp_hang_debug_heartbeat_task,
		CP_HANG_HEARTBEAT_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		BK_LOGE(CP_HANG_TAG, "create cp hang heartbeat task failed: %d\r\n", ret);
	}

	return ret;
}
