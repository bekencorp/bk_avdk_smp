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

#include "sdkconfig.h"
#include "bk_arch.h"
#include "arch_interrupt.h"
#include "components/log.h"
#include "interrupt_controller.h"

#define TO_NVIC_IRQ(irq)            ((uint32_t)(irq))

void arch_int_enable_irq(uint32_t irq)
{
	NVIC_EnableIRQ(TO_NVIC_IRQ(irq));
}

void arch_int_disable_irq(uint32_t irq)
{
	NVIC_DisableIRQ(TO_NVIC_IRQ(irq));
}

uint32_t arch_int_get_enable_irq(uint32_t irq)
{
	return NVIC_GetEnableIRQ(TO_NVIC_IRQ(irq));
}

void arch_int_set_target_state(uint32_t irq)
{
	NVIC_SetTargetState(TO_NVIC_IRQ(irq));
}

void arch_int_clear_target_state(uint32_t irq)
{
	NVIC_ClearTargetState(TO_NVIC_IRQ(irq));
}

uint32_t arch_int_get_target_state(uint32_t irq)
{
	return NVIC_GetTargetState(TO_NVIC_IRQ(irq));
}

void arch_interrupt_set_priority(uint32_t int_number, uint32_t int_priority)
{
	if (int_number > 0 && int_number < __INT_NUMBER_MAX) {
		NVIC_SetPriority(TO_NVIC_IRQ(int_number), int_priority);
	}

	return;
}

__IRAM_SEC void arch_int_set_default_priority(void)
{
	/* group priority is depends on macro:__NVIC_PRIO_BITS.
	   please refer to: www.freertos.org/zh-cn-cmn-s/RTOS-Cortex-M3-M4.html*/
	NVIC_SetPriorityGrouping(PRI_GOURP_BITS_7_5);
	for (uint32_t irq_type = 0; irq_type < __INT_NUMBER_MAX; irq_type++) {
		NVIC_SetPriority(irq_type, IRQ_DEFAULT_PRIORITY);
	}
}

void arch_interrupt_register_int(uint32_t int_number, int_group_isr_t isr_callback)
{
	int ret;

	ret = int_controller_connect_by_intc_id(INT_CONTROLLER_ID_PRIMARY, int_number, isr_callback);
	BK_ASSERT(ret == BK_OK);
#if CONFIG_SOC_SMP
	ret = int_controller_connect_by_intc_id(INT_CONTROLLER_ID_SECONDARY, int_number, isr_callback);
	BK_ASSERT(ret == BK_OK);
#endif

	NVIC_EnableIRQ(int_number);
}

void arch_interrupt_unregister_int(uint32_t int_number)
{
	int ret;

	if (int_number > (__INT_NUMBER_MAX - 1)) {
		return;
	}
	// NVIC_DisableIRQ(int_number);
	ret = int_controller_disconnect_by_intc_id(INT_CONTROLLER_ID_PRIMARY, int_number);
	BK_ASSERT(ret == BK_OK);
#if CONFIG_SOC_SMP
	ret = int_controller_disconnect_by_intc_id(INT_CONTROLLER_ID_SECONDARY, int_number);
	BK_ASSERT(ret == BK_OK);
#endif
}

void arch_int_init_all_irq(void)
{
	__disable_irq();
	__disable_fault_irq();

	for (uint32_t irq_type = 0; irq_type < __INT_NUMBER_MAX; irq_type++) {
		NVIC_SetPriority(irq_type, IRQ_DEFAULT_PRIORITY);
		NVIC_EnableIRQ(irq_type);
	}
}

void arch_int_enable_all_irq(void)
{
	for (uint32_t irq_type = 0; irq_type < __INT_NUMBER_MAX; irq_type++) {
		NVIC_SetPriority(irq_type, IRQ_DEFAULT_PRIORITY);
		NVIC_EnableIRQ(irq_type);
	}

	__enable_fault_irq();
	__enable_irq();
}

void arch_int_disable_all_irq(void)
{
	__disable_irq();
	__disable_fault_irq();

	for (uint32_t irq_type = 0; irq_type < __INT_NUMBER_MAX; irq_type++) {
		NVIC_DisableIRQ(irq_type);
	}
}


bk_err_t arch_isr_entry_init(void)
{
	/* group priority is depends on macro:__NVIC_PRIO_BITS.
	   please refer to: www.freertos.org/zh-cn-cmn-s/RTOS-Cortex-M3-M4.html*/
	NVIC_SetPriorityGrouping(PRI_GOURP_BITS_7_5);
	arch_int_init_all_irq();

	return BK_OK;
}

void arch_int_dump_statis(void)
{
#if CONFIG_ARCH_INT_STATIS
	for (uint32_t irq_type = 0; irq_type < __INT_NUMBER_MAX; irq_type++) {
		BK_LOGD(TAG, "[%d] = %u\r\n", irq_type, s_int_statis[irq_type]);
	}
#endif
}

__attribute__((section(".itcm_sec_code"))) int_group_isr_t arch_interrupt_get_handler(uint32_t int_number)
{
	isr_item_t *item;

	item = int_controller_get_isr_item_by_id(INT_CONTROLLER_ID_PRIMARY, int_number);
	return item->func;
}

__attribute__((section(".itcm_sec_code")))uint64_t arch_int_check_irq_pending(void)
{
	return NVIC_GetAllPendingIRQ();
}
// eof

