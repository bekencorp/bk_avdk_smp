#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "bk_coredump.h"
#include "bk_dump_manifest.h"
#include "bk_arch.h"
#include "wdt_driver.h"
#include "os/mem.h"
#include "os/os.h"
#include "reg_base.h"
#include "bk_rtos_debug.h"
#include <driver/aon_rtc.h>
#include <modules/pm.h>
#if CONFIG_SOC_SMP
#include "multicore_driver.h"
#endif
#include "multicore_hal.h"
#include "sys_ll.h"
#include "sys_ahbp_ll.h"

/* P1-2: bounded wait for the peer-core stop to be confirmed via reset-status
 * readback before trusting cross-core/cross-domain reads. Fixed behaviour (no
 * feature Kconfig); the timeout is a board-tuned constant (readback settle time
 * must be measured on the target, FI-11/FI-12). */
#ifndef COREDUMP_STOP_READBACK_TIMEOUT_US
#define COREDUMP_STOP_READBACK_TIMEOUT_US 2000U
#endif

#if CONFIG_SUPPORT_WWDT
#include <driver/wwdt.h>
#include "wwdt_driver.h"
#endif

#define BK_EXCEPTION_MAGIC 0xA55AA55A
#define BK_ASSERT_MAGIC 0x55AA55AA
#define RISCV_USB_CTRL_REG_ADDR (0x480A0714U + SOC_ADDR_OFFSET)
#define RISCV_USB_POWER_EN      (1U << 14)
#define RISCV_USB_CORE_EN       (1U << 15)
static volatile bk_assert_info_t s_bk_assert_info;
static volatile uint32_t s_bk_exception_magic = 0;
static volatile uint32_t s_core_id = 0;
volatile uint32_t g_ap_dump_flag = 0;

static hook_func s_wifi_dump_func = NULL;
static hook_func s_ble_dump_func = NULL;

void bk_coredump_feed_watchdogs(void)
{
#if CONFIG_WDT_EN
    bk_wdt_force_feed();
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

static inline void coredump_stop_riscv(void)
{
    /* The RISC-V core and its USB-HS control register live in the AP power
     * domain. Reading/writing that register while the AP domain is powered
     * down stalls the CP bus and would truncate the dump - the same hazard the
     * AP-memory probes guard against. If the AP never booted, the RISC-V core
     * is already off, so there is nothing to stop. */
    if (!bk_pm_ap_boot_success_get()) {
        return;
    }

    volatile uint32_t *riscv_ctrl = (volatile uint32_t *)RISCV_USB_CTRL_REG_ADDR;
    uint32_t value = *riscv_ctrl;

    value &= ~RISCV_USB_CORE_EN;
    *riscv_ctrl = value;
    __DSB();

    value &= ~RISCV_USB_POWER_EN;
    *riscv_ctrl = value;
    __DSB();
}

/* Reset-status readback (M0-confirmed registers). NOTE the differing polarity:
 * CPU1 sw_rst is active-HIGH (1 = in reset); the AP cores' sw_rstn are
 * active-LOW (0 = in reset). */
static inline bool cp_peer_cpu1_is_reset(void)
{
    return sys_ll_get_cpu1_int_halt_clk_op_cpu1_sw_rst() == 1U;   /* active-high */
}
static inline bool ap_cpu2_is_reset(void)
{
    return sys_ahbp_ll_get_reg4_cpu0_sw_rstn() == 0U;             /* active-low */
}
static inline bool ap_cpu3_is_reset(void)
{
    return sys_ahbp_ll_get_reg5_cpu1_sw_rstn() == 0U;             /* active-low */
}

/* Set when a stopped AP core could not be confirmed in reset: cross-domain AP
 * reads are then downgraded to safe register-only reads to avoid a bus stall. */
static volatile bool s_coredump_ap_stop_unconfirmed = false;

bool bk_coredump_ap_stop_unconfirmed(void)
{
    return s_coredump_ap_stop_unconfirmed;
}

static bool coredump_wait_reset_confirmed(bool (*is_reset)(void))
{
    uint64_t start_us = bk_aon_rtc_get_us();

    while (!is_reset()) {
        if ((bk_aon_rtc_get_us() - start_us) >= COREDUMP_STOP_READBACK_TIMEOUT_US) {
            return is_reset();
        }
        bk_coredump_feed_watchdogs();
    }
    return true;
}

static inline void coredump_stop_other_cores(void)
{
    // smp needs stop other cores
#if CONFIG_SOC_SMP
    uint32_t core_id = rtos_get_core_id();

    if (core_id == CPU0_CORE_ID) {
        bk_multicore_stop(CPU1_CORE_ID);
        /* P1-2: confirm the CP peer actually entered reset. */
        if (coredump_wait_reset_confirmed(cp_peer_cpu1_is_reset)) {
            BK_DUMP_OUT("@STOP_CONFIRMED core=CPU1\r\n");
        } else {
            BK_DUMP_OUT("@STOP_UNCONFIRMED core=CPU1\r\n");
        }
    } else if (core_id == CPU1_CORE_ID) {
        bk_multicore_stop(CPU0_CORE_ID);
        /* CPU0 exposes no sw_rst readback; best-effort stop only. */
    } else {
        BK_DUMP_OUT("warning: unexpected CP core id %u, cannot stop peer core\r\n", core_id);
    }
#endif

    coredump_stop_riscv();
    multicore_hal_stop(CPU2_CORE_ID);
    multicore_hal_stop(CPU3_CORE_ID);

    /* P1-2: confirm the AP cores stopped before the CP cross-reads AP memory.
     * Only meaningful when the AP domain is powered (otherwise the readback
     * itself would target a powered-down domain). */
    if (bk_pm_ap_boot_success_get()) {
        bool ap2 = coredump_wait_reset_confirmed(ap_cpu2_is_reset);
        bool ap3 = coredump_wait_reset_confirmed(ap_cpu3_is_reset);

        if (ap2 && ap3) {
            BK_DUMP_OUT("@STOP_CONFIRMED ap_cores (cpu2,cpu3)\r\n");
        } else {
            s_coredump_ap_stop_unconfirmed = true;
            BK_DUMP_OUT("@STOP_UNCONFIRMED ap_cores cpu2=%d cpu3=%d, downgrade AP cross-read to registers only\r\n",
                        (int)ap2, (int)ap3);
        }
    }
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

#if CONFIG_SUPPORT_WWDT
    bk_wwdt_driver_deinit();
#endif
    bk_coredump_feed_watchdogs();
    bk_misc_set_reset_reason(self->reset_reason);
    
    bk_set_printf_sync(true);  // set printf sync
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

static void coredump_prompt_info(void)
{

#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
    // dump memory heap stats
    os_dump_memory_stats(0, 0, NULL);
#endif

    rtos_dump_backtrace();
    rtos_dump_task_list();
#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif
}

static inline bool is_valid_function_addr(void *func)
{
    if (func <= (void *)0x20) {
        return false;
    }
    if (((uint32_t)func & 0x1) == 0) {  // valid thumb function addr is odd number.
        return false;
    }
    return true;
}

static void coredump_execute_hook_function(void)
{
    
    if (is_valid_function_addr(s_wifi_dump_func)) {
        bk_coredump_feed_watchdogs();
        s_wifi_dump_func();
        bk_coredump_feed_watchdogs();
    }
    if (is_valid_function_addr(s_ble_dump_func)) {
        bk_coredump_feed_watchdogs();
        s_ble_dump_func();
        bk_coredump_feed_watchdogs();
    }
}

static void bk_exception_dump_main(bk_exception_t *self)
{
    bk_coredump_feed_watchdogs();
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
    /* Header - ALWAYS emitted (Debug and Release): CPU registers, system info
     * (fault type / build / core) and the reboot reason. In a Release build
     * (CONFIG_DUMP_ENABLE=n and no CONFIG_DEBUG_VERSION) this header is the
     * ENTIRE dump: no memory image, no peripheral banks - then reboot. */
    bk_coredump_write_prompt("@dump_format_version: %u\r\n", (unsigned)BK_DUMP_FORMAT_VERSION);
    bk_coredump_dump_time(self->exception_time_us);
    bk_coredump_write_prompt("@reset-reason: 0x%x\r\n", self->reset_reason);

    if (self->secure_context != NULL) {
        bk_coredump_secure_registers(self->secure_context);
    } else {
        bk_coredump_registers(self);
    }

    coredump_prompt_prologue();

#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    /* Memory image + peripheral banks + task-list/backtrace + hang-prone probes:
     * Debug only (or where the self-exception dump is explicitly enabled). A
     * Release build skips ALL of this - registers + system info + reboot reason
     * are the whole dump (see header above). */
    bk_coredump_feed_watchdogs();
    bk_coredump_memory_essential();

    /* P0-4/P2-1: the wifi/ble forensic hooks are an engineering feature and are
     * gated by build version, not by the dump *level* (MEMDUMP_ALL). */
#if CONFIG_DEBUG_VERSION
    coredump_execute_hook_function();
#endif

    bk_coredump_memory_extended();
    /* Peripheral register banks only (safe). The hang-prone AP/PSRAM probes
     * are deferred to bk_dump_peri_probes() below, so they run only after the
     * task-list/backtrace/epilogue are safely emitted. */
    bk_coredump_memory_peripherals();

    bk_coredump_feed_watchdogs();
    coredump_prompt_info();

#if CONFIG_CM_BACKTRACE
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT &&
        self->secure_context == NULL) {
        bk_coredump_feed_watchdogs();
        cm_backtrace_fault(self->lr, self->sp);
    }
#endif
#endif /* CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE */

    coredump_prompt_epilogue();

#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    /* Hang-prone AP/PSRAM diagnostic probes run LAST - after the epilogue but
     * still inside the UART lock (writer_deinit releases it). The wedge sweep
     * can stall on a wedged AP bus and trip the (never-stopped) AON WDT reset;
     * by running it here, the essential dump and epilogue are already out. */
    bk_coredump_feed_watchdogs();
    bk_dump_peri_probes();
#endif

    bk_coredump_writer_deinit();
}

void bk_coredump_dump_ap_memory_for_trap(void)
{
    uint64_t dump_time_us = bk_aon_rtc_get_us();

#if CONFIG_SUPPORT_WWDT
    bk_wwdt_driver_deinit();
#endif
    bk_coredump_feed_watchdogs();
    bk_coredump_writer_init();
    bk_coredump_write_prompt("@dump_format_version: %u\r\n", (unsigned)BK_DUMP_FORMAT_VERSION);
    bk_coredump_dump_time(dump_time_us);
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    bk_coredump_write_prompt("*************************************AP memory dump begin**************************************\r\n");
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    bk_coredump_feed_watchdogs();
    /* Integrity-first: RAM image + safe peripheral registers only. P1-2: if the
     * AP cores' stop was not confirmed, cross-reading AP RAM can stall the CP
     * bus, so downgrade to safe register reads only. */
    if (bk_coredump_ap_stop_unconfirmed()) {
        bk_coredump_write_prompt(">>>>AP cores stop unconfirmed: skip AP RAM cross-read, dump registers only\r\n");
        bk_coredump_ap_dump_regs();
    } else {
        bk_coredump_ap_dump_ram_and_regs();
    }
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    bk_coredump_write_prompt("**************************************AP memory dump end***************************************\r\n");
    bk_coredump_write_prompt("***********************************************************************************************\r\n");
    /* P1-3: emit the end marker while the UART lock is still held (previously it
     * was written after writer_deinit and silently dropped), BEFORE the
     * hang-prone probes, so the offline parser always sees a complete record. */
    coredump_prompt_epilogue();
    /* P2-2: destructive AP probes are Debug-only (no-op banner in Release) and
     * run last so a bus stall here cannot cost us the RAM image or end marker. */
    bk_coredump_feed_watchdogs();
    bk_coredump_ap_dump_probes();
    bk_coredump_writer_deinit();
}

static void bk_exception_postprocess(bk_exception_t *self)
{
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        BK_LOG_FLUSH();
    }
    bk_reboot_ex(self->reset_reason);
}

static void bk_exception_handler_common(bk_exception_t *exception)
{
    bk_exception_preprocess(exception);
    bk_exception_dump_main(exception);
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

void bk_exception_handler_from_secure(const cp_secure_fault_context_t *context)
{
    bool source_secure =
        (context->flags & CP_SEC_DUMP_FLAG_SOURCE_SECURE) != 0U;
    uint32_t reset_reason = !source_secure && bk_check_assert()
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
