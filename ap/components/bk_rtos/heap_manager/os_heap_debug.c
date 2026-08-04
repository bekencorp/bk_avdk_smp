
#if CONFIG_MEM_DEBUG
#include <os/str.h>
#include "os_heap_debug.h"
#include "bk_heap/port/port_heap.h"

#if CONFIG_SUPPORT_WWDT
#include "wwdt_driver.h"
#include <driver/wwdt.h>
#endif
#if CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif
#define MEM_OVERFLOW_TAG        0xcd

#if CONFIG_HEAP_UAF_AUDIT_POISON
#define MEM_UAF_FREE_POISON_TAG 0xee

#if !defined(CONFIG_HEAP_UAF_FREE_RECORD_MAX) || (CONFIG_HEAP_UAF_FREE_RECORD_MAX <= 0)
#define CONFIG_HEAP_UAF_FREE_RECORD_MAX 128
#endif

#if defined(CONFIG_AP_PSRAM_DATA_SECTION_ADDR) && (CONFIG_AP_PSRAM_DATA_SECTION_ADDR)
#define HEAP_UAF_META_ATTR __attribute__((section(".psram.bss")))
#else
#define HEAP_UAF_META_ATTR
#endif

#if CONFIG_HEAP_UAF_QUARANTINE
#if !defined(CONFIG_HEAP_UAF_QUARANTINE_BYTES) || (CONFIG_HEAP_UAF_QUARANTINE_BYTES <= 0)
#define CONFIG_HEAP_UAF_QUARANTINE_BYTES 65536
#endif
#if !defined(CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX) || (CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX <= 0)
#define CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX 512
#endif

#define HEAP_UAF_QUARANTINE_MAX_BLOCK_BYTES (CONFIG_HEAP_UAF_QUARANTINE_BYTES / 2U)
#endif

typedef struct {
    void *user_ptr;
    uint32_t size;
    const char *alloc_func;
    uint16_t alloc_line;
    const char *alloc_task;
    uint16_t alloc_tick;
    const char *free_func;
    uint16_t free_line;
    const char *free_task;
    uint32_t free_tick;
} bk_heap_free_record_t;

static HEAP_UAF_META_ATTR bk_heap_free_record_t s_free_history[CONFIG_HEAP_UAF_FREE_RECORD_MAX];
static uint32_t s_free_history_idx;
static uint32_t s_free_history_count;

#if CONFIG_HEAP_UAF_QUARANTINE
typedef struct {
    void *ptr;
    uint32_t size;
    uint32_t payload_size;
    bk_heap_debug_free_func_t free_func;
} bk_heap_quarantine_record_t;

static HEAP_UAF_META_ATTR bk_heap_quarantine_record_t s_quarantine[CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX];
static uint32_t s_quarantine_head;
static uint32_t s_quarantine_count;
static uint32_t s_quarantine_bytes;

static void bk_heap_debug_show_free_record(const bk_heap_free_record_t *record);

static int bk_heap_debug_check_quarantine_poison(const bk_heap_quarantine_record_t *record)
{
    uint8_t *user_ptr = (uint8_t *)bk_heap_debug_get_ptr(record->ptr);
    uint32_t payload_size = record->payload_size;
    uint32_t max_payload_size = 0;

    if (record->size > (sizeof(bk_heap_debug_info_t) + MEM_CHECK_TAG_LEN)) {
        max_payload_size = record->size - sizeof(bk_heap_debug_info_t) - MEM_CHECK_TAG_LEN;
    }
    if (payload_size > max_payload_size) {
        payload_size = max_payload_size;
    }

    for (uint32_t i = 0; i < payload_size; i++) {
        if (user_ptr[i] != MEM_UAF_FREE_POISON_TAG) {
            BK_DUMP_OUT("\r\n!!!!! HEAP UAF QUARANTINE POISON BROKEN !!!!!\r\n");
            BK_DUMP_OUT("!!!!! Freed block was modified before real free. !!!!!\r\n");
            BK_DUMP_OUT("  addr=0x%08x offset=%u expect=0x%02x actual=0x%02x size=%u\r\n",
                (unsigned int)(uint32_t)&user_ptr[i],
                (unsigned int)i,
                (unsigned int)MEM_UAF_FREE_POISON_TAG,
                (unsigned int)user_ptr[i],
                (unsigned int)record->payload_size);
            bk_heap_debug_trace_free_addr((uint32_t)user_ptr);
            BK_ASSERT(0);
            return -1;
        }
    }

    return 0;
}
#endif

static void bk_heap_debug_show_free_record(const bk_heap_free_record_t *record)
{
    BK_DUMP_OUT("addr 0x%08x size %-5u\r\n",
        (unsigned int)(uint32_t)record->user_ptr,
        (unsigned int)record->size);
    BK_DUMP_OUT("  A %s:%u\r\n",
        record->alloc_func ? record->alloc_func : "NULL",
        (unsigned int)record->alloc_line);
    BK_DUMP_OUT("    task=%s, tick=%u\r\n",
        record->alloc_task ? record->alloc_task : "NULL",
        (unsigned int)record->alloc_tick);
    BK_DUMP_OUT("  F %s:%u\r\n",
        record->free_func ? record->free_func : "NULL",
        (unsigned int)record->free_line);
    BK_DUMP_OUT("    task=%s, tick=%u\r\n",
        record->free_task ? record->free_task : "NULL",
        (unsigned int)record->free_tick);
}

void bk_heap_debug_record_free(void *ptr, const char *free_func, uint16_t free_line)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    bk_heap_free_record_t record;

    record.user_ptr = bk_heap_debug_get_ptr(ptr);
    record.size = info->wantedSize;
    record.alloc_func = info->funcName;
    record.alloc_line = info->line;
    record.alloc_task = info->taskName;
    record.alloc_tick = info->allocTime;
    record.free_func = free_func;
    record.free_line = free_line;
    record.free_task = port_heap_get_task_name();
    record.free_tick = rtos_get_time();

    port_heap_enter_critical();
    s_free_history[s_free_history_idx] = record;
    s_free_history_idx = (s_free_history_idx + 1U) % CONFIG_HEAP_UAF_FREE_RECORD_MAX;
    if (s_free_history_count < CONFIG_HEAP_UAF_FREE_RECORD_MAX) {
        s_free_history_count++;
    }
    port_heap_exit_critical();
}

void bk_heap_debug_poison_after_free(void *ptr)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;

    os_memset(bk_heap_debug_get_ptr(ptr), MEM_UAF_FREE_POISON_TAG, info->wantedSize);
}

#if CONFIG_HEAP_UAF_QUARANTINE
static int bk_heap_debug_quarantine_pop_oldest(bk_heap_quarantine_record_t *record)
{
    port_heap_enter_critical();
    if (s_quarantine_count == 0U) {
        port_heap_exit_critical();
        return -1;
    }

    *record = s_quarantine[s_quarantine_head];
    s_quarantine_head = (s_quarantine_head + 1U) % CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX;
    s_quarantine_count--;
    s_quarantine_bytes -= record->size;
    port_heap_exit_critical();

    return 0;
}

void bk_heap_debug_quarantine_free(void *ptr, uint32_t size, bk_heap_debug_free_func_t free_func)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    bk_heap_quarantine_record_t record;

    if ((ptr == NULL) || (size == 0U) || (free_func == NULL)) {
        if ((ptr != NULL) && (free_func != NULL)) {
            free_func(ptr);
        }
        return;
    }

    if (size > HEAP_UAF_QUARANTINE_MAX_BLOCK_BYTES) {
        free_func(ptr);
        return;
    }

    while (1) {
        port_heap_enter_critical();
        if ((s_quarantine_count < CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX) &&
            (size <= (CONFIG_HEAP_UAF_QUARANTINE_BYTES - s_quarantine_bytes))) {
            uint32_t idx = (s_quarantine_head + s_quarantine_count) % CONFIG_HEAP_UAF_QUARANTINE_RECORD_MAX;

            s_quarantine[idx].ptr = ptr;
            s_quarantine[idx].size = size;
            s_quarantine[idx].payload_size = info->wantedSize;
            s_quarantine[idx].free_func = free_func;
            s_quarantine_count++;
            s_quarantine_bytes += size;
            port_heap_exit_critical();
            return;
        }
        port_heap_exit_critical();

        if (bk_heap_debug_quarantine_pop_oldest(&record) != 0) {
            free_func(ptr);
            return;
        }
        if (bk_heap_debug_check_quarantine_poison(&record) != 0) {
            return;
        }
        record.free_func(record.ptr);
    }
}
#endif

void bk_heap_debug_dump_free_history(uint32_t count)
{
    uint32_t dump_count;
    uint32_t idx;
    uint32_t history_count;

    port_heap_enter_critical();
    history_count = s_free_history_count;
    idx = s_free_history_idx;
    port_heap_exit_critical();

    if (history_count == 0U) {
        BK_DUMP_OUT("heap free history is empty.\r\n");
        return;
    }

    dump_count = count;
    if ((dump_count == 0U) || (dump_count > history_count)) {
        dump_count = history_count;
    }

    BK_DUMP_OUT(">>>>> heap free history dump, newest first.\r\n");
    BK_DUMP_OUT("Each record: addr/size, A=alloc, F=free.\r\n");

    for (uint32_t i = 0; i < dump_count; i++) {
        bk_heap_free_record_t record;

        port_heap_enter_critical();
        idx = (idx == 0U) ? (CONFIG_HEAP_UAF_FREE_RECORD_MAX - 1U) : (idx - 1U);
        record = s_free_history[idx];
        port_heap_exit_critical();

        bk_heap_debug_show_free_record(&record);
    }
}

void bk_heap_debug_trace_free_addr(uint32_t addr)
{
    uint32_t idx;
    uint32_t matched = 0;
    uint32_t history_count;

    port_heap_enter_critical();
    history_count = s_free_history_count;
    idx = s_free_history_idx;
    port_heap_exit_critical();

    if (history_count == 0U) {
        BK_DUMP_OUT("heap free history is empty.\r\n");
        return;
    }

    for (uint32_t i = 0; i < history_count; i++) {
        bk_heap_free_record_t record;
        uint32_t start;

        port_heap_enter_critical();
        idx = (idx == 0U) ? (CONFIG_HEAP_UAF_FREE_RECORD_MAX - 1U) : (idx - 1U);
        record = s_free_history[idx];
        port_heap_exit_critical();

        start = (uint32_t)record.user_ptr;
        if ((addr >= start) && ((addr - start) < record.size)) {
            if (matched == 0U) {
                BK_DUMP_OUT(">>>>> heap free history trace addr 0x%x.\r\n", addr);
            }
            bk_heap_debug_show_free_record(&record);
            matched++;
        }
    }

    if (matched == 0U) {
        BK_DUMP_OUT("addr 0x%x is not found in heap free history.\r\n", addr);
    }
}

#endif

void bk_heap_debug_add_debug_info(struct list_head *list, void *ptr,
    const char *func_name, uint16_t line, uint32_t size)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    info->funcName = func_name;
    info->line = line;
    info->allocTime = (uint16_t)rtos_get_time();
    info->wantedSize = size;
    info->taskName = port_heap_get_task_name();
    port_heap_enter_critical();
    list_add_tail(&info->node, list);
    port_heap_exit_critical();
}

void bk_heap_debug_remove_debug_info(void *ptr)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    port_heap_enter_critical();
    list_del(&info->node);
    port_heap_exit_critical();
}

static void show_mem_info(bk_heap_debug_info_t *info)
{
    BK_DUMP_OUT("%-8d   0x%-8x   %-4d   %-5d   %-32s   %-16s\r\n",
                info->allocTime, bk_heap_debug_get_ptr(info), info->wantedSize,
                info->line, info->funcName, info->taskName);

#if (CONFIG_TASK_WDT)
    bk_task_wdt_feed();
#endif

#if CONFIG_SUPPORT_WWDT
    (void)bk_wwdt_feed();
#endif

}

int bk_heap_debug_dump_mem_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char *task, struct list_head *list)
{
    bk_heap_debug_info_t *info;
    if (list->next == NULL) {
        return -1;
    }

    if (arch_is_enter_exception() == 0)
    {
        port_heap_enter_critical();
    }

    BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "tick", "addr", "size", "line");
    BK_DUMP_OUT("   %-32s", "func");
    BK_DUMP_OUT("   %-16s", "task");
    BK_DUMP_OUT("\r\n");
    BK_DUMP_OUT("%-8s   %-10s   %-4s   %-5s", "--------", "----------", "----", "-----");
    BK_DUMP_OUT("   %-32s", "--------------------------------");
    BK_DUMP_OUT("   %-16s", "----------------");
    BK_DUMP_OUT("\r\n");
    list_for_each_entry(info, list, node)
    {

        if (info->allocTime < start_tick)
            continue;

        if ((info->allocTime - start_tick) < ticks_since_malloc)
            continue;

        if (task && os_strncmp(task, info->taskName, sizeof(info->taskName)))
            continue;

        show_mem_info(info);

        bk_heap_overflow_check((void *)info);
    }

    if (arch_is_enter_exception() == 0)
    {
        port_heap_exit_critical();
    }

    return 0;
}

void bk_heap_fill_overflow_tag(void *ptr)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    uint8_t *mem_end = (uint8_t *)bk_heap_debug_get_ptr(ptr) + info->wantedSize;
    uint32_t mem_end_len = os_heap_get_allocated_size(ptr) - sizeof(bk_heap_debug_info_t) - info->wantedSize;
    os_memset(mem_end, MEM_OVERFLOW_TAG, mem_end_len);
}

void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
void bk_heap_overflow_check(void *ptr)
{
    bk_heap_debug_info_t *info = (bk_heap_debug_info_t *)ptr;
    uint8_t *mem_end = (uint8_t *)bk_heap_debug_get_ptr(ptr) + info->wantedSize;
    uint32_t mem_end_len = os_heap_get_allocated_size(ptr) - sizeof(bk_heap_debug_info_t) - info->wantedSize;

    for ( int i = 0; i < mem_end_len; i++) {
        if (MEM_OVERFLOW_TAG != mem_end[i]) {
            BK_DUMP_OUT("Mem Overflow ......mem_end[%p + %d]=[0x%02x].....\r\n", mem_end, i, mem_end[i]);
            show_mem_info(info);
#if !(CONFIG_DEBUG_VERSION || CONFIG_DUMP_ENABLE)
            uint8_t *block_end = mem_end + mem_end_len;
            stack_mem_dump((uint32_t)info - 64, (uint32_t)(block_end + 64));
#endif
            if (0 == arch_is_enter_exception()) {
                BK_ASSERT(0);
            }
            break;
        }
    }
}

#endif