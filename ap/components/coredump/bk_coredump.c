#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "bk_coredump.h"
#include "os/mem.h"
#include "reg_base.h"
#include "bk_rtos_debug.h"
#include "multicore_driver.h"
#include "mb_ipc_cmd.h"
#include "sys_sw_regs.h"
#include "memory.h"
#include "cache.h"

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

bool bk_check_assert(void)
{
    if (s_bk_assert_info.magic == BK_ASSERT_MAGIC &&
        s_bk_assert_info.func != NULL &&
        s_bk_assert_info.line != 0) {
        return true;
    }
    return false;
}

static inline void coredump_stop_other_cores(void)
{
    // smp needs stop other cores
#if CONFIG_SOC_SMP
    uint32_t core_id = rtos_get_core_id();

    if (core_id == CPU2_CORE_ID) {
        bk_multicore_stop(CPU3_CORE_ID);
    } else if (core_id == CPU3_CORE_ID) {
        bk_multicore_stop(CPU2_CORE_ID);
    } else {
        BK_DUMP_OUT("warning: unexpected AP core id %u, cannot stop peer core\r\n", core_id);
    }
#endif
}


static void bk_exception_preprocess(bk_exception_t *self)
{
    rtos_disable_int();
    bk_coredump_lock();
    if (s_bk_exception_magic == BK_EXCEPTION_MAGIC) {
        BK_DUMP_OUT("A secondary exception occurred, reset_reason: 0x%x\r\n", self->reset_reason);
        bk_reboot_ex(self->reset_reason);
    }
    s_bk_exception_magic = BK_EXCEPTION_MAGIC;
    s_core_id = rtos_get_core_id();
    coredump_stop_other_cores();

    coredump_feed_watchdogs();
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

static void coredump_notify_cp_end(void)
{
#if (CONFIG_CPU_CNT > 1)
    ipc_send_trap_handle_end();
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

#if CONFIG_DCACHE
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
#if CONFIG_DCACHE
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
     * AP memory.c currently maps bk_get_psram_bss_info() to the actual
     * .psram.data range, and bk_get_psram_data_info() to .psram.bss.
     */
    bk_get_psram_bss_info(&mem_info);
    if ((mem_info.start_addr != 0U) && (mem_info.size != 0U)) {
        bk_sys_sw_regs_update_ap_heap_dump(BK_SYS_SW_REGS_AP_HEAP_SRAM,
            mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }

    bk_get_psram_data_info(&mem_info);
    if ((mem_info.start_addr != 0U) && (mem_info.size != 0U)) {
        bk_sys_sw_regs_update_ap_heap_dump(BK_SYS_SW_REGS_AP_HEAP_HSRAM,
            mem_info.start_addr, mem_info.start_addr + mem_info.size);
    }
#endif
}

static void bk_exception_dump_main(bk_exception_t *self)
{
    bk_coredump_writer_init();

    bk_coredump_meta_info();

    bk_coredump_registers(self);

    coredump_publish_ap_psram_windows();
    coredump_check_psram_code();
    coredump_notify_cp_begin();

    coredump_prompt_prologue();

    // coredump_prompt_info();

#if CONFIG_CM_BACKTRACE
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        cm_backtrace_fault(self->lr, self->sp);
    }
#endif

    // coredump_prompt_epilogue();

    bk_coredump_writer_deinit();

    coredump_flush_for_cp_dump();
    coredump_notify_cp_end();
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

void bk_exception_handler(uint32_t reset_reason, uint32_t lr, uint32_t sp)
{
    if (bk_check_assert()) {
        reset_reason = RESET_SOURCE_CRASH_ASSERT;
    }
    bk_exception_t exception = {
        .lr = lr,
        .sp = sp,
        .reset_reason = reset_reason,
    };
    bk_exception_preprocess(&exception);
#if CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE
    bk_exception_dump_main(&exception);
#endif
    bk_exception_postprocess(&exception);
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
            return i + 1;
        }
    }
    return 0;
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
