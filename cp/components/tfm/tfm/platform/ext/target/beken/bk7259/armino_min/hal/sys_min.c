// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal sys-control shims.
//
// Provides the lightweight sys primitives the secure world needs without
// compiling the full SDK sys_driver.c. These reuse the pure inline sys_ll.h:
//
//   - sys_drv_enable_int / disable_int / set_base_addr: critical-section
//     primitives used by common/secure/bk_tfm_ppc.c when re-attributing
//     Flash/SYS to secure (migrated from the old sys_drv_int_shim.c).
//   - sys_hal_flash_set_clk / sys_hal_flash_set_clk_div: single-register flash
//     clock select/divider writes used by the BL2/S startup.
//
// NOTE: the heavy chip clock/voltage bring-up (sys_drv_early_init,
// sys_hal_switch_cpu_bus_freq, sys_drv_init, sys_drv_get_chip_id) is NOT
// reimplemented here; see the design discussion (it depends on the full
// analog/PLL/DVFS subsystem). Those remain provided by the SDK sys files until
// the bring-up sequence is fully self-contained.

#include <stdint.h>
#include <stdbool.h>
#include "cmsis.h"
#include "sys_driver.h"
#include "sys_ll.h"

void sys_drv_disable_int(sys_lock_ctx_t *ctx)
{
	if (ctx) {
		ctx->int0 = sys_ll_get_cpu0_int_0_31_en_value();
		ctx->int1 = sys_ll_get_cpu0_int_32_63_en_value();
	}
	sys_ll_set_cpu0_int_0_31_en_value(0);
	sys_ll_set_cpu0_int_32_63_en_value(0);
}

void sys_drv_enable_int(sys_lock_ctx_t *ctx)
{
	if (ctx) {
		sys_ll_set_cpu0_int_0_31_en_value(ctx->int0);
		sys_ll_set_cpu0_int_32_63_en_value(ctx->int1);
	}
}

void sys_drv_set_base_addr(uint32_t addr)
{
	/* BK7259 sys_ll uses the fixed SOC_SYS_REG_BASE; nothing to do. */
	(void)addr;
}

/* ---- Self-contained shims for symbols the kept SDK sys_hal.c drags in ----
 * The SDK sys_hal.c is reused for the (silicon-validated) clock/voltage
 * bring-up, but it references RTOS critical sections (aspl), a busy-wait delay,
 * and runtime PM/PHY-calibration helpers. None of those subsystems exist in the
 * secure world; the secure-boot path only uses the clock-setup code. Provide a
 * minimal interrupt-based critical section, a cycle busy-wait delay, and weak
 * no-op PM/PHY stubs (overridable by a real driver later). */

uint32_t bk_aspl_driver_enter_critical(void)
{
	uint32_t primask = __get_PRIMASK();
	__disable_irq();
	return primask;
}

void bk_aspl_driver_exit_critical(uint32_t flags)
{
	__set_PRIMASK(flags);
}

/* Coarse busy-wait. BL2 runs the CPU/bus at 120 MHz (see startup); the loop
 * body is a few cycles, so scale conservatively. Used by flash/otp/qspi LL
 * settling delays where exact timing is not critical, only a lower bound. */
void bk_delay_us(uint32_t us)
{
	volatile uint32_t loops = us * 40u;
	while (loops--) {
		__asm volatile("nop");
	}
}

__attribute__((weak)) bool bk_pm_phy_cali_state_get(void)
{
	return false;
}

__attribute__((weak)) int bk_pm_phy_cali_state_set(bool cali_state)
{
	(void)cali_state;
	return 0;
}

__attribute__((weak)) int bk_pm_phy_reinit_flag_set(bool reinit_flag)
{
	(void)reinit_flag;
	return 0;
}

__attribute__((weak)) uint32_t bk_pm_vote_power_module_get(void)
{
	return 0;
}

__attribute__((weak)) void phy_wakeup_reinit(void)
{
}
