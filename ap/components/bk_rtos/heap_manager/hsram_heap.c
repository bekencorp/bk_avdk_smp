#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
#include <stdint.h>
#include <stdbool.h>
#include "bk_heap.h"
#include "bk_heap/port/port_heap.h"
#include "bk_heap_impl/heap_4_1.h"
#include "os_heap_debug.h"

#define HSRAM_HEAP_START_ADDRESS    (uint32_t)(CONFIG_AP_HSRAM_HEAP_ADDR)
#define HSRAM_HEAP_END_ADDRESS      (uint32_t)(CONFIG_AP_HSRAM_HEAP_ADDR + CONFIG_AP_HSRAM_HEAP_SIZE)
#define HSRAM_HEAP_SIZE             CONFIG_AP_HSRAM_HEAP_SIZE

static bk_heap_region_id_t s_hsram_region_id = BK_HEAP_INVALID_REGION_ID;
static Heap_4_1_Data_t s_hsram_heap_data;

static void hsram_heap_init(void)
{
    if (s_hsram_region_id != BK_HEAP_INVALID_REGION_ID) {
        return;
    }
    s_hsram_region_id = bk_heap_add_region(HSRAM_HEAP_START_ADDRESS, HSRAM_HEAP_SIZE,
                                            &heap_4_1_ops, &s_hsram_heap_data);
    BK_ASSERT(s_hsram_region_id != BK_HEAP_INVALID_REGION_ID);
#if CONFIG_MEM_DEBUG
    bk_heap_hsram_debug_init();
#endif
}

static void insure_hsram_heap_init(void)
{
    if (s_hsram_region_id == BK_HEAP_INVALID_REGION_ID) {
        port_heap_enter_critical();
        hsram_heap_init();
        port_heap_exit_critical();
    }
}

void *hsram_malloc_impl(size_t size)
{
    insure_hsram_heap_init();
    return bk_heap_malloc(s_hsram_region_id, size);
}

void hsram_free_impl(void *ptr)
{
    bk_heap_free(s_hsram_region_id, ptr);
}

bool ptr_is_hsram_heap(void *ptr)
{
    return (ptr >= (void *)HSRAM_HEAP_START_ADDRESS && ptr < (void *)HSRAM_HEAP_END_ADDRESS);
}

#if CONFIG_MEM_DEBUG
size_t hsram_get_allocated_size(void *ptr)
{
    return bk_heap_get_allocated_size(s_hsram_region_id, ptr);
}
#endif

size_t xPortGetHsramTotalHeapSize(void)
{
    return HSRAM_HEAP_SIZE;
}

size_t xPortGetHsramFreeHeapSize(void)
{
    if (s_hsram_region_id == BK_HEAP_INVALID_REGION_ID) {
        return HSRAM_HEAP_SIZE;
    }
    return bk_heap_get_free_size(s_hsram_region_id);
}

size_t xPortGetHsramMinimumFreeHeapSize(void)
{
    if (s_hsram_region_id == BK_HEAP_INVALID_REGION_ID) {
        return HSRAM_HEAP_SIZE;
    }
    return bk_heap_get_min_free_size(s_hsram_region_id);
}

#endif