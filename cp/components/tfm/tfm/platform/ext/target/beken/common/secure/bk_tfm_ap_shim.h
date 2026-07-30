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

/* Reserved Secure RAM for the AP boot shim: first 4K block of smem3 (Secure
 * alias). Kept Secure by the smem3 MPC while the rest of the AP RAM is
 * Non-Secure. The shim vector head and code must be loaded here and the AP boot
 * vector pointed at this address. */
#define AP_SHIM_BASE   0x28100000u

/* core1 Non-Secure vector table (AP __vector_core1_table, RAM NS alias). It is
 * fixed by the AP link layout; the AP linker asserts this value so a layout
 * shift is caught at build time. */
#define AP_CORE1_NS_VECTOR   0x3C181000u

/* Install the shim into AP_SHIM_BASE, patch the core0/core1 Non-Secure vectors,
 * and return the AP boot address (shim entry vector). */
uint32_t bk_ap_shim_install(uint32_t core1_ns_vector);

#ifdef __cplusplus
}
#endif
