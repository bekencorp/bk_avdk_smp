
#if CONFIG_MEM_DEBUG
#include <os/str.h>
#include "os_heap_debug.h"
#include "bk_heap/port/port_heap.h"

#define MEM_OVERFLOW_TAG        0xcd

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
#if CONFIG_WDT_EN
#if (CONFIG_TASK_WDT)
    bk_task_wdt_feed();
#endif
    if (arch_is_enter_exception())
    {
        void bk_wdt_force_feed(void);
        bk_wdt_force_feed();
    }
#endif // CONFIG_WDT_EN
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
    uint8_t *block_end = mem_end + mem_end_len;

    for ( int i = 0; i < mem_end_len; i++) {
        if (MEM_OVERFLOW_TAG != mem_end[i]) {
            BK_DUMP_OUT("Mem Overflow ......mem_end[%p + %d]=[0x%02x].....\r\n", mem_end, i, mem_end[i]);
            show_mem_info(info);
            stack_mem_dump((uint32_t)info - 64, (uint32_t)(block_end + 64));
            if (0 == arch_is_enter_exception()) {
                BK_ASSERT(0);
            }
            break;
        }
    }
}

#endif