#include <stdint.h>
#include <stdbool.h>
#include <components/log.h>
#include <components/system.h>
#include <driver/ipi_driver.h>
#include <driver/aon_rtc.h>
#include <os/mem.h>
#include <os/os.h>
#include <soc/soc.h>
#include "bk_arch.h"
#include "bk_coredump.h"
#include "multicore_driver.h"
#include "reg_base.h"
#include "sys_sw_regs.h"

#if CONFIG_SUPPORT_WWDT
#include <driver/wwdt.h>
#include "wwdt_driver.h"
#endif

#define CP_HANG_TAG "cp_hang"
#define CP_HANG_HEARTBEAT_EVENT 1U
#define CP_HANG_MONITOR_STACK_SIZE 2048U
#define CP_HANG_MONITOR_PRIORITY (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define CP_HANG_MONITOR_CHECK_MS 500U
/* A monitor loop gap this much larger than the fixed check interval means the AP
 * core was powered off (LV/deep sleep) and just resumed: rtos_get_time() jumped. */
#define CP_HANG_MONITOR_WAKE_JUMP_MS (CP_HANG_MONITOR_CHECK_MS * 3U)
#define CP_HANG_TIMEOUT_MARGIN_MS 2000U
#define CP_HANG_TIMEOUT_FALLBACK_MS 6000U
#define CP_HANG_TIMEOUT_MIN_MS (CONFIG_CP_HANG_DUMP_BY_AP_PERIOD_MS + 500U)
#define CP_HANG_TIMEOUT_MAX_MS (UINT16_MAX)
#define CP_HANG_AON_WDT_PERIOD_MAX 0x00ffffffU
#define CP_HANG_AON_WDT_KEY_1ST 0x5aU
#define CP_HANG_AON_WDT_KEY_2ND 0xa5U
#define CP_HANG_AON_WDT_PERIOD_LOW_MASK 0x0000ffffU
#define CP_HANG_AON_WDT_PERIOD_HIGH_MASK 0x00ff0000U
#define CP_HANG_AON_WDT_PERIOD_HIGH_SHIFT 8U
#define CP_HANG_AON_WDT_KEY_SHIFT 16U
#ifndef SOC_AON_WDT_REG_BASE
#define SOC_AON_WDT_REG_BASE (0x44000600U + SOC_ADDR_OFFSET)
#endif

typedef struct {
	volatile uint8_t seen;
	volatile uint8_t dumping;
	volatile uint8_t src_cpu;
	volatile uint16_t timeout_ms;
	volatile uint32_t last_tick;
	volatile uint32_t timeout_tick;
} cp_hang_watch_state_t;

static cp_hang_watch_state_t s_cp_hang_state;
extern volatile const uint8_t build_version[];

static void cp_hang_set_ap_dumping(uint32_t value)
{
	bk_sys_sw_regs_set_ap_cp_hang_dumping(value);
}

static inline uint32_t cp_hang_effective_timeout_ms(void)
{
	uint32_t timeout_ms = s_cp_hang_state.timeout_ms;

	if (timeout_ms < CP_HANG_TIMEOUT_MIN_MS) {
		timeout_ms = CP_HANG_TIMEOUT_MIN_MS;
	}
	if (timeout_ms > CP_HANG_TIMEOUT_MAX_MS) {
		timeout_ms = CP_HANG_TIMEOUT_MAX_MS;
	}

	return timeout_ms;
}

__attribute__((weak)) void bk_cp_hang_dump_by_ap_feed_aon_wdt(void)
{
	uint32_t period_ms = cp_hang_effective_timeout_ms() + CP_HANG_TIMEOUT_MARGIN_MS;
	uint32_t ctrl_val;

	if (period_ms > CP_HANG_AON_WDT_PERIOD_MAX) {
		period_ms = CP_HANG_AON_WDT_PERIOD_MAX;
	}

	ctrl_val = (period_ms & CP_HANG_AON_WDT_PERIOD_LOW_MASK) |
		((period_ms & CP_HANG_AON_WDT_PERIOD_HIGH_MASK) << CP_HANG_AON_WDT_PERIOD_HIGH_SHIFT) |
		(CP_HANG_AON_WDT_KEY_1ST << CP_HANG_AON_WDT_KEY_SHIFT);
	REG_WRITE(SOC_AON_WDT_REG_BASE, ctrl_val);

	ctrl_val = (period_ms & CP_HANG_AON_WDT_PERIOD_LOW_MASK) |
		((period_ms & CP_HANG_AON_WDT_PERIOD_HIGH_MASK) << CP_HANG_AON_WDT_PERIOD_HIGH_SHIFT) |
		(CP_HANG_AON_WDT_KEY_2ND << CP_HANG_AON_WDT_KEY_SHIFT);
	REG_WRITE(SOC_AON_WDT_REG_BASE, ctrl_val);
}

static inline uint32_t cp_hang_now(void)
{
	return (uint32_t)rtos_get_time();
}

static inline uint32_t cp_hang_elapsed(uint32_t now, uint32_t last)
{
	return (now >= last) ? (now - last) : (now + (~last) + 1U);
}

static inline void cp_hang_feed_watchdog(void)
{
	bk_cp_hang_dump_by_ap_feed_aon_wdt();
}

static void cp_hang_stop_other_ap_cores(void)
{
#if CONFIG_SOC_SMP
	uint32_t core_id = rtos_get_core_id();

	if (core_id == CPU2_CORE_ID) {
		bk_multicore_stop(CPU3_CORE_ID);
	} else if (core_id == CPU3_CORE_ID) {
		bk_multicore_stop(CPU2_CORE_ID);
	} else {
		bk_coredump_write_prompt("warning: unexpected AP core id %lu, cannot stop peer core\r\n",
			(unsigned long)core_id);
	}
#endif
}

static void cp_hang_reboot(void)
{
	cp_hang_feed_watchdog();
	bk_reboot_ex(RESET_SOURCE_CRASH_ASSERT);
	while (1) {
	}
}

static void cp_hang_ipi_callback(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param)
{
	(void)core_id;
	(void)value;
	(void)param;

	if (event != CP_HANG_HEARTBEAT_EVENT) {
		return;
	}

	s_cp_hang_state.timeout_ms = payload ? payload : CP_HANG_TIMEOUT_FALLBACK_MS;
	s_cp_hang_state.src_cpu = src_cpu;
	s_cp_hang_state.last_tick = cp_hang_now();
	s_cp_hang_state.seen = 1U;
}

static void cp_hang_dump_window(const char *name, uint32_t start, uint32_t size)
{
	if ((start == 0U) || (size == 0U)) {
		return;
	}

	cp_hang_feed_watchdog();
	bk_coredump_write_memory(name, start, start + size);
	cp_hang_feed_watchdog();
}

static void cp_hang_dump_observer_context(uint32_t now)
{
	uint32_t last_tick = s_cp_hang_state.last_tick;
	uint16_t timeout_ms = (uint16_t)cp_hang_effective_timeout_ms();
	uint8_t src_cpu = s_cp_hang_state.src_cpu;

	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	bk_coredump_write_prompt("******************************CP heartbeat timeout observed by AP******************************\r\n");
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	bk_coredump_write_prompt("observer=AP target=CP reason=cp_heartbeat_timeout confidence=second_scene\r\n");
	bk_coredump_write_prompt("cp_hb last_tick=%lu now=%lu elapsed=%lu timeout_ms=%u src_cpu=%u\r\n",
		(unsigned long)last_tick, (unsigned long)now,
		(unsigned long)cp_hang_elapsed(now, last_tick),
		timeout_ms, src_cpu);
	bk_coredump_write_prompt("ipi_status_all=0x%08lx ipi_device_status=0x%08lx\r\n",
		(unsigned long)bk_ipi_get_all_status(),
		(unsigned long)bk_ipi_get_device_status());
}

static void cp_hang_dump_peripheral_context(void)
{
#if defined(SOC_SYS_REG_BASE)
	cp_hang_dump_window("CP_HANG_SYS", (uint32_t)SOC_SYS_REG_BASE, 0x5cU * 4U);
#endif
#if defined(SOC_SYS_AHBP_REG_BASE)
	cp_hang_dump_window("CP_HANG_SYS_AHBP", (uint32_t)SOC_SYS_AHBP_REG_BASE, 0x60U * 4U);
#endif
#if defined(SOC_AON_PMU_REG_BASE)
	cp_hang_dump_window("CP_HANG_AON_PMU", (uint32_t)SOC_AON_PMU_REG_BASE, 0x7fU * 4U);
#endif
#if defined(SOC_AON_RTC_REG_BASE)
	cp_hang_dump_window("CP_HANG_AON_RTC", (uint32_t)SOC_AON_RTC_REG_BASE, 0x0aU * 4U);
#endif
#if defined(SOC_MBOX0_REG_BASE)
	cp_hang_dump_window("CP_HANG_MBOX0", (uint32_t)SOC_MBOX0_REG_BASE, 0x38U * 4U);
#endif
#if defined(SOC_WDT_REG_BASE)
	cp_hang_dump_window("CP_HANG_WDT", (uint32_t)SOC_WDT_REG_BASE, 0x20U * 4U);
#endif
#if defined(SOC_PPHS_REG_BASE)
	cp_hang_dump_window("CP_HANG_PPHS", (uint32_t)SOC_PPHS_REG_BASE, 0x10U * 4U);
#endif
#if defined(SOC_PPRO_REG_BASE)
	cp_hang_dump_window("CP_HANG_PPRO", (uint32_t)SOC_PPRO_REG_BASE, 0x24U * 4U);
#endif
}

static void cp_hang_dump_memory_context(void)
{
#if CONFIG_CP_HANG_DUMP_BY_AP_MEMDUMP
#if defined(CONFIG_CP_RAM_ADDR) && defined(CONFIG_CP_RAM_SIZE) && CONFIG_CP_RAM_SIZE
	cp_hang_dump_window("CP_RAM", (uint32_t)CONFIG_CP_RAM_ADDR, (uint32_t)CONFIG_CP_RAM_SIZE);
#endif
#if defined(CONFIG_CP_PSRAM_HEAP_ADDR) && defined(CONFIG_CP_PSRAM_HEAP_SIZE) && CONFIG_CP_PSRAM_HEAP_SIZE
	cp_hang_dump_window("CP_PSRAM_HEAP", (uint32_t)CONFIG_CP_PSRAM_HEAP_ADDR,
		(uint32_t)CONFIG_CP_PSRAM_HEAP_SIZE);
#endif
#else
	bk_coredump_write_prompt("CP memory dump skipped: CONFIG_CP_HANG_DUMP_BY_AP_MEMDUMP=0\r\n");
#endif
}

static void cp_hang_dump_prompt_prologue(void)
{
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	bk_coredump_write_prompt("***********************************user except handler begin***********************************\r\n");
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
}

static void cp_hang_dump_current_context(void)
{
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t lr = __get_LR();
	uint32_t pc = (uint32_t)__builtin_return_address(0);

	__asm volatile("mov %0, r0" : "=r"(r0));
	__asm volatile("mov %0, r1" : "=r"(r1));
	__asm volatile("mov %0, r2" : "=r"(r2));
	__asm volatile("mov %0, r3" : "=r"(r3));
	__asm volatile("mov %0, r4" : "=r"(r4));
	__asm volatile("mov %0, r5" : "=r"(r5));
	__asm volatile("mov %0, r6" : "=r"(r6));
	__asm volatile("mov %0, r7" : "=r"(r7));
	__asm volatile("mov %0, r8" : "=r"(r8));
	__asm volatile("mov %0, r9" : "=r"(r9));
	__asm volatile("mov %0, r10" : "=r"(r10));
	__asm volatile("mov %0, r11" : "=r"(r11));
	__asm volatile("mov %0, r12" : "=r"(r12));

	bk_coredump_write_meta_info(COREDUMP_REGISTERS_INFO, (void *)rtos_get_core_id());
	bk_coredump_write_registers("0 r0", r0);
	bk_coredump_write_registers("1 r1", r1);
	bk_coredump_write_registers("2 r2", r2);
	bk_coredump_write_registers("3 r3", r3);
	bk_coredump_write_registers("4 r4", r4);
	bk_coredump_write_registers("5 r5", r5);
	bk_coredump_write_registers("6 r6", r6);
	bk_coredump_write_registers("7 r7", r7);
	bk_coredump_write_registers("8 r8", r8);
	bk_coredump_write_registers("9 r9", r9);
	bk_coredump_write_registers("10 r10", r10);
	bk_coredump_write_registers("11 r11", r11);
	bk_coredump_write_registers("12 r12", r12);
	bk_coredump_write_registers("14 sp", __get_PSP());
	bk_coredump_write_registers("15 lr", lr);
	bk_coredump_write_registers("16 pc", pc);
	bk_coredump_write_registers("17 xpsr", __get_xPSR());
	bk_coredump_write_registers("18 msp", __get_MSP());
	bk_coredump_write_registers("19 psp", __get_PSP());
	bk_coredump_write_registers("20 primask", __get_PRIMASK());
	bk_coredump_write_registers("21 basepri", __get_BASEPRI());
	bk_coredump_write_registers("22 faultmask", __get_FAULTMASK());
	bk_coredump_write_registers("23 fpscr", __get_FPSCR());
	bk_coredump_write_registers("31 ER", 0xfffffffdU);
	bk_coredump_write_registers("32 control", __get_CONTROL());
	bk_coredump_write_registers("40 MMFAR", SCB->MMFAR);
	bk_coredump_write_registers("41 BFAR", SCB->BFAR);
	bk_coredump_write_registers("42 CFSR", SCB->CFSR);
	bk_coredump_write_registers("43 HFSR", SCB->HFSR);
	bk_coredump_write_prompt("Traceback:\r\n");
	bk_coredump_write_prompt("\tarm-none-eabi-addr2line -piaf -e app.elf 0x%08lx 0x%08lx \r\n",
		(unsigned long)pc, (unsigned long)lr);
}

static void cp_hang_dump_from_ap(uint32_t now)
{
	uint64_t dump_time_us = bk_aon_rtc_get_us();

	if (now == 0U) {
		now = cp_hang_now();
	}

#if CONFIG_SUPPORT_WWDT
	bk_wwdt_driver_deinit();
#endif
	cp_hang_feed_watchdog();
	rtos_disable_int();
	cp_hang_stop_other_ap_cores();
	bk_set_printf_sync(true);

	cp_hang_set_ap_dumping(1U);
	bk_coredump_writer_init();
	bk_coredump_dump_time(dump_time_us);
	bk_coredump_write_meta_info(COREDUMP_EXCEPTION_INFO, (void *)"Assert");
	bk_coredump_write_meta_info(COREDUMP_BUILD_INFO, (void *)build_version);
#if CONFIG_SOC_SMP
	bk_coredump_write_meta_info(COREDUMP_CORE_INFO, (void *)(rtos_get_core_id() & 0x1));
#endif
	cp_hang_dump_observer_context(now);
	cp_hang_dump_current_context();
	cp_hang_dump_prompt_prologue();

	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	bk_coredump_write_prompt("*************************************CP memory dump begin**************************************\r\n");
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	cp_hang_dump_peripheral_context();
	cp_hang_dump_memory_context();
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	bk_coredump_write_prompt("**************************************CP memory dump end***************************************\r\n");
	bk_coredump_write_prompt("***********************************************************************************************\r\n");
	cp_hang_feed_watchdog();
	bk_coredump_writer_deinit();
	cp_hang_set_ap_dumping(0U);
	cp_hang_reboot();
}

static void cp_hang_monitor_task(void *param)
{
	uint32_t last_check;

	(void)param;

	last_check = cp_hang_now();

	while (1) {
		uint32_t now;
		uint32_t last_tick;
		uint32_t loop_gap;

		rtos_delay_milliseconds(CP_HANG_MONITOR_CHECK_MS);

		now = cp_hang_now();
		/* AP self power-down / LV deep-sleep guard: while the AP core is powered
		 * off the monitor task cannot be scheduled, so on resume rtos_get_time()
		 * jumps far past the fixed check interval. During that window the CP has
		 * deliberately paused its heartbeat (it waits for "AP power on"), so a
		 * stale last_tick must NOT be treated as a CP timeout. Rebase the
		 * heartbeat reference to now and grant one full timeout window of grace. */
		loop_gap = cp_hang_elapsed(now, last_check);
		last_check = now;
		if (loop_gap > CP_HANG_MONITOR_WAKE_JUMP_MS) {
			s_cp_hang_state.last_tick = now;
			continue;
		}

		if ((s_cp_hang_state.seen == 0U) || (s_cp_hang_state.dumping != 0U)) {
			continue;
		}

		last_tick = s_cp_hang_state.last_tick;
		if (cp_hang_elapsed(now, last_tick) < cp_hang_effective_timeout_ms()) {
			continue;
		}

		s_cp_hang_state.dumping = 1U;
		s_cp_hang_state.timeout_tick = now;
		/* Publish "AP is taking over due to CP hang" before any logging so the
		 * shell log path stops forwarding to the (dead) CP over the mailbox and
		 * the dump can go straight out the UART. */
		cp_hang_set_ap_dumping(1U);
		BK_LOGE(CP_HANG_TAG, "CP heartbeat timeout: last=%u now=%u timeout=%u src=%u\r\n",
			last_tick, now, (unsigned)cp_hang_effective_timeout_ms(), s_cp_hang_state.src_cpu);
		cp_hang_dump_from_ap(now);
	}
}

bk_err_t bk_cp_hang_dump_by_ap_init(void)
{
	bk_err_t ret;

	os_memset((void *)&s_cp_hang_state, 0, sizeof(s_cp_hang_state));
	s_cp_hang_state.timeout_ms = CP_HANG_TIMEOUT_FALLBACK_MS;
	cp_hang_set_ap_dumping(0U);

	ret = bk_ipi_register_domain_callback(IPI_DOMAIN_CP_HANG_DEBUG,
		cp_hang_ipi_callback, NULL);
	if (ret != BK_OK) {
		BK_LOGE(CP_HANG_TAG, "register cp hang IPI callback failed: %d\r\n", ret);
		return ret;
	}

	ret = rtos_create_thread(NULL, CP_HANG_MONITOR_PRIORITY,
		"cp_hang_mon", cp_hang_monitor_task,
		CP_HANG_MONITOR_STACK_SIZE, NULL);
	if (ret != BK_OK) {
		BK_LOGE(CP_HANG_TAG, "create cp hang monitor task failed: %d\r\n", ret);
	}

	return ret;
}
