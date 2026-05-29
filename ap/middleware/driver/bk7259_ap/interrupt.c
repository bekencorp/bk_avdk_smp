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

#include <os/os.h>
#include "icu_driver.h"
#include "interrupt_base.h"
#include <driver/int.h>
#include "interrupt.h"
#include "sdkconfig.h"
#include "arch_interrupt.h"
#include "components/log.h"
#include "cmsis_gcc.h"
#include <common/bk_assert.h>
#include "interrupt_controller.h"
#include "stack_base.h"
#include "sys_hal.h"
#include "sys_driver.h"

#define TAG "INT"

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#if CONFIG_INTERRUPT_DEBUG_RECORDER
#include <driver/aon_rtc.h>
#include <string.h>
#include "components/log.h"
#endif

#include "bk_arch.h"

#if CONFIG_SUPPORT_WWDT
#include "wwdt_driver.h"
#endif

#if CONFIG_ARCH_INT_STATIS
static uint32_t s_int_statis[InterruptMAX_IRQn] = {0};
#define INT_INC_STATIS(irq) s_int_statis[(irq)] ++
#else
#define INT_INC_STATIS(irq)
#endif

#if (CONFIG_FREERTOS_TRACE)
#define IRQ_TRACE_BEGIN(irq)	xTraceISRBegin(xGetTraceISRHandle(irq))
#define IRQ_TRACE_END()			xTraceISREnd(0)
#else
#define IRQ_TRACE_BEGIN(irq)
#define IRQ_TRACE_END()
#endif

#if CONFIG_INTERRUPT_DEBUG_RECORDER
#define BK_INTERRUPT_DEBUG_EXIT_FLAG 0xF0000000U
#ifndef CONFIG_INTERRUPT_GAP_DETECT_US
#define CONFIG_INTERRUPT_GAP_DETECT_US 50000U
#endif
/* Skip warmup events (cold start cache fills, slow paths) before statistics. */
#define BK_INTERRUPT_GAP_WARMUP 32U

typedef struct {
	uint32_t int_flag;
	uint32_t current_cnt;
	uint64_t enter_time;
	uint64_t exit_time;
} interrupt_recorder_t;

typedef struct {
	volatile uint32_t count;
	/* Layout preserved: 'recorder' stays at offset 8 due to uint64 alignment.
	 * Extension fields placed after the recorder array stay backward-compatible
	 * with parsers that only read `count` (+0) and `recorder[]` (+8).
	 */
	volatile interrupt_recorder_t recorder[CONFIG_INTERRUPT_RECORDER_CNT];
	/* AON interrupt-silence monitor extension */
	volatile uint64_t last_isr_exit_us;       /* AON-RTC us of previous ISR exit */
	volatile uint64_t max_gap_us;             /* largest ISR-exit silence since boot */
	volatile uint64_t last_warn_print_us;     /* reserved; kept for dump ABI stability */
	volatile uint32_t max_gap_irq;            /* IRQ that ended the max silence window */
	volatile uint32_t max_gap_cnt;            /* recorder cnt at max gap */
	volatile uint32_t gap_event_total;        /* total silence windows > threshold */
	volatile uint32_t gap_threshold_us;       /* compiled threshold */
} interrupt_recorder_dump_t;

__attribute__((__used__)) static volatile interrupt_recorder_dump_t s_interrupt_core0_dump;
__attribute__((__used__)) static volatile interrupt_recorder_dump_t s_interrupt_core1_dump;
static volatile uint32_t s_interrupt_debug_dump_registered = 0;

static inline volatile interrupt_recorder_dump_t *bk_interrupt_debug_get_core_dump(uint32_t core_id)
{
	return (core_id == 0) ? &s_interrupt_core0_dump : &s_interrupt_core1_dump;
}

void bk_interrupt_debug_isr_enter(uint32_t irq)
{
	uint32_t core_id = portGET_CORE_ID();
	volatile interrupt_recorder_dump_t *core_dump = bk_interrupt_debug_get_core_dump(core_id);
	uint32_t current_cnt = core_dump->count;
	uint32_t index = current_cnt % CONFIG_INTERRUPT_RECORDER_CNT;

	core_dump->recorder[index].int_flag = irq;
	core_dump->recorder[index].current_cnt = current_cnt;
	core_dump->recorder[index].enter_time = bk_aon_rtc_get_us();
	core_dump->recorder[index].exit_time = 0;
}

void bk_interrupt_debug_isr_exit(uint32_t irq)
{
	uint32_t core_id = portGET_CORE_ID();
	volatile interrupt_recorder_dump_t *core_dump = bk_interrupt_debug_get_core_dump(core_id);
	uint32_t current_cnt = core_dump->count;
	uint32_t index = current_cnt % CONFIG_INTERRUPT_RECORDER_CNT;
	uint64_t exit_us = bk_aon_rtc_get_us();

	core_dump->recorder[index].int_flag = irq | BK_INTERRUPT_DEBUG_EXIT_FLAG;
	core_dump->recorder[index].exit_time = exit_us;

	/* AON-RTC IRQ-silence statistics: ISR-exit to ISR-exit on this core.
	 * This is NOT the current IRQ handler cost (that is exit_time - enter_time).
	 * The IRQ saved in max_gap_irq is the first IRQ that completed after the
	 * silence window, not necessarily the IRQ that caused the stall.
	 */
	uint64_t prev_exit = core_dump->last_isr_exit_us;
	if ((prev_exit != 0U) && (current_cnt >= BK_INTERRUPT_GAP_WARMUP) && (exit_us > prev_exit)) {
		uint64_t gap_us = exit_us - prev_exit;
		if (gap_us > core_dump->max_gap_us) {
			core_dump->max_gap_us = gap_us;
			core_dump->max_gap_irq = irq;
			core_dump->max_gap_cnt = current_cnt;
		}
		if (gap_us >= (uint64_t)CONFIG_INTERRUPT_GAP_DETECT_US) {
			core_dump->gap_event_total++;
		}
	}
	core_dump->last_isr_exit_us = exit_us;
	core_dump->count = current_cnt + 1;
}

static void bk_interrupt_debug_dump_core_recorder(const char *core_name, volatile interrupt_recorder_dump_t *core_dump)
{
	uint32_t total_cnt = core_dump->count;
	uint32_t recorder_cnt = (total_cnt < CONFIG_INTERRUPT_RECORDER_CNT) ? total_cnt : CONFIG_INTERRUPT_RECORDER_CNT;
	uint32_t start_cnt;

	BK_DUMP_OUT("interrupt recorder %s total=%u depth=%u last_isr_exit_us=%llu max_silence_us=%llu post_silence_irq=%u post_silence_cnt=%u silence_event_total=%u silence_threshold_us=%u\r\n",
		core_name, total_cnt, recorder_cnt,
		(unsigned long long)core_dump->last_isr_exit_us,
		(unsigned long long)core_dump->max_gap_us,
		(unsigned)core_dump->max_gap_irq,
		(unsigned)core_dump->max_gap_cnt,
		(unsigned)core_dump->gap_event_total,
		(unsigned)core_dump->gap_threshold_us);
	if (recorder_cnt == 0) {
		BK_DUMP_OUT("interrupt recorder %s empty\r\n", core_name);
		return;
	}

	start_cnt = total_cnt - recorder_cnt;
	for (uint32_t seq = start_cnt; seq < total_cnt; seq++) {
		uint32_t index = seq % CONFIG_INTERRUPT_RECORDER_CNT;
		const volatile interrupt_recorder_t *rec = &core_dump->recorder[index];
		uint32_t irq = rec->int_flag & ~BK_INTERRUPT_DEBUG_EXIT_FLAG;
		uint32_t completed = (rec->int_flag & BK_INTERRUPT_DEBUG_EXIT_FLAG) ? 1 : 0;
	#if CONFIG_SUPPORT_WWDT
		bk_wwdt_force_feed();
	#endif
		BK_DUMP_OUT("  [%s][%u] irq=%u done=%u enter=%llu exit=%llu\r\n",
			core_name,
			rec->current_cnt,
			irq,
			completed,
			(unsigned long long)rec->enter_time,
			(unsigned long long)rec->exit_time);
	}
}

void bk_interrupt_dump_recorder(void)
{
	bk_interrupt_debug_dump_core_recorder("core0", &s_interrupt_core0_dump);
	bk_interrupt_debug_dump_core_recorder("core1", &s_interrupt_core1_dump);
}

static void bk_interrupt_debug_init(void)
{
	memset((void *)&s_interrupt_core0_dump, 0, sizeof(s_interrupt_core0_dump));
	memset((void *)&s_interrupt_core1_dump, 0, sizeof(s_interrupt_core1_dump));
	s_interrupt_core0_dump.gap_threshold_us = (uint32_t)CONFIG_INTERRUPT_GAP_DETECT_US;
	s_interrupt_core1_dump.gap_threshold_us = (uint32_t)CONFIG_INTERRUPT_GAP_DETECT_US;

	if (s_interrupt_debug_dump_registered == 0) {
		rtos_regist_plat_dump_hook((uint32_t)&s_interrupt_core0_dump, sizeof(s_interrupt_core0_dump));
		rtos_regist_plat_dump_hook((uint32_t)&s_interrupt_core1_dump, sizeof(s_interrupt_core1_dump));
		s_interrupt_debug_dump_registered = 1;
	}
}
#else
void bk_interrupt_debug_isr_enter(uint32_t irq)
{
	(void)irq;
}

void bk_interrupt_debug_isr_exit(uint32_t irq)
{
	(void)irq;
}

void bk_interrupt_dump_recorder(void)
{
}
#endif

extern uint32_t get_relocate_vector_table(void);
#if CONFIG_SOC_SMP
extern uint32_t get_core1_vtor_addr(void);
#endif

void soc_isr_init(void)
{
	uint32_t vtor_addr = get_relocate_vector_table();
	int_controller_t *primary_intc = NULL;
#if CONFIG_SOC_SMP
	uint32_t core1_vtor_addr = get_core1_vtor_addr();
	int_controller_t *secondary_intc = NULL;
#endif
	arch_isr_entry_init();
	
	if (portGET_CORE_ID() == 0) {
#if CONFIG_INTERRUPT_DEBUG_RECORDER
		bk_interrupt_debug_init();
#endif
    	primary_intc = int_controller_create(INT_CONTROLLER_ID_PRIMARY, __INT_NUMBER_MAX, (void *)vtor_addr); /* primary controller with capacity 16 */
		BK_ASSERT(NULL != primary_intc);

#if CONFIG_SOC_SMP
		secondary_intc = int_controller_create(INT_CONTROLLER_ID_SECONDARY, __INT_NUMBER_MAX, (void *)core1_vtor_addr); /* secondary controller with capacity 16 */
		BK_ASSERT(NULL != secondary_intc);
#endif
	}
}

void soc_isr_deinit(void)
{
    /*to do something*/
}

__IRAM_SEC int32_t sys_drv_set_m55sub_int_en(uint32_t int_num, uint32_t int_en)
{
	int32_t ret = 0;
	uint32_t int_level = sys_drv_enter_critical();
	ret = sys_hal_set_m55sub_int_en(int_num, int_en);
	sys_drv_exit_critical(int_level);
	return ret;
}

__IRAM_SEC int32_t sys_drv_get_m55sub_int_en(void)
{
	int32_t ret = 0;
	uint32_t int_level = sys_drv_enter_critical();
	ret = sys_hal_get_m55sub_int_en();
	sys_drv_exit_critical(int_level);
	return ret;
}

__IRAM_SEC int32_t sys_drv_get_m55sub_int_status0(void)
{
	int32_t ret = 0;
	uint32_t int_level = sys_drv_enter_critical();
	ret = sys_hal_get_m55sub_int_status0();
	sys_drv_exit_critical(int_level);
	return ret;
}

__IRAM_SEC int32_t sys_drv_get_m55sub_int_status1(void)
{
	int32_t ret = 0;
	uint32_t int_level = sys_drv_enter_critical();
	ret = sys_hal_get_m55sub_int_status1();
	sys_drv_exit_critical(int_level);
	return ret;
}

__IRAM_SEC int32_t sys_drv_get_m55sub_int_status2(void)
{
	int32_t ret = 0;
	uint32_t int_level = sys_drv_enter_critical();
	ret = sys_hal_get_m55sub_int_status2();
	sys_drv_exit_critical(int_level);
	return ret;
}

static int_group_isr_t s_m55sub_irq_handler[INT_SRC_CP_MAX_NUM];
static uint32_t s_int_m55sub_nest = 0;
__attribute__((section(".itcm_sec_code"))) void soc_m55sub_handler(void);

void bk_interrupt_register_m55sub_int(uint32_t int_number, int_group_isr_t isr_callback)
{
	if ((int_number > (INT_SRC_CP_MAX_NUM - 1)) || isr_callback == NULL) {
		BK_LOGE(TAG, "register m55sub interrupt failed: int_number(%d), isr_callback(%p)\n", int_number, isr_callback);
		return;
	}

    // BK_LOGE(TAG, "register  int_number(%d), isr_callback(%p)\n", int_number, isr_callback);
	s_m55sub_irq_handler[int_number] = isr_callback;
    sys_drv_set_m55sub_int_en(int_number, 1);
	s_int_m55sub_nest++;
    if (s_int_m55sub_nest == 1) {
		/// SMP should enable the interrupt on core 0
		bk_int_isr_register(INT_SRC_M52S, soc_m55sub_handler, NULL);
#if CONFIG_SOC_SMP
        sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_M52S, 1);
#else
        sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_M52S, 1);
#endif
    }
}

void bk_interrupt_unregister_m55sub_int(uint32_t int_number)
{
	if (int_number > (INT_SRC_CP_MAX_NUM - 1)) {
		BK_LOGE(TAG, "unregister int failed: int_number(%d)\n", int_number);
		return;
	}
    sys_drv_set_m55sub_int_en(int_number, 0);
    s_m55sub_irq_handler[int_number] = NULL;
	s_int_m55sub_nest--;
    if (s_int_m55sub_nest == 0) {
		/// SMP should disable the interrupt on core 0
#if CONFIG_SOC_SMP
        sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_M52S, 0);
#else
        sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_M52S, 0);
#endif
    }
}

__attribute__((section(".itcm_sec_code"))) int_group_isr_t bk_interrupt_get_handler(uint32_t int_number)
{
	return s_m55sub_irq_handler[int_number];
}

__attribute__((section(".itcm_sec_code")))void bk_interrupt_m55sub_irq_handler(uint32_t int_number)
{
	if (int_number > (INT_SRC_CP_MAX_NUM - 1)) {
		BK_LOGE(TAG, "bk_interrupt_m55sub_irq_handler failed: int_number(%d)\n", int_number);
		return;
	}

	if (s_m55sub_irq_handler[int_number]) {
		s_m55sub_irq_handler[int_number]();
	} else {
		BK_LOGE(TAG, "m55sub interrupt %d not register hanlder.\n", int_number);
	}
}


__attribute__((section(".itcm_sec_code"))) void soc_m55sub_handler(void) {

	uint32_t m55sub_int_status0 = sys_drv_get_m55sub_int_status0();
	uint32_t m55sub_int_status1 = sys_drv_get_m55sub_int_status1();
	uint32_t m55sub_int_status2 = sys_drv_get_m55sub_int_status2();

	//BK_LOGI(TAG, "status %x %x %x \r\n", m55sub_int_status0, m55sub_int_status1, m55sub_int_status2);
	if (m55sub_int_status0 ) // check m55sub_status0
	{
		for (uint8_t i = 0; i < 32; i++)
		{
			if (m55sub_int_status0 & (1 << i))
			{
				bk_interrupt_m55sub_irq_handler(i);
			}               
		}           
	}
	if (m55sub_int_status1 ) // check m55sub_status1
	{
		for (uint8_t i = 0; i < 32; i++)
		{
			if (m55sub_int_status1 & (1 << i))
			{
				bk_interrupt_m55sub_irq_handler(i + 32);
			}               
		}           
	}
	if (m55sub_int_status2 ) // check m55sub_status2
	{
		for (uint8_t i = 0; i < INT_SRC_CP_MAX_NUM - 64; i++)
		{
			if (m55sub_int_status2 & (1 << i))
			{
				bk_interrupt_m55sub_irq_handler(i + 64);
			}               
		}           
	}
}

// eof