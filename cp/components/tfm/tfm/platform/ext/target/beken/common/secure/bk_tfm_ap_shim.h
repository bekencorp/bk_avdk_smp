// Copyright 2023-2028 Beken
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* core1 Non-Secure vector table (AP __vector_core1_table, RAM NS alias). It is
 * fixed by the AP link layout; the AP linker asserts this value so a layout
 * shift is caught at build time. */
#define AP_CORE1_NS_VECTOR   0x3C181000u

/* Install the shim, patch the core0/core1 Non-Secure vectors, and return the
 * shim entry address. */
uint32_t bk_ap_shim_install(uint32_t core1_ns_vector);

#ifdef __cplusplus
}
#endif
