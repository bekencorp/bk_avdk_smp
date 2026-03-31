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
// eof