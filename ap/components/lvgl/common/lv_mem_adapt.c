#include <os/mem.h>

#if CONFIG_LVGL_V9
#include "src/stdlib/lv_mem.h"
#endif

void *lv_malloc_core(size_t size)
{
#if CONFIG_LVGL_MEM_USE_PSRAM
    return psram_malloc(size);
#else
    return hsram_malloc(size);
#endif
}

void *lv_realloc_core(void *ptr, size_t size)
{
#if CONFIG_LVGL_MEM_USE_PSRAM
    return psram_realloc(ptr, size);
#else
    return hsram_realloc(ptr, size);
#endif
}

void lv_free_core(void *ptr)
{
#if CONFIG_LVGL_MEM_USE_PSRAM
    psram_free(ptr);
#else
    hsram_free(ptr);
#endif
}

#if CONFIG_LVGL_V9

void lv_mem_init(void)
{
    return; /* Nothing to init */
}

void lv_mem_deinit(void)
{
    return; /* Nothing to deinit */
}

lv_mem_pool_t lv_mem_add_pool(void *mem, size_t bytes)
{
    LV_UNUSED(mem);
    LV_UNUSED(bytes);
    return NULL;
}

void lv_mem_remove_pool(lv_mem_pool_t pool)
{
    LV_UNUSED(pool);
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p)
{
    LV_UNUSED(mon_p);
}

lv_result_t lv_mem_test_core(void)
{
    return LV_RESULT_OK;
}

#endif
