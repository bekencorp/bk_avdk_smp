// Copyright     2023-2028 Beken
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

#include "driver/mpc.h"
#include "tfm_flash_partition.h"

int bk_mpc_cfg(void);

/* Re-apply only the AP-domain AHBP MPC attributes. Used when the AP power domain
 * is brought up again after a power-off, since the AP MPC state is lost with the
 * domain. The AP power domain must already be on before calling. */
int bk_mpc_ap_cfg(void);
