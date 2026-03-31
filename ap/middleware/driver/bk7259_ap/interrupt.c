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
#include "sys_hal.h"
#include "sys_driver.h"

#define TAG "INT"

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#include "bk_arch.h"

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