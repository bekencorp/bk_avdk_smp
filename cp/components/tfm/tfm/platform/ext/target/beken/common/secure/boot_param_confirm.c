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
 * resets boot the new slot directly (no more PMU trial-count bumps or rollback).
 *
 * The record layout and ping-pong algorithm are shared with BL2 via boot_param.h
 * (static-inline ab_record_read_latest / ab_record_commit); the flash back-end +
 * CRC32 are the shared boot_param_ops.c (boot_param_ops /
 * boot_param_partition_base). This unit only holds the SPE confirm transition. */

#include <string.h>
#include "boot_param.h"
#include "bk_tfm_log.h"

#define TAG "bp_confirm"

/* Route to the unified SPM-sink log. Force: the A/B confirm trace is always
 * emitted, even with CONFIG_TFM_LOG_LEVEL lowered for production. */
#define BP_LOG(fmt, ...) BK_LOG_FORCE(TAG ": " fmt, ##__VA_ARGS__)

/* op_sw erase/PP are ignored while the flash is in QUAD continuous-read (the XIP
 * path leaves it there), so a commit must drop to TWO first and restore after.
 * Mirrors boot_param_commit() in BL2. */
extern void bk_flash_min_unprotect_once(void);
extern void bk_flash_min_switch_line_mode_two(void);
extern void bk_flash_min_restore_line_mode(void);

/* Running A/B slot from the flash XIP remap enable (0=A / 1=B), set by MCUboot
 * for the slot it actually booted. */
extern uint32_t flash_get_excute_enable(void);

/* Confirm the currently running TRIAL image. Idempotent: a non-TRIAL record (or
 * a virgin partition) does nothing and writes no flash. Reaching here means the
 * secure world came up on the CURRENTLY RUNNING slot, so that slot is adopted as
 * the committed exec_slot -- never the record's update_slot blindly: if MCUboot
 * fell back off the trial slot (bad signature) and BL2 reconcile failed to
 * rewrite the record, the running slot is still the exec_slot, and promoting the
 * rejected update_slot here would defeat the rollback. Returns 0 on success /
 * nothing-to-do, -1 if no valid record exists or the commit failed. */
int boot_param_confirm(void)
{
	ab_flag_record_t rec;
	uint32_t base = boot_param_partition_base();
	uint8_t running = flash_get_excute_enable() ? (uint8_t)AB_SLOT_B : (uint8_t)AB_SLOT_A;
	int idx;

	idx = ab_record_read_latest(base, &boot_param_ops, &rec);
	if (idx < 0) {
		BP_LOG("no valid record (%d), skip\r\n", idx);
		return -1;
	}

	if (rec.boot_state != AB_STATE_TRIAL) {
		BP_LOG("state=%x not TRIAL, skip\r\n", rec.boot_state);
		return 0;
	}

	BP_LOG("TRIAL run=%d exec=%d upd=%d -> NORMAL\r\n",
		running, rec.exec_slot, rec.update_slot);

	/* Adopt the slot that actually brought up the secure world; drop the pending
	 * update. This is a promotion when running==update_slot (trial succeeded) and
	 * a repair when running==exec_slot (MCUboot fell back, reconcile did not
	 * persist). */
	rec.exec_slot   = running;
	rec.update_slot = running;
	rec.boot_state  = AB_STATE_NORMAL;
	memset(rec.rsvd0, 0, sizeof(rec.rsvd0));
	rec.dl_state    = AB_DL_IDLE;

	/* BK7259SW-2937: unprotect first or erase/PP are no-ops under status
	 * protect (same trap as ota_confirm). Then leave QUAD continuous-read so
	 * op_sw is accepted; XIP fetch still works in TWO mode. */
	bk_flash_min_unprotect_once();
	bk_flash_min_switch_line_mode_two();
	idx = ab_record_commit(base, &boot_param_ops, &rec);
	bk_flash_min_restore_line_mode();

	if (idx < 0) {
		BK_LOGE(TAG, "commit failed %d\r\n", idx);
		return -1;
	}

	/* Trial confirmed: clear the AON_PMU counter (register-only, no flash). */
	boot_param_pmu_try_clear();

	BP_LOG("done, exec_slot=%d sector=%d\r\n", running, idx);
	return 0;
}
