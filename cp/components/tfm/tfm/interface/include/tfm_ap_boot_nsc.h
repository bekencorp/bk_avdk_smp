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

/* Non-secure prototype for the AP boot release gateway (ap_boot_nsc.c). The NS
 * side calls this as an ordinary function; the linker resolves it to the
 * SG-stub veneer in libtfm_s_veneers.a. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Release the AP core (shim install + reset de-assert). AP power domain must
 * already be up from TF-M isolation init. Idempotent: returns 0 if already
 * released. Returns -1 on failure. */
int psa_ap_boot(void);

#ifdef __cplusplus
}
#endif
