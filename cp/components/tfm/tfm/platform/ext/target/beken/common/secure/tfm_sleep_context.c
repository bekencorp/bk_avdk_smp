// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "boot_param.h"
#include "bk_tfm_ppc.h"
#include "cmsis.h"
#include "soc/soc.h"
#include "tfm_sleep_context.h"

#define TFM_SLEEP_CONTEXT_MAGIC          (0x43534454u)
#define TFM_SLEEP_CONTEXT_MAGIC_INV      (~TFM_SLEEP_CONTEXT_MAGIC)
#define TFM_SLEEP_CONTEXT_VERSION        (1u)
#define TFM_SLEEP_CONTEXT_POLICY_VERSION (2u)

#define TFM_SLEEP_PPRO_FIRST_REG         (0x04u)
#define TFM_SLEEP_PPRO_REG_COUNT         (12u)
#define TFM_SLEEP_DBUS_REG_COUNT         (4u)
#define TFM_SLEEP_MPC_COUNT              (4u)
#define TFM_SLEEP_MPC_MAX_LUT_WORDS      (256u)
#define TFM_SLEEP_MPC_MAX_RUNS           (16u)

#define TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP   (1u << 4)
#define TFM_SLEEP_MPC_CTRL_AUTO_INC      (1u << 8)
#define TFM_SLEEP_MPC_CTRL_SEC_LOCK      (1u << 31)

typedef struct {
	uint16_t start;
	uint16_t count;
	uint32_t value;
} tfm_sleep_mpc_run_t;

typedef struct {
	uint32_t ctrl;
	uint32_t block_index_max;
	uint32_t block_size;
	uint32_t run_count;
	tfm_sleep_mpc_run_t runs[TFM_SLEEP_MPC_MAX_RUNS];
} tfm_sleep_mpc_context_t;

typedef struct {
	uint32_t magic;
	uint32_t magic_inv;
	uint16_t version;
	uint16_t size;
	uint32_t policy_version;
	uint32_t ppro_reg2;
	uint32_t ppro[TFM_SLEEP_PPRO_REG_COUNT];
	uint32_t flash_dbus[TFM_SLEEP_DBUS_REG_COUNT];
	tfm_sleep_mpc_context_t mpc[TFM_SLEEP_MPC_COUNT];
	uint32_t crc32;
} tfm_sleep_context_t;

_Static_assert(sizeof(tfm_sleep_context_t) <= 0x400u,
	       "TF-M sleep context exceeds retention limit");

static volatile tfm_sleep_context_t s_tfm_sleep_context
	__attribute__((section(".tfm_sleep_retention"), aligned(32), used));

static const uintptr_t s_tfm_sleep_mpc_base[TFM_SLEEP_MPC_COUNT] = {
	SOC_MPC_FLASH_REG_BASE,
	SOC_MPC_SMEM0_REG_BASE,
	SOC_MPC_SMEM1_REG_BASE,
	SOC_MPC_SMEM2_REG_BASE,
};

extern void flush_all_dcache(void);

static uint32_t tfm_sleep_crc(const volatile tfm_sleep_context_t *context)
{
	const uint8_t *start = (const uint8_t *)&context->magic_inv;
	uint32_t length = (uint32_t)(offsetof(tfm_sleep_context_t, crc32) -
				    offsetof(tfm_sleep_context_t, magic_inv));

	return boot_param_crc32(start, length);
}

static void tfm_sleep_context_clean(void)
{
	flush_all_dcache();
	__DSB();
}

static void tfm_sleep_context_invalidate(void)
{
	s_tfm_sleep_context.magic = 0u;
	tfm_sleep_context_clean();
}

static int tfm_sleep_mpc_backup(uintptr_t base, tfm_sleep_mpc_context_t *saved)
{
	volatile uint32_t *regs = (volatile uint32_t *)base;
	uint32_t max_index = regs[4];
	uint32_t original_index = regs[6];
	uint32_t run_count = 0u;
	uint32_t previous = 0u;

	if (max_index >= TFM_SLEEP_MPC_MAX_LUT_WORDS) {
		return -1;
	}

	saved->ctrl = regs[0] &
		(TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP | TFM_SLEEP_MPC_CTRL_SEC_LOCK);
	saved->block_index_max = max_index;
	saved->block_size = regs[5];

	for (uint32_t index = 0u; index <= max_index; index++) {
		uint32_t value;

		regs[6] = index;
		__DSB();
		if (regs[6] != index) {
			regs[6] = original_index;
			return -1;
		}
		value = regs[7];

		if ((index == 0u) || (value != previous)) {
			if (run_count >= TFM_SLEEP_MPC_MAX_RUNS) {
				regs[6] = original_index;
				return -1;
			}
			saved->runs[run_count].start = (uint16_t)index;
			saved->runs[run_count].count = 1u;
			saved->runs[run_count].value = value;
			run_count++;
		} else {
			saved->runs[run_count - 1u].count++;
		}
		previous = value;
	}

	regs[6] = original_index;
	saved->run_count = run_count;
	return 0;
}

static bool tfm_sleep_mpc_context_valid(const tfm_sleep_mpc_context_t *saved)
{
	uint32_t next_index = 0u;

	if ((saved->block_index_max >= TFM_SLEEP_MPC_MAX_LUT_WORDS) ||
	    (saved->run_count == 0u) ||
	    (saved->run_count > TFM_SLEEP_MPC_MAX_RUNS)) {
		return false;
	}

	for (uint32_t run = 0u; run < saved->run_count; run++) {
		const tfm_sleep_mpc_run_t *item = &saved->runs[run];

		if ((item->count == 0u) || (item->start != next_index)) {
			return false;
		}
		next_index += item->count;
	}

	return next_index == (saved->block_index_max + 1u);
}

static int tfm_sleep_mpc_restore(uintptr_t base,
				 const tfm_sleep_mpc_context_t *saved)
{
	volatile uint32_t *regs = (volatile uint32_t *)base;
	uint32_t current_ctrl = regs[0];

	if (!tfm_sleep_mpc_context_valid(saved) ||
	    (regs[4] != saved->block_index_max) ||
	    (regs[5] != saved->block_size)) {
		return -1;
	}

	if ((current_ctrl & TFM_SLEEP_MPC_CTRL_SEC_LOCK) == 0u) {
		regs[0] = (current_ctrl &
			   ~(TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP |
			     TFM_SLEEP_MPC_CTRL_AUTO_INC |
			     TFM_SLEEP_MPC_CTRL_SEC_LOCK)) |
			  (saved->ctrl & TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP);
		__DSB();

		for (uint32_t run = 0u; run < saved->run_count; run++) {
			const tfm_sleep_mpc_run_t *item = &saved->runs[run];

			for (uint32_t offset = 0u; offset < item->count; offset++) {
				uint32_t index = (uint32_t)item->start + offset;

				regs[6] = index;
				regs[7] = item->value;
			}
		}
		__DSB();
	}

	for (uint32_t run = 0u; run < saved->run_count; run++) {
		const tfm_sleep_mpc_run_t *item = &saved->runs[run];

		for (uint32_t offset = 0u; offset < item->count; offset++) {
			uint32_t index = (uint32_t)item->start + offset;

			regs[6] = index;
			__DSB();
			if ((regs[6] != index) || (regs[7] != item->value)) {
				return -1;
			}
		}
	}

	if ((saved->ctrl & TFM_SLEEP_MPC_CTRL_SEC_LOCK) != 0u) {
		regs[0] |= TFM_SLEEP_MPC_CTRL_SEC_LOCK;
		__DSB();
	}
	return 0;
}

static bool tfm_sleep_context_valid(void)
{
	if ((s_tfm_sleep_context.magic != TFM_SLEEP_CONTEXT_MAGIC) ||
	    (s_tfm_sleep_context.magic_inv != TFM_SLEEP_CONTEXT_MAGIC_INV) ||
	    (s_tfm_sleep_context.version != TFM_SLEEP_CONTEXT_VERSION) ||
	    (s_tfm_sleep_context.size != sizeof(tfm_sleep_context_t)) ||
	    (s_tfm_sleep_context.policy_version !=
	     TFM_SLEEP_CONTEXT_POLICY_VERSION)) {
		return false;
	}

	for (uint32_t dev = 0u; dev < TFM_SLEEP_MPC_COUNT; dev++) {
		if (!tfm_sleep_mpc_context_valid(
			    (const tfm_sleep_mpc_context_t *)&s_tfm_sleep_context.mpc[dev])) {
			return false;
		}
	}

	return s_tfm_sleep_context.crc32 == tfm_sleep_crc(&s_tfm_sleep_context);
}

bool tfm_sleep_context_is_valid(void)
{
	return tfm_sleep_context_valid();
}

static int tfm_sleep_context_fill_header(tfm_sleep_context_t *context)
{
	uint32_t *words = (uint32_t *)context;

	tfm_sleep_context_invalidate();
	for (uint32_t index = 0u;
	     index < (sizeof(tfm_sleep_context_t) / sizeof(uint32_t));
	     index++) {
		words[index] = 0u;
	}

	context->magic_inv = TFM_SLEEP_CONTEXT_MAGIC_INV;
	context->version = TFM_SLEEP_CONTEXT_VERSION;
	context->size = sizeof(tfm_sleep_context_t);
	context->policy_version = TFM_SLEEP_CONTEXT_POLICY_VERSION;
	return 0;
}

static int tfm_sleep_context_commit(tfm_sleep_context_t *context)
{
	context->crc32 = tfm_sleep_crc(context);
	tfm_sleep_context_clean();
	context->magic = TFM_SLEEP_CONTEXT_MAGIC;
	tfm_sleep_context_clean();
	return 0;
}

int tfm_sleep_context_build_snapshot(void)
{
	tfm_sleep_context_t *context = (tfm_sleep_context_t *)&s_tfm_sleep_context;

	tfm_sleep_context_fill_header(context);

	context->ppro_reg2 = REG_READ(SOC_PPRO_REG_BASE + (0x02u << 2));

	for (uint32_t index = 0u; index < TFM_SLEEP_DBUS_REG_COUNT; index++) {
		context->flash_dbus[index] =
			REG_READ(SOC_FLASH_REG_BASE + ((0x0du + index) << 2));
	}

	for (uint32_t dev = 0u; dev < TFM_SLEEP_MPC_COUNT; dev++) {
		if (tfm_sleep_mpc_backup(s_tfm_sleep_mpc_base[dev],
					 &context->mpc[dev]) != 0) {
			tfm_sleep_context_invalidate();
			return -1;
		}
	}

	return tfm_sleep_context_commit(context);
}

int tfm_sleep_context_refresh_ppro(void)
{
	tfm_sleep_context_t *context = (tfm_sleep_context_t *)&s_tfm_sleep_context;

	if (context->magic != TFM_SLEEP_CONTEXT_MAGIC) {
		return -1;
	}

	bk_ppc_copy_cp_config_to_snapshot(context->ppro);
	context->crc32 = tfm_sleep_crc(context);
	tfm_sleep_context_clean();
	return 0;
}

int tfm_sleep_context_restore(void)
{
	if (!tfm_sleep_context_valid()) {
		tfm_sleep_context_invalidate();
		return -1;
	}

	const tfm_sleep_context_t *saved =
		(const tfm_sleep_context_t *)&s_tfm_sleep_context;

	tfm_sleep_context_invalidate();

	for (uint32_t dev = 0u; dev < TFM_SLEEP_MPC_COUNT; dev++) {
		if (tfm_sleep_mpc_restore(s_tfm_sleep_mpc_base[dev],
					  &saved->mpc[dev]) != 0) {
			return -1;
		}
	}

	REG_WRITE(SOC_PPRO_REG_BASE + (0x02u << 2), saved->ppro_reg2);

	if (bk_ppc_init() != 0) {
		return -1;
	}

	for (uint32_t index = 0u; index < TFM_SLEEP_PPRO_REG_COUNT; index++) {
		REG_WRITE(SOC_PPRO_REG_BASE +
			  ((TFM_SLEEP_PPRO_FIRST_REG + index) << 2),
			  saved->ppro[index]);
	}

	bk_ppc_set_ap_master_nsec();

	for (uint32_t index = 0u; index < TFM_SLEEP_DBUS_REG_COUNT; index++) {
		REG_WRITE(SOC_FLASH_REG_BASE + ((0x0du + index) << 2),
			  saved->flash_dbus[index]);
	}
	__DSB();
	__ISB();

	for (uint32_t index = 0u; index < TFM_SLEEP_PPRO_REG_COUNT; index++) {
		if (REG_READ(SOC_PPRO_REG_BASE +
			     ((TFM_SLEEP_PPRO_FIRST_REG + index) << 2)) !=
		    saved->ppro[index]) {
			return -1;
		}
	}
	for (uint32_t index = 0u; index < TFM_SLEEP_DBUS_REG_COUNT; index++) {
		if (REG_READ(SOC_FLASH_REG_BASE + ((0x0du + index) << 2)) !=
		    saved->flash_dbus[index]) {
			return -1;
		}
	}

	return 0;
}
