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
#include <os/os.h>
#include <os/mem.h>
#include "multicore_hal.h"
#include "multicore_driver.h"

void bk_multicore_set_cpu_id(uint32_t cpu_id)
{
	multicore_hal_set_cpu_id(cpu_id);
}

uint32_t bk_multicore_get_cpu_id(void)
{
	return multicore_hal_get_cpu_id();
}

bk_err_t bk_multicore_start(uint32_t cpu_id)
{
	return multicore_hal_start(cpu_id);
}

bk_err_t bk_multicore_reset(uint32_t cpu_id)
{
	return multicore_hal_reset(cpu_id);
}

bk_err_t bk_multicore_stop(uint32_t cpu_id)
{
	return multicore_hal_stop(cpu_id);
}