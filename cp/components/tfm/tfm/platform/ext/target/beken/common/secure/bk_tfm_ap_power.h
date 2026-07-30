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

/* Power on the AP power domain and enable its clock. Must be called before any
 * access to resources that sit in the AP power domain (AP AHBP MPC/PPHS, PSRAM
 * controller). The AP core is left in reset; it does not start executing here. */
void bk_ap_power_domain_on(void);

/* Release the AP core from software reset (SYS_AHBP reg4) so it boots from the
 * Secure flash alias. Call from the Secure world after isolation is configured. */
void bk_ap_release(void);
