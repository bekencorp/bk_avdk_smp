// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"
#include "components/system.h"
#include "modules/pm.h"
#include "driver/timer.h"
#if CONFIG_OPENTHREAD_AON_US_TICK
#include "driver/aon_rtc.h"
#endif
#include "os/os.h"
#include <soc/soc.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>

#if CONFIG_OPENTHREAD

#define BK_OT_ALARM__LOG_EN 0
#if BK_OT_ALARM__LOG_EN
#define bk_ot_alarm_log os_printf
#else
#define bk_ot_alarm_log
#endif


volatile static uint32_t s_alarm_ms = 0;
volatile bool s_is_ms_running = false;
volatile static uint32_t s_alarm_us =0;
volatile bool s_is_us_running =false;
#if !CONFIG_OPENTHREAD_AON_US_TICK
volatile uint64_t bk_ot_alarm_us_tick=0;


#define TIMER_CLOCK_MHZ (TIMER_CLOCK_FREQ_XTAL/1000)
#define TIMER0_REG_SET(reg_id, l, h, v) REG_SET((SOC_TIMER0_REG_BASE + ((reg_id) << 2)), (l), (h), (v))
__IRAM_SEC static inline uint32_t timer_hal_get_timer0_cnt(void)
{
	volatile uint32_t int_level = 0;
	uint32_t cnt = 0;
	int_level = rtos_enter_critical();
	TIMER0_REG_SET(8, 2, 3, 0);
	TIMER0_REG_SET(8, 0, 0, 1);
	while (REG_READ((SOC_TIMER0_REG_BASE + (8 << 2))) & BIT(0));

	cnt = REG_READ(SOC_TIMER0_REG_BASE + (9 << 2));
	rtos_exit_critical(int_level);
	return cnt;
}
uint64_t bk_get_us_cnt(void)
{
    return (uint64_t)((bk_ot_alarm_us_tick + timer_hal_get_timer0_cnt())/TIMER_CLOCK_MHZ);
}
#endif

// The current time in microseconds.
uint64_t otPlatTimeGet(void)
{
#if CONFIG_OPENTHREAD_AON_US_TICK
    return bk_aon_rtc_get_us();
#else
    return bk_get_us_cnt();
#endif
}

//The current platform clock accuracy, in PPM
uint16_t otPlatTimeGetXtalAccuracy(void)
{
    if(bk_pm_lpo_src_get() == PM_LPO_SRC_DIVD)
        return 20;
    else if(bk_pm_lpo_src_get() == PM_LPO_SRC_X32K)
        return 100;
    else
        return 1000;
}


/**
 * Set the alarm to fire at @p aDt milliseconds after @p aT0.
 *
 * For @p aT0 the platform MUST support all values in [0, 2^32-1].
 * For @p aDt, the platform MUST support all values in [0, 2^31-1].
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aT0        The reference time.
 * @param[in] aDt        The time delay in milliseconds from @p aT0.
 */
void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    s_alarm_ms= aT0+aDt;
    s_is_ms_running = true;
}

/**
 * Stop the alarm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    s_is_ms_running = false;
}

/**
 * Get the current time.
 *
 * The current time MUST represent a free-running timer. When maintaining current time, the time value MUST utilize the
 * entire range [0, 2^32-1] and MUST NOT wrap before 2^32.
 *
 * @returns The current time in milliseconds.
 */
uint32_t otPlatAlarmMilliGetNow(void)
{//rtos_get_time()
#if CONFIG_OPENTHREAD_AON_US_TICK
    return bk_aon_rtc_get_ms();
#else
    return (uint32_t)(otPlatTimeGet()/OT_US_PER_MS);
#endif
}

/**
 * Set the alarm to fire at @p aDt microseconds after @p aT0.
 *
 * For @p aT0, the platform MUST support all values in [0, 2^32-1].
 * For @p aDt, the platform MUST support all values in [0, 2^31-1].
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aT0        The reference time.
 * @param[in]  aDt        The time delay in microseconds from @p aT0.
 */
void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    s_alarm_us = aT0+aDt;
    s_is_us_running = true;
}

/**
 * Stop the alarm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    s_is_us_running = false;
}

/**
 * Get the current time.
 *
 * The current time MUST represent a free-running timer. When maintaining current time, the time value MUST utilize the
 * entire range [0, 2^32-1] and MUST NOT wrap before 2^32.
 *
 * @returns  The current time in microseconds.
 */
uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)otPlatTimeGet();
}

#if CONFIG_OPENTHREAD_AON_US_TICK
void bk_ot_alarm_init(void)
{
    bk_ot_alarm_log("[%s] use aon_rtc as clock source,no need init\r\n",__func__);
}
void bk_ot_alarm_deinit(void)
{
    bk_ot_alarm_log("[%s] use aon_rtc as clock source,no need deinit\r\n",__func__);
}

#else
//===use timer0 =============
#define MAX_TIIMER0_VAL 4000000000//2400000000//4280000000//4294960000//4294966000
#define  MAX_TIMER0_VAL_MS 100000//60000//107000// 107374//ms 165191//ms
__IRAM_SEC static void bk_ot_alarm_timer_isr(timer_id_t chan)
{
    volatile uint32_t int_level=rtos_enter_critical();
    bk_ot_alarm_us_tick+=MAX_TIIMER0_VAL;
    rtos_exit_critical(int_level);
}
extern bk_err_t bk_timer_us_start_callback(timer_id_t timer_id, uint64_t time_us, timer_isr_t callback);
void bk_ot_alarm_init(void)
{
    bk_ot_alarm_log("[%s] enter alarm\r\n",__func__);
    bk_ot_alarm_us_tick=0;
    bk_timer_start(TIMER_ID0,MAX_TIMER0_VAL_MS,bk_ot_alarm_timer_isr);
}
void bk_ot_alarm_deinit(void)
{
    bk_timer_stop(TIMER_ID0);
}
#endif
static inline bool is_expired(uint32_t target, uint32_t now)
{
    bk_ot_alarm_log("[%s] target:%lu now:%lu\r\n",__func__,target,now);
    return (now>=target)?true:false;
}

static inline uint32_t calculate_duration(uint32_t target,uint32_t now)
{
    return is_expired(target,now)?0:(target-now);
}
#if CONFIG_OT_TRIP_COEX_EN
void bk_ot_target_val(uint32_t *tgtMs, uint32_t *tgtUs)
{
    if(s_is_ms_running)
    {
        *tgtMs = calculate_duration(s_alarm_ms, otPlatAlarmMilliGetNow());
    }
    else
        *tgtMs = 0;
    if(s_is_us_running)
    {
        *tgtUs = calculate_duration(s_alarm_us, otPlatAlarmMicroGetNow());
    }
    else
        *tgtUs = 0;
}

#endif
bk_err_t bk_ot_alarm_process(otInstance *aInstance)
{
    bk_ot_alarm_log("[%s] is_ms:%d ms:%lu is_us:%d us:%lu\r\n",__func__,s_is_ms_running,s_alarm_ms,s_is_us_running,s_alarm_us);

    if(s_is_ms_running && is_expired(s_alarm_ms, otPlatAlarmMilliGetNow()))
    {
        s_is_ms_running = false;
        if(aInstance==NULL)
            os_printf("ms instance not ready\r\n");
        else
        {
#if OPENTHREAD_CONFIG_DIAG_ENABLE
            if(otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired(aInstance);
            }
            else
#endif
            {
                otPlatAlarmMilliFired(aInstance);
            }
        }
    }
    if(s_is_us_running && is_expired(s_alarm_us, otPlatAlarmMicroGetNow()))
    {
        s_is_us_running = false;
        if(aInstance==NULL)
            os_printf("us instance not ready\r\n");
        else
        {
            otPlatAlarmMicroFired(aInstance);
        }
    }

    return BK_OK;
}
#endif
