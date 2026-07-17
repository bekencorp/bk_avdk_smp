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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <common/sys_config.h>

#if CONFIG_ARM_CORE_STAR
#include "armstar.h"
#elif CONFIG_ARM_CORE_CM52
#include "armcm52.h"
#elif CONFIG_ARM_CORE_CM55
#include "armcm55.h"
#else
#endif

#define BK_ARCH_BASEPRI_IRQ_MASK        (1UL << (8UL - __NVIC_PRIO_BITS))

static inline uint32_t bk_arch_get_basepri(void)
{
	return __get_BASEPRI();
}

static inline void bk_arch_set_basepri(uint32_t basepri)
{
	__set_BASEPRI(basepri);
	__DSB();
	__ISB();
}

static inline uint32_t bk_arch_raise_basepri(void)
{
	uint32_t old_basepri = bk_arch_get_basepri();

	if ((old_basepri == 0UL) || (old_basepri > BK_ARCH_BASEPRI_IRQ_MASK)) {
		bk_arch_set_basepri(BK_ARCH_BASEPRI_IRQ_MASK);
	}

	return old_basepri;
}

void arch_init(void);
void arch_wait_for_interrupt(void);
void arch_parse_stack_backtrace(const char *str_type, uint32_t stack_top,
uint32_t stack_bottom, uint32_t stack_size, bool thumb_mode);
void arch_sleep(void);
void arch_deep_sleep(void);
uint64_t arch_int_check_irq_pending(void);
