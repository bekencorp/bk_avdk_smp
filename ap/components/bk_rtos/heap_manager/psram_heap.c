
#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
#include <stdint.h>
#include <stdbool.h>
#include "bk_heap.h"
#include "bk_heap/port/port_heap.h"
#include "bk_heap_impl/heap_4_1.h"
#include "os_heap_debug.h"

static bk_heap_region_id_t s_psram_region_id = BK_HEAP_INVALID_REGION_ID;
static Heap_4_1_Data_t s_psram_heap_data;
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
static bk_heap_region_id_t s_psram_nocache_region_id = BK_HEAP_INVALID_REGION_ID;
static Heap_4_1_Data_t s_psram_nocache_heap_data;
#endif

#define PSRAM_START_ADDRESS    (uint32_t)(CONFIG_AP_PSRAM_HEAP_ADDR)
#define PSRAM_END_ADDRESS      (uint32_t)(CONFIG_AP_PSRAM_HEAP_ADDR + CONFIG_AP_PSRAM_HEAP_SIZE)
#define PSRAM_HEAP_SIZE        CONFIG_AP_PSRAM_HEAP_SIZE
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
#define PSRAM_NOCACHE_START_ADDRESS    (uint32_t)(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR)
#define PSRAM_NOCACHE_END_ADDRESS      (uint32_t)(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR + CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE)
#define PSRAM_NOCACHE_HEAP_SIZE        CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE
#endif

static void psram_heap_init(void)
{
    if (s_psram_region_id != BK_HEAP_INVALID_REGION_ID) {
        return;
    }
    s_psram_region_id = bk_heap_add_region(PSRAM_START_ADDRESS, PSRAM_HEAP_SIZE, &heap_4_1_ops, &s_psram_heap_data);
    BK_ASSERT(s_psram_region_id != BK_HEAP_INVALID_REGION_ID);
#if CONFIG_MEM_DEBUG
    bk_heap_psram_debug_init();
#endif
}

static void insure_psram_heap_available(void)
{
    if (s_psram_region_id == BK_HEAP_INVALID_REGION_ID) {
        port_heap_enter_critical();
        psram_heap_init();
        port_heap_exit_critical();
    }
    // TODO
}

#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
static void psram_nocache_heap_init(void)
{
    if (s_psram_nocache_region_id != BK_HEAP_INVALID_REGION_ID) {
        return;
    }
    s_psram_nocache_region_id = bk_heap_add_region(PSRAM_NOCACHE_START_ADDRESS,
                                                   PSRAM_NOCACHE_HEAP_SIZE,
                                                   &heap_4_1_ops,
                                                   &s_psram_nocache_heap_data);
    BK_ASSERT(s_psram_nocache_region_id != BK_HEAP_INVALID_REGION_ID);
#if CONFIG_MEM_DEBUG
    bk_heap_psram_nocache_debug_init();
#endif
}

static void insure_psram_nocache_heap_available(void)
{
    if (s_psram_nocache_region_id == BK_HEAP_INVALID_REGION_ID) {
        port_heap_enter_critical();
        psram_nocache_heap_init();
        port_heap_exit_critical();
    }
}
#endif

void *psram_malloc_impl(size_t size)
{
    insure_psram_heap_available();
    return bk_heap_malloc(s_psram_region_id, size);
}

void psram_free_impl(void *ptr)
{
    bk_heap_free(s_psram_region_id, ptr);
}

bool ptr_is_psram_heap(void *ptr)
{
    return (ptr >= (void *)PSRAM_START_ADDRESS && ptr < (void *)PSRAM_END_ADDRESS);
}

#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
void *psram_nocache_malloc_impl(size_t size)
{
    insure_psram_nocache_heap_available();
    return bk_heap_malloc(s_psram_nocache_region_id, size);
}

void psram_nocache_free_impl(void *ptr)
{
    bk_heap_free(s_psram_nocache_region_id, ptr);
}

bool ptr_is_psram_nocache_heap(void *ptr)
{
    return (ptr >= (void *)PSRAM_NOCACHE_START_ADDRESS && ptr < (void *)PSRAM_NOCACHE_END_ADDRESS);
}
#endif

#if CONFIG_MEM_DEBUG
size_t psram_get_allocated_size(void *ptr)
{
    return bk_heap_get_allocated_size(s_psram_region_id, ptr);
}

#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
size_t psram_nocache_get_allocated_size(void *ptr)
{
    return bk_heap_get_allocated_size(s_psram_nocache_region_id, ptr);
}
#endif
#endif

size_t xPortGetPsramTotalHeapSize(void)
{
    return PSRAM_HEAP_SIZE;
}

size_t xPortGetPsramFreeHeapSize(void)
{
    if (s_psram_region_id == BK_HEAP_INVALID_REGION_ID) {
        return PSRAM_HEAP_SIZE;
    }
    return bk_heap_get_free_size(s_psram_region_id);
}

size_t xPortGetPsramMinimumFreeHeapSize(void)
{
    if (s_psram_region_id == BK_HEAP_INVALID_REGION_ID) {
        return PSRAM_HEAP_SIZE;
    }
    return bk_heap_get_min_free_size(s_psram_region_id);
}

uint32_t bk_psram_heap_get_used_count(void) {
    if (s_psram_region_id == BK_HEAP_INVALID_REGION_ID) {
        return 0;
    }
    return PSRAM_HEAP_SIZE - xPortGetPsramFreeHeapSize();
}
#endif