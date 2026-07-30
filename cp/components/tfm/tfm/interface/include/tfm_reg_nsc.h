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

/* Non-secure prototypes for the CMSE register-access gateways implemented in the
 * secure world (reg_nsc.c). No cmse_nonsecure_entry here: the NS side calls these
 * as ordinary functions and the linker resolves them to the SG-stub veneers in
 * libtfm_s_veneers.a. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t psa_reg_read(uint32_t addr);
void psa_reg_write(uint32_t addr, uint32_t value);
void psa_reg_dump(uint32_t addr, uint32_t size);

#ifdef __cplusplus
}
#endif
