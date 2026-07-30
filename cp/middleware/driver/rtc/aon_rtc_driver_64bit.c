// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include <os/mem.h>
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

#include <driver/int.h>
#include "sys_driver.h"
#include "sys/time.h"
#include "bk_arch.h"

#include <driver/aon_rtc.h>
#include "aon_rtc_hal_64bit.h"
#include "aon_rtc_driver_64bit.h"

#if CONFIG_ROSC_COMPENSATION
#include <driver/rosc_32k.h>
#endif

/* 
 * NOTES: System entery deepsleep or reboot, the aon rtc time should reserved.
 * 1.When enter deep sleep, the DTCM power is lost,so have to save time to flash.
 * 2.Write time to flash takes about 200us~3ms, When reboot system,the easy flash API call rtos_get_semaphore in ISR cause assert
 * 3.When reboot system,DTCM doesn't loss power,so can save time in DTCM.
 */
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static volatile spinlock_t rtc_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

#define AON_RTC_UNIT_NUM (AON_RTC_HAL_UNIT_NUM)
#define AON_RTC_NAME ((uint8_t *)"aon_rtc")

typedef struct {
	//aon_rtc_id_t id;	//no needs it until now:the id is matched from APP,DRIVER,HAL,HW/SOC layer.
	bool inited;
	//uint8_t using_cnt;	//remove it as only one HW can't for many APPs

	aon_rtc_hal_t hal;

	//Record APP param's
	//bool period;
	//uint32_t tick;

	//Alarm list
	alarm_node_t *alarm_head_p;
	uint32_t alarm_node_cnt;
} aon_rtc_driver_t;

typedef struct {
	aon_rtc_isr_t callback;
	void *isr_param;
} aon_rtc_callback_t;

typedef struct {
	alarm_node_t nodes[AON_RTC_MAX_ALARM_CNT];
	uint64_t busy_bits;
} aon_rtc_nodes_memory_t;

static aon_rtc_driver_t s_aon_rtc[AON_RTC_UNIT_NUM] = {0};
static aon_rtc_callback_t s_aon_rtc_tick_isr[AON_RTC_UNIT_NUM] = {NULL};
static aon_rtc_callback_t s_aon_rtc_upper_isr[AON_RTC_UNIT_NUM] = {NULL};
static aon_rtc_nodes_memory_t *s_aon_rtc_nodes_p[AON_RTC_UNIT_NUM];
static uint64_t s_last_setted_lpo_tick = 0;
static uint64_t s_last_set_time = 0;

static void aon_rtc_interrupt_disable(aon_rtc_id_t id);

static inline void aon_rtc_clear_irq_pending(aon_rtc_id_t id)
{
	if (id == AON_RTC_ID_1) {
		__NVIC_ClearPendingIRQ(INT_SRC_RTC);
#if (SOC_AON_RTC_UNIT_NUM > 1)
	} else if (id == AON_RTC_ID_2) {
		__NVIC_ClearPendingIRQ(INT_SRC_RTC2);
#endif
	}
}

#define AONRTC_GET_SET_TIME_RTC_ID AON_RTC_ID_1

#if CONFIG_RTC_ANA_WAKEUP_SUPPORT
#define RTC_ANA_TIME_PERIOD_MAX 16
static uint32_t s_wkup_time_period = 0;
#endif

#define AON_RTC_OPS_SAFE_TICK_CNT (5)
#define AON_RTC_OPS_SAFE_DELAY_US (125)
/*
 * AON RTC uses 32k clock,and 3 cycles later can be clock sync.
 * CPU clock is more faster then AON RTC, so software operates AON RTC register
 * will be effect after 125 us(RTC 3+1 clock cycles).
 * So if operate the same register in 125 us, the second operation will be failed.
 */
static void aon_rtc_delay_to_grantee_ops_safe()
{
	extern void bk_delay_us(UINT32 us);

	bk_delay_us(AON_RTC_OPS_SAFE_DELAY_US);
}

#ifdef CONFIG_EXTERN_32K
static uint32_t s_aon_rtc_clock_freq = AON_RTC_EXTERN_32K_CLOCK_FREQ;
#else
static uint32_t s_aon_rtc_clock_freq = AON_RTC_DEFAULT_CLOCK_FREQ;
#endif
static uint64_t s_time_base_us = 0;
static uint64_t s_time_base_tick = 0;

static inline uint32_t rtc_enter_critical()
{
       uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
       spin_lock(&rtc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       return flags;
}

static inline void rtc_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
       spin_unlock(&rtc_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       rtos_enable_int(flags);
}

__IRAM_SEC float bk_rtc_get_ms_tick_count(void) {
	return (float)s_aon_rtc_clock_freq/1000;
}

uint32_t bk_rtc_get_clock_freq(void) {
	return s_aon_rtc_clock_freq;
}

uint64_t rtc_tick_to_us(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick * 125LL * 125LL) >> 9); // rtc_tick * 1000 * 1000 / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return ((rtc_tick * 125LL) >> 2);   // rtc_tick * 1000 * 1000 / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return (rtc_tick * 1000LL * 1000LL) / s_aon_rtc_clock_freq;
	}

	return -1;
}

uint64_t rtc_us_to_tick(uint64_t us)
{
    return (us * bk_rtc_get_clock_freq()) / 1000000;
}

static uint64_t rtc_tick_to_ms(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick * 125) >> 12); // rtc_tick * 1000 / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return (rtc_tick >> 5);   // rtc_tick * 1000 / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return (rtc_tick * 1000LL) / s_aon_rtc_clock_freq;
	}

	return -1;
}

static uint64_t rtc_tick_to_s(uint64_t rtc_tick)
{
	if(s_aon_rtc_clock_freq == AON_RTC_EXTERN_32K_CLOCK_FREQ)
	{
		return ((rtc_tick) >> 15); // rtc_tick / 32768;
	}
	else if(s_aon_rtc_clock_freq == AON_RTC_DEFAULT_CLOCK_FREQ)
	{
		return (rtc_tick >> 8) / 125LL;   // rtc_tick / 32000;
	}
	else if(s_aon_rtc_clock_freq != 0)
	{
		return rtc_tick / s_aon_rtc_clock_freq;
	}

	return -1;
}

static inline uint64_t get_diff_time_us(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_us(time_tick - s_time_base_tick); // *1000LL/bk_rtc_get_ms_tick_count();
	return time_diff;
}

void bk_rtc_update_base_time(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_us(time_tick - s_time_base_tick);  // *1000LL/bk_rtc_get_ms_tick_count();

	s_time_base_tick = time_tick;
	s_time_base_us += time_diff;
	// AON_RTC_LOGD("s_time_base_tick: 0x%x:0x%08x\r\n", (u32)(s_time_base_tick >> 32), (u32)(s_time_base_tick & 0xFFFFFFFF));
	// AON_RTC_LOGD("s_time_base_us: 0x%x:0x%08x\r\n", (u32)(s_time_base_us >> 32), (u32)(s_time_base_us & 0xFFFFFFFF));
}

void bk_rtc_set_clock_freq(uint32_t clock_freq){
#if CONFIG_AON_RTC_DYNAMIC_CLOCK_SUPPORT
	AON_RTC_LOGD("Set clock freq: %d\n", clock_freq);
	if (clock_freq == s_aon_rtc_clock_freq) {
		return;
	}
	uint32_t int_level = rtc_enter_critical();
	{
		bk_rtc_update_base_time();
		s_aon_rtc_clock_freq = clock_freq;
	}
	rtc_exit_critical(int_level);
#endif
}

 __IRAM_SEC uint64_t bk_aon_rtc_get_us(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_us(time_tick - s_time_base_tick);  // *1000LL/bk_rtc_get_ms_tick_count();
    uint64_t time_us = s_time_base_us + time_diff;
    return  time_us;
}

 __IRAM_SEC uint64_t bk_aon_rtc_get_ms(void) {
	uint64_t time_tick = bk_aon_rtc_get_current_tick(AONRTC_GET_SET_TIME_RTC_ID);
	uint64_t time_diff = rtc_tick_to_ms(time_tick - s_time_base_tick);
    uint64_t time_ms = s_time_base_us/1000 + time_diff;
    return  time_ms;
}

static void alarm_dump_node(alarm_node_t *node_p)
{
#if CONFIG_AON_RTC_DEBUG
	AON_RTC_LOGV("%s[+]\r\n", __func__);

	AON_RTC_LOGV("node_p=0x%x\r\n", node_p);
	if(node_p)
	{		
		AON_RTC_LOGV("next=0x%x\r\n", node_p->next);
		AON_RTC_LOGV("name=%s\r\n", node_p->name);
		AON_RTC_LOGV("period_tick=0x%x\r\n", (uint32_t)node_p->period_tick);
		AON_RTC_LOGV("period_cnt=%d\r\n", node_p->period_cnt);
		AON_RTC_LOGV("start_tick=0x%x\r\n", (uint32_t)node_p->start_tick);
		AON_RTC_LOGV("expired_tick=0x%x\r\n", (uint32_t)node_p->expired_tick);
	}

	AON_RTC_LOGV("%s[-]\r\n", __func__);
#endif
}

static void alarm_dump_list(alarm_node_t *head_p)
{
#if CONFIG_AON_RTC_DEBUG
	alarm_node_t *cur_p = head_p;
	uint32_t count = 0;
	uint32_t int_level = 0;

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	int_level = rtc_enter_critical();
	while(cur_p)
	{
		alarm_dump_node(cur_p);
		count++;

		cur_p = cur_p->next;
	}
	rtc_exit_critical(int_level);

	AON_RTC_LOGV("node cnt=%d\r\n", count);

	AON_RTC_LOGV("%s[-]\r\n", __func__);
#endif
}

static alarm_node_t* aon_rtc_request_node(aon_rtc_id_t id)
{
	uint32_t i = 0; 

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	for(i = 0; i < AON_RTC_MAX_ALARM_CNT; i++)
	{
		if((s_aon_rtc_nodes_p[id]->busy_bits & (0x1<<i)) == 0)
		{
			AON_RTC_LOGV("%s[-]:node[%d]=0x%x\r\n", __func__, i, &s_aon_rtc_nodes_p[id]->nodes[i]);
			s_aon_rtc_nodes_p[id]->busy_bits |= (0x1<<i);
			return &s_aon_rtc_nodes_p[id]->nodes[i];
		}
	}

	return NULL;
}

static void aon_rtc_release_node(aon_rtc_id_t id, alarm_node_t *node_p)
{
	uint32_t i = 0; 

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	for(i = 0; i < AON_RTC_MAX_ALARM_CNT; i++)
	{
		if(&s_aon_rtc_nodes_p[id]->nodes[i] == node_p)
		{
			s_aon_rtc_nodes_p[id]->busy_bits &= ~(0x1<<i);
			os_memset(&s_aon_rtc_nodes_p[id]->nodes[i], 0, sizeof(alarm_node_t));
			AON_RTC_LOGV("%s[-]:node[%d]=0x%x\r\n", __func__, i, &s_aon_rtc_nodes_p[id]->nodes[i]);
			break;
		}
	}

	if(i >= AON_RTC_MAX_ALARM_CNT)
	{
		AON_RTC_LOGW("release node err\r\n");
	}
}

static int32_t alarm_insert_node(aon_rtc_id_t id, alarm_node_t *node_p)
{
	alarm_node_t *cur_p = NULL;
	alarm_node_t *next_p = NULL;
	uint32_t int_level = 0;

	AON_RTC_LOGV("%s[+]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);

	alarm_dump_list(s_aon_rtc[id].alarm_head_p);

	int_level = rtc_enter_critical();

	//check whether the same name
	cur_p = s_aon_rtc[id].alarm_head_p;
	while(cur_p)
	{
		if(strncmp((const char *)cur_p->name, (const char *)node_p->name, ALARM_NAME_MAX_LEN) == 0)
		{
			AON_RTC_LOGW("name=%s has registered\r\n", node_p->name);
			rtc_exit_critical(int_level);
			return -1;
		}

		cur_p = cur_p->next;
	}

	//search the node position
	cur_p = s_aon_rtc[id].alarm_head_p;

	//no node
	if(cur_p == NULL)
	{
		s_aon_rtc[id].alarm_head_p = node_p;
		s_aon_rtc[id].alarm_node_cnt++;
		AON_RTC_LOGV("insert first node 0x%x,name=%s\r\n", node_p, node_p->name);
		
		rtc_exit_critical(int_level);
		return 0;
	}

	//only one node
	next_p = cur_p->next;
	if(next_p == NULL)
	{
		if(cur_p->expired_tick <= node_p->expired_tick)
			cur_p->next = node_p;
		else
		{
			node_p->next = cur_p;
			s_aon_rtc[id].alarm_head_p = node_p;
		}
		s_aon_rtc[id].alarm_node_cnt++;
		rtc_exit_critical(int_level);

		//TODO:log debug
		AON_RTC_LOGV("list total has two nodes\r\n");

		return 0;
	}

	//more then 2 nodes
	while(next_p)
	{
		if(cur_p->expired_tick <= node_p->expired_tick)	//move after cur_p
		{
			if(next_p->expired_tick <= node_p->expired_tick)	//search next
			{
				cur_p = next_p;
				next_p = next_p->next;
				continue;
			}
			else	//insert
			{
				node_p->next = next_p;
				cur_p->next = node_p;
				s_aon_rtc[id].alarm_node_cnt++;
				rtc_exit_critical(int_level);
				return 0;
			}
		}
		else	//insert before cur_p, means the first node, head
		{
			node_p->next = cur_p;
			s_aon_rtc[id].alarm_head_p = node_p;
			s_aon_rtc[id].alarm_node_cnt++;
			rtc_exit_critical(int_level);
			return 0;
		}
	}

	//the last one
	cur_p->next = node_p;
	s_aon_rtc[id].alarm_node_cnt++;
	rtc_exit_critical(int_level);

	//dump list info
	alarm_dump_list(s_aon_rtc[id].alarm_head_p);

	AON_RTC_LOGV("%s[-]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);

	return 0;
}

static alarm_node_t *alarm_remove_node(aon_rtc_id_t id, uint8_t *name_p)
{
	alarm_node_t *cur_p = NULL;
	alarm_node_t *previous_p = NULL;
	alarm_node_t *remove_node_p = NULL;
	uint32_t int_level = 0;
	uint32_t node_cnt = 0;

	AON_RTC_LOGV("%s[+]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);

	int_level = rtc_enter_critical();
	//
	previous_p = cur_p = s_aon_rtc[id].alarm_head_p;
	while(cur_p)
	{
		//double check pointer is valid
		node_cnt++;
		BK_ASSERT(node_cnt <= AON_RTC_MAX_ALARM_CNT); /* ASSERT VERIFIED */

		if(strncmp((const char *)cur_p->name, (const char *)name_p, ALARM_NAME_MAX_LEN) == 0)
		{
			//first one
			if(previous_p == cur_p)
			{
				remove_node_p = s_aon_rtc[id].alarm_head_p;
				s_aon_rtc[id].alarm_head_p = cur_p->next;
				s_aon_rtc[id].alarm_node_cnt--;

				AON_RTC_LOGV("free=0x%x,name=%s\r\n", cur_p, cur_p->name);
				aon_rtc_release_node(id, cur_p);
				break;
			}
			else
			{
				remove_node_p = cur_p;
				previous_p->next = cur_p->next;
				s_aon_rtc[id].alarm_node_cnt--;
				AON_RTC_LOGV("free=0x%x,name=%s\r\n", cur_p, cur_p->name);
				aon_rtc_release_node(id, cur_p);
				break;
			}
		}

		previous_p = cur_p;
		cur_p = cur_p->next;
	}

	rtc_exit_critical(int_level);

	if(remove_node_p == NULL)
	{
		AON_RTC_LOGV("%s:can't find %s alarm\r\n", __func__, name_p);
	}

	//dump list info
	alarm_dump_list(s_aon_rtc[id].alarm_head_p);

	AON_RTC_LOGV("%s[-]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);

	return remove_node_p;
}

static void alarm_update_expeired_nodes(aon_rtc_id_t id)
{
	alarm_node_t *cur_p = NULL;
	alarm_node_t *next_p = NULL;
	//uint32_t node_cnt = 0;
	uint64_t cur_tick = 0;
	uint32_t int_level = 0;

	AON_RTC_LOGV("%s[+]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);
	
	alarm_dump_list(s_aon_rtc[id].alarm_head_p);

	int_level = rtc_enter_critical();

	//search the node position
	while(s_aon_rtc[id].alarm_head_p)
	{
		cur_p = s_aon_rtc[id].alarm_head_p;
		next_p = cur_p->next;

		alarm_dump_node(cur_p);
		alarm_dump_node(cur_p->next);

		//double check pointer is valid
		//node_cnt++;
		//BK_ASSERT(node_cnt <= AON_RTC_MAX_ALARM_CNT); /* ASSERT VERIFIED */ ==>Removed:Maybe ISR delay and lots off alram needs to callback

		cur_tick = bk_aon_rtc_get_current_tick(id);
		//maybe callback runs too much time,so assume the time in bk_rtc_get_ms_tick_count() means has expired
		if(cur_p->expired_tick <= cur_tick + AON_RTC_OPS_SAFE_TICK_CNT)
		{
			uint32_t experied_cnt = 1;

			//maybe isr delay which causes many times experied
			experied_cnt += (cur_tick + AON_RTC_OPS_SAFE_TICK_CNT - cur_p->expired_tick) / cur_p->period_tick;
			if(experied_cnt >= cur_p->period_cnt)
			{
				experied_cnt = cur_p->period_cnt;
				cur_p->period_cnt = 0;
			}
			else if(cur_p->period_cnt != ALARM_LOOP_FOREVER)
				cur_p->period_cnt -= experied_cnt;

			if(cur_p->callback)
			{
				for(uint32_t i = 0; i < experied_cnt; i++)
				{
					cur_p->callback(id, cur_p->name, cur_p->cb_param_p);
				}
			}

			//last time alarm
			if(cur_p->period_cnt == 0)
			{
				s_aon_rtc[id].alarm_head_p = cur_p->next;	//head move to next
				s_aon_rtc[id].alarm_node_cnt--;

				aon_rtc_release_node(id, cur_p);
			}
			//loop timer not complete
			else 
			{
				//has next
				if(next_p)	//move to switable position
				{
					s_aon_rtc[id].alarm_head_p = cur_p->next;	//head move to next
					cur_p->expired_tick += cur_p->period_tick * experied_cnt;
					cur_p->next = NULL;		//cur_p is removed
					s_aon_rtc[id].alarm_node_cnt--; //it will ++ in alarm_insert_node
					if(alarm_insert_node(id, cur_p) != 0)
					{
						AON_RTC_LOGW("alarm name=%s insert fail\r\n", cur_p->name);
						rtc_exit_critical(int_level);
						return;
					}
				}
				else	//only self
				{
					//just update self expired time
					cur_p->expired_tick += cur_p->period_tick * experied_cnt;
					AON_RTC_LOGV("%s update next expired time %d \r\n", cur_p->name, cur_p->expired_tick);
				}
			}
		}
		else	//no expired
		{
			break;
		}

		alarm_dump_list(s_aon_rtc[id].alarm_head_p);
	}

	rtc_exit_critical(int_level);

	alarm_dump_list(s_aon_rtc[id].alarm_head_p);

	AON_RTC_LOGV("%s[-]cnt=%d\r\n", __func__, s_aon_rtc[id].alarm_node_cnt);
}

bk_err_t bk_aon_rtc_register_tick_isr(aon_rtc_id_t id, aon_rtc_isr_t isr, void *param)
{
	//AON_RTC_RETURN_ON_INVALID_ID(id);
	uint32_t int_level = rtc_enter_critical();
	s_aon_rtc_tick_isr[id].callback = isr;
	s_aon_rtc_tick_isr[id].isr_param = param;
	rtc_exit_critical(int_level);
	return BK_OK;
}

/*
 * aon rtc set tick uses 3 cycles of 32k in ASIC,
 * cpu should check whether set tick success.
 * If twice set tick in 3/32 ms, the second time set tick value will be failed.
 */
static void aon_rtc_set_tick(aon_rtc_hal_t *hal, uint64_t val)
{
	uint64_t cur_tick = 0;
	volatile uint64_t valid_val = val;

	uint32_t int_level = rtc_enter_critical();
	if(s_last_setted_lpo_tick == valid_val)	//maybe set the same value,but last set value sync to LPO doesn't complete
	{
		rtc_exit_critical(int_level);
		return;
	}

	//wait enough safe time to set new tick
	cur_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	if(cur_tick < (AON_RTC_OPS_SAFE_TICK_CNT + s_last_set_time))
	{
		while((bk_aon_rtc_get_current_tick(AON_RTC_ID_1)) < (AON_RTC_OPS_SAFE_TICK_CNT + s_last_set_time))
		{

		}
	}

	//maybe after wait few ticks, the will be setted time is over ahead
	cur_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	if(cur_tick >= valid_val - AON_RTC_OPS_SAFE_TICK_CNT)
	{
		//AON_RTC_LOGI("%s:set tick interval too small,set_tick=0x%llx, cur_tick=0x%llx\r\n", __func__, valid_val, cur_tick);
		valid_val = cur_tick + AON_RTC_OPS_SAFE_TICK_CNT;	//TODO: Optimize: can set really value by system running speed, example:2 ticks maybe enough.
	}

	aon_rtc_hal_set_tick_val(hal, valid_val);	//just set it
	s_last_setted_lpo_tick = valid_val;
	s_last_set_time = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);

	aon_rtc_hal_enable_tick_int(hal);
	rtc_exit_critical(int_level);
}

bk_err_t bk_aon_rtc_register_upper_isr(aon_rtc_id_t id, aon_rtc_isr_t isr, void *param)
{
	//AON_RTC_RETURN_ON_INVALID_ID(id);
	uint32_t int_level = rtc_enter_critical();
	s_aon_rtc_upper_isr[id].callback = isr;
	s_aon_rtc_upper_isr[id].isr_param = param;
	rtc_exit_critical(int_level);
	return BK_OK;
}

#if 0
static bk_err_t aon_rtc_isr_handler(aon_rtc_id_t id)
{
	//uses tick as one time timer
	if(aon_rtc_hal_get_tick_int_status(&s_aon_rtc[id].hal))
	{
		if (s_aon_rtc_tick_isr[id].callback) {
			s_aon_rtc_tick_isr[id].callback(id, AON_RTC_NAME, s_aon_rtc_tick_isr[id].isr_param);
		}
		aon_rtc_hal_clear_tick_int_status(&s_aon_rtc[id].hal);

		bk_aon_rtc_destroy(id);
	}

	//uses upper timer as period timer
	if(aon_rtc_hal_get_upper_int_status(&s_aon_rtc[id].hal))
	{
		if (s_aon_rtc_upper_isr[id].callback) {
			s_aon_rtc_upper_isr[id].callback(id, AON_RTC_NAME, s_aon_rtc_upper_isr[id].isr_param);
		}

		aon_rtc_hal_clear_upper_int_status(&s_aon_rtc[id].hal);
	}

	aon_rtc_clear_irq_pending(id);

	return BK_OK;
}
#else

#if CONFIG_AON_RTC_DEBUG
#define AON_RTC_ISR_DEBUG_MAX_CNT (256)
static uint32_t s_isr_cnt = 0;
static uint64_t s_isr_debug_in_tick[AON_RTC_ISR_DEBUG_MAX_CNT];
static uint64_t s_isr_debug_out_tick[AON_RTC_ISR_DEBUG_MAX_CNT];
static uint64_t s_isr_debug_set_tick[AON_RTC_ISR_DEBUG_MAX_CNT];
#endif

static bk_err_t aon_rtc_isr_handler(aon_rtc_id_t id)
{
	uint32_t int_level = rtc_enter_critical();

#if CONFIG_AON_RTC_DEBUG
	s_isr_debug_in_tick[(s_isr_cnt)%AON_RTC_ISR_DEBUG_MAX_CNT] = bk_aon_rtc_get_current_tick(id);
#endif

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	//uses tick as one time timer
	if(aon_rtc_hal_get_tick_int_status(&s_aon_rtc[id].hal))
	{
		//maybe the isr callback runs too much time and set next tick value too small, caused next isr can't response.
		aon_rtc_hal_clear_tick_int_status(&s_aon_rtc[id].hal);

		alarm_update_expeired_nodes(id);

		//reset the timer tick
		if(s_aon_rtc[id].alarm_head_p)
		{
			aon_rtc_set_tick(&s_aon_rtc[id].hal, s_aon_rtc[id].alarm_head_p->expired_tick);
#if CONFIG_AON_RTC_DEBUG
			s_isr_debug_set_tick[(s_isr_cnt)%AON_RTC_ISR_DEBUG_MAX_CNT] = s_aon_rtc[id].alarm_head_p->expired_tick;
#endif
			AON_RTC_LOGV("next tick=0x%x, cur_tick=0x%x\r\n", (uint32_t)s_aon_rtc[id].alarm_head_p->expired_tick, (uint32_t)bk_aon_rtc_get_current_tick(id));
		}
		else
		{
			aon_rtc_set_tick(&s_aon_rtc[id].hal, AON_RTC_ROUND_TICK);
			AON_RTC_LOGV("no alarm:cur_tick=0x%x\r\n", (uint32_t)bk_aon_rtc_get_current_tick(id));
		}
	}

	aon_rtc_clear_irq_pending(id);

	AON_RTC_LOGV("%s[-]\r\n", __func__);
#if CONFIG_AON_RTC_DEBUG
	s_isr_debug_out_tick[(s_isr_cnt)%AON_RTC_ISR_DEBUG_MAX_CNT] = bk_aon_rtc_get_current_tick(id);
	s_isr_cnt++;
#endif

	rtc_exit_critical(int_level);

	return BK_OK;
}

#endif

static void aon_rtc1_isr_handler(void)
{
	aon_rtc_isr_handler(AON_RTC_ID_1);
}

#if (SOC_AON_RTC_UNIT_NUM > 1)
static void aon_rtc2_isr_handler(void)
{
	aon_rtc_isr_handler(AON_RTC_ID_2);
}
#endif

#if (CONFIG_SYSTEM_CTRL)
static void aon_rtc_interrupt_enable(aon_rtc_id_t id)
{
	switch(id)
	{
		case AON_RTC_ID_1:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_RTC, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(),INT_SRC_RTC,1);
#endif
			break;
#if (SOC_AON_RTC_UNIT_NUM > 1)
		case AON_RTC_ID_2:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_RTC2, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(),INT_SRC_RTC2,1);
#endif
			break;
#endif
		default:
			break;
	}
}

static void aon_rtc_interrupt_disable(aon_rtc_id_t id)
{
	switch(id)
	{
		case AON_RTC_ID_1:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_RTC, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(),INT_SRC_RTC,0);
#endif
			break;
#if (SOC_AON_RTC_UNIT_NUM > 1)
		case AON_RTC_ID_2:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU0_CORE_ID, INT_SRC_RTC2, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(),INT_SRC_RTC2,0);
#endif
			break;
#endif
		default:
			break;
	}
}
#endif

static bk_err_t aon_rtc_sw_init(aon_rtc_id_t id)
{
	os_memset(&s_aon_rtc[id], 0, sizeof(s_aon_rtc[id]));
	os_memset(&s_aon_rtc_tick_isr[id], 0, sizeof(s_aon_rtc_tick_isr[id]));
	os_memset(&s_aon_rtc_upper_isr[id], 0, sizeof(s_aon_rtc_upper_isr[id]));

	s_aon_rtc_nodes_p[id] = os_zalloc(sizeof(aon_rtc_nodes_memory_t));
	if(s_aon_rtc_nodes_p[id] == NULL)
	{
		return BK_ERR_NO_MEM;
	}

	return BK_OK;
}

static void aon_rtc_hw_init(aon_rtc_id_t id)
{
	aon_rtc_int_config_t int_config_table[] = AON_RTC_INT_CONFIG_TABLE;
	aon_rtc_int_config_t *cur_int_cfg = &int_config_table[id];

	AON_RTC_LOGV("%s[+]cur_tick=%d\r\n", __func__, (uint32_t)bk_aon_rtc_get_current_tick(id));

	if(!aon_rtc_hal_is_enable(&s_aon_rtc[id].hal))
	{
		aon_rtc_hal_init(&s_aon_rtc[id].hal);
	}
	//set upper to max value
	aon_rtc_hal_set_upper_val(&s_aon_rtc[id].hal, AON_RTC_ROUND_TICK);
	aon_rtc_hal_enable_upper_int(&s_aon_rtc[id].hal);

	bk_int_isr_register(cur_int_cfg->int_src, cur_int_cfg->isr, NULL);
#if (CONFIG_SYSTEM_CTRL)
	aon_rtc_interrupt_enable(id);
#endif
	aon_rtc_hal_start_counter(&s_aon_rtc[id].hal);

	AON_RTC_LOGV("%s[-]cur_tick=%d\r\n", __func__, (uint32_t)bk_aon_rtc_get_current_tick(id));
}

bk_err_t bk_aon_rtc_driver_init(void)
{
	AON_RTC_LOGV("%s[+]\r\n", __func__);

	for (int id = AON_RTC_ID_1; id < AON_RTC_ID_MAX; id++) {
		if(!s_aon_rtc[id].inited)
		{

			aon_rtc_sw_init(id);
			aon_rtc_hw_init(id);
			s_aon_rtc[id].inited = true;
		}
	}

	AON_RTC_LOGV("%s[-]\r\n", __func__);

	return BK_OK;
}

bk_err_t bk_aon_rtc_driver_deinit(void)
{
	aon_rtc_int_config_t int_cfg_table[] = AON_RTC_INT_CONFIG_TABLE;

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	for (int id = AON_RTC_ID_1; id < AON_RTC_ID_MAX; id++) {
		if(s_aon_rtc[id].inited)
		{
			aon_rtc_hal_deinit(&s_aon_rtc[id].hal);
#if (CONFIG_SYSTEM_CTRL)
			aon_rtc_interrupt_disable(id);
#endif
			bk_int_isr_unregister(int_cfg_table[id].int_src);

			if(s_aon_rtc_nodes_p[id] != NULL)
			{
				os_free(s_aon_rtc_nodes_p[id]);
				s_aon_rtc_nodes_p[id] = NULL;
			}

			s_aon_rtc[id].inited = false;
		}
	}


	AON_RTC_LOGV("%s[-]\r\n", __func__);
	return BK_OK;
}

#if 0	//remove it, only one HW can't be used for many APPs.
bk_err_t bk_aon_rtc_create(aon_rtc_id_t id, rtc_tick_t tick, bool period)
{
	//Avoid APP call this function before driver has done bk_aon_rtc_driver_init
	if(s_aon_rtc[id].inited == false)
	{
		//TODO: logs: call aon_rtc_init first.
		return BK_ERR_NOT_INIT;
	}

	if(s_aon_rtc[id].using_cnt)
	{
		//TODO: logs: call bk_aon_rtc_destroy first.
		return BK_ERR_BUSY;
	}

	//TOTO: Enter critical protect

	s_aon_rtc[id].using_cnt++;
	s_aon_rtc[id].tick = tick;
	s_aon_rtc[id].period = period;

	//init HW
	s_aon_rtc[id].hal.id = id;
	aon_rtc_hal_init(&s_aon_rtc[id].hal);

	if(period)	//use upper value int
	{
		aon_rtc_hal_set_upper_val(&s_aon_rtc[id].hal, tick);
		aon_rtc_hal_enable_upper_int(&s_aon_rtc[id].hal);
	}
	else
	{
		//confirm tick val <= upper value, or tick int never be entry.
		aon_rtc_hal_set_upper_val(&s_aon_rtc[id].hal, AON_RTC_UPPER_VAL_MAX);

		aon_rtc_set_tick(&s_aon_rtc[id].hal, tick);
		aon_rtc_hal_enable_tick_int(&s_aon_rtc[id].hal);
	}

	//start to run
	aon_rtc_start_run(id);

	//TOTO: Exit critical protect

	return BK_OK;
}

bk_err_t bk_aon_rtc_destroy(aon_rtc_id_t id)
{
	//TOTO: Enter critical protect

	if(s_aon_rtc[id].inited == false)
	{
		//TODO: logs: call aon_rtc_init first.
		//TOTO: Exit critical protect
		return BK_ERR_NOT_INIT;
	}

	if(s_aon_rtc[id].using_cnt == 0)
	{
		//TODO: logs: call bk_aon_rtc_create first.
		//TOTO: Exit critical protect
		return BK_ERR_NOT_INIT;
	}

	//stop HW before SW change value, avoid ISR status was update to INTC/NVIC/PLIC...
	//but doesn't response ISR, after HW deinit, the ISR comes caused error.
	aon_rtc_hal_deinit(&s_aon_rtc[id].hal);

	s_aon_rtc[id].using_cnt = 0;
	s_aon_rtc[id].tick = 0;
	s_aon_rtc[id].period = false;

	//TOTO: Exit critical protect

	return BK_OK;
}
#endif

bk_err_t bk_alarm_register(aon_rtc_id_t id, alarm_info_t *alarm_info_p)
{
	alarm_node_t *node_p = NULL;
	uint32_t int_level = 0;

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	if(id >= AON_RTC_ID_MAX)
	{
		AON_RTC_LOGW("%s:id=%d\r\n", __func__, id);
		return BK_ERR_PARAM;
	}

	if(alarm_info_p == NULL)
	{
		return BK_ERR_PARAM;
	}

	if(alarm_info_p->period_tick < AON_RTC_PRECISION_TICK)	//in protect area to reduce consume time before set tick.
	{
		AON_RTC_LOGW("period_tick should not smaller then %d\r\n", AON_RTC_PRECISION_TICK);
		return BK_FAIL;
	}

	int_level = rtc_enter_critical();

	if(s_aon_rtc[id].alarm_node_cnt >= AON_RTC_MAX_ALARM_CNT)
	{
		rtc_exit_critical(int_level);
		AON_RTC_LOGW("alarm registered too much:%d\r\n", AON_RTC_MAX_ALARM_CNT);
		return BK_FAIL;
	}

	//request a node
	node_p = aon_rtc_request_node(id);
	if(node_p == NULL)
	{
		rtc_exit_critical(int_level);
		AON_RTC_LOGW("alarm registered:no memory\r\n");
		return BK_ERR_NO_MEM;
	}

	memset(node_p, 0, sizeof(alarm_node_t));
	node_p->callback = alarm_info_p->callback;
	node_p->cb_param_p = alarm_info_p->param_p;
	memcpy(&node_p->name[0], alarm_info_p->name, ALARM_NAME_MAX_LEN);
	node_p->name[ALARM_NAME_MAX_LEN] = 0;
	node_p->start_tick = bk_aon_rtc_get_current_tick(id);	//tick
	node_p->period_tick = alarm_info_p->period_tick;
	//BK_ASSERT(alarm_info_p->period_cnt);
	if(alarm_info_p->period_cnt == 0)
	{
		rtc_exit_critical(int_level);
		AON_RTC_LOGW("no set period cnt\r\n");
		return BK_ERR_PARAM;
	}

	node_p->period_cnt = alarm_info_p->period_cnt;
	node_p->expired_tick = node_p->start_tick + (alarm_info_p->period_tick);
	
	//push to alarm list
	if(alarm_insert_node(id, node_p) != 0)
	{
		AON_RTC_LOGW("alarm name=%s has registered\r\n", alarm_info_p->name);
		aon_rtc_release_node(id, node_p);
		rtc_exit_critical(int_level);
		return BK_FAIL;
	}

	//reset the timer tick
	if(node_p == s_aon_rtc[id].alarm_head_p)	//insert node is the first one, should reset tick val
	{
		aon_rtc_set_tick(&s_aon_rtc[id].hal, s_aon_rtc[id].alarm_head_p->expired_tick);
	}

	aon_rtc_hal_enable_tick_int(&s_aon_rtc[id].hal);
	AON_RTC_LOGV("next tick=0x%x, cur_tick=0x%x\r\n", (uint32_t)s_aon_rtc[id].alarm_head_p->expired_tick, (uint32_t)bk_aon_rtc_get_current_tick(id));

	rtc_exit_critical(int_level);

	AON_RTC_LOGV("%s[-]\r\n", __func__);

	return BK_OK;
}


//the timer isn't expired, but app un-register it.
bk_err_t bk_alarm_unregister(aon_rtc_id_t id, uint8_t *name_p)
{
	alarm_node_t *remove_node_p = NULL;
	alarm_node_t *previous_head_node_p = NULL;
	uint32_t int_level = 0;

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	if(id >= AON_RTC_ID_MAX)
	{
		AON_RTC_LOGW("%s:id=%d\r\n", __func__, id);
		return BK_ERR_PARAM;
	}

	int_level = rtc_enter_critical();
	
	previous_head_node_p = s_aon_rtc[id].alarm_head_p;
	remove_node_p = alarm_remove_node(id, name_p);

	//reset the timer tick
	if(previous_head_node_p == remove_node_p)	//the previous head is removed
	{
		if(s_aon_rtc[id].alarm_head_p)	//new head exist
		{
			aon_rtc_set_tick(&s_aon_rtc[id].hal, s_aon_rtc[id].alarm_head_p->expired_tick);
			AON_RTC_LOGV("next tick=0x%x, cur_tick=0x%x\r\n", s_aon_rtc[id].alarm_head_p->expired_tick, bk_aon_rtc_get_current_tick(id));
		}
		else	//has no nodes now
		{
			//If the ISR at enable status, and the previous set tick time come, it will produce an Interrupt and maybe wakeup system.
			aon_rtc_hal_disable_tick_int(&s_aon_rtc[id].hal);
			// aon_rtc_set_tick(&s_aon_rtc[id].hal, AON_RTC_ROUND_TICK);
			// AON_RTC_LOGV("no alarm:cur_tick=0x%x\r\n", bk_aon_rtc_get_current_tick(id));
		}
	}

	rtc_exit_critical(int_level);

	AON_RTC_LOGV("%s[-]\r\n", __func__);
	return BK_OK;
}

#if (CONFIG_AON_RTC && (!CONFIG_AON_RTC_MANY_USERS))
bk_err_t bk_aon_rtc_tick_init()
{
	aon_rtc_hal_init(&s_aon_rtc[AON_RTC_ID_1].hal);

	//set upper to max value
	aon_rtc_hal_set_upper_val(&s_aon_rtc[AON_RTC_ID_1].hal, AON_RTC_ROUND_TICK);
	aon_rtc_hal_set_tick_val(&s_aon_rtc[AON_RTC_ID_1].hal, 0);
	return BK_OK;
}

bk_err_t bk_aon_rtc_open_rtc_wakeup(rtc_tick_t period)
{
    uint64_t wakeup_period= 0;

    wakeup_period = aon_rtc_hal_get_upper_val(&s_aon_rtc[AON_RTC_ID_1].hal) + period;
    aon_rtc_hal_set_tick_val(&s_aon_rtc[AON_RTC_ID_1].hal, wakeup_period);

    aon_rtc_hal_enable_tick_int(&s_aon_rtc[AON_RTC_ID_1].hal);

    return BK_OK;
}
#endif

__IRAM_SEC uint64_t bk_aon_rtc_get_upper_val(aon_rtc_id_t id)
{
	return bk_aon_rtc_get_max_value(id);
}

__IRAM_SEC uint64_t bk_aon_rtc_get_max_value(aon_rtc_id_t id)
{
	if(id >= AON_RTC_ID_MAX)
	{
		AON_RTC_LOGW("%s:id=%d\r\n", __func__, id);
		return 0;
	}

	return (aon_rtc_hal_get_upper_val(&s_aon_rtc[id].hal));
}

__IRAM_SEC uint64_t bk_aon_rtc_get_current_tick(aon_rtc_id_t id)
{
	if(id >= AON_RTC_ID_MAX)
	{
		AON_RTC_LOGW("%s:id=%d\r\n", __func__, id);
		return 0;
	}

	return (aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal));
}

#if CONFIG_ROSC_COMPENSATION
__IRAM_SEC uint64_t bk_aon_rtc_get_current_tick_with_compensation(aon_rtc_id_t id)
{
	uint64_t tick_val = bk_aon_rtc_get_current_tick(id);
	return (tick_val + bk_rosc_32k_get_tick_diff(tick_val));
}
#endif

#if CONFIG_RTC_ANA_WAKEUP_SUPPORT
static int ana_wakesource_rtc_enter_cb(uint64_t sleep_time, void *args)
{
	uint32_t period = s_wkup_time_period;
	if (period >= RTC_ANA_TIME_PERIOD_MAX) {
		AON_RTC_LOGW("rtc wakeup period range 0~15\r\n");
	}
	sys_drv_rtc_ana_wakeup_enable(period);
	return 0;
}

bk_err_t bk_rtc_ana_register_wakeup_source(uint32_t period)
{
	pm_cb_conf_t enter_conf;

	s_wkup_time_period = period;
	AON_RTC_LOGD("regist wakeup source rtc period: %d\r\n", period);

	enter_conf.cb = ana_wakesource_rtc_enter_cb;
	enter_conf.args = NULL;

	return bk_pm_sleep_register_cb(PM_MODE_SUPER_DEEP_SLEEP, PM_DEV_ID_RTC, &enter_conf, NULL);
}
#endif

#if CONFIG_AON_RTC_DEBUG
void bk_aon_rtc_timing_test(aon_rtc_id_t id, uint32_t round, uint32_t cycles, rtc_tick_t set_tick)
{
	uint32_t int_level = 0;
	uint32_t i = 0, j = 0;
	uint64_t start_tick = 0, end_tick = 0;
	uint64_t u64_start_tick = 0, u64_end_tick = 0;
	uint32_t max_offset_tick = 0, min_offset_tick = 0xffffffff;
	uint32_t fail_cnt = 0;

	AON_RTC_LOGV("%s[+]\r\n", __func__);

	int_level = rtc_enter_critical();
	
	//get uint32_t tick counter check
	for(i = 0; i < round; i++)
	{
		start_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);
		for(j = 0; j < cycles; j++)
		{
			aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);
		}
		end_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);

		if(min_offset_tick > end_tick - start_tick)
			min_offset_tick = end_tick - start_tick;
		if(max_offset_tick < end_tick - start_tick)
			max_offset_tick = end_tick - start_tick;
	}
	AON_RTC_LOGD("Gettick uint32:%d rounds*%d times:max=%d,min=%d\r\n", i, j, max_offset_tick, min_offset_tick);

	//get uint64_t tick counter check
	max_offset_tick = 0;
	min_offset_tick = 0xffffffff;
	for(i = 0; i < round; i++)
	{
		u64_start_tick = bk_aon_rtc_get_current_tick(id);
		for(j = 0; j < cycles; j++)
		{
			bk_aon_rtc_get_current_tick(id);
		}
		u64_end_tick = bk_aon_rtc_get_current_tick(id);

		if(min_offset_tick > (uint32_t)(u64_end_tick - u64_start_tick))
			min_offset_tick = (uint32_t)(u64_end_tick - u64_start_tick);
		if(max_offset_tick < u64_end_tick - u64_start_tick)
			max_offset_tick = u64_end_tick - u64_start_tick;
	}
	AON_RTC_LOGD("Gettick uint64:%d rounds*%d times:max=%d,min=%d\r\n", i, j, max_offset_tick, min_offset_tick);

	//set tick val check
	max_offset_tick = 0;
	min_offset_tick = 0xffffffff;
	for(i = 0; i < round; i++)
	{
		start_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);
		for(j = 0; j < cycles; j++)
		{
			aon_rtc_set_tick(&s_aon_rtc[id].hal, set_tick);
		}
		end_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);

		if(min_offset_tick > end_tick - start_tick)
			min_offset_tick = end_tick - start_tick;
		if(max_offset_tick < end_tick - start_tick)
			max_offset_tick = end_tick - start_tick;
	}
	AON_RTC_LOGD("Settick:%d rounds*%d times:max=%d,min=%d\r\n", i, j, max_offset_tick, min_offset_tick);

	fail_cnt = 0;
	max_offset_tick = 0;
	min_offset_tick = 0xffffffff;
	for(i = 0; i < round; i++)
	{
		start_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);
		for(j = 0; j < cycles; j++)
		{
			aon_rtc_set_tick(&s_aon_rtc[id].hal, set_tick);
			if(set_tick != aon_rtc_hal_get_tick_val(&s_aon_rtc[id].hal))
			{
				fail_cnt++;
			}
		}
		end_tick = aon_rtc_hal_get_counter_val(&s_aon_rtc[id].hal);

		if(min_offset_tick > end_tick - start_tick)
			min_offset_tick = end_tick - start_tick;
		if(max_offset_tick < end_tick - start_tick)
			max_offset_tick = end_tick - start_tick;
	}
	AON_RTC_LOGD("Settick:%d rounds*%d times:max=%d,min=%d\r\n", i, j, max_offset_tick, min_offset_tick);
	AON_RTC_LOGD("Settick:%d rounds*%d times:check fail_cnt=%d\r\n", i, j, fail_cnt);

	rtc_exit_critical(int_level);
	AON_RTC_LOGV("%s[-]\r\n", __func__);
}
#endif

void bk_aon_rtc_dump(aon_rtc_id_t id)
{
#if CONFIG_AON_RTC_DEBUG
	uint32_t i = 0, index = 0;

	for(i = s_isr_cnt - AON_RTC_ISR_DEBUG_MAX_CNT; i < s_isr_cnt; i++)
	{
		index = i % AON_RTC_ISR_DEBUG_MAX_CNT;

		for(volatile uint32_t j = 0; j < 1800; j++);	//confirm log output normarlly
		
		AON_RTC_LOGD("isr_in[%d]=0x%llx,out=0x%llx,set=0x%llx\r\n", index, s_isr_debug_in_tick[index], s_isr_debug_out_tick[index], s_isr_debug_set_tick[index]);
	}
#endif
	aon_rtc_struct_dump();

	alarm_dump_list(s_aon_rtc[id].alarm_head_p);
}

void aon_rtc_check_list(aon_rtc_id_t id)
{
	alarm_node_t *cur_p = NULL;
	uint32_t cnt = 0;
	uint64_t is_up_sequence = 0xFFFFFFFFFFFFFFFFLL;
	uint64_t is_timeout = 0;
	uint32_t int_level = rtc_enter_critical();

	cur_p = s_aon_rtc[id].alarm_head_p;
	while(cur_p)
	{
		cnt++;
		cur_p = cur_p->next;

		if(cur_p->next)
		{
			if(cur_p->expired_tick > cur_p->next->expired_tick)
				is_up_sequence &= ~(1<<cnt);
		}

		if(bk_aon_rtc_get_current_tick(id) > cur_p->expired_tick)
			is_timeout |= (1<<cnt);
	}
	BK_ASSERT(cnt == s_aon_rtc[id].alarm_node_cnt);
	BK_ASSERT(is_up_sequence == 0xFFFFFFFFFFFFFFFFLL);

	rtc_exit_critical(int_level);
	AON_RTC_LOGD("cnt=%d,istimeout=0x%x\r\n",cnt, is_timeout);
}

uint8_t *bk_rtc_get_first_alarm_name(void)
{
	alarm_node_t *first_node = NULL;
	uint32_t int_level = 0;
	uint8_t *name = NULL;

	int_level = rtc_enter_critical();

	// Get the first node (head of the sorted list)
	first_node = s_aon_rtc[AON_RTC_ID_1].alarm_head_p;

	if (first_node != NULL)
	{
		// Return pointer to the name field
		name = first_node->name;
		AON_RTC_LOGV("%s: first alarm name=%s\r\n", __func__, name);
	}
	else
	{
		AON_RTC_LOGV("%s: no alarm registered\r\n", __func__);
	}

	rtc_exit_critical(int_level);

	return name;
}