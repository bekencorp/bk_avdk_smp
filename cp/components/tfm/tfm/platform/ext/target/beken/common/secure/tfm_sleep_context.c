// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "boot_param.h"
#include "bk_tfm_ppc.h"
#include "cmsis.h"
#include "region_defs.h"
#include "soc/soc.h"
#include "tfm_sleep_context.h"

#define TFM_SLEEP_CONTEXT_MAGIC          (0x43534454u)
#define TFM_SLEEP_CONTEXT_MAGIC_INV      (~TFM_SLEEP_CONTEXT_MAGIC)
#define TFM_SLEEP_CONTEXT_VERSION        (2u)
#define TFM_SLEEP_CONTEXT_POLICY_VERSION (2u)

/* Place a function in the SRAM-resident fastboot resume section. These run at
 * the reset-default low clock on the wakeup path, so build them optimized
 * instead of the project-wide -O0 to cut the instruction count. */
#define TFM_RAMFUNC \
	__attribute__((section(".tfm_fastboot_code"), noinline, optimize("O2")))

#define TFM_SLEEP_PPRO_FIRST_REG         (0x04u)
#define TFM_SLEEP_PPRO_REG_COUNT         (12u)
#define TFM_SLEEP_DBUS_REG_COUNT         (4u)
#define TFM_SLEEP_MPC_COUNT              (5u)
#define TFM_SLEEP_MPC_MAX_LUT_WORDS      (256u)
#define TFM_SLEEP_MPC_MAX_RUNS           (16u)

#define TFM_SLEEP_NVIC_ISER_WORDS        (2u)
#define TFM_SLEEP_NVIC_IPR_WORDS         (16u)
#define TFM_SLEEP_SHPR_COUNT             (3u)

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
	uint32_t nvic_iser[TFM_SLEEP_NVIC_ISER_WORDS];
	uint32_t nvic_itns[TFM_SLEEP_NVIC_ISER_WORDS];
	uint32_t nvic_ipr[TFM_SLEEP_NVIC_IPR_WORDS];
	uint32_t shpr[TFM_SLEEP_SHPR_COUNT];
	uint32_t systick_load;
	uint32_t systick_val;
	uint32_t systick_ctrl;
	tfm_sleep_mpc_context_t mpc[TFM_SLEEP_MPC_COUNT];
	uint32_t sum_check;
	uint32_t xor_check;
} tfm_sleep_context_t;

_Static_assert(sizeof(tfm_sleep_context_t) <= TFM_SLEEP_CONTEXT_MAX_SIZE,
	       "TF-M sleep context exceeds retention limit");

static volatile tfm_sleep_context_t s_tfm_sleep_context
	__attribute__((section(".tfm_sleep_retention"), aligned(32), used));

/* Kept in the SRAM fastboot rodata section so the resume path never reads
 * flash: the QSPI stays in its reset low-speed mode after a fastboot wakeup. */
static const uintptr_t s_tfm_sleep_mpc_base[TFM_SLEEP_MPC_COUNT]
	__attribute__((section(".tfm_fastboot_rodata"), used)) = {
	SOC_MPC_FLASH_REG_BASE,
	SOC_MPC_SMEM0_REG_BASE,
	SOC_MPC_SMEM1_REG_BASE,
	SOC_MPC_SMEM2_REG_BASE,
	SOC_MPC_OTP2_REG_BASE,
};

extern void flush_all_dcache(void);

/* Fast integrity check over the snapshot: 32-bit sum + xor. Four words per
 * iteration; both operations are associative modulo 2^32, so the unrolled
 * form gives the same result as a word-by-word loop. */
static TFM_RAMFUNC void tfm_sleep_checksum(const uint32_t *words, uint32_t count,
					   uint32_t *sum, uint32_t *xr)
{
	uint32_t s = 0u;
	uint32_t x = 0u;

	while (count >= 4u) {
		uint32_t a = *words++;
		uint32_t b = *words++;
		uint32_t c = *words++;
		uint32_t d = *words++;

		s += a + b + c + d;
		x ^= a ^ b ^ c ^ d;
		count -= 4u;
	}
	while (count-- > 0u) {
		uint32_t w = *words++;

		s += w;
		x ^= w;
	}

	*sum = s;
	*xr = x;
}

static TFM_RAMFUNC uint32_t tfm_sleep_checksum_words(void)
{
	return (uint32_t)((offsetof(tfm_sleep_context_t, sum_check) -
			   offsetof(tfm_sleep_context_t, magic_inv)) /
			  sizeof(uint32_t));
}

/* Checksum the snapshot words from magic_inv up to sum_check. The pointer is
 * derived from the context base so the whole range stays inside the object. */
static TFM_RAMFUNC void tfm_sleep_checksum_context(
		const tfm_sleep_context_t *context, uint32_t *sum, uint32_t *xr)
{
	const uint32_t *words = (const uint32_t *)context +
				(offsetof(tfm_sleep_context_t, magic_inv) /
				 sizeof(uint32_t));

	tfm_sleep_checksum(words, tfm_sleep_checksum_words(), sum, xr);
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

static TFM_RAMFUNC bool
tfm_sleep_mpc_context_valid(const tfm_sleep_mpc_context_t *saved)
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

static TFM_RAMFUNC int tfm_sleep_mpc_restore(uintptr_t base,
					     const tfm_sleep_mpc_context_t *saved)
{
	volatile uint32_t *regs = (volatile uint32_t *)base;
	uint32_t current_ctrl = regs[0];

	/* The snapshot metadata was already validated by tfm_sleep_context_valid();
	 * only the live hardware geometry needs re-checking here. */
	if ((regs[4] != saved->block_index_max) ||
	    (regs[5] != saved->block_size)) {
		return -1;
	}

	if ((current_ctrl & TFM_SLEEP_MPC_CTRL_SEC_LOCK) == 0u) {
		/* Enable auto-increment: the LUT index advances on every data write. */
		regs[0] = (current_ctrl &
			   ~(TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP |
			     TFM_SLEEP_MPC_CTRL_AUTO_INC |
			     TFM_SLEEP_MPC_CTRL_SEC_LOCK)) |
			  (saved->ctrl & TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP) |
			  TFM_SLEEP_MPC_CTRL_AUTO_INC;
		__DSB();

		for (uint32_t run = 0u; run < saved->run_count; run++) {
			const tfm_sleep_mpc_run_t *item = &saved->runs[run];

			regs[6] = (uint32_t)item->start;
			for (uint32_t offset = 0u; offset < item->count; offset++) {
				regs[7] = item->value;
			}
		}

		regs[0] = (current_ctrl &
			   ~(TFM_SLEEP_MPC_CTRL_AUTO_INC |
			     TFM_SLEEP_MPC_CTRL_SEC_LOCK)) |
			  (saved->ctrl & TFM_SLEEP_MPC_CTRL_CFG_SEC_RSP);
		__DSB();
	}

	/* Write-only restore: the snapshot content was already covered by the
	 * checksum in tfm_sleep_context_valid(), and reading every LUT word back
	 * would double the APB traffic on the wakeup path. */
	__DSB();

	if ((saved->ctrl & TFM_SLEEP_MPC_CTRL_SEC_LOCK) != 0u) {
		regs[0] |= TFM_SLEEP_MPC_CTRL_SEC_LOCK;
		__DSB();
	}
	return 0;
}

static TFM_RAMFUNC bool tfm_sleep_context_valid(void)
{
	uint32_t sum;
	uint32_t xr;

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

	tfm_sleep_checksum_context(&s_tfm_sleep_context, &sum, &xr);

	return (s_tfm_sleep_context.sum_check == sum) &&
	       (s_tfm_sleep_context.xor_check == xr);
}

bool tfm_sleep_context_is_valid(void)
{
	return tfm_sleep_context_valid();
}

/* Save NVIC/SysTick runtime state. Runs on the cold-boot path only. */
static void tfm_sleep_backup_core_state(tfm_sleep_context_t *context)
{
	const volatile uint32_t *ipr = (const volatile uint32_t *)&NVIC->IPR[0];
	const volatile uint32_t *shpr = (const volatile uint32_t *)&SCB->SHPR[0];

	for (uint32_t i = 0u; i < TFM_SLEEP_NVIC_ISER_WORDS; i++) {
		context->nvic_iser[i] = NVIC->ISER[i];
		context->nvic_itns[i] = NVIC->ITNS[i];
	}
	for (uint32_t i = 0u; i < TFM_SLEEP_NVIC_IPR_WORDS; i++) {
		context->nvic_ipr[i] = ipr[i];
	}
	for (uint32_t i = 0u; i < TFM_SLEEP_SHPR_COUNT; i++) {
		context->shpr[i] = shpr[i];
	}

	context->systick_load = SysTick->LOAD;
	context->systick_val = SysTick->VAL;
	context->systick_ctrl = SysTick->CTRL;
}

/* Restore NVIC/SysTick runtime state. Runs from SRAM on the wakeup path. */
static TFM_RAMFUNC void tfm_sleep_restore_core_state(
	const tfm_sleep_context_t *context)
{
	volatile uint32_t *ipr = (volatile uint32_t *)&NVIC->IPR[0];
	volatile uint32_t *shpr = (volatile uint32_t *)&SCB->SHPR[0];

	for (uint32_t i = 0u; i < TFM_SLEEP_NVIC_ISER_WORDS; i++) {
		NVIC->ICER[i] = 0xFFFFFFFFu;
		__DSB();
		/* ITNS is rebuilt by nvic_interrupt_target_state_cfg(). */
		NVIC->ISER[i] = context->nvic_iser[i];
	}
	for (uint32_t i = 0u; i < TFM_SLEEP_NVIC_IPR_WORDS; i++) {
		ipr[i] = context->nvic_ipr[i];
	}
	for (uint32_t i = 0u; i < TFM_SLEEP_SHPR_COUNT; i++) {
		shpr[i] = context->shpr[i];
	}

	SysTick->CTRL = 0u;
	SysTick->LOAD = context->systick_load;
	SysTick->VAL = 0u;
	SysTick->CTRL = context->systick_ctrl;
	__DSB();
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
	uint32_t sum;
	uint32_t xr;

	tfm_sleep_checksum_context(context, &sum, &xr);
	context->sum_check = sum;
	context->xor_check = xr;
	tfm_sleep_context_clean();
	context->magic = TFM_SLEEP_CONTEXT_MAGIC;
	tfm_sleep_context_clean();
	return 0;
}

int tfm_sleep_context_build_snapshot(void)
{
	tfm_sleep_context_t *context = (tfm_sleep_context_t *)&s_tfm_sleep_context;

	tfm_sleep_context_fill_header(context);
	tfm_sleep_backup_core_state(context);

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
	{
		uint32_t sum;
		uint32_t xr;

		tfm_sleep_checksum_context(context, &sum, &xr);
		context->sum_check = sum;
		context->xor_check = xr;
	}
	tfm_sleep_context_clean();
	return 0;
}

TFM_RAMFUNC int tfm_sleep_context_restore(void)
{
	if (!tfm_sleep_context_valid()) {
		tfm_sleep_context_invalidate();
		return -1;
	}

	const tfm_sleep_context_t *saved =
		(const tfm_sleep_context_t *)&s_tfm_sleep_context;

	for (uint32_t dev = 0u; dev < TFM_SLEEP_MPC_COUNT; dev++) {
		if (tfm_sleep_mpc_restore(s_tfm_sleep_mpc_base[dev],
					  &saved->mpc[dev]) != 0) {
			tfm_sleep_context_invalidate();
			return -1;
		}
	}

	/* Flash DBUS before any PPRO word that marks Flash NS. */
	for (uint32_t index = 0u; index < TFM_SLEEP_DBUS_REG_COUNT; index++) {
		REG_WRITE(SOC_FLASH_REG_BASE + ((0x0du + index) << 2),
			  saved->flash_dbus[index]);
	}
	__DSB();
	__ISB();

	for (uint32_t index = 0u; index < TFM_SLEEP_DBUS_REG_COUNT; index++) {
		if (REG_READ(SOC_FLASH_REG_BASE + ((0x0du + index) << 2)) !=
		    saved->flash_dbus[index]) {
			tfm_sleep_context_invalidate();
			return -1;
		}
	}

	tfm_sleep_restore_core_state(saved);

	return 0;
}

TFM_RAMFUNC int tfm_sleep_context_apply_ppc(void)
{
	const tfm_sleep_context_t *saved =
		(const tfm_sleep_context_t *)&s_tfm_sleep_context;

	_Static_assert(TFM_SLEEP_PPRO_REG_COUNT == BK_PPC_CONFIG_WORD_COUNT,
		       "sleep PPRO snapshot size must match CP PPC config");

	if (bk_ppc_init() != 0) {
		return -1;
	}

	/* Same as ns_init_hook: full retained CP PPRO, then AP master nsec. */
	if (bk_ppc_apply_config(saved->ppro) != 0) {
		return -1;
	}

	bk_ppc_set_ap_master_nsec();
	__DSB();
	__ISB();
	return 0;
}
