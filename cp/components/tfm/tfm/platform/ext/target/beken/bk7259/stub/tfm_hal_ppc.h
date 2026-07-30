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

// BK7259 PPC init stub.
//
// On other Beken SoCs this generated header drives the PRRO peripheral
// privilege/secure attribution via the bk_prro_set_* driver API. BK7259 does
// not port that PRRO driver into the secure world; the equivalent PPC secure
// attribution (SYS/Flash/GPIO/AON_WDT) is performed by
// common/secure/bk_tfm_ppc.c (bk_ppc_init), which tfm_hal_isolation.c invokes.
// tfm_hal_ppc_init() here is unused on bk7259, so keep it a no-op to avoid
// pulling in the unported PRRO enums/driver symbols.

#pragma once

static inline void tfm_hal_ppc_init(void)
{
}