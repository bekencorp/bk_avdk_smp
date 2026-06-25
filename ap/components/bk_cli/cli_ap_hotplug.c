// Copyright 2020-2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include <common/bk_include.h>
#include <os/os.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include "multicore_driver.h"
#include "sys_ahbp_ll.h"
#include "dbg_probe.h"
#include "dbg_sink_ram.h"
#include "components/shell_task.h"

#if CONFIG_DBG_PROBE
/* DWT cycle counter (same addresses as system_cpu0.c; avoid pulling core_cm55.h). */
#define CLI_DBG_COREDEBUG_BASE   (0xE000EDF0UL)
#define CLI_DBG_DWT_BASE         (0xE0001000UL)
#define CLI_DBG_DWT_CTRL         (*((volatile uint32_t *)(CLI_DBG_DWT_BASE + 0x000UL)))
#define CLI_DBG_DWT_CYCCNT       (*((volatile uint32_t *)(CLI_DBG_DWT_BASE + 0x004UL)))
#define CLI_DBG_DEMCR            (*((volatile uint32_t *)(CLI_DBG_COREDEBUG_BASE + 0x00CUL)))
#define CLI_DBG_DEMCR_TRCENA     (1UL << 24)
#define CLI_DBG_DWT_CYCCNTENA    (1UL << 0)
#endif

#if (CONFIG_SOC_SMP && CONFIG_CPU_HOTPLUG)

#if CONFIG_DBG_PROBE
/* dbg_probe stress: two same-priority tasks pinned to AP core0/core1 each burst
 * dbg_probe_rt_raw frames (mirrors the CP cli, invoked here via `ap_cmd dbgp`).
 * Exclusive: core0->U1, core1->U2; shared: both->one wire + spinlock. */
#define DBG_PROBE_BURST_STACK   512
#define DBG_PROBE_BURST_MOD0    0x11u
#define DBG_PROBE_BURST_MOD1    0x22u

/* Number of burst tasks still in flight. dbg_probe_set_shared() must only run
 * with probes quiesced (see dbg_probe.h), so shared/excl/[se]test refuse while
 * this is non-zero. Atomics: bumped on the CLI core, dropped on either core. */
static volatile uint32_t s_dbg_burst_active;

static inline void cli_dbg_dwt_init(void)
{
	if ((CLI_DBG_DEMCR & CLI_DBG_DEMCR_TRCENA) == 0u) {
		CLI_DBG_DEMCR |= CLI_DBG_DEMCR_TRCENA;
		CLI_DBG_DWT_CYCCNT = 0u;
		CLI_DBG_DWT_CTRL |= CLI_DBG_DWT_CYCCNTENA;
	}
}

static void cli_dbg_probe_measure(const char *tag, uint32_t warm, uint32_t n,
	void (*fn)(void))
{
	uint32_t i, dt, min_c = UINT32_MAX, max_c = 0u, sum_c = 0u, samples = 0u;

	for (i = 0; i < warm + n; i++) {
		uint32_t t0 = CLI_DBG_DWT_CYCCNT;
		fn();
		dt = CLI_DBG_DWT_CYCCNT - t0;
		if (i < warm) {
			continue;
		}
		if (dt < min_c) {
			min_c = dt;
		}
		if (dt > max_c) {
			max_c = dt;
		}
		sum_c += dt;
		samples++;
	}
	CLI_LOGI("dbgp cycles %s: min=%u avg=%u max=%u (n=%u)\r\n",
		tag, min_c, samples ? (sum_c / samples) : 0u, max_c, samples);
}

static void cli_dbg_probe_fn_stage4(void)
{
	dbg_probe_stage(DBG_MOD_BOOT, 0xBEEFu);
}

static void cli_dbg_probe_fn_stage8(void)
{
	dbg_probe_stage_ts(DBG_MOD_BOOT, 0xBEEFu);
}

static void cli_dbg_probe_fn_coreid(void)
{
	(void)bk_multicore_get_cpu_id();
}

static void cli_dbg_probe_cycles(void)
{
	cli_dbg_dwt_init();
	dbg_probe_set_shared(false);
	cli_dbg_probe_measure("warm", 0, 32, cli_dbg_probe_fn_stage4);
	cli_dbg_probe_measure("core_id", 8, 256, cli_dbg_probe_fn_coreid);
	cli_dbg_probe_measure("stage4", 8, 256, cli_dbg_probe_fn_stage4);
	cli_dbg_probe_measure("stage8", 8, 256, cli_dbg_probe_fn_stage8);
	CLI_LOGI("dbgp cycles: AP core0 excl U1, tee=RAM+UART, CPU=480MHz\r\n");
}

static void cli_dbg_probe_burst_task(void *arg)
{
	uint32_t loops = (uint32_t)(uintptr_t)arg;
	uint8_t  mod = (portGET_CORE_ID() == SMP_CORE0_ID) ?
		DBG_PROBE_BURST_MOD0 : DBG_PROBE_BURST_MOD1;

	for (uint32_t i = 0; i < loops; i++) {
		for (uint32_t j = 0; j < 8u; j++) {
			dbg_probe_rt_raw(DBG_PROBE_KIND_U16, mod, (uint16_t)((i << 3) | j));
		}
		rtos_delay_milliseconds(2);
	}
	(void)__atomic_sub_fetch(&s_dbg_burst_active, 1u, __ATOMIC_RELEASE);
	vTaskDelete(NULL);
}

/* Ver5 self-test: emit one of every frame kind on the calling core's port
 * (pairs of {plain, _ts}). Run pinned to core1 so the frames hit UART2/COM11. */
static void cli_dbg_probe_v5_task(void *arg)
{
	(void)arg;
	dbg_probe_stage(DBG_MOD_KERNEL, 0x0101u);
	dbg_probe_stage_ts(DBG_MOD_KERNEL, 0x0102u);
	dbg_probe_irq(DBG_MOD_IRQ, 0x0010u);
	dbg_probe_irq_ts(DBG_MOD_IRQ, 0x0011u);
	dbg_probe_u16(DBG_MOD_MEM, 0x1234u);
	dbg_probe_u16_ts(DBG_MOD_MEM, 0x1235u);
	dbg_probe_u32(DBG_MOD_MEM, 0xDEADBEEFu);
	dbg_probe_u32_ts(DBG_MOD_MEM, 0xCAFEBABEu);
	dbg_probe_task(DBG_MOD_SCHED, "RXth");
	dbg_probe_task_ts(DBG_MOD_SCHED, "TXth");
	dbg_probe_exc(DBG_MOD_KERNEL, 0x0003u);
	dbg_probe_exc_ts(DBG_MOD_KERNEL, 0x0004u);
	vTaskDelete(NULL);
}

static uint8_t cli_dbg_probe_parse_u8(const char *s)
{
	return (uint8_t)os_strtoul(s, NULL, 0);
}

static void cli_dbg_probe_mod_show(void)
{
	uint32_t mask[DBG_PROBE_MOD_MASK_WORDS];
	uint8_t sink = dbg_probe_get_sink_mask();
	const char *sink_s = "both";

	if (sink == DBG_PROBE_SINK_RAM) {
		sink_s = "ram";
	} else if (sink == DBG_PROBE_SINK_UART) {
		sink_s = "uart";
	} else if (sink != DBG_PROBE_SINK_BOTH) {
		sink_s = "?";
	}
	dbg_probe_mod_uart_get_mask(mask);
	CLI_LOGI("dbgp mod: sink=%s uart_mask=", sink_s);
	for (uint32_t i = 0; i < (uint32_t)DBG_PROBE_MOD_MASK_WORDS; i++) {
		CLI_LOGI("%08x%s", (unsigned)mask[i],
			(i + 1u < (uint32_t)DBG_PROBE_MOD_MASK_WORDS) ? "," : "");
	}
	CLI_LOGI(" (bypass EXC/SYNC/DROP)\r\n");
}

static bool cli_dbg_probe_mod_cmd(int argc, char **argv)
{
	if (argc < 3) {
		return false;
	}
	if (os_strcmp(argv[2], "show") == 0) {
		cli_dbg_probe_mod_show();
		return true;
	}
	if (os_strcmp(argv[2], "all") == 0) {
		dbg_probe_mod_uart_set_all(true);
		CLI_LOGI("dbgp mod: all modules enabled on UART\r\n");
		return true;
	}
	if (os_strcmp(argv[2], "none") == 0) {
		dbg_probe_mod_uart_set_all(false);
		CLI_LOGI("dbgp mod: all modules disabled on UART (EXC/SYNC/DROP bypass)\r\n");
		return true;
	}
	if (argc >= 4 && os_strcmp(argv[2], "on") == 0) {
		uint8_t m = cli_dbg_probe_parse_u8(argv[3]);
		dbg_probe_mod_uart_set(m, true);
		CLI_LOGI("dbgp mod: 0x%02x on UART\r\n", (unsigned)m);
		return true;
	}
	if (argc >= 4 && os_strcmp(argv[2], "off") == 0) {
		uint8_t m = cli_dbg_probe_parse_u8(argv[3]);
		dbg_probe_mod_uart_set(m, false);
		CLI_LOGI("dbgp mod: 0x%02x off UART\r\n", (unsigned)m);
		return true;
	}
	return false;
}

static bool cli_dbg_probe_sink_cmd(int argc, char **argv)
{
	if (argc < 3) {
		return false;
	}
	if (os_strcmp(argv[2], "show") == 0) {
		cli_dbg_probe_mod_show();
		return true;
	}
	if (os_strcmp(argv[2], "both") == 0 || os_strcmp(argv[2], "tee") == 0) {
		dbg_probe_set_sink_mask(DBG_PROBE_SINK_BOTH);
		CLI_LOGI("dbgp sink: RAM+UART\r\n");
		return true;
	}
	if (os_strcmp(argv[2], "ram") == 0 || os_strcmp(argv[2], "ramonly") == 0) {
		dbg_probe_set_sink_mask(DBG_PROBE_SINK_RAM);
		CLI_LOGI("dbgp sink: RAM only\r\n");
		return true;
	}
	if (os_strcmp(argv[2], "uart") == 0 || os_strcmp(argv[2], "uartonly") == 0) {
		dbg_probe_set_sink_mask(DBG_PROBE_SINK_UART);
		CLI_LOGI("dbgp sink: UART only (no RAM ring writes)\r\n");
		return true;
	}
	return false;
}
#endif /* CONFIG_DBG_PROBE */

#if CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING
/* AP output goes to the SHARED console via the async-log raw channel
 * (shell_log_raw_data, forwarded over mailbox/IPC). cli_printf would write the
 * AP-local CLI_UART, which is not bridged back to the host COM. */
static void ap_dbg_out(const char *s)
{
	shell_log_raw_data((const u8 *)s, (u16)os_strlen(s));
}

/* Dump one RAM ring as text (BEGIN header + hex oldest->newest + END), parsed
 * offline by dbg_parse.py --ram-dump. Yields every few lines so the async-log
 * queue can drain (avoids dropping lines on a bulk dump).
 *
 * DESIGN CONSTRAINT (lock-free dump): the ring writer runs on its OWNER core
 * with only that core's IRQs disabled — there is NO cross-core lock, so this
 * reader cannot stop a live writer. Two consequences, both handled here:
 *  1) header pair-tear: (wr_off, wrap_cnt, seq) are three separate 32-bit
 *     reads; a wrap between them yields an inconsistent snapshot. Fixed by the
 *     stable-snapshot retry loop below (re-read until two passes agree).
 *  2) data churn: while the hex dump streams out (tens of ms), a still-active
 *     owner core keeps overwriting bytes near the write head. This is NOT
 *     prevented — it is DETECTED: the END line carries seq_end, and the offline
 *     parser compares it with the BEGIN seq. Equal => dump is clean/trusted;
 *     different => N frames churned, bytes near the head are untrusted (the
 *     self-describing frames let the parser resync past any garbled region).
 * For a 100%-clean dump, quiesce the probes first (normal test flow: dump
 * after bursts finish, or post-mortem when nothing is running). */
static void cli_dbg_probe_dump_ring(uint8_t core, uint8_t bucket)
{
	static const char hexd[] = "0123456789ABCDEF";
	const uint8_t *buf = NULL;
	uint32_t size = 0;
	const dbg_ram_ring_hdr_t *h = dbg_sink_ram_get(core, bucket, &buf, &size);
	uint32_t wr, wrap, seq0, start, count, lines = 0;
	char line[136];
	uint32_t col = 0;

	if (h == NULL || buf == NULL || size == 0u) {
		return;
	}
	/* Stable snapshot: re-read (wr,wrap,seq) until two consecutive reads agree
	 * (1 pass when probes are quiet; bounded retries when they are not — then
	 * the seq/seq_end mismatch below still flags the dump as dirty). */
	for (uint32_t tries = 0u; ; tries++) {
		uint32_t w2, p2, s2;
		wr = h->wr_off; wrap = h->wrap_cnt; seq0 = h->seq;
		w2 = h->wr_off; p2 = h->wrap_cnt; s2 = h->seq;
		if ((wr == w2 && wrap == p2 && seq0 == s2) || tries >= 16u) {
			break;
		}
	}
	if (wrap == 0u) {
		start = 0u;
		count = wr;
	} else {
		start = wr;
		count = size;
	}
	snprintf(line, sizeof(line),
		"==DBGRING BEGIN core=%u bucket=%u hdr_ver=%u fmt=%u size=%u wr=%u wrap=%u seq=%u==\r\n",
		(unsigned)h->core_id, (unsigned)h->bucket_id, (unsigned)h->hdr_ver,
		(unsigned)h->frame_fmt, (unsigned)size, (unsigned)wr, (unsigned)wrap,
		(unsigned)seq0);
	ap_dbg_out(line);
	for (uint32_t j = 0; j < count; j++) {
		uint8_t b = buf[(start + j) % size];
		line[col++] = hexd[(b >> 4) & 0xF];
		line[col++] = hexd[b & 0xF];
		if (col >= 128u) {
			line[col++] = '\r';
			line[col++] = '\n';
			line[col] = '\0';
			ap_dbg_out(line);
			col = 0;
			if ((++lines % 8u) == 0u) {
				rtos_delay_milliseconds(3);   /* let async-log TX drain */
			}
		}
	}
	if (col > 0u) {
		line[col++] = '\r';
		line[col++] = '\n';
		line[col] = '\0';
		ap_dbg_out(line);
	}
	/* seq_end: re-read seq AFTER streaming. seq_end == BEGIN seq => clean dump;
	 * a delta means the ring advanced mid-dump (head region untrusted). */
	snprintf(line, sizeof(line), "==DBGRING END seq_end=%u==\r\n", (unsigned)h->seq);
	ap_dbg_out(line);
	rtos_delay_milliseconds(5);
}

static void cli_dbg_probe_dump_all(void)
{
	for (uint8_t c = 0; c < (uint8_t)DBG_PROBE_NUM_CORES; c++) {
		for (uint8_t b = 0; b < (uint8_t)DBG_RAM_NUM_BUCKETS; b++) {
			cli_dbg_probe_dump_ring(c, b);
		}
	}
}
#endif /* CONFIG_DBG_PROBE && CONFIG_DBG_PROBE_RAMRING */

#define CPU_HOTPLUG_CMD_CNT (sizeof(s_cpu_hotplug_commands) / sizeof(struct cli_command))
#define CPU_HOTPLUG_CLI_MIGRATE_RETRY (20)
#define CPU_HOTPLUG_BUSY_TEST_STACK_SIZE (512)

static void cli_cpu_hotplug_help(void)
{
	CLI_LOGI("cpu list\r\n");
	CLI_LOGI("cpu state\r\n");
	CLI_LOGI("cpu offline 3\r\n");
	CLI_LOGI("cpu online 3\r\n");
	CLI_LOGI("cpu irq-affinity\r\n");
	CLI_LOGI("cpu task-affinity\r\n");
	CLI_LOGI("cpu stress 3 <loops>\r\n");
	CLI_LOGI("cpu busy-test\r\n");
#if CONFIG_CPU_HP_GOVERNOR
	CLI_LOGI("cpu gov on|off|status\r\n");
	CLI_LOGI("cpu gov stress <cycles>\r\n");
#endif
}

#if CONFIG_CPU_HP_GOVERNOR

#define CPU_HP_GOV_STRESS_LOAD_TASKS           (CONFIG_SMP_CORE_CNT)
#define CPU_HP_GOV_STRESS_DEFAULT_CYCLES       (3)
#define CPU_HP_GOV_STRESS_LOAD_STACK           (512)
#define CPU_HP_GOV_STRESS_POLL_MS              (50)
#define CPU_HP_GOV_STRESS_ONLINE_TIMEOUT_MS    (CONFIG_CPU_HP_GOVERNOR_COOLDOWN_MS * (CONFIG_SMP_CORE_CNT + 2))
#define CPU_HP_GOV_STRESS_OFFLINE_TIMEOUT_MS   (CONFIG_CPU_HP_GOVERNOR_COOLDOWN_MS * (CONFIG_SMP_CORE_CNT + 5))

static volatile uint32_t s_gov_load_run;
static volatile uint32_t s_gov_load_active;

void delay_ms(UINT32 ms);

static void cli_cpu_hp_gov_load_task(void *arg)
{
	(void)arg;

	s_gov_load_active++;
	while (s_gov_load_run) {
		for (uint32_t i = 0; i < CONFIG_CPU_HP_GOVERNOR_COOLDOWN_MS; i++) {
			delay_ms(1);
		}
	}
	s_gov_load_active--;

	rtos_delete_thread(NULL);
}

static bk_err_t cli_cpu_hp_gov_load_start(void)
{
	s_gov_load_run = 1;
	s_gov_load_active = 0;

	for (uint32_t i = 0; i < CPU_HP_GOV_STRESS_LOAD_TASKS; i++) {
		bk_err_t ret = rtos_create_thread(NULL, BEKEN_DEFAULT_WORKER_PRIORITY,
			"gov_load", cli_cpu_hp_gov_load_task, CPU_HP_GOV_STRESS_LOAD_STACK, NULL);
		if (ret != BK_OK) {
			CLI_LOGE("cpu gov stress: create load task %u failed ret=%d\r\n", i, ret);
			s_gov_load_run = 0;
			return ret;
		}
	}

	return BK_OK;
}

static void cli_cpu_hp_gov_load_stop(void)
{
	s_gov_load_run = 0;

	for (uint32_t waited = 0; s_gov_load_active != 0 && waited < 2000;
		waited += CPU_HP_GOV_STRESS_POLL_MS) {
		rtos_delay_milliseconds(CPU_HP_GOV_STRESS_POLL_MS);
	}
}


static uint32_t cli_cpu_hp_gov_wait_online(uint32_t want_online, uint32_t timeout_ms)
{
	for (uint32_t waited = 0; waited <= timeout_ms; waited += CPU_HP_GOV_STRESS_POLL_MS) {
		if (bk_cpu_hp_is_online(CPU3_CORE_ID) == want_online) {
			return 1;
		}
		rtos_delay_milliseconds(CPU_HP_GOV_STRESS_POLL_MS);
	}

	return (bk_cpu_hp_is_online(CPU3_CORE_ID) == want_online);
}


static void cli_cpu_hp_governor_stress(uint32_t cycles)
{
	bk_cpu_hp_governor_status_t st;
	uint32_t saved_enabled;
	uint32_t pass = 0;

	if (cycles == 0) {
		cycles = CPU_HP_GOV_STRESS_DEFAULT_CYCLES;
	}

	bk_cpu_hp_governor_get_status(&st);
	saved_enabled = st.enabled;

	/* The governor must be active for the auto online/offline decisions. */
	bk_cpu_hp_governor_start();

	CLI_LOGI("cpu gov stress: start, cycles=%u load_tasks=%u\r\n",
		cycles, CPU_HP_GOV_STRESS_LOAD_TASKS);

	for (uint32_t c = 0; c < cycles; c++) {
		uint32_t online_ok;
		uint32_t offline_ok;

		if (cli_cpu_hp_gov_load_start() != BK_OK) {
			break;
		}
		online_ok = cli_cpu_hp_gov_wait_online(1, CPU_HP_GOV_STRESS_ONLINE_TIMEOUT_MS);

		cli_cpu_hp_gov_load_stop();
		offline_ok = cli_cpu_hp_gov_wait_online(0, CPU_HP_GOV_STRESS_OFFLINE_TIMEOUT_MS);

		bk_cpu_hp_governor_get_status(&st);
		CLI_LOGI("cpu gov stress: cycle %u/%u %s (online=%s offline=%s) online_cnt=%u offline_cnt=%u\r\n",
			c + 1, cycles, (online_ok && offline_ok) ? "PASS" : "FAIL",
			online_ok ? "yes" : "no", offline_ok ? "yes" : "no",
			st.online_cnt, st.offline_cnt);

		if (online_ok && offline_ok) {
			pass++;
		}
	}

	/* Restore the governor to whatever the user had before the test. */
	if (!saved_enabled) {
		bk_cpu_hp_governor_stop();
	}

	CLI_LOGI("cpu gov stress: done, %u/%u cycles PASS\r\n", pass, cycles);
}

static void cli_cpu_hp_governor_cmd(int argc, char **argv)
{
	if (argc < 3) {
		CLI_LOGI("cpu gov on|off|status|stress [cycles]\r\n");
		return;
	}

	if (os_strcmp(argv[2], "on") == 0) {
		CLI_LOGI("cpu gov start ret=%d\r\n", bk_cpu_hp_governor_start());
	} else if (os_strcmp(argv[2], "off") == 0) {
		CLI_LOGI("cpu gov stop ret=%d\r\n", bk_cpu_hp_governor_stop());
	} else if (os_strcmp(argv[2], "status") == 0) {
		bk_cpu_hp_governor_status_t st;
		bk_cpu_hp_governor_get_status(&st);
		CLI_LOGI("cpu gov: enabled=%u cpu3_online=%u load0=%u%% load1=%u%% up_cnt=%u down_cnt=%u online=%u offline=%u\r\n",
			st.enabled, st.cpu1_online, st.load0, st.load1,
			st.up_cnt, st.down_cnt, st.online_cnt, st.offline_cnt);
	} else if (os_strcmp(argv[2], "stress") == 0) {
		uint32_t cycles = CPU_HP_GOV_STRESS_DEFAULT_CYCLES;

		if (argc >= 4) {
			cycles = os_strtoul(argv[3], NULL, 10);
		}
		cli_cpu_hp_governor_stress(cycles);
	} else {
		CLI_LOGI("cpu gov on|off|status|stress [cycles]\r\n");
	}
}
#endif

static void cli_cpu_print_state(void)
{
	for (uint32_t cpu = CPU2_CORE_ID; cpu <= CPU3_CORE_ID; cpu++) {
		CLI_LOGI("cpu%u: state=%s online=%u active=%u domain possible=0x%x online=0x%x active=0x%x dying=0x%x offline=0x%x\r\n",
			cpu, bk_cpu_hp_get_state_name(cpu), bk_cpu_hp_is_online(cpu), bk_cpu_hp_is_active(cpu),
			bk_cpu_hp_get_domain_possible_mask(cpu), bk_cpu_hp_get_domain_online_mask(cpu),
			bk_cpu_hp_get_domain_active_mask(cpu), bk_cpu_hp_get_domain_dying_mask(cpu),
			bk_cpu_hp_get_domain_offline_mask(cpu));
	}
}

static uint32_t cli_cpu_hotplug_target_valid(uint32_t cpu)
{
	if (cpu != CPU3_CORE_ID) {
		CLI_LOGE("AP hotplug only supports cpu3, cpu%u is not allowed\r\n", cpu);
		return 0;
	}

	return 1;
}

static void cli_cpu_print_irq_affinity(void)
{
	CLI_LOGI("AP irq affinity routes: cpu2 reg10=0x%x reg11=0x%x, cpu3 reg12=0x%x reg13=0x%x reg14=0x%x\r\n",
		sys_ahbp_ll_get_reg10_value(), sys_ahbp_ll_get_reg11_value(),
		sys_ahbp_ll_get_reg12_value(), sys_ahbp_ll_get_reg13_value(),
		sys_ahbp_ll_get_reg14_value());
}

static void cli_cpu_hotplug_busy_task(void *arg)
{
	(void)arg;

	while (1) {
		rtos_delay_milliseconds(50);
	}
}

static void cli_cpu_hotplug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t cpu = CPU3_CORE_ID;
	uint32_t loops = 1;
	BaseType_t old_core_id = tskNO_AFFINITY;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)old_core_id;

	if (argc < 2) {
		cli_cpu_hotplug_help();
		return;
	}

	if ((os_strcmp(argv[1], "list") == 0) || (os_strcmp(argv[1], "state") == 0) ||
		(os_strcmp(argv[1], "status") == 0)) {
		cli_cpu_print_state();
		return;
	}

	if (os_strcmp(argv[1], "offline") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}
		ret = bk_cpu_hp_offline(cpu);
		CLI_LOGI("cpu%u offline ret=%d\r\n", cpu, ret);
		return;
	}

	if (os_strcmp(argv[1], "online") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}
		ret = bk_cpu_hp_online(cpu);
		CLI_LOGI("cpu%u online ret=%d\r\n", cpu, ret);
		return;
	}

	if (os_strcmp(argv[1], "irq-affinity") == 0) {
		cli_cpu_print_irq_affinity();
		return;
	}

#if CONFIG_CPU_HP_GOVERNOR
	if (os_strcmp(argv[1], "gov") == 0) {
		cli_cpu_hp_governor_cmd(argc, argv);
		return;
	}
#endif

	if (os_strcmp(argv[1], "task-affinity") == 0) {
		CLI_LOGI("hard-pinned task on AP cpu3/core1: %s\r\n",
			xTaskHasTasksPinnedToCore(SMP_CORE1_ID) ? "yes" : "no");
		return;
	}

	if (os_strcmp(argv[1], "busy-test") == 0) {
		TaskHandle_t busy_task = NULL;
		BaseType_t task_ret;
		bk_err_t recover_ret;
		const bk_err_t expected_ret = BK_ERR_BUSY;

		task_ret = xTaskCreatePinnedToCore(cli_cpu_hotplug_busy_task, "hp_busy",
			CPU_HOTPLUG_BUSY_TEST_STACK_SIZE, NULL, BEKEN_DEFAULT_WORKER_PRIORITY,
			&busy_task, SMP_CORE1_ID);
		if (task_ret != pdPASS) {
			CLI_LOGE("cpu busy-test create pinned task failed, ret=%d\r\n", task_ret);
			return;
		}

		taskYIELD();
		rtos_delay_milliseconds(2);

		ret = bk_cpu_hp_offline(CPU3_CORE_ID);
		if (ret != expected_ret) {
			recover_ret = bk_cpu_hp_online(CPU3_CORE_ID);
			CLI_LOGE("cpu busy-test recovery online ret=%d\r\n", recover_ret);
		}

		vTaskDelete(busy_task);
		CLI_LOGI("cpu busy-test %s expect=busy(%d) actual=%d state=%s\r\n",
			(ret == expected_ret) ? "PASS" : "FAIL", expected_ret, ret,
			bk_cpu_hp_get_state_name(CPU3_CORE_ID));
		return;
	}

	if (os_strcmp(argv[1], "stress") == 0) {
		if (argc >= 3) {
			cpu = os_strtoul(argv[2], NULL, 10);
		}
		if (argc >= 4) {
			loops = os_strtoul(argv[3], NULL, 10);
		}
		if (!cli_cpu_hotplug_target_valid(cpu)) {
			return;
		}


		for (uint32_t i = 0; i < loops; i++) {
			ret = bk_cpu_hp_offline(cpu);
			if (ret != BK_OK) {
				CLI_LOGE("cpu%u offline failed at loop %u, ret=%d\r\n", cpu, i, ret);
				return;
			}
			rtos_delay_milliseconds(1);

			ret = bk_cpu_hp_online(cpu);
			if (ret != BK_OK) {
				CLI_LOGE("cpu%u online failed at loop %u, ret=%d\r\n", cpu, i, ret);
				return;
			}
			rtos_delay_milliseconds(1);
		}

		CLI_LOGI("cpu%u hotplug stress %u loops done\r\n", cpu, loops);
		return;
	}

	cli_cpu_hotplug_help();
}

#if CONFIG_DBG_PROBE
static void cli_dbg_probe_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc >= 2 && os_strcmp(argv[1], "mod") == 0) {
		if (cli_dbg_probe_mod_cmd(argc, argv)) {
			return;
		}
		CLI_LOGI("dbgp mod {show|all|none|on <id>|off <id>}\r\n");
		return;
	}
	if (argc >= 2 && os_strcmp(argv[1], "sink") == 0) {
		if (cli_dbg_probe_sink_cmd(argc, argv)) {
			return;
		}
		CLI_LOGI("dbgp sink {show|both|ram|uart}\r\n");
		return;
	}
	if (argc >= 2 &&
	    (os_strcmp(argv[1], "test")  == 0 ||    /* legacy alias = shared */
	     os_strcmp(argv[1], "stest") == 0 ||    /* shared:   both -> U1 + spinlock */
	     os_strcmp(argv[1], "etest") == 0)) {   /* exclusive: core0->U1, core1->U2 */
		bool shared = (os_strcmp(argv[1], "etest") != 0);
		uint32_t loops = 1500u;   /* ~3 s @ 2 ms/round */
		BaseType_t r0, r1;

		if (argc >= 3) {
			loops = os_strtoul(argv[2], NULL, 10);
		}
		if (__atomic_load_n(&s_dbg_burst_active, __ATOMIC_ACQUIRE) != 0u) {
			CLI_LOGI("dbgp: burst still running, retry when it finishes\r\n");
			return;
		}
		dbg_probe_set_shared(shared);
		__atomic_store_n(&s_dbg_burst_active, 2u, __ATOMIC_RELEASE);
		r0 = xTaskCreatePinnedToCore(cli_dbg_probe_burst_task, "dbg_b0",
			DBG_PROBE_BURST_STACK, (void *)(uintptr_t)loops,
			BEKEN_DEFAULT_WORKER_PRIORITY, NULL, SMP_CORE0_ID);
		r1 = xTaskCreatePinnedToCore(cli_dbg_probe_burst_task, "dbg_b1",
			DBG_PROBE_BURST_STACK, (void *)(uintptr_t)loops,
			BEKEN_DEFAULT_WORKER_PRIORITY, NULL, SMP_CORE1_ID);
		if (r0 != pdPASS) {
			(void)__atomic_sub_fetch(&s_dbg_burst_active, 1u, __ATOMIC_RELEASE);
		}
		if (r1 != pdPASS) {
			(void)__atomic_sub_fetch(&s_dbg_burst_active, 1u, __ATOMIC_RELEASE);
		}
		CLI_LOGI("dbgp %s: %s burst x%u core0(%d)+core1(%d) started\r\n",
			argv[1], shared ? "shared" : "excl", loops, (int)r0, (int)r1);
		return;
	}
	if (argc >= 2 && os_strcmp(argv[1], "excl") == 0) {
		if (__atomic_load_n(&s_dbg_burst_active, __ATOMIC_ACQUIRE) != 0u) {
			CLI_LOGI("dbgp: burst still running, mode switch refused\r\n");
			return;
		}
		dbg_probe_set_shared(false);
		CLI_LOGI("dbgp: exclusive (core0->U1, core1->U2) restored\r\n");
		return;
	}
	if (argc >= 2 && os_strcmp(argv[1], "shared") == 0) {
		if (__atomic_load_n(&s_dbg_burst_active, __ATOMIC_ACQUIRE) != 0u) {
			CLI_LOGI("dbgp: burst still running, mode switch refused\r\n");
			return;
		}
		dbg_probe_set_shared(true);
		CLI_LOGI("dbgp: shared (both->U1, spinlock) set\r\n");
		return;
	}
#if CONFIG_DBG_PROBE_RAMRING
	if (argc >= 2 && os_strcmp(argv[1], "dump") == 0) {
		cli_dbg_probe_dump_all();
		CLI_LOGI("dbgp dump: done\r\n");
		return;
	}
#endif
	if (argc >= 2 && os_strcmp(argv[1], "cycles") == 0) {
		cli_dbg_probe_cycles();
		return;
	}
	if (argc >= 2 && os_strcmp(argv[1], "v5") == 0) {
		int core = SMP_CORE1_ID;
		BaseType_t r;

		if (argc >= 3 && os_strcmp(argv[2], "0") == 0) {
			core = SMP_CORE0_ID;
		}
		r = xTaskCreatePinnedToCore(cli_dbg_probe_v5_task, "dbg_v5",
			DBG_PROBE_BURST_STACK, NULL,
			BEKEN_DEFAULT_WORKER_PRIORITY, NULL, core);
		CLI_LOGI("dbgp v5: 12-frame self-test on core%d (%d)\r\n", core, (int)r);
		return;
	}
	CLI_LOGI("dbgp {mod|sink|etest|stest|shared|excl|v5|dump|cycles}\r\n");
}
#endif /* CONFIG_DBG_PROBE */

static const struct cli_command s_cpu_hotplug_commands[] = {
	{"cpu", "cpu {list|state|offline 3|online 3|irq-affinity|task-affinity|stress 3 <loops>|busy-test|gov on|off|status|gov stress <cycles>}", cli_cpu_hotplug_cmd},
#if CONFIG_DBG_PROBE
	{"dbgp", "dbgp {mod|sink|etest|stest|shared|excl|v5|dump|cycles}", cli_dbg_probe_cmd},
#endif
};

int cli_ap_hotplug_init(void)
{
#if CONFIG_CPU_HP_GOVERNOR
	bk_cpu_hp_governor_init();
#endif
	return cli_register_commands(s_cpu_hotplug_commands, CPU_HOTPLUG_CMD_CNT);
}

#endif
