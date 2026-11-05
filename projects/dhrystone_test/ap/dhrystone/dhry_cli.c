/*
 * Dhrystone 2.1 (DMIPS) CLI command - BK7259 AP core
 *
 * The command is registered by the project itself (same pattern as
 * kvs_webrtc_example's kvs_webrtc_cli.c), so the benchmark, its sources and its
 * CLI entry move together and bk_cli knows nothing about Dhrystone.
 *
 * The wrapper does what the old bk_cli hook did:
 *   1. stop the watchdogs, block idle WFI and boost the CPU clock;
 *   2. dump the Flash clock and run the DWT 100 ms self-test;
 *   3. call dhrystone_run() (the benchmark proper, in dhry_glue.c);
 *   4. restore the clock, idle sleep and watchdogs.
 */
#include <stdint.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <modules/pm.h>
#include <os/os.h>

#include "bk_arch.h"
#include "bk_cli.h"
#include "cli.h"

#define TAG "dhrystone"

#ifndef DHRY_CPU_FREQ_MHZ
#define DHRY_CPU_FREQ_MHZ 480
#endif

#if (DHRY_CPU_FREQ_MHZ == 240)
#define DHRY_CPU_FREQ_ENUM PM_CPU_FRQ_240M
#elif (DHRY_CPU_FREQ_MHZ == 480)
#define DHRY_CPU_FREQ_ENUM PM_CPU_FRQ_480M
#else
#error "Unsupported DHRY_CPU_FREQ_MHZ"
#endif

#define DHRY_CPU_FREQ_HZ ((uint32_t)DHRY_CPU_FREQ_MHZ * 1000000UL)

/* Dump the Flash clock and verify with a DWT self-test that the core really runs
 * at DHRY_CPU_FREQ_MHZ by comparing DWT cycles against the 26 MHz Timer0. */
static void dhry_bench_dump_env(void)
{
	/* BK7259: cksel_flash is 2 bits, ckdiv_flash is 3 bits, f(flash) = src / (div + 1).
	 * src: 0 = XTAL 26M, 1 = DCO, 2 = 240M, 3 = 320M (cksel_sys_flash_t) */
	static const char *const src_name[] = { "XTAL-26M", "DCO", "240M", "320M" };
	static const uint32_t src_hz[] = { 26000000UL, 0UL, 240000000UL, 320000000UL };
	extern uint32_t sys_hal_flash_get_clk_sel(void);
	extern uint32_t sys_hal_flash_get_clk_div(void);
	extern uint64_t get_timer_value(void);
	uint32_t fsel, fdiv, div, src;

	fsel = sys_hal_flash_get_clk_sel();
	fdiv = sys_hal_flash_get_clk_div();
	src = fsel & 0x3;
	div = fdiv + 1;

	if (src_hz[src])
		BK_LOGI(TAG, "Flash clk: cksel=%u ckdiv=%u => %s /%u = %u MHz\r\n",
		        fsel, fdiv, src_name[src], div, src_hz[src] / div / 1000000UL);
	else
		BK_LOGI(TAG, "Flash clk: cksel=%u ckdiv=%u => %s /%u (freq n/a)\r\n",
		        fsel, fdiv, src_name[src], div);

	/* DWT self-test: measure a known 100ms delay with both DWT and Timer0.
	 * DWT is a per-core (PPB) block and this runs in an ordinary task, which the SMP
	 * scheduler may migrate to the other core mid-loop. A migrated sample then reads
	 * the other core's counter and produces a bogus delta. Detect both the core
	 * change and an implausible result, then retry. */
	{
		const uint32_t expect = (uint32_t)(DHRY_CPU_FREQ_HZ / 10u); /* 100ms of cycles */
		uint32_t attempt;

		for (attempt = 0; attempt < 3; attempt++) {
			uint32_t c0, c1, dwt0, dwt1, dwt_cy, tmr_tk;
			uint64_t tmr0, tmr1;

			c0 = rtos_get_core_id();

			DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
			DWT->CYCCNT = 0;
			DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
			__DSB(); __ISB();

			/* busy-wait 100ms using Timer0 as reference (26MHz -> 2600000 ticks) */
			dwt0  = DWT->CYCCNT;
			tmr0  = get_timer_value();
			while ((uint32_t)(get_timer_value() - tmr0) < 2600000u) { /* spin ~100ms */ }
			dwt1  = DWT->CYCCNT;
			tmr1  = get_timer_value();
			c1 = rtos_get_core_id();

			dwt_cy = dwt1 - dwt0;
			tmr_tk = (uint32_t)(tmr1 - tmr0);

			/* migrated, or the counter was clobbered by another user: sample again */
			if (c0 != c1 || dwt_cy < (expect / 2u) || dwt_cy > (expect * 2u))
				continue;

			BK_LOGI(TAG, "DWT self-test 100ms: DWT=%u cy (%.3fs@%uM)  Timer0=%u tk (%.3fs@26M)  ratio=%.2f\r\n",
			        dwt_cy, (double)dwt_cy/(double)DHRY_CPU_FREQ_HZ,
			        (unsigned)DHRY_CPU_FREQ_MHZ,
			        tmr_tk, (double)tmr_tk/26000000.0,
			        (double)dwt_cy / (double)DHRY_CPU_FREQ_HZ / ((double)tmr_tk / 26000000.0 + 1e-9));
			break;
		}

		if (attempt == 3)
			BK_LOGW(TAG, "DWT self-test: N/A (task migrated between cores, sample discarded)\r\n");
	}
}

static void dhrystone_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern void dhrystone_run(int argc, char *argv[]);
#if CONFIG_SUPPORT_WWDT
	extern bk_err_t bk_wwdt_stop(void);
	extern bk_err_t bk_wwdt_start(uint32_t timeout_ms, bool is_enable_window, uint32_t window_val);
#endif
#if (CONFIG_TASK_WDT)
	extern void bk_task_wdt_stop(void);
	extern void bk_task_wdt_start(void);
#endif
	extern bk_err_t sys_drv_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq);

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

#if CONFIG_SUPPORT_WWDT
	bk_wwdt_stop();
#endif
#if (CONFIG_TASK_WDT)
	bk_task_wdt_stop();
#endif

	/* Block idle WFI so the CPU clock is not gated during measurement */
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0, 0);
	sys_drv_switch_cpu_bus_freq(DHRY_CPU_FREQ_ENUM);
	BK_LOGI(TAG, "CPU boosted to %u MHz\r\n", (unsigned)DHRY_CPU_FREQ_MHZ);

	dhry_bench_dump_env();

	dhrystone_run(argc, argv);

	sys_drv_switch_cpu_bus_freq(PM_CPU_FRQ_480M);
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);
	BK_LOGI(TAG, "CPU freq restored\r\n");

#if CONFIG_SUPPORT_WWDT
	bk_wwdt_start(CONFIG_INT_WWDT_PERIOD_MS, false, 0);
#endif
#if (CONFIG_TASK_WDT)
	bk_task_wdt_start();
#endif
}

static const struct cli_command s_cmds[] = {
	{"dhrystone", "dhrystone [runs] (Dhrystone 2.1 benchmark, reports DMIPS)", dhrystone_cli_cmd},
};

int dhry_cli_init(void)
{
	return cli_register_commands(s_cmds, sizeof(s_cmds) / sizeof(s_cmds[0]));
}
