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
#include <soc/soc.h>
#include "common/bk_err.h"

#define START_TYPE_ADDR        (SOC_AON_PMU_REG_BASE + (3 << 2))
/*REBOOT_TAG_ADDR For CPU0-APP set reset tag in nmi wdt reboot*/
#define REBOOT_TAG_ADDR        (0x0FFF8 + SOC_DTCM_DATA_BASE)
#define PERSIST_MEMORY_ADDR    (0x0FFFC + SOC_DTCM_DATA_BASE)

uint32_t rr_hal_get_persist_mem_addr(void)
{
    return PERSIST_MEMORY_ADDR;
}

uint32_t rr_hal_set_persist_mem_val(uint32_t val)
{
    REG_WRITE(PERSIST_MEMORY_ADDR, val);

    return BK_OK;
}

uint32_t rr_hal_get_persist_mem_val(void)
{
    return REG_READ(PERSIST_MEMORY_ADDR);
}

uint32_t rr_hal_get_start_type_addr(void)
{
    return START_TYPE_ADDR;
}

uint32_t rr_hal_set_start_type_val(uint32_t val)
{
    REG_WRITE(START_TYPE_ADDR, val);

    return BK_OK;
}

uint32_t rr_hal_get_start_type_val(void)
{
    return REG_READ(START_TYPE_ADDR);
}

uint32_t rr_hal_get_reboot_tag_addr(void)
{
    return REBOOT_TAG_ADDR;
}

uint32_t rr_hal_set_reboot_tag_val(uint32_t val)
{
    REG_WRITE(REBOOT_TAG_ADDR, val);

    return BK_OK;
}

uint32_t rr_hal_get_reboot_tag_val(void)
{
    return REG_READ(REBOOT_TAG_ADDR);
}