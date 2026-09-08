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

/* boot_param: load / decide / reconcile A/B record in the boot_param partition. */

#define MODULE_DEBUG_LOG_ENABLE 1

#include <string.h>
#include "security.h"
#include "boot_param.h"
#include "flash_partition.h"
#include "partitions.h"
#include "bk_tfm_log.h"
#include <common/bk_err.h>

#define TAG "bp"

/* A/B decision trace (BK7234 has no BK_LOG_FORCE; use BK_LOGI). */
#define BP_FORCE(fmt, ...) BK_LOGI(TAG, fmt, ##__VA_ARGS__)

typedef struct {
	int8_t           latest_sector_idx; /* ping-pong sector 0/1 holding `active`; -1=virgin */
	ab_flag_record_t latest_record;     /* authoritative record (valid, largest seq) */
	uint8_t          preferred_slot;    /* cached A/B slot from decide_slot() */
	bool             inited;            /* load() has run */
} ab_flag_ctx_t;

static ab_flag_ctx_t s_ab;

/* Flash/CRC back-end: boot_param_ops.c */

uint32_t boot_param_get_sector_addr(int idx)
{
	return boot_param_partition_base() + (uint32_t)idx * AB_FLAG_SECTOR;
}

int boot_param_load(void)
{
	int latest_idx;

	memset(&s_ab, 0, sizeof(s_ab));
	s_ab.preferred_slot = AB_SLOT_A;

	/* Shared ping-pong selector: reads both sectors, validates each and returns
	 * the freshest valid copy (or -1 = virgin) in s_ab.latest_record. */
	latest_idx = ab_record_read_latest(boot_param_partition_base(),
					  &boot_param_ops, &s_ab.latest_record);
	s_ab.latest_sector_idx = (int8_t)latest_idx;
	s_ab.inited = true;

	if (s_ab.latest_sector_idx < 0) {
		if (latest_idx == AB_FLAG_ERR_IO) {
			BK_LOGE(TAG, "load: boot_param read failed\r\n");
		} else {
			BK_LOGW(TAG, "virgin: no valid boot_param record\r\n");
		}
		return -1;
	}

	BP_FORCE("idx=%d seq=%u exec=%d upd=%d st=%x dl=%x try=%u/%u\r\n",
		s_ab.latest_sector_idx, s_ab.latest_record.seq, s_ab.latest_record.exec_slot,
		s_ab.latest_record.update_slot, s_ab.latest_record.boot_state, s_ab.latest_record.dl_state,
		boot_param_pmu_try_get(), s_ab.latest_record.try_max);

	return 0;
}

uint8_t boot_param_decide_slot(void)
{
	if (!s_ab.inited) {
		(void)boot_param_load();
	}

	boot_param_pmu_try_inc(); /* shared CRC + TRIAL counter */

	/* Virgin / invalid -> slot A. */
	if (s_ab.latest_sector_idx < 0) {
		s_ab.preferred_slot = AB_SLOT_A;
		BP_FORCE("decide: virgin -> slot A (try=%u)\r\n",
			boot_param_pmu_try_get());
		return s_ab.preferred_slot;
	}

	ab_flag_record_t rec = s_ab.latest_record;  /* working copy for write-back */
	uint8_t preferred;
	bool need_commit = false;

	switch (rec.boot_state) {
	case AB_STATE_TRIAL: {
		uint8_t try_max = rec.try_max ? rec.try_max : (uint8_t)AB_TRY_MAX_DEFAULT;
		uint8_t try_cnt = boot_param_pmu_try_get();

		if (try_max >= BOOT_PARAM_PMU_TRY_MASK) {
			try_max = AB_TRY_MAX_DEFAULT;
		}

		/* cnt<=try_max: trial; else rollback (e.g. 5 -> cnt 1..5 trial, 6 rollback). */
		if (try_cnt <= try_max) {
			preferred = rec.update_slot;
			BP_FORCE("decide: TRIAL %u/%u -> slot %d\r\n",
				try_cnt, try_max, preferred);
		} else {
			preferred = rec.exec_slot;
			rec.boot_state = AB_STATE_NORMAL;
			rec.dl_state = AB_DL_IDLE;
			rec.update_slot = rec.exec_slot;
			memset(rec.rsvd0, 0, sizeof(rec.rsvd0));
			need_commit = true;
			BP_FORCE("decide: TRIAL done %u/%u -> rollback slot %d\r\n",
				try_cnt, try_max, preferred);
		}
		break;
	}
	case AB_STATE_CONFIRMED:
	case AB_STATE_NORMAL:
	default:
		preferred = rec.exec_slot;
		BP_FORCE("decide: NORMAL -> slot %d (try=%u)\r\n",
			preferred, boot_param_pmu_try_get());
		break;
	}

	if (preferred != AB_SLOT_A && preferred != AB_SLOT_B) {
		BP_FORCE("decide: bad slot %d -> force A\r\n", preferred);
		preferred = AB_SLOT_A;
	}

	if (need_commit) {
		if (boot_param_commit(&rec) == 0) {
			boot_param_pmu_try_clear();
		} else {
			BK_LOGE(TAG, "decide: rollback commit failed\r\n");
		}
	}

	s_ab.preferred_slot = preferred;
	return preferred;
}

uint8_t boot_param_preferred_slot(void)
{
	return s_ab.preferred_slot;
}

int boot_param_get_latest_record(ab_flag_record_t *out)
{
	if (out == NULL || s_ab.latest_sector_idx < 0) {
		return -1;
	}
	*out = s_ab.latest_record;
	return 0;
}

int boot_param_commit(const ab_flag_record_t *rec)
{
	if (rec == NULL) {
		return -1;
	}

	ab_flag_record_t out = *rec;
	int write_idx = ab_record_commit(boot_param_partition_base(), &boot_param_ops, &out);

	if (write_idx < 0) {
		BK_LOGE(TAG, "commit failed: %d\r\n", write_idx);
		return -1;
	}

	s_ab.latest_sector_idx = (int8_t)write_idx;
	s_ab.latest_record = out;

	BK_LOGI(TAG, "commit ok sector %d seq=%u crc=%x\r\n", write_idx, out.seq, out.crc32);
	return 0;
}

/* MCUboot hook: return preferred A/B slot (still validated by MCUboot). */
int boot_get_active_slot_hook(int img_index, uint32_t *slot)
{
	(void)img_index;
	*slot = (uint32_t)boot_param_preferred_slot();
	return 0;
}

/* image_off == secondary_all base -> B, else A. */
static uint8_t boot_param_slot_from_offset(uint32_t image_off)
{
	uint32_t sec = partition_get_phy_offset(PARTITION_SECONDARY_ALL);

	if (sec != 0u && image_off == sec) {
		return AB_SLOT_B;
	}
	return AB_SLOT_A;
}

void boot_param_reconcile_booted(uint32_t image_off)
{
	ab_flag_record_t rec;

	if (!s_ab.inited) {
		return;
	}

	uint8_t booted    = boot_param_slot_from_offset(image_off);
	uint8_t preferred = s_ab.preferred_slot;

	/* Match preferred: clear PMU unless still TRIAL. */
	if (booted == preferred) {
		if (boot_param_get_latest_record(&rec) != 0 ||
		    rec.boot_state != AB_STATE_TRIAL) {
			boot_param_pmu_try_clear();
		}
		return;
	}

	/* Preferred failed: commit booted slot as NORMAL. */
	if (boot_param_get_latest_record(&rec) != 0) {
		memset(&rec, 0, sizeof(rec));
		rec.try_max = AB_TRY_MAX_DEFAULT;
	}
	rec.exec_slot   = booted;
	rec.update_slot = booted;
	rec.boot_state  = AB_STATE_NORMAL;
	memset(rec.rsvd0, 0, sizeof(rec.rsvd0));
	rec.dl_state    = AB_DL_IDLE;

	if (boot_param_commit(&rec) == 0) {
		boot_param_pmu_try_clear();
	} else {
		BK_LOGE(TAG, "reconcile: fallback commit failed\r\n");
	}
	s_ab.preferred_slot = booted;
	BP_FORCE("reconcile: pref[%d] failed, booted[%d] -> NORMAL\r\n",
		preferred, booted);
}
