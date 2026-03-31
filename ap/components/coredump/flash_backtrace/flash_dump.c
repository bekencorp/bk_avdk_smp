// Copyright 2025-2026 Beken
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

#include "flash_dump.h"
#include "soc_flash_dump.h"

uint32_t fdump_cpu_registers(uint32_t mcause, SAVED_CONTEXT *context)
{
    return soc_fdump_cpu_registers(mcause, context);
}

uint32_t fdump_save(void)
{
    uint32_t ret;

    ret = soc_fdump_save();

    return ret;
}

//eof

