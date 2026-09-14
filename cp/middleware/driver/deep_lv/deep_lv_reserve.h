// Copyright 2023-2024 Beken
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

/* Turning the CPU domain off for Deep-LV sleep stops the SRAM bad-point repair
 * from being applied: Reg03 mchk_valid goes 1 -> 0, so on wake a repaired word
 * reads back its raw defective cell instead of the spare it was redirected to.
 * The SRAM array itself stays retained, and so do the bad-point addresses in
 * the MEM_CHECK registers - those are still readable after the wake, which is
 * why the addresses are decoded from the registers on both passes rather than
 * kept anywhere. The registers are read-only to software, so the repair cannot
 * be re-programmed; the words it covered are read before sleep and written back
 * on wake instead. Ported from BK7236N (Jira BK7236NSW-712).
 *
 * The captured words are held in a table private to deep_lv_reserve.c, which is
 * time-shared with the deepest end of the MSP stack and therefore only valid
 * between the save below and the matching restore. */

/* Capture the repaired words. Call just before arch_deep_sleep(), after the
 * memory content is final. */
void sys_hal_mem_check_bad_point_value_save(void);

/* Write the captured words back. Call as early as possible on the wake path,
 * before any other code executes or reads SRAM. */
void sys_hal_mem_check_bad_point_value_restore(void);

#ifdef __cplusplus
}
#endif
