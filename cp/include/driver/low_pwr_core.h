#ifndef __LOW_POWER_CORE_H__
#define __LOW_POWER_CORE_H__

typedef enum
{
	/* Use a dedicated range to avoid clashing with PM thread local events (0~2). */
	PM_CP_CORE_STATE_ENTER_DEEPSLEEP = 0x100,
	PM_CP_CORE_CTRL_CP2_STATE,
	PM_CP_CORE_CP2_RECOVERY,
	PM_CP_CORE_RTC_DEEPSLEEP,
	PM_CP_CORE_GET_CP_DATA,
	PM_CP_CORE_POWER_CTRL,
	PM_CP_CORE_CLK_CTRL,
	PM_CP_CORE_SLEEP_CTRL,
	PM_CP_CORE_FREQ_CTRL,
	PM_CP_CORE_EXTERNAL_LDO,
	PM_CP_CORE_PSRAM_POWER,
	PM_CP_CORE_WAKEUP_SRC_CFG,
	PM_CP_CORE_RTC_WAKEUPED,
	PM_CP_CORE_GPIO_WAKEUPED,
	PM_CP_CORE_STATE_MAX
}pm_cp_core_state_e;

typedef struct
{
	pm_cp_core_state_e event;
	uint32_t 			 param1;
	uint32_t             param2;
	uint32_t             param3;
} low_pwr_core_msg_t;
/**
 * @brief low pwr core init
 *
 * aov pm init
 *
 * @attention
 * - This API is to init low pwr core
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_core_init();
/**
 * @brief send pwr_core msg
 *
 * send pwr_core msg
 *
 * @attention
 * - This API is to send low pwr_core msg
 *
 * @param
 * -msg
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_core_send_msg(low_pwr_core_msg_t *msg);
#endif
