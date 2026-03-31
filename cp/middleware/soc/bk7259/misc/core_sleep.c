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

#include "bk_arch.h"

__attribute__((section(".iram"))) void arch_deep_sleep(void)
{
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__WFI();
}

__attribute__((section(".iram"))) void arch_sleep(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	__WFI();
}
__attribute__((section(".iram"))) uint64_t check_IRQ_pending(void)
{
	return NVIC_GetAllPendingIRQ();
}