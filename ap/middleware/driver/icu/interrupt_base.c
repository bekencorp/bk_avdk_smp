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

#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include <os/mem.h>
#include "bk_arm_arch.h"
#include "arch_interrupt.h"
#include "icu_driver.h"
#include "interrupt_base.h"
#include "interrupt.h"
#include "arch_interrupt.h"
#include <driver/int_types.h>
#include <driver/int.h>
#include <common/bk_assert.h>

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#define ICU_RETURN_ON_INVALID_DEVS(dev) do {\
				if ((dev) >= INT_SRC_NONE) {\
					return BK_ERR_INT_DEVICE_NONE;\
				}\
			} while(0)

#if CONFIG_FREERTOS_TRACE
#define IRQ_TRACE_BEGIN(irq) xTraceISRBegin(xGetTraceISRHandle(irq))
#define IRQ_TRACE_END()      xTraceISREnd(0)
#else
#define IRQ_TRACE_BEGIN(irq)
#define IRQ_TRACE_END()
#endif

#if CONFIG_FREERTOS_TRACE || CONFIG_INTERRUPT_DEBUG_RECORDER
extern void bk_interrupt_debug_isr_enter(uint32_t irq);
extern void bk_interrupt_debug_isr_exit(uint32_t irq);

static int_group_isr_t s_registered_isr[INT_SRC_NONE];

static __attribute__((section(".itcm_sec_code"))) void bk_int_dispatch_registered_isr(uint32_t irq)
{
	int_group_isr_t isr_callback = s_registered_isr[irq];

	IRQ_TRACE_BEGIN(irq);
	bk_interrupt_debug_isr_enter(irq);
	if (isr_callback) {
		isr_callback();
	}
	bk_interrupt_debug_isr_exit(irq);
	IRQ_TRACE_END();
}

#define DEFINE_INT_TRAMPOLINE(irq) \
	static __attribute__((section(".itcm_sec_code"))) void bk_int_trampoline_##irq(void) \
	{ \
		bk_int_dispatch_registered_isr(irq); \
	}

DEFINE_INT_TRAMPOLINE(0)
DEFINE_INT_TRAMPOLINE(1)
DEFINE_INT_TRAMPOLINE(2)
DEFINE_INT_TRAMPOLINE(3)
DEFINE_INT_TRAMPOLINE(4)
DEFINE_INT_TRAMPOLINE(5)
DEFINE_INT_TRAMPOLINE(6)
DEFINE_INT_TRAMPOLINE(7)
DEFINE_INT_TRAMPOLINE(8)
DEFINE_INT_TRAMPOLINE(9)
DEFINE_INT_TRAMPOLINE(10)
DEFINE_INT_TRAMPOLINE(11)
DEFINE_INT_TRAMPOLINE(12)
DEFINE_INT_TRAMPOLINE(13)
DEFINE_INT_TRAMPOLINE(14)
DEFINE_INT_TRAMPOLINE(15)
DEFINE_INT_TRAMPOLINE(16)
DEFINE_INT_TRAMPOLINE(17)
DEFINE_INT_TRAMPOLINE(18)
DEFINE_INT_TRAMPOLINE(19)
DEFINE_INT_TRAMPOLINE(20)
DEFINE_INT_TRAMPOLINE(21)
DEFINE_INT_TRAMPOLINE(22)
DEFINE_INT_TRAMPOLINE(23)
DEFINE_INT_TRAMPOLINE(24)
DEFINE_INT_TRAMPOLINE(25)
DEFINE_INT_TRAMPOLINE(26)
DEFINE_INT_TRAMPOLINE(27)
DEFINE_INT_TRAMPOLINE(28)
DEFINE_INT_TRAMPOLINE(29)
DEFINE_INT_TRAMPOLINE(30)
DEFINE_INT_TRAMPOLINE(31)
DEFINE_INT_TRAMPOLINE(32)
DEFINE_INT_TRAMPOLINE(33)
DEFINE_INT_TRAMPOLINE(34)
DEFINE_INT_TRAMPOLINE(35)
DEFINE_INT_TRAMPOLINE(36)
DEFINE_INT_TRAMPOLINE(37)
DEFINE_INT_TRAMPOLINE(38)
DEFINE_INT_TRAMPOLINE(39)
DEFINE_INT_TRAMPOLINE(40)
DEFINE_INT_TRAMPOLINE(41)
DEFINE_INT_TRAMPOLINE(42)
DEFINE_INT_TRAMPOLINE(43)
DEFINE_INT_TRAMPOLINE(44)
DEFINE_INT_TRAMPOLINE(45)
DEFINE_INT_TRAMPOLINE(46)
DEFINE_INT_TRAMPOLINE(47)
DEFINE_INT_TRAMPOLINE(48)
DEFINE_INT_TRAMPOLINE(49)
DEFINE_INT_TRAMPOLINE(50)
DEFINE_INT_TRAMPOLINE(51)
DEFINE_INT_TRAMPOLINE(52)
DEFINE_INT_TRAMPOLINE(53)

static const int_group_isr_t s_int_trampoline_table[INT_SRC_NONE] = {
	bk_int_trampoline_0,
	bk_int_trampoline_1,
	bk_int_trampoline_2,
	bk_int_trampoline_3,
	bk_int_trampoline_4,
	bk_int_trampoline_5,
	bk_int_trampoline_6,
	bk_int_trampoline_7,
	bk_int_trampoline_8,
	bk_int_trampoline_9,
	bk_int_trampoline_10,
	bk_int_trampoline_11,
	bk_int_trampoline_12,
	bk_int_trampoline_13,
	bk_int_trampoline_14,
	bk_int_trampoline_15,
	bk_int_trampoline_16,
	bk_int_trampoline_17,
	bk_int_trampoline_18,
	bk_int_trampoline_19,
	bk_int_trampoline_20,
	bk_int_trampoline_21,
	bk_int_trampoline_22,
	bk_int_trampoline_23,
	bk_int_trampoline_24,
	bk_int_trampoline_25,
	bk_int_trampoline_26,
	bk_int_trampoline_27,
	bk_int_trampoline_28,
	bk_int_trampoline_29,
	bk_int_trampoline_30,
	bk_int_trampoline_31,
	bk_int_trampoline_32,
	bk_int_trampoline_33,
	bk_int_trampoline_34,
	bk_int_trampoline_35,
	bk_int_trampoline_36,
	bk_int_trampoline_37,
	bk_int_trampoline_38,
	bk_int_trampoline_39,
	bk_int_trampoline_40,
	bk_int_trampoline_41,
	bk_int_trampoline_42,
	bk_int_trampoline_43,
	bk_int_trampoline_44,
	bk_int_trampoline_45,
	bk_int_trampoline_46,
	bk_int_trampoline_47,
	bk_int_trampoline_48,
	bk_int_trampoline_49,
	bk_int_trampoline_50,
	bk_int_trampoline_51,
	bk_int_trampoline_52,
	bk_int_trampoline_53,
};
#endif

bk_err_t bk_int_isr_register(icu_int_src_t src, int_group_isr_t isr_callback, void*arg)
{
	ICU_RETURN_ON_INVALID_DEVS(src);

	arch_interrupt_unregister_int(src);
#if CONFIG_FREERTOS_TRACE || CONFIG_INTERRUPT_DEBUG_RECORDER
	s_registered_isr[src] = isr_callback;
	arch_interrupt_register_int(src, isr_callback ? s_int_trampoline_table[src] : NULL);
#else
	arch_interrupt_register_int(src, isr_callback);
#endif
	arch_interrupt_set_priority(src, IQR_PRI_DEFAULT);

	return 0;
}

void interrupt_init(void)
{
	soc_isr_init();
}

void interrupt_deinit(void)
{
	soc_isr_deinit();
}

bk_err_t bk_int_set_priority(icu_int_src_t int_src, uint32_t int_priority)
{
	arch_interrupt_set_priority(int_src, int_priority);

	return BK_OK;
}

bk_err_t bk_int_isr_unregister(icu_int_src_t src)
{
	ICU_RETURN_ON_INVALID_DEVS(src);

#if CONFIG_FREERTOS_TRACE || CONFIG_INTERRUPT_DEBUG_RECORDER
	s_registered_isr[src] = NULL;
#endif
	arch_interrupt_unregister_int(src);

	return BK_OK;
}

