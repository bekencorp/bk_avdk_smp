#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#include <driver/aon_rtc.h>
#include <driver/aon_rtc_types.h>
#endif
#include "bk_pm_internal_api.h"
#include <driver/pwr_clk.h>
#include "cpu_id.h"

#define TAG "os"

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
#define OS_MS_PER_TICK          (1000/configTICK_RATE_HZ)

static uint64_t base_aon_time = 0;
static uint32_t base_os_time = 0;
#endif

void rtos_init_base_time(void) {
#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	base_aon_time = bk_aon_rtc_get_us()/1000;
	base_os_time = rtos_get_time();
	BK_LOGI(TAG, "os time(%dms).\r\n", base_os_time);
	BK_LOGI(TAG, "base aon rtc time: %d:%d\r\n", (uint32_t)(base_aon_time >> 32),
		(uint32_t)(base_aon_time & 0xFFFFFFFF));
#endif
}

uint32_t rtos_get_time_diff(void) {
#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	//uint64_t cur_aon_time = bk_aon_rtc_get_us()/1000;
	uint64_t cur_aon_time = bk_aon_rtc_get_ms();
	uint32_t cur_os_time = rtos_get_time(); //ms
	uint64_t diff_time = (cur_aon_time - base_aon_time); //ms
	uint32_t diff_ms = 0;

	if((uint32_t)(diff_time >> 32) & 0x7FF0000) {
		BK_LOGI(TAG, "aon_rtc overfollow....\r\n");
		BK_DUMP_OUT("diff time: 0x%x:0x%08x\r\n", (u32)(diff_time >> 32), (u32)(diff_time & 0xFFFFFFFF));
		return 0;
	}

	if(cur_os_time >= base_os_time) {
		if (diff_time + base_os_time < cur_os_time) {
			return 0;
		}
		diff_ms = (uint32_t)(diff_time + base_os_time - cur_os_time);
	} else {
		uint64_t cur_os_time_64 = (0x100000000U + cur_os_time);
		if((base_os_time + diff_time) < cur_os_time_64){
			return 0;
		}
		diff_ms = (uint32_t)((base_os_time + diff_time) - cur_os_time_64);
	}

	if (diff_ms > 20000) {
		BK_LOGI(TAG, "aon_rtc diff_ms: %dms.\r\n", diff_ms);
		BK_LOGI(TAG, "cur aon time: 0x%x:0x%08x\r\n", (u32)(cur_aon_time >> 32), (u32)(cur_aon_time & 0xFFFFFFFF));
	}

	return  diff_ms / OS_MS_PER_TICK; // tick
#else
	return 0;
#endif
}


// Workaroud for BK7236 systick rate.
// TODO remove it once BK7236 supports systick of fixed 32K
static uint32_t s_tick_clock_rate = configSYSTICK_CLOCK_HZ;
void os_update_tick_clock_rate(uint32_t rate)
{
	if (rate == s_tick_clock_rate) {
		return;
	}

	/* Calculate the constants required to configure the tick interrupt. */
#if ( configUSE_TICKLESS_IDLE >= 1 )
	ulTimerCountsForOneTick = ( rate / configTICK_RATE_HZ );
	xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
	ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / rate );
	BK_LOGI(TAG, "ulTimerCountsForOneTick=%u, xMaximumPossibleSuppressedTicks=%u, ulStoppedTimerCompensation=%u\r\n",
		ulTimerCountsForOneTick, xMaximumPossibleSuppressedTicks, ulStoppedTimerCompensation);
#endif

	uint32_t cur_val = portNVIC_SYSTICK_CURRENT_VALUE_REG;
	uint32_t cur_load = portNVIC_SYSTICK_LOAD_REG;
	uint32_t new_load = ( rate / configTICK_RATE_HZ ) - 1UL;
	uint32_t new_val = 0;

	if (cur_load && cur_val) {
		new_val = (uint32_t)((cur_val * new_load) / cur_load);
	}

	/* Stop and reset the SysTick. */
	portNVIC_SYSTICK_CTRL_REG = 0UL;
	portNVIC_SYSTICK_CURRENT_VALUE_REG = new_val;

	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = new_load;
	portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;

	BK_LOGI(TAG, "cur_val=%u, cur_load=%u, new_val=%u, new_load=%u\r\n",
		cur_val, cur_load, new_val, new_load);
}

void dump_os_tick_info(void)
{
	BK_LOGI(TAG, "configSYSTICK_CLOCK_HZ=%x\r\n", configSYSTICK_CLOCK_HZ);
	BK_LOGI(TAG, "configCPU_CLOCK_HZ=%x\r\n", configCPU_CLOCK_HZ);
	BK_LOGI(TAG, "configTICK_RATE_HZ=%x\r\n", configTICK_RATE_HZ);
	BK_LOGI(TAG, "portMISSED_COUNTS_FACTOR=%x\r\n", portMISSED_COUNTS_FACTOR);

#if ( configUSE_TICKLESS_IDLE >= 1 )
	BK_LOGI(TAG, "ulTimerCountsForOneTick=%x\r\n", ulTimerCountsForOneTick);
	BK_LOGI(TAG, "xMaximumPossibleSuppressedTicks=%x\r\n", xMaximumPossibleSuppressedTicks);
	BK_LOGI(TAG, "ulStoppedTimerCompensation=%x\r\n", ulStoppedTimerCompensation);
#endif

	BK_LOGI(TAG, "portNVIC_SYSTICK_CURRENT_VALUE_REG3=%u\r\n", portNVIC_SYSTICK_CURRENT_VALUE_REG);
}
