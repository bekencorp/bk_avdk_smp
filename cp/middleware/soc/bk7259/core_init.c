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

#include "components/log.h"
#include "components/system.h"
#include "soc/soc.h"
#include "wwdt_hw.h"

void core_init(void)
{
	REG_WRITE(WWDT_WDT_CONFIG_ADDR, (0x5A0000 | 0));
	REG_WRITE(WWDT_WDT_CONFIG_ADDR, (0xA50000 | 0));

	extern void timer_hal_us_init(void);
	timer_hal_us_init();
}
