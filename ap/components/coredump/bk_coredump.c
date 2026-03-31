#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "bk_coredump.h"
#include "wdt_driver.h"
#include "os/mem.h"
#include "reg_base.h"
#include "bk_rtos_debug.h"

#define BK_EXCEPTION_MAGIC 0xA55AA55A
#define BK_ASSERT_MAGIC 0x55AA55AA
static volatile bk_assert_info_t s_bk_assert_info;
static volatile uint32_t s_bk_exception_magic = 0;
static volatile uint32_t s_core_id = 0;

static hook_func s_wifi_dump_func = NULL;
static hook_func s_ble_dump_func = NULL;

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
    // TODO
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

    // bk_wdt_force_feed();
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
        s_wifi_dump_func();
    }
    if (is_valid_function_addr(s_ble_dump_func)) {
        s_ble_dump_func();
    }
}

static void bk_exception_dump_main(bk_exception_t *self)
{
    bk_coredump_writer_init();

    bk_coredump_meta_info();

    bk_coredump_registers(self);

    coredump_prompt_prologue();

    bk_coredump_memory();

#if CONFIG_MEMDUMP_ALL
    coredump_execute_hook_function();
#endif

    coredump_prompt_info();

#if CONFIG_CM_BACKTRACE
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        cm_backtrace_fault(self->lr, self->sp);
    }
#endif

    coredump_prompt_epilogue();

    bk_coredump_writer_deinit();
}

static void bk_exception_postprocess(bk_exception_t *self)
{
    if (self->reset_reason != RESET_SOURCE_CRASH_ASSERT) {
        BK_LOG_FLUSH();
    }
    bk_reboot_ex(self->reset_reason);
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
