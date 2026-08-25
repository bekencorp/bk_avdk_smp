#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "bk_coredump.h"
#include "bk_dump_manifest.h"
#include "bk_arch.h"
#include "os/mem.h"
#include "reg_base.h"
#include "bk_rtos_debug.h"
#include <driver/aon_rtc.h>
#include "multicore_driver.h"
#include "mb_ipc_cmd.h"
#include "sys_sw_regs.h"
#include "sys_ahbp_ll.h"
#include "memory.h"
#include "cache.h"
#include <components/log.h>

/* P1-2: bounded wait to confirm the peer AP core actually entered reset via
 * reset-status readback. Fixed behaviour; timeout is a board-tuned constant. */
#ifndef COREDUMP_STOP_READBACK_TIMEOUT_US
#define COREDUMP_STOP_READBACK_TIMEOUT_US 2000U
#endif

#if CONFIG_SUPPORT_WWDT
#include <driver/wwdt.h>
#include "wwdt_driver.h"
#endif

#define BK_EXCEPTION_MAGIC 0xA55AA55A
#define BK_ASSERT_MAGIC 0x55AA55AA
#define COREDUMP_IRAM __attribute__((section(".iram"), noinline))
#define PSRAM_CODE_COMPARE_GRANULARITY 32U
static volatile bk_assert_info_t s_bk_assert_info;
static volatile uint32_t s_bk_exception_magic = 0;
static volatile uint32_t s_core_id = 0;

static hook_func s_wifi_dump_func = NULL;
static hook_func s_ble_dump_func = NULL;

#if CONFIG_INTERRUPT_DEBUG_RECORDER
extern void bk_interrupt_dump_recorder(void);
#endif

static inline void coredump_feed_watchdogs(void)
{
#if CONFIG_SUPPORT_WWDT
    bk_wwdt_force_feed();
#endif
}

void bk_coredump_dump_time(uint64_t time_us)
{
    BK_DUMP_OUT("@Dump-time(AON-RTC): %llu us\r\n", (unsigned long long)time_us);
}

bool bk_check_assert(void)
{
    if (s_bk_assert_info.magic == BK_ASSERT_MAGIC &&
        s_bk_assert_info.func != NULL &&
        s_bk_assert_info.line != 0) {
        return true;
    }
    return false;
}

/* AP-core reset-status readback (M0-confirmed registers, active-LOW: 0 = in
 * reset). CPU2 = AP core0 (reg4 cpu0_sw_rstn), CPU3 = AP core1 (reg5
 * cpu1_sw_rstn). */
static inline bool ap_cpu2_is_reset(void)
{
    return sys_ahbp_ll_get_reg4_cpu0_sw_rstn() == 0U;
}
static inline bool ap_cpu3_is_reset(void)
{
    return sys_ahbp_ll_get_reg5_cpu1_sw_rstn() == 0U;
}

static bool coredump_wait_reset_confirmed(bool (*is_reset)(void))
{
    uint64_t start_us = bk_aon_rtc_get_us();

    while (!is_reset()) {
        if ((bk_aon_rtc_get_us() - start_us) >= COREDUMP_STOP_READBACK_TIMEOUT_US) {
            return is_reset();
        }
        coredump_feed_watchdogs();
    }
    return true;
}

static inline void coredump_stop_other_cores(void)
{
    // smp needs stop other cores
#if CONFIG_SOC_SMP
    uint32_t core_id = rtos_get_core_id();

    if (core_id == CPU2_CORE_ID) {
        bk_multicore_stop(CPU3_CORE_ID);
        /* P1-2: confirm the peer AP core entered reset before dumping shared AP
         * memory (same power domain, so best-effort log-only downgrade). */
        if (coredump_wait_reset_confirmed(ap_cpu3_is_reset)) {
            BK_DUMP_OUT("@STOP_CONFIRMED core=CPU3\r\n");
        } else {
            BK_DUMP_OUT("@STOP_UNCONFIRMED core=CPU3\r\n");
        }
    } else if (core_id == CPU3_CORE_ID) {
        bk_multicore_stop(CPU2_CORE_ID);
        if (coredump_wait_reset_confirmed(ap_cpu2_is_reset)) {
            BK_DUMP_OUT("@STOP_CONFIRMED core=CPU2\r\n");
        } else {
            BK_DUMP_OUT("@STOP_UNCONFIRMED core=CPU2\r\n");
        }
    } else {
        BK_DUMP_OUT("warning: unexpected AP core id %u, cannot stop peer core\r\n", core_id);
    }
#endif
}


static void bk_exception_preprocess(bk_exception_t *self)
{
    bool secondary;

    rtos_disable_int();

    /* Mark "in exception" BEFORE taking any resource lock, so the HSPL/SSPL
     * lock layer (see arch_is_enter_exception()) skips its blocking/assert path
     * while a peer core might still hold a shared lock. Otherwise the UART_LOG
     * lock taken by bk_coredump_lock() below could spin/assert and trigger a
     * secondary exception. */
    secondary = (s_bk_exception_magic == BK_EXCEPTION_MAGIC);
    s_bk_exception_magic = BK_EXCEPTION_MAGIC;
    s_core_id = rtos_get_core_id();

    bk_coredump_lock();
    if (secondary) {
        BK_DUMP_OUT("A secondary exception occurred, reset_reason: 0x%x\r\n", self->reset_reason);
        bk_reboot_ex(self->reset_reason);
    }
    coredump_stop_other_cores();

    coredump_feed_watchdogs();
    bk_misc_set_reset_reason(self->reset_reason);

    bk_set_printf_sync(true);
#if CONFIG_SHELL_ASYNCLOG
    BK_LOG_FLUSH();
#endif
}

// print fault type
void bk_coredump_fault_type(void)
{
    if (bk_check_assert()) {
        bk_coredump_write_meta_info(COREDUMP_EXCEPTION_INFO, (void *)"Assert");
        bk_coredump_write_meta_info(COREDUMP_ASSERT_INFO, (void *)&s_bk_assert_info);
        memset((void *)&s_bk_assert_info, 0, sizeof(bk_assert_info_t)); // clear assert info
    } else {
        bk_coredump_write_meta_info(COREDUMP_EXCEPTION_INFO, (void *)bk_coredump_get_fault_type());
    }
}

extern volatile const uint8_t build_version[];
void bk_coredump_meta_info(void)
{
    bk_coredump_fault_type();
    bk_coredump_write_meta_info(COREDUMP_BUILD_INFO, (void *)build_version);

#if CONFIG_SOC_SMP
    bk_coredump_write_meta_info(COREDUMP_CORE_INFO, (void *)(portGET_CORE_ID() & 0x1));
#endif
}

static inline void coredump_prompt_prologue(void)
{
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    bk_coredump_write_prompt("***********************************user except handler begin***********************************\r\n");
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
}

static inline void coredump_prompt_epilogue(void)
{
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    bk_coredump_write_prompt("************************************user except handler end************************************\r\n");
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
}

extern void bk_dump_peri_regs(void);

static void coredump_prompt_info(void)
{

#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
    // dump memory heap stats
    os_dump_memory_stats(0, 0, NULL);
#endif

#if CONFIG_INTERRUPT_DEBUG_RECORDER
    // bk_interrupt_dump_recorder();
#endif
    /* Snapshot bus-master and bus-slave controller registers (HPDMA, ISP,
     * H26E, PSRAM0/1) before backtrace, so that even if the CP-side AP
     * memory pull fails (severe bus hang), the AP's own UART dump still
     * captures the peripherals most relevant to AXI/PSRAM stalls. */
    bk_dump_peri_regs();
    rtos_dump_backtrace();
    rtos_dump_task_list();
#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif
}

static void coredump_notify_cp_begin(void)
{
#if (CONFIG_CPU_CNT > 1)
    if (ipc_send_trap_handle_begin() != BK_OK) {
        BK_DUMP_OUT("warning: notify CP trap begin failed\r\n");
    }
#endif
}

static bk_err_t coredump_notify_cp_end(void)
{
#if (CONFIG_CPU_CNT > 1)
    /* P0-1: propagate the handoff result so the caller can fall back to an AP
     * self-dump + reset when the CP does not accept the trap-end request. */
    bk_err_t ret = ipc_send_trap_handle_end();
    if (ret != BK_OK) {
        BK_DUMP_OUT("warning: notify CP trap end failed, ret=%d\r\n", ret);
    }
    return ret;
#else
    return BK_OK;
#endif
}

static COREDUMP_IRAM bool coredump_memory_is_different(uint32_t run_addr, uint32_t load_addr, uint32_t size)
{
    uint32_t offset = 0;

    while ((offset + sizeof(uint32_t)) <= size) {
        volatile const uint32_t *run = (volatile const uint32_t *)(run_addr + offset);
        volatile const uint32_t *load = (volatile const uint32_t *)(load_addr + offset);

        if (*run != *load) {
            return true;
        }
        offset += sizeof(uint32_t);
    }

    while (offset < size) {
        volatile const uint8_t *run = (volatile const uint8_t *)(run_addr + offset);
        volatile const uint8_t *load = (volatile const uint8_t *)(load_addr + offset);

        if (*run != *load) {
            return true;
        }
        offset++;
    }

    return false;
}

static COREDUMP_IRAM bool coredump_publish_ap_extra_dump_range(uint32_t start_addr, uint32_t size)
{
    ap_extra_dump_info_t info = {0};

    for (uint32_t i = 0; i < BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX; i++) {
        if (bk_sys_sw_regs_get_ap_extra_dump(i, &info)) {
            if ((info.start_addr == start_addr) && (info.size == size)) {
                return true;
            }
            continue;
        }

        bk_sys_sw_regs_update_ap_extra_dump(i, start_addr, size);
        return true;
    }

    return false;
}

static COREDUMP_IRAM void coredump_report_psram_code_range(uint32_t start_addr, uint32_t end_addr,
    uint32_t range_index, uint32_t *published_count, uint32_t *dropped_count)
{
    uint32_t size = end_addr - start_addr;
    bool published = coredump_publish_ap_extra_dump_range(start_addr, size);

    if (published) {
        (*published_count)++;
    } else {
        (*dropped_count)++;
    }

    BK_DUMP_OUT("AP_PSRAM_CODE modified range[%lu]: start=0x%08lx, end=0x%08lx, size=0x%08lx, publish=%lu\r\n",
        range_index, start_addr, end_addr, size, published ? 1UL : 0UL);
}

static COREDUMP_IRAM void coredump_check_psram_code(void)
{
#if CONFIG_PSRAM
    bk_psram_code_info_t info = {0};
    uint32_t range_start = 0;
    uint32_t range_end = 0;
    uint32_t range_count = 0;
    uint32_t published_count = 0;
    uint32_t dropped_count = 0;

    bk_get_psram_code_info(&info);
    if ((info.run_addr == 0U) || (info.load_addr == 0U) || (info.size == 0U)) {
        BK_DUMP_OUT("AP_PSRAM_CODE empty window, skip compare\r\n");
        return;
    }

#if CONFIG_CACHE_MAINTENANCE
    arch_dcache_flush_and_invd_range((void *)info.run_addr, info.size);
    __DSB();
    BK_DUMP_OUT("AP_PSRAM_CODE compare after dcache clean-invalidate, run=0x%08lx, load=0x%08lx, size=0x%08lx\r\n",
        info.run_addr, info.load_addr, info.size);
#else
    BK_DUMP_OUT("AP_PSRAM_CODE compare, run=0x%08lx, load=0x%08lx, size=0x%08lx\r\n",
        info.run_addr, info.load_addr, info.size);
#endif

    for (uint32_t offset = 0; offset < info.size; offset += PSRAM_CODE_COMPARE_GRANULARITY) {
        uint32_t chunk_size = info.size - offset;
        uint32_t chunk_start = info.run_addr + offset;

        if (chunk_size > PSRAM_CODE_COMPARE_GRANULARITY) {
            chunk_size = PSRAM_CODE_COMPARE_GRANULARITY;
        }

        if (coredump_memory_is_different(chunk_start, info.load_addr + offset, chunk_size)) {
            if (range_start == 0U) {
                range_start = chunk_start;
            }
            range_end = chunk_start + chunk_size;
        } else if (range_start != 0U) {
            coredump_report_psram_code_range(range_start, range_end, range_count,
                &published_count, &dropped_count);
            range_count++;
            range_start = 0;
            range_end = 0;
        }
    }

    if (range_start != 0U) {
        coredump_report_psram_code_range(range_start, range_end, range_count,
            &published_count, &dropped_count);
        range_count++;
    }

    BK_DUMP_OUT("AP_PSRAM_CODE compare done, modified_ranges=%lu, published=%lu, dropped=%lu\r\n",
        range_count, published_count, dropped_count);
#else
    BK_DUMP_OUT("AP_PSRAM_CODE compare skipped, PSRAM disabled\r\n");
#endif
}

static COREDUMP_IRAM void coredump_flush_for_cp_dump(void)
{
#if CONFIG_CACHE_MAINTENANCE
    arch_dcache_flush_all();
    __DSB();
    BK_DUMP_OUT("AP coredump dcache flushed before CP RAM dump\r\n");
#endif
}

static void coredump_publish_ap_psram_windows(void)
{
#if CONFIG_PSRAM
    bk_dump_mem_info_t mem_info = {0};

    /*
     * Publish the PSRAM .data/.bss section ranges into the shared-reg slots the
     * CP dumps. The CP reads AP_HEAP_SRAM as "AP_PSRAM_DATA" and AP_HEAP_HSRAM
     * as "AP_PSRAM_BSS", so publish .psram.data to the SRAM slot and .psram.bss
     * to the HSRAM slot. The published ranges are unchanged versus before; only
     * the getter name<->range mapping was made self-consistent (see memory.c).
     */
    bk_get_psram_data_info(&mem_info);
    if ((mem_info.start_addr != 0U) && (mem_info.size != 0U)) {
        bk_sys_sw_regs_update_ap_heap_dump(BK_SYS_SW_REGS_AP_HEAP_SRAM,
            mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }

    bk_get_psram_bss_info(&mem_info);
    if ((mem_info.start_addr != 0U) && (mem_info.size != 0U)) {
        bk_sys_sw_regs_update_ap_heap_dump(BK_SYS_SW_REGS_AP_HEAP_HSRAM,
            mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
#endif
}

/* P0-1: AP waits for the CP to close the AP power domain and reset the board
 * after a successful handoff. Bound the wait (< AON-WDT 8000ms) so a dead/slow
 * CP cannot hang the AP forever. On a successful CP reboot the chip resets and
 * this never returns; returning means the CP did NOT reboot within the budget,
 * i.e. the AP must fall back to a self-dump + reset.
 *
 * TUNING (FI-2, board-measured): the budget MUST exceed the worst-case healthy
 * CP AP-memory dump time, otherwise a slow-but-alive CP causes the AP to also
 * self-dump (double dump / UART collision); it MUST stay below the AON-WDT
 * period, otherwise the AON-WDT resets the board before the AP fallback runs. */
#ifndef CONFIG_AP_HANDOFF_CP_REBOOT_BUDGET_MS
#define CONFIG_AP_HANDOFF_CP_REBOOT_BUDGET_MS 6000U
#endif
#define AP_HANDOFF_CP_REBOOT_BUDGET_MS  ((uint32_t)CONFIG_AP_HANDOFF_CP_REBOOT_BUDGET_MS)

static void ap_wait_cp_reboot(uint32_t budget_ms)
{
    uint64_t start_us = bk_aon_rtc_get_us();
    uint64_t budget_us = (uint64_t)budget_ms * 1000ULL;

    while ((bk_aon_rtc_get_us() - start_us) < budget_us) {
        coredump_feed_watchdogs();
    }
}

static void bk_exception_dump_main(bk_exception_t *self)
{
    bk_err_t handoff;

    bk_coredump_writer_init();

    if (self->secure_context != NULL &&
        self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        bk_coredump_write_meta_info(COREDUMP_EXCEPTION_INFO, (void *)"SecureFault");
        bk_coredump_write_meta_info(COREDUMP_BUILD_INFO, (void *)build_version);
#if CONFIG_SOC_SMP
        bk_coredump_write_meta_info(COREDUMP_CORE_INFO,
            (void *)(self->secure_context->core_id & 0x1U));
#endif
    } else {
        bk_coredump_meta_info();
    }
    bk_coredump_write_prompt("@dump_format_version: %u\r\n", (unsigned)BK_DUMP_FORMAT_VERSION);
    bk_coredump_dump_time(self->exception_time_us);

    if (self->secure_context != NULL) {
        bk_coredump_secure_registers(self->secure_context);
    } else {
        bk_coredump_registers(self);
    }

    coredump_publish_ap_psram_windows();
#if CONFIG_DEBUG_VERSION
    /* PSRAM code compare is an engineering forensic probe: Debug builds only. */
    coredump_check_psram_code();
#endif
    coredump_notify_cp_begin();

    coredump_prompt_prologue();

    // coredump_prompt_info();

#if CONFIG_CM_BACKTRACE
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT &&
        self->secure_context == NULL) {
        cm_backtrace_fault(self->lr, self->sp);
    }
#endif

    bk_coredump_writer_deinit();

    coredump_flush_for_cp_dump();          /* flush PSRAM L2 before CP reads AP RAM */
    handoff = coredump_notify_cp_end();    /* prefer handoff: CP dumps AP mem + resets */

    if (handoff == BK_OK) {
        ap_wait_cp_reboot(AP_HANDOFF_CP_REBOOT_BUDGET_MS);
    }

    /* Reached here => the CP rejected the handoff, or did not reset the board
     * within the budget (CP hung). Take over: AP self-dumps its full memory and
     * resets deterministically so the failure is never a silent no-dump hang. */
    BK_DUMP_OUT("@AP_HANDOFF_FAILED: AP self-dump full memory then reset\r\n");
    bk_coredump_writer_init();             /* re-acquire UART lock (released above) */
    bk_coredump_self_full_memory();        /* manifest(AP), local reads (path 4) */
    coredump_prompt_epilogue();            /* end marker before any (debug-only) probe */
    bk_coredump_writer_deinit();
    coredump_feed_watchdogs();
    bk_reboot_ex(self->reset_reason);      /* AP is the sole reset issuer here */
}

static void bk_exception_postprocess(bk_exception_t *self)
{
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
#if CONFIG_SUPPORT_WWDT
    bk_wwdt_driver_deinit();
#endif
#else
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        BK_LOG_FLUSH();
    }
    bk_reboot_ex(self->reset_reason);
#endif
}

static void bk_exception_handler_common(bk_exception_t *exception)
{
    bk_exception_preprocess(exception);
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    bk_exception_dump_main(exception);
#endif
    bk_exception_postprocess(exception);
}

void bk_exception_handler(uint32_t reset_reason, uint32_t lr, uint32_t sp)
{
    /* Capture the AON-RTC time first, so it reflects the exception moment as
     * closely as possible (before peer-core stop / wdt feed in preprocess). */
    uint64_t exception_time_us = bk_aon_rtc_get_us();

    if (bk_check_assert()) {
        reset_reason = RESET_SOURCE_CRASH_ASSERT;
    }
    /* Capture the special registers here, before bk_exception_preprocess()
     * disables interrupts, so PRIMASK/BASEPRI/FAULTMASK/CONTROL reflect the
     * real pre-exception state instead of the dump handler's own state. */
    bk_exception_t exception = {
        .lr = lr,
        .sp = sp,
        .reset_reason = reset_reason,
        .primask = __get_PRIMASK(),
        .basepri = __get_BASEPRI(),
        .faultmask = __get_FAULTMASK(),
        .control = __get_CONTROL(),
        .secure_context = NULL,
        .exception_time_us = exception_time_us,
    };
    bk_exception_handler_common(&exception);
}

void bk_exception_handler_from_secure(const ap_secure_fault_context_t *context)
{
    uint32_t reset_reason = bk_check_assert()
        ? RESET_SOURCE_CRASH_ASSERT
        : RESET_SOURCE_SECURE_FAULT;

    bk_exception_t exception = {
        .lr = context->exception_lr,
        .sp = context->frame_sp,
        .reset_reason = reset_reason,
        .primask = context->primask_ns,
        .basepri = context->basepri_ns,
        .faultmask = context->faultmask_ns,
        .control = context->control_ns,
        .secure_context = context,
        .exception_time_us = bk_aon_rtc_get_us(),
    };

    bk_exception_handler_common(&exception);
}

void bk_assert_handler(const char *func, int line)
{
    s_bk_assert_info.magic = BK_ASSERT_MAGIC;
    s_bk_assert_info.func = func;
    s_bk_assert_info.line = line;
    // cppcheck-suppress nullPointer
    *((volatile int *) 0) = 0;   // trigger exception
}

unsigned int arch_is_enter_exception(void)
{
    return s_bk_exception_magic == BK_EXCEPTION_MAGIC && s_core_id == rtos_get_core_id();
}

#define MAX_DUMP_SYS_MEM_COUNT 8
static bk_mem_addr_t s_dump_sys_mem_info[MAX_DUMP_SYS_MEM_COUNT] = {0};
void rtos_regist_plat_dump_hook(uint32_t mem_base_addr, uint32_t mem_size)
{
    if (mem_base_addr >= SOC_SRAM0_DATA_BASE
        && (mem_base_addr + mem_size) < SOC_SRAM_DATA_END) {
        return;
    }
    if ((mem_base_addr & 0x3) != 0 || (mem_size & 0x3) != 0) {
        return;
    }
    for (int i = 0; i < MAX_DUMP_SYS_MEM_COUNT; i++) {
        if (s_dump_sys_mem_info[i].start_addr == 0 && s_dump_sys_mem_info[i].size == 0) {
            s_dump_sys_mem_info[i].start_addr = mem_base_addr;
            s_dump_sys_mem_info[i].size = mem_size;
            bk_sys_sw_regs_update_ap_extra_dump((uint32_t)i, mem_base_addr, mem_size);
            return;
        }
    }
}

uint32_t bk_get_dump_sys_mem_count(void)
{
    for (int i = 0; i < MAX_DUMP_SYS_MEM_COUNT; i++) {
        if (s_dump_sys_mem_info[i].start_addr == 0 && s_dump_sys_mem_info[i].size == 0) {
            /* i is the first empty slot, i.e. the number of registered
             * regions [0..i-1]. Returning i+1 used to dump one extra empty
             * region (0-length EXTRA_MEM). */
            return i;
        }
    }
    /* Table is full: every slot holds a registered region. */
    return MAX_DUMP_SYS_MEM_COUNT;
}

bk_mem_addr_t *bk_get_dump_sys_mem_info(void)
{
    return s_dump_sys_mem_info;
}

void rtos_regist_wifi_dump_hook(hook_func wifi_func)
{
    s_wifi_dump_func = wifi_func;
}

void rtos_regist_ble_dump_hook(hook_func ble_func)
{
    s_ble_dump_func = ble_func;
}
