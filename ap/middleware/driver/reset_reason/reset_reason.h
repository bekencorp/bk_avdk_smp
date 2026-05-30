// Copyright 2020-2021 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
#include "soc/soc.h"

#define REBOOT_TAG_REQ               (0xAA55AA55)
#define CRASH_ILLEGAL_JUMP_VALUE      0xbedead00
#define CRASH_UNDEFINED_VALUE         0xbedead01
#define CRASH_PREFETCH_ABORT_VALUE    0xbedead02
#define CRASH_DATA_ABORT_VALUE        0xbedead03
#define CRASH_UNUSED_VALUE            0xbedead04
#define POWERON_INIT_MEM_TAG          (0xaaaaaaaa)


void show_reset_reason(void);
uint32_t reset_reason_init(void);
void bk_misc_set_cp_reset_reason(uint32_t type);
void bk_misc_set_ap_reset_reason(uint32_t type);



void set_nmi_vector(void);

#ifdef __cplusplus
}
#endif
