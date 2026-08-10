#include <stdint.h>
#include <stdbool.h>
#include "bk_heap.h"
#include "bk_heap/port/port_heap.h"
#include "bk_heap_impl/heap_4_1.h"
#include "os_heap_debug.h"

extern unsigned char _heap_start, _heap_end;
#define SRAM_HEAP_START_ADDRESS    (uint32_t)&_heap_start
#define SRAM_HEAP_END_ADDRESS      (uint32_t)&_heap_end

static bk_heap_region_id_t s_sram_region_id = BK_HEAP_INVALID_REGION_ID;
static Heap_4_1_Data_t s_sram_heap_data;

static void sram_heap_init(void)
{
    if (s_sram_region_id != BK_HEAP_INVALID_REGION_ID) {
        return;
    }
    uint32_t sram_heap_size = SRAM_HEAP_END_ADDRESS - SRAM_HEAP_START_ADDRESS;
    s_sram_region_id = bk_heap_add_region(SRAM_HEAP_START_ADDRESS, sram_heap_size, &heap_4_1_ops, &s_sram_heap_data);
    BK_ASSERT(s_sram_region_id != BK_HEAP_INVALID_REGION_ID);
#if CONFIG_MEM_DEBUG
    bk_heap_sram_debug_init();
#endif
}

static void insure_sram_heap_init(void)
{
    if (s_sram_region_id == BK_HEAP_INVALID_REGION_ID) {
        port_heap_enter_critical();
        sram_heap_init();
        port_heap_exit_critical();
    }
}

void *sram_malloc_impl(size_t size)
{
    insure_sram_heap_init();
    return bk_heap_malloc(s_sram_region_id, size);
}


void sram_free_impl(void *ptr)
{
    bk_heap_free(s_sram_region_id, ptr);
}

bool ptr_is_sram_heap(void *ptr)
{
    return (ptr >= (void *)SRAM_HEAP_START_ADDRESS && ptr < (void *)SRAM_HEAP_END_ADDRESS);
}

size_t sram_get_allocated_size(void *ptr)
{
    return bk_heap_get_allocated_size(s_sram_region_id, ptr);
}

uint32_t prvHeapGetTotalSize(void)
{
    return SRAM_HEAP_END_ADDRESS - SRAM_HEAP_START_ADDRESS;
}

size_t xPortGetFreeHeapSize(void)
{
    insure_sram_heap_init();
    return bk_heap_get_free_size(s_sram_region_id);
}

size_t xPortGetMinimumEverFreeHeapSize(void)
{
    insure_sram_heap_init();
    return bk_heap_get_min_free_size(s_sram_region_id);
}
