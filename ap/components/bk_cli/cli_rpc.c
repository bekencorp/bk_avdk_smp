/**
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 BEKEN Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of BEKEN Corporation.
 *
 */
#include <stdlib.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <modules/pm.h>
#include <common/bk_kernel_err.h>

#include "bk_cli.h"
#include "stdarg.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_phy.h"
#include "cli.h"
#include "cli_config.h"
#include <components/log.h>
#include <driver/uart.h>
#include "bk_rtos_debug.h"
#if CONFIG_SHELL_ASYNCLOG
#include "components/shell_task.h"
#endif
#include "bk_api_cli.h"


#define TAG    "debug"

static void debug_help_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
#include "mb_ipc_cmd.h"

#include "amp_lock_api.h"

#include "spinlock.h"

static void debug_ipc_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_rpc_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_rpc_gpio_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_cpulock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_spinlock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#if CONFIG_SLAVE_HEART_BEAT
static void debug_hb_test_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

static u8     ipc_inited = 0;

spinlock_t SPINLOCK_SECTION gpio_spinlock;
spinlock_t  *	gpio_spinlock_ptr;

#endif

#if CONFIG_ARCH_RISCV
static void debug_perfmon_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_show_boot_time(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#define CORE_MARK_ENABLED

#ifdef CORE_MARK_ENABLED
static void debug_core_mark(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

const struct cli_command debug_cmds[] = {
	{"help", "list debug cmds", debug_help_command},
#if (CONFIG_CPU_CNT > 1) && CONFIG_MAILBOX
	{"ipc", "ipc [spinlock addr]", debug_ipc_command},
	{"cpu_lock", "cpu_lock [timeout 1~20]", debug_cpulock_command},
	{"spin_lock", "spin_lock [timeout 1~20]", debug_spinlock_command},
#if CONFIG_SLAVE_HEART_BEAT
	{"hb_test", "hb_test <stop|start>  -- pause/resume AP heartbeat to CP", debug_hb_test_command},
#endif
#endif

#if CONFIG_ARCH_RISCV
	{"perfmon", "perfmon(calc MIPS)", debug_perfmon_command},
	{"boottime", "boottime(show boot mtime info)", debug_show_boot_time},
#endif

#ifdef CORE_MARK_ENABLED
	{"core_mark", "core_mark [seed1 seed2 seed3 iteration Algorithms]", debug_core_mark},
#endif /* CORE_MARK_ENABLED */ 
};

const int cli_debug_table_size = ARRAY_SIZE(debug_cmds);

void print_cmd_table(const struct cli_command *cmd_table, int table_items)
{
	int i;

	for (i = 0; i < table_items; i++)
	{
		if (cmd_table[i].name)
		{
			if (cmd_table[i].help)
				BK_LOGD(NULL, "%s: %s\r\n", cmd_table[i].name, cmd_table[i].help);
			else
				BK_LOGD(NULL, "%s\r\n", cmd_table[i].name);
		}
	}
}

void print_cmd_help(const struct cli_command *cmd_table, int table_items, void *func)
{
	int i;

	for (i = 0; i < table_items; i++)
	{
		if(cmd_table[i].function == func)
		{
			if (cmd_table[i].help)
				BK_LOGD(NULL, "%s\r\n", cmd_table[i].help);

			break;
		}
	}
}

static void debug_help_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	BK_LOGD(NULL, "====Debug Commands====\r\n");

	print_cmd_table(debug_cmds, ARRAY_SIZE(debug_cmds));
}

#if (CONFIG_CPU_CNT > 1)
static void print_debug_cmd_help(void *func)
{
	print_cmd_help(debug_cmds, ARRAY_SIZE(debug_cmds), func);
}

#if CONFIG_MAILBOX
static void debug_ipc_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret_val;

	if(ipc_inited)
	{
		BK_LOGD(TAG,"ipc started\r\n");
		return;
	}


	if (argc < 2)
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "usage: ipc spinlock_addr\r\n");

		return;
	}


	ret_val = ipc_init();
	BK_LOGD(TAG,"ipc init: %d\r\n", ret_val);

	ipc_inited = 1;



	ret_val = ipc_send_power_up();
	BK_LOGD(TAG,"ipc client power: %d\r\n", ret_val);

	ret_val = ipc_send_heart_beat(0x34);
	BK_LOGD(TAG,"ipc client heartbeat: %d\r\n", ret_val);

	ret_val = ipc_send_test_cmd(0x12);
	BK_LOGD(TAG,"ipc client test: 0x%x\r\n", ret_val);

	gpio_spinlock_ptr = (spinlock_t *)strtoul(argv[1], NULL, 0);
	snprintf(pcWriteBuffer, xWriteBufferLen, "spinlock_addr: 0x%x\r\n", gpio_spinlock_ptr);

}

#if CONFIG_SLAVE_HEART_BEAT
static void debug_hb_test_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern void mb_ipc_heartbeat_pause(u8 pause);

	if (argc < 2)
	{
		snprintf(pcWriteBuffer, xWriteBufferLen,
			"usage: hb_test <stop|start>\r\n"
			"  stop  -- pause heartbeat to simulate AP crash (CP will reboot after ~6s)\r\n"
			"  start -- resume heartbeat sending\r\n");
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0)
	{
		mb_ipc_heartbeat_pause(1);
		snprintf(pcWriteBuffer, xWriteBufferLen,
			"[hb_test] heartbeat STOPPED. CP should detect timeout in ~6s and reboot.\r\n");
	}
	else if (os_strcmp(argv[1], "start") == 0)
	{
		mb_ipc_heartbeat_pause(0);
		snprintf(pcWriteBuffer, xWriteBufferLen,
			"[hb_test] heartbeat RESUMED.\r\n");
	}
	else
	{
		snprintf(pcWriteBuffer, xWriteBufferLen, "unknown arg: %s\r\n", argv[1]);
	}
}
#endif

static void debug_spinlock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u32 timeout_second = 10;  // 10s

	if(ipc_inited == 0)
	{
		BK_LOGD(TAG,"Failed: no rpc client/server in CPU0/CPU1.\r\n");
		return;
	}

	if (argc > 1)
	{
		timeout_second = strtoul(argv[1], NULL, 0);

		if(timeout_second > 20)
			timeout_second = 20;
		if(timeout_second == 0)
			timeout_second = 10;
	}

	int i;

	for(i = 0; i < 10; i++)
	{
		BK_LOGD(TAG,"times: %d\r\n", i);

		uint32_t flag = spinlock_acquire(gpio_spinlock_ptr, BEKEN_WAIT_FOREVER);
		BK_LOGD(TAG,"client spinlock acquired %d\r\n", gpio_spinlock_ptr->owner);
		rtos_delay_milliseconds(timeout_second * 1000);
		spinlock_release(gpio_spinlock_ptr, flag);
		BK_LOGD(TAG,"client spinlock released %d\r\n", gpio_spinlock_ptr->owner);

	}
}

static void debug_cpulock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u32 timeout_second = 10;  // 10s

	if(ipc_inited == 0)
	{
		BK_LOGD(TAG,"Failed: no ipc client/server in CPU0/CPU1.\r\n");
		return;
	}

	if (argc > 1)
	{
		timeout_second = strtoul(argv[1], NULL, 0);

		if(timeout_second > 20)
			timeout_second = 20;
		if(timeout_second == 0)
			timeout_second = 10;
	}
	else
	{
		print_debug_cmd_help(debug_cpulock_command);
		BK_LOGD(TAG,"default timeout 10s is used.\r\n");
	}

	int	ret_val = BK_FAIL;

	ret_val = amp_res_init(AMP_RES_ID_GPIO);
	BK_LOGD(TAG,"amp res init:ret=%d\r\n", ret_val);

	ret_val = amp_res_acquire(AMP_RES_ID_GPIO, timeout_second * 1000);
	BK_LOGD(TAG,"amp res acquire:ret=%d\r\n", ret_val);

	rtos_delay_milliseconds(timeout_second * 1000);

	if(ret_val == 0)
	{
		ret_val = amp_res_release(AMP_RES_ID_GPIO);
		BK_LOGD(TAG,"amp res release:ret=%d\r\n", ret_val);
	}
	else
	{
		BK_LOGD(TAG,"amp res release: no release\r\n");
	}

}

#endif
#endif

const struct cli_command * cli_debug_cmd_table(int *num)
{
	*num = ARRAY_SIZE(debug_cmds);

	return &debug_cmds[0];
}

#if CONFIG_ARCH_RISCV

extern u64 riscv_get_instruct_cnt(void);
extern u64 riscv_get_mtimer(void);

static u64 		saved_time = 0;
static u64 		saved_inst_cnt = 0;

static void debug_perfmon_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u64 cur_time = riscv_get_mtimer();
	u64 cur_inst_cnt = riscv_get_instruct_cnt();

	BK_LOGD(TAG,"cur time: %x:%08x\r\n", (u32)(cur_time >> 32), (u32)(cur_time & 0xFFFFFFFF));
	BK_LOGD(TAG,"cur inst_cnt: %x:%08x\r\n", (u32)(cur_inst_cnt >> 32), (u32)(cur_inst_cnt & 0xFFFFFFFF));

	saved_time = (cur_time - saved_time) / 26;
	saved_inst_cnt = cur_inst_cnt - saved_inst_cnt;

//	BK_LOGD(TAG,"elapse time(us): %x:%08x\r\n", (u32)(saved_time >> 32), (u32)(saved_time & 0xFFFFFFFF));
//	BK_LOGD(TAG,"diff inst_cnt: %x:%08x\r\n", (u32)(saved_inst_cnt >> 32), (u32)(saved_inst_cnt & 0xFFFFFFFF));

	if (saved_time == 0) {
		snprintf(pcWriteBuffer, xWriteBufferLen, "MIPS: N/A (time is 0)\r\n");
	} else {
		snprintf(pcWriteBuffer, xWriteBufferLen, "MIPS: %d KIPS\r\n", (u32)(saved_inst_cnt * 1000 / saved_time));
	}

	saved_time = cur_time;
	saved_inst_cnt = cur_inst_cnt;
}

static void debug_show_boot_time(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u64 cur_time = riscv_get_mtimer();
	u64 cur_inst_cnt = riscv_get_instruct_cnt();

	BK_LOGD(TAG,"cur time: %x:%08x\r\n", (u32)(cur_time >> 32), (u32)(cur_time & 0xFFFFFFFF));
	BK_LOGD(TAG,"cur time: %ldms\r\n", (u32)(cur_time/26000));
	BK_LOGD(TAG,"cur inst_cnt: %x:%08x\r\n", (u32)(cur_inst_cnt >> 32), (u32)(cur_inst_cnt & 0xFFFFFFFF));

#if	CONFIG_SAVE_BOOT_TIME_POINT
	show_saved_mtime_info();
#endif

}
#endif

#ifdef CORE_MARK_ENABLED

/* PMU helpers for M55 cache miss measurement (CMSIS memory-mapped PMU_Type) */
#if !CONFIG_ARCH_RISCV
/* armcm55.h defines __FPU_PRESENT/__DSP_PRESENT/__PMU_PRESENT/__NVIC_PRIO_BITS
 * which core_cm55.h requires to be set before inclusion */
#include "armcm55.h"
#include "core_cm55.h"
#define COREMARK_PMU_ENABLED 1

/* Use CMSIS PMU_CTRL_* macros for enable/reset bits */
#define CM_PMU_ENABLE (PMU_CTRL_ENABLE_Msk)
#define CM_PMU_RESET  (PMU_CTRL_EVENTCNT_RESET_Msk | PMU_CTRL_CYCCNT_RESET_Msk)

/* ARMv8 standard PMU event numbers */
#define PMU_EV_L1I_CACHE_REFILL  0x0001u
#define PMU_EV_L1D_CACHE_REFILL  0x0003u
#define PMU_EV_L1D_CACHE         0x0004u
#define PMU_EV_BR_MIS_PRED       0x0010u

#endif /* !CONFIG_ARCH_RISCV */

#ifdef COREMARK_PMU_ENABLED

/* M55 PMU — each core has its own PMU registers at the same virtual address.
 * The PMU is banked per-PE, so we must start/stop from within the task that
 * runs the workload (not from an outer CLI task on a potentially different core).
 *
 * PMCR bits: E=bit0 (global enable), P=bit1 (reset event counters),
 *            C=bit2 (reset cycle counter).
 * Write E|P|C to enable and simultaneously reset all counters.
 *
 * CoreSight LAR at PMU_BASE+0xFB0 must be unlocked before writing PMU regs.
 * On many SoCs the ROM table already unlocks it at boot; write it anyway.
 */
#define PMU_LAR  (*((volatile uint32_t *)(PMU_BASE + 0xFB0u)))
#define PMU_LAR_UNLOCK_KEY  0xC5ACCE55u

/* PMCR bits */
#define PMCR_E  (1u << 0)
#define PMCR_P  (1u << 1)
#define PMCR_C  (1u << 2)

static void pmu_core_start(void)
{
	/* DEMCR.TRCENA must be 1 to enable the PMU (and DWT/ETM) clock domain.
	 * Without this bit the PMU registers accept writes but counters never tick. */
	DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
	__DSB(); __ISB();

	PMU_LAR = PMU_LAR_UNLOCK_KEY;
	__DSB();

	/* Step 1: disable all counters */
	PMU->CNTENCLR = 0xFFFFFFFFu;
	__DSB();

	/* Step 2: reset event counters and cycle counter (P and C are self-clearing) */
	PMU->CTRL = PMCR_P | PMCR_C;
	__DSB();

	/* Step 3: configure event types BEFORE enabling */
	PMU->EVTYPER[0] = PMU_EV_L1I_CACHE_REFILL;
	PMU->EVTYPER[1] = PMU_EV_L1D_CACHE_REFILL;
	PMU->EVTYPER[2] = PMU_EV_BR_MIS_PRED;
	PMU->EVTYPER[3] = PMU_EV_L1D_CACHE;
	__DSB();

	/* Step 4: select which counters to enable (cycle counter = bit31) */
	PMU->CNTENSET = (1u << 31) | (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3);
	__DSB();

	/* Step 5: global enable only — P/C already reset above, don't reset again */
	PMU->CTRL = PMCR_E;
	__ISB();

	BK_LOGI("pmu", "DEMCR=0x%08x CTRL=0x%08x CNTENSET=0x%08x\r\n",
	        (unsigned)DCB->DEMCR, (unsigned)PMU->CTRL, (unsigned)PMU->CNTENSET);
}

static void pmu_core_stop_report(const char *label)
{
	/* Stop counting first, then read */
	PMU->CTRL = 0;
	__DSB();

	uint32_t cycles = PMU->CCNTR;
	uint32_t ev0    = PMU->EVCNTR[0];   /* L1I refill (miss) */
	uint32_t ev1    = PMU->EVCNTR[1];   /* L1D refill (miss) */
	uint32_t ev2    = PMU->EVCNTR[2];   /* branch mispredict */
	uint32_t ev3    = PMU->EVCNTR[3];   /* L1D access */

	uint32_t d_miss_pct = ev3 ? (ev1 * 100u / ev3) : 0u;
	BK_LOGI("pmu", "[%s] cycles=%u\r\n", label, cycles);
	BK_LOGI("pmu", "[%s] L1I-miss=%u\r\n", label, ev0);
	BK_LOGI("pmu", "[%s] L1D-miss=%u  L1D-access=%u  D-miss%%=%u%%\r\n",
	        label, ev1, ev3, d_miss_pct);
	BK_LOGI("pmu", "[%s] BR-mispredict=%u\r\n", label, ev2);
}
#endif /* COREMARK_PMU_ENABLED */

#ifndef COREMARK_CPU_FREQ_MHZ
#define COREMARK_CPU_FREQ_MHZ 480
#endif

#if (COREMARK_CPU_FREQ_MHZ == 240)
#define COREMARK_CPU_FREQ_ENUM PM_CPU_FRQ_240M
#elif (COREMARK_CPU_FREQ_MHZ == 480)
#define COREMARK_CPU_FREQ_ENUM PM_CPU_FRQ_480M
#else
#error "Unsupported COREMARK_CPU_FREQ_MHZ"
#endif

#define COREMARK_CPU_FREQ_HZ ((uint32_t)COREMARK_CPU_FREQ_MHZ * 1000000UL)

static void debug_core_mark(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	extern void core_mark(int argc, char *argv[]);
	extern bk_err_t bk_wwdt_stop(void);
	extern bk_err_t bk_wwdt_start(uint32_t timeout_ms, bool is_enable_window, uint32_t window_val);
#if (CONFIG_TASK_WDT)
	extern void bk_task_wdt_stop(void);
#endif
	extern bk_err_t sys_drv_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq);

	bk_wwdt_stop();
#if (CONFIG_TASK_WDT)
	bk_task_wdt_stop();
#endif

	/* Prevent idle WFI during benchmark so CPU clock is never gated.
	 * Without this, DWT stops during WFI and CoreMark reports ~4.5x
	 * lower score than the actual CPU computation rate. */
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0, 0);

	sys_drv_switch_cpu_bus_freq(COREMARK_CPU_FREQ_ENUM);
	BK_LOGI("coremark", "CPU boosted to %u MHz\r\n", (unsigned)COREMARK_CPU_FREQ_MHZ);

	/* Dump Flash clock register */
	{
		extern uint32_t sys_hal_flash_get_clk_sel(void);
		extern uint32_t sys_hal_flash_get_clk_div(void);
		uint32_t fsel = sys_hal_flash_get_clk_sel();
		uint32_t fdiv = sys_hal_flash_get_clk_div();
		BK_LOGI("coremark", "Flash clk: cksel=%u ckdiv=%u => source=%s div=/%u\r\n",
		        fsel, fdiv,
		        fsel == 0 ? "XTAL-26M" : fsel == 1 ? "DPLL" : "DCO",
		        fdiv + 1);
	}

	/* DWT self-test: measure a known 100ms delay with both DWT and Timer0. */
	{
		extern uint64_t get_timer_value(void);
		DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
		DWT->CYCCNT = 0;
		DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
		__DSB(); __ISB();

		/* busy-wait 100ms using Timer0 as reference (26MHz → 2600000 ticks) */
		uint32_t dwt0  = DWT->CYCCNT;
		uint64_t tmr0  = get_timer_value();
		while ((uint32_t)(get_timer_value() - tmr0) < 2600000u) { /* spin ~100ms */ }
		uint32_t dwt1  = DWT->CYCCNT;
		uint64_t tmr1  = get_timer_value();

		uint32_t dwt_cy = dwt1 - dwt0;
		uint32_t tmr_tk = (uint32_t)(tmr1 - tmr0);
		BK_LOGI("coremark", "DWT self-test 100ms: DWT=%u cy (%.3fs@%uM)  Timer0=%u tk (%.3fs@26M)  ratio=%.2f\r\n",
		        dwt_cy, (double)dwt_cy/(double)COREMARK_CPU_FREQ_HZ,
		        (unsigned)COREMARK_CPU_FREQ_MHZ,
		        tmr_tk, (double)tmr_tk/26000000.0,
		        (double)dwt_cy / (double)COREMARK_CPU_FREQ_HZ / ((double)tmr_tk / 26000000.0 + 1e-9));
	}

	/* PMU per-core measurement is now done inside coremark_parallel_task()
	 * in core_portme.c — each core reports its own cache/branch stats. */
	core_mark(argc, argv);

	/* Restore default frequency and re-enable idle sleep */
	sys_drv_switch_cpu_bus_freq(PM_CPU_FRQ_480M);
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);
	BK_LOGI("coremark", "CPU freq restored\r\n");

	bk_wwdt_start(CONFIG_INT_WWDT_PERIOD_MS, false, 0);
}

#include "./core_mark/core_main.c"
#endif /* CORE_MARK_ENABLED */

