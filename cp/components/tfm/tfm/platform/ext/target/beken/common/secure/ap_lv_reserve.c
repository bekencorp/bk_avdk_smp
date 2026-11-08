// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

/* AP SRAM bad-point handling, secure-world half (secure project).
 *
 * The AP MEM_CHECK window (0x48010000, SOC_AP_MEM_CHECK_REG_BASE) and its OTP
 * region are Secure Only, and the AP bus master is forced Non-Secure, so the
 * CP NS world drives both through the NSC gateways below.
 * The plain project does the same work CP-side,
 * with the import in mcheck_hal.c and the retention in deep_lv/ap_lv_reserve.c;
 * the geometry and the decode are shared through deep_lv_reserve_layout.h.
 *
 * IMPORT MUST PRECEDE RESTORE: the repair redirect has to be active for the
 * write-back to reach the redundant cell instead of the bad main array.
 *
 * The table is a plain .bss object on purpose: the warm path never runs
 * Pre_Main(), so a capture survives; a cold boot zeroes it and the write-back is
 * magic-gated. */

#include <stdint.h>

#include "cmsis.h"
#include "security_defs.h"
#include "reg_base.h"
#include "deep_lv_reserve_layout.h"
#include "otp_s.h"

extern void flush_all_dcache(void);

/* --- AP MEM_CHECK repair import, the twin of mcheck_hal.c ------------------- */

/* AP MEM_CHECK register window, Secure alias (the window is Secure Only). The
 * address comes from reg_base.h like every other register block. */

/* Same repair-slot layout as mcheck_hal.c. */
#define AP_MEM_CHECK_OTP_WORDS           58U
#define AP_MEM_CHECK_OTP_BYTES           (AP_MEM_CHECK_OTP_WORDS * 4U)
#define AP_MEM_CHECK_REPAIR_LIMIT        20U
#define AP_MEM_CHECK_REPAIR_SLOT_NUM     128U
#define AP_MEM_CHECK_REPAIR_REG_FIRST    8U
#define AP_MEM_CHECK_OTP_VALID_MASK      0x80000000U
#define AP_MEM_CHECK_OTP_SLOT_MASK       0x007F0000U
#define AP_MEM_CHECK_OTP_SLOT_SHIFT      16U
#define AP_MEM_CHECK_OTP_DATA_MASK       0x0000FFFFU

/* Low-half write masks for MCHK_CFG[8..135], generated from the MEM_CHECK
 * address-mapping sheet of BK7259v2. Kept in sync with mcheck_hal.c. */
static const uint32_t s_ap_mem_check_restore_mask[AP_MEM_CHECK_REPAIR_SLOT_NUM] = {
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU, 0x000003FFU,
	0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU, 0x000007FFU, 0x000007FFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x00007FFFU, 0x00007FFFU, 0x00007FFFU, 0x00007FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU, 0x00001FFFU,
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00007FFFU, 0x00007FFFU, 0x00003FFFU, 0x00003FFFU,
	0x00003FFFU, 0x00003FFFU, 0x00001FFFU, 0x00001FFFU, 0x00003FFFU, 0x00003FFFU, 0x00001FFFU, 0x00001FFFU,
	0x00001FFFU, 0x00001FFFU, 0x00000FFFU, 0x00000FFFU, 0x00001FFFU, 0x00001FFFU, 0x0000FFFFU, 0x0000FFFFU,
	0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU, 0x00003FFFU, 0x00003FFFU, 0x00003FFFU, 0x00003FFFU,
	0x00003FFFU, 0x00003FFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x0000FFFFU, 0x00001FFFU, 0x00001FFFU,
	0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU, 0x00000FFFU,
};

static uint32_t load_le32(const uint8_t *data)
{
	return (uint32_t)data[0]
		| ((uint32_t)data[1] << 8)
		| ((uint32_t)data[2] << 16)
		| ((uint32_t)data[3] << 24);
}

/* --- AP SRAM bad-point retention ------------------------------------------- */

#define AP_LV_RESERVE_MAGIC             0x414C5652U /* "ALVR" */

typedef struct {
	uint32_t addr;
	uint32_t words;
	uint32_t values[AP_MEM_CHECK_POINT_MAX_WORDS];
} ap_lv_reserve_entry_t;

typedef struct {
	uint32_t magic;
	uint32_t point_mask;   /* bit i set: entry i has a bad cell to write back */
	ap_lv_reserve_entry_t entry[AP_MEM_CHECK_POINT_NUM];
} ap_lv_reserve_t;

static volatile ap_lv_reserve_t s_ap_lv_reserve;

static void ap_lv_reserve_restore(void)
{
	volatile ap_lv_reserve_t *tbl = &s_ap_lv_reserve;
	uint32_t point_mask;

	if (tbl->magic != AP_LV_RESERVE_MAGIC) {
		return;
	}

	point_mask = tbl->point_mask;
	tbl->magic = 0U;

	while (point_mask != 0U) {
		uint32_t i = (uint32_t)__builtin_ctz(point_mask);
		uint32_t addr;
		uint32_t words;

		point_mask &= point_mask - 1U;

		addr = tbl->entry[i].addr;
		words = tbl->entry[i].words;

		/* Retained RAM can flip a word: keep the write inside the AP banks. */
		if (!ap_mem_check_addr_in_ap_ram(addr) ||
			(words > AP_MEM_CHECK_POINT_MAX_WORDS)) {
			continue;
		}

		for (uint32_t w = 0; w < words; w++) {
			((volatile uint32_t *)addr)[w] = tbl->entry[i].values[w];
		}
	}

	__DSB();
}

static void ap_lv_reserve_save(void)
{
	volatile ap_lv_reserve_t *tbl = &s_ap_lv_reserve;
	uint32_t point_mask = 0U;

	for (uint32_t i = 0; i < AP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t addr = ap_mem_check_bad_point_addr_get(i);
		uint32_t words = (addr != 0U) ? ap_mem_check_bad_point_words_get(i) : 0U;

		if (addr != 0U) {
			point_mask |= 1U << i;
		}

		tbl->entry[i].addr = addr;
		tbl->entry[i].words = words;
		for (uint32_t w = 0; w < words; w++) {
			tbl->entry[i].values[w] = ((volatile uint32_t *)addr)[w];
		}
	}

	tbl->point_mask = point_mask;
	flush_all_dcache();
	__DSB();

	/* Magic last: a capture that did not complete must never be consumed. */
	tbl->magic = AP_LV_RESERVE_MAGIC;
	__DSB();
}

/* --- NSC gateways, for the CP Non-Secure world ------------------------------ */

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() so this member of
 * libplatform_s.a is pulled into tfm_s at all. Nothing Secure-side calls the
 * gateways below, and an un-pulled member yields no SG stubs for them. */
void psa_ap_lv_reserve_nsc_stub(void)
{
}

/* The decoded repair records, one word each: repair data in [31:16], repair slot
 * in [6:0]. Kept in secure .bss (retained across CP deep-LV); decoding once per
 * CP boot is enough, every later AP bring-up only replays the plan. Twin of
 * mcheck_hal.c. */
#define AP_MEM_CHECK_PLAN_MAGIC         0x414D434AU /* "JCMA" */
#define AP_MEM_CHECK_PLAN_DATA_SHIFT    16U
#define AP_MEM_CHECK_PLAN_SLOT_MASK     0x0000007FU

static uint32_t s_ap_mem_check_plan[AP_MEM_CHECK_REPAIR_LIMIT];
static uint32_t s_ap_mem_check_plan_num;
static uint32_t s_ap_mem_check_plan_magic;

static void ap_mem_check_plan_build(const uint8_t *otp)
{
	uint32_t num = 0;

	s_ap_mem_check_plan_magic = 0U;

	for (uint32_t off = 0; off + 4U <= AP_MEM_CHECK_OTP_BYTES; off += 4U) {
		uint32_t word = load_le32(&otp[off]);
		uint32_t slot;

		if ((word & AP_MEM_CHECK_OTP_VALID_MASK) == 0U) {
			continue;
		}

		slot = (word & AP_MEM_CHECK_OTP_SLOT_MASK) >> AP_MEM_CHECK_OTP_SLOT_SHIFT;
		if (slot >= AP_MEM_CHECK_REPAIR_SLOT_NUM) {
			continue;
		}

		s_ap_mem_check_plan[num] = (slot & AP_MEM_CHECK_PLAN_SLOT_MASK) |
			((word & AP_MEM_CHECK_OTP_DATA_MASK) << AP_MEM_CHECK_PLAN_DATA_SHIFT);

		if (++num >= AP_MEM_CHECK_REPAIR_LIMIT) {
			break;
		}
	}

	s_ap_mem_check_plan_num = num;
	/* Magic last: a partially built plan must never be replayed. */
	s_ap_mem_check_plan_magic = AP_MEM_CHECK_PLAN_MAGIC;
}

/* The AP memory-check OTP region is written once, in production, so it cannot
 * change while the device runs. The image is read into a local buffer, so the
 * 232 bytes are only on the stack during the cold-boot build; keep this out of
 * line so a warm bring-up never pays that frame. */
static __attribute__((noinline)) bk_err_t ap_mem_check_plan_rebuild(void)
{
	uint8_t otp[AP_MEM_CHECK_OTP_BYTES];

	if (otp_read(OTP_MAP_APB, OTP_M55SUB_MEMCHECK, otp, sizeof(otp)) != BK_OK) {
		return BK_FAIL;
	}

	ap_mem_check_plan_build(otp);
	return BK_OK;
}

/* Return the repair plan, decoding the OTP image on the first call after a cold
 * boot; the plan lives in retained secure RAM, so every later AP bring-up gets
 * it back without touching OTP. Returns NULL when the OTP could not be read. */
static const uint32_t *ap_mem_check_plan_get(uint32_t *num)
{
	if (s_ap_mem_check_plan_magic != AP_MEM_CHECK_PLAN_MAGIC) {
		if (ap_mem_check_plan_rebuild() != BK_OK) {
			return NULL;
		}
	}

	*num = s_ap_mem_check_plan_num;
	return s_ap_mem_check_plan;
}

/* Import the AP repair records from OTP into the MEM_CHECK registers. Returns 0,
 * or -1 when the OTP could not be read. */
__tz_c_veneer int psa_ap_mem_check_restore_from_otp(void)
{
	uint32_t num;
	const uint32_t *plan = ap_mem_check_plan_get(&num);

	if (plan == NULL) {
		return -1;
	}

	/* AON PMU R0 bit0 (memchk_bps): make sure the memory-check path is not
	 * bypassed before the AP starts. Reached through the NS alias, like
	 * bk_tfm_ap_power.c. */
	*(volatile uint32_t *)SOC_GET_NS_ADDR(SOC_AON_PMU_REG_BASE) &= ~1U;
	__DSB();

	for (uint32_t i = 0; i < num; i++) {
		uint32_t record = plan[i];
		uint32_t slot = record & AP_MEM_CHECK_PLAN_SLOT_MASK;
		uint32_t value = (record >> AP_MEM_CHECK_PLAN_DATA_SHIFT) &
			AP_MEM_CHECK_OTP_DATA_MASK;
		uint32_t mask = s_ap_mem_check_restore_mask[slot];
		volatile uint32_t *reg = (volatile uint32_t *)SOC_AP_MEM_CHECK_REG_BASE +
			AP_MEM_CHECK_REPAIR_REG_FIRST + slot;

		*reg = (*reg & ~mask) | (value & mask);
	}

	__DSB();

	return 0;
}

__tz_c_veneer int psa_ap_lv_reserve_restore(void)
{
	/* No arguments on purpose: the NS caller must not pick an address. */
	ap_lv_reserve_restore();

	return 0;
}

__tz_c_veneer int psa_ap_lv_reserve_save(void)
{
	s_ap_lv_reserve.magic = 0U;
	ap_lv_reserve_save();

	return 0;
}
