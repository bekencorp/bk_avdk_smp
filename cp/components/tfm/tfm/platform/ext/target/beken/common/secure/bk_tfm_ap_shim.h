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

/* AP Non-Secure vector reserved slots used to publish the Secure-fault dump
 * callback ABI. Keep these values in sync with ap/components/coredump. */
#define AP_SEC_DUMP_VECTOR_CALLBACK_OFFSET  0x20
#define AP_SEC_DUMP_VECTOR_CONTEXT_OFFSET   0x24
#define AP_SEC_DUMP_VECTOR_ABI_OFFSET       0x28
#define AP_SEC_DUMP_ABI_INFO                0x53440130
#define AP_SEC_DUMP_CONTEXT_MAGIC           0x41505346
#define AP_SEC_DUMP_CONTEXT_VERSION         1
#define AP_SEC_DUMP_CONTEXT_SIZE            0xC0

#define AP_SEC_CTX_MAGIC        0x00
#define AP_SEC_CTX_VERSION      0x04
#define AP_SEC_CTX_SIZE         0x08
#define AP_SEC_CTX_CORE_ID      0x0C
#define AP_SEC_CTX_FLAGS        0x10
#define AP_SEC_CTX_EXC_RETURN   0x14
#define AP_SEC_CTX_FRAME_SP     0x18
#define AP_SEC_CTX_FRAME_VALID  0x1C
#define AP_SEC_CTX_R0           0x20
#define AP_SEC_CTX_SP           0x54
#define AP_SEC_CTX_LR           0x58
#define AP_SEC_CTX_PC           0x5C
#define AP_SEC_CTX_XPSR         0x60
#define AP_SEC_CTX_MSP_S        0x64
#define AP_SEC_CTX_PSP_S        0x68
#define AP_SEC_CTX_MSP_NS       0x6C
#define AP_SEC_CTX_PSP_NS       0x70
#define AP_SEC_CTX_CONTROL_S    0x74
#define AP_SEC_CTX_CONTROL_NS   0x78
#define AP_SEC_CTX_SFSR         0x7C
#define AP_SEC_CTX_SFAR         0x80
#define AP_SEC_CTX_CFSR_S       0x84
#define AP_SEC_CTX_HFSR_S       0x88
#define AP_SEC_CTX_BFAR_S       0x8C
#define AP_SEC_CTX_MMFAR_S      0x90
#define AP_SEC_CTX_IPSR         0x94
#define AP_SEC_CTX_PRIMASK_S    0x98
#define AP_SEC_CTX_BASEPRI_S    0x9C
#define AP_SEC_CTX_FAULTMASK_S  0xA0
#define AP_SEC_CTX_FPSCR_S      0xA4
#define AP_SEC_CTX_PRIMASK_NS   0xA8
#define AP_SEC_CTX_BASEPRI_NS   0xAC
#define AP_SEC_CTX_FAULTMASK_NS 0xB0

#define AP_SEC_CTX_FLAG_SOURCE_SECURE  0x01
#define AP_SEC_CTX_FLAG_SOURCE_THREAD  0x02
#define AP_SEC_CTX_FLAG_SOURCE_PSP     0x04
#define AP_SEC_CTX_FLAG_DCRS_STACKED   0x08
#define AP_SEC_CTX_FLAG_FP_STACKED     0x10

/* Install the shim, patch the core0/core1 Non-Secure vectors, and return the
 * shim entry address. */
uint32_t bk_ap_shim_install(uint32_t core1_ns_vector);

#ifdef __cplusplus
}
#endif
