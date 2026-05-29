// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include <sys_sw_regs.h>
#include "pm_debug.h"

bool bk_pm_ap_first_boot_get(void)
{
	pm_shared_info_t shared_info = {0};

	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	return shared_info.pm_ap_first_boot;
}