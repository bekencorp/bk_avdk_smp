// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

/* CP SRAM bad-point capture, secure-world half. The mechanism and the entry
 * layout live in deep_lv_reserve_layout.h.
 *
 * In the secure project the CP app is Non-Secure and SRAM0's secure prefix is
 * TF-M's own RAM, so both halves run here: the capture from the NS deep-LV path
 * (NSC below), the write-back from the SRAM fastboot resume path
 * (tfm_fastboot_resume() calls cp_lv_reserve_restore() / cp_lv_reserve_discard()).
 *
 * The table lives in a dedicated 64-byte slot reserved by the link script
 * (.cp_lv_reserve in bk7259_s.ld), so the capture no longer borrows the deepest
 * word of the TF-M MSP stack (&__STACK_LIMIT).
 *
 * Unlike .bss this memory is not zeroed on boot, so a capture can outlive a
 * reset. The magic is therefore cleared explicitly: every save clears it before
 * it starts, and the write-back is only reached from the fastboot resume, after
 * the AON dlv_startup marker says the last boot really came back from deep
 * sleep. A capture left behind by a reset is never consumed: the next deep sleep
 * rewrites it (clearing the magic first) before any wake can look at it, and the
 * resume path drops it when the marker is absent.
 *
 * The warm path never runs Pre_Main(), so the capture survives; the write-back
 * therefore uses only locals, section symbols and immediates. It is kept in the
 * SRAM-resident .tfm_fastboot_code section (CP_LV_RAMFUNC): after a deep-sleep
 * wakeup the secure world runs from flash at its slowest, and the write-back is
 * part of that wake path. */

#include <stdint.h>

#include "cmsis.h"
#include "security_defs.h"
#include "deep_lv_reserve_layout.h"

#define CP_LV_RESERVE_MAGIC     0x52564C43u /* "CLVR" */
#define CP_LV_RESERVE_SIZE      0x40u

typedef struct {
	uint32_t values[CP_MEM_CHECK_POINT_NUM];
	uint32_t magic;
} cp_lv_reserve_t;

_Static_assert(sizeof(cp_lv_reserve_t) <= CP_LV_RESERVE_SIZE,
               "cp_lv_reserve table overflows its link slot");

/* The 64-byte slot reserved by bk7259_s.ld; see the file header. */
extern uint8_t __cp_lv_reserve_start__[];
extern uint8_t __cp_lv_reserve_end__[];

#define CP_LV_RESERVE()  ((volatile cp_lv_reserve_t *)__cp_lv_reserve_start__)

extern void flush_all_dcache(void);

/* The write-back runs on the deep-sleep wake path, where flash XIP is at its
 * slowest: keep it in the SRAM-resident section next to tfm_fastboot_resume().
 * The same attribute is used by startup/sleep_fastboot_tfm.c (TFM_RAMFUNC). */
#define CP_LV_RAMFUNC __attribute__((section(".tfm_fastboot_code"), noinline))

static int cp_lv_reserve_save(void)
{
	volatile cp_lv_reserve_t *tbl = CP_LV_RESERVE();
	uintptr_t lo = (uintptr_t)__cp_lv_reserve_start__;
	uintptr_t hi = (uintptr_t)__cp_lv_reserve_end__;

	for (uint32_t i = 0; i < CP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t addr = cp_mem_check_bad_point_addr_get(i);

		/* A repaired cell inside the slot itself: treat the board as bad and
		 * do not sleep on this capture, it would be written back as raw garbage
		 * on wake. Very unlikely, and the slot is the small price of not
		 * borrowing the stack. */
		if ((addr >= lo) && (addr < hi)) {
			return -1; /* only reached when asserts are compiled out */
		}

		tbl->values[i] = (addr != 0u) ? *(volatile uint32_t *)addr : 0u;
	}

	__DSB();

	/* Magic last: a capture that did not complete must never be consumed. */
	tbl->magic = CP_LV_RESERVE_MAGIC;
	__DSB();
	return 0;
}

CP_LV_RAMFUNC void cp_lv_reserve_restore(void)
{
	volatile cp_lv_reserve_t *tbl = CP_LV_RESERVE();

	if (tbl->magic != CP_LV_RESERVE_MAGIC) {
		return;
	}

	tbl->magic = 0u;

	for (uint32_t i = 0; i < CP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t addr = cp_mem_check_bad_point_addr_get(i);

		if (addr != 0u) {
			*(volatile uint32_t *)addr = tbl->values[i];
		}
	}

	__DSB();
}

CP_LV_RAMFUNC void cp_lv_reserve_discard(void)
{
	CP_LV_RESERVE()->magic = 0u;
	__DSB();
}

__tz_c_veneer int psa_cp_lv_reserve_save(void)
{
	/* No arguments and no SDK logging on purpose: the NS caller must not pick an
	 * address or a value, and the log path is unsafe at the S<->NS boundary
	 * (see reg_nsc.c). A bad point inside the table aborts the capture here. */
	CP_LV_RESERVE()->magic = 0u;

	return cp_lv_reserve_save();
}
