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

/* boot_param_confirm: TF-M runtime (SPE) side of the A/B trial-boot handshake.
 *
 * BL2 boots a TRIAL image (boot_param_decide_slot picks update_slot) but leaves
 * the record in TRIAL: it is the running firmware's job to CONFIRM once it has
 * proven itself healthy. This module does that confirm by adopting update_slot
 * as the new exec_slot and settling the record back to NORMAL, so subsequent
 * resets boot the new slot directly (no more try_count bumps, no rollback).
 *
 * The record layout and ping-pong algorithm are shared with BL2 via boot_param.h
 * (static-inline ab_record_read_latest / ab_record_commit); the flash back-end +
 * CRC32 are the shared boot_param_ops.c (boot_param_ops /
 * boot_param_partition_base). This unit only holds the SPE confirm transition. */

#include "boot_param.h"

/* op_sw erase/PP are ignored while the flash is in QUAD continuous-read (the XIP
 * path leaves it there), so a commit must drop to TWO first and restore after.
 * Mirrors boot_param_commit() in BL2. */
extern void bk_flash_min_switch_line_mode_two(void);
extern void bk_flash_min_restore_line_mode(void);

/* Confirm the currently running TRIAL image. Idempotent: a non-TRIAL record (or
 * a virgin partition) does nothing and writes no flash. On TRIAL it adopts
 * update_slot as the committed exec_slot and settles to NORMAL. Returns 0 on
 * success / nothing-to-do, -1 if no valid record exists. */
int boot_param_confirm(void)
{
	ab_flag_record_t rec;
	uint32_t base = boot_param_partition_base();
	int idx;

	idx = ab_record_read_latest(base, &boot_param_ops, &rec);
	if (idx < 0) {
		return -1;
	}

	if (rec.boot_state != AB_STATE_TRIAL) {
		return 0;
	}

	/* Adopt the trial slot as the new committed slot; drop the pending update. */
	rec.exec_slot   = rec.update_slot;
	rec.boot_state  = AB_STATE_NORMAL;
	rec.try_count   = 0;
	rec.dl_state    = AB_DL_IDLE;

	/* op_sw erase/PP are ignored while the flash is in QUAD continuous-read (the
	 * XIP path leaves it there after BL2 hands over), so drop to TWO around the
	 * commit and restore after. XIP code fetch keeps working in TWO mode. */
	bk_flash_min_switch_line_mode_two();
	(void)ab_record_commit(base, &boot_param_ops, &rec);
	bk_flash_min_restore_line_mode();

	/* Trial confirmed: clear the AON_PMU counter (register-only, no flash). */
	boot_param_pmu_try_clear();

	return 0;
}
