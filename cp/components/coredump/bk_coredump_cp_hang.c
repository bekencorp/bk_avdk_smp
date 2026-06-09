#include <stdint.h>
#include <stdbool.h>
#include <components/log.h>
#include <driver/ipi_driver.h>
#include <os/os.h>

#define CP_HANG_TAG "cp_hang"
#define CP_HANG_HEARTBEAT_EVENT 1U
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

static volatile uint8_t s_cp_hang_debug_heartbeat_paused;

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

void bk_cp_hang_debug_heartbeat_pause(bool pause)
{
	s_cp_hang_debug_heartbeat_paused = pause ? 1U : 0U;
}

static void cp_hang_debug_heartbeat_task(void *param)
{
	uint16_t ap_timeout_ms = cp_hang_get_ap_timeout_ms();

	(void)param;

	while (1) {
		rtos_delay_milliseconds(CONFIG_CP_HANG_DUMP_BY_AP_PERIOD_MS);
		if (s_cp_hang_debug_heartbeat_paused != 0U) {
			continue;
		}
		if (bk_ipi_send_domain(IPI_AP_CORE0, IPI_DOMAIN_CP_HANG_DEBUG,
			CP_HANG_HEARTBEAT_EVENT, ap_timeout_ms) != BK_OK) {
			BK_LOGW(CP_HANG_TAG, "send cp hang debug heartbeat failed\r\n");
		}
	}
}

bk_err_t bk_cp_hang_debug_heartbeat_init(void)
{
	bk_err_t ret;

	ret = rtos_create_thread(NULL, CP_HANG_HEARTBEAT_PRIORITY,
		"cp_hang_hb", cp_hang_debug_heartbeat_task,
		CP_HANG_HEARTBEAT_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		BK_LOGE(CP_HANG_TAG, "create cp hang heartbeat task failed: %d\r\n", ret);
	}

	return ret;
}
