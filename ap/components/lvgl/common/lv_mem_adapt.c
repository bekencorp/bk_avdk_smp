#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#define TAG "lv_mem"

#if CONFIG_LVGL_V9
#include "src/stdlib/lv_mem.h"
#endif

#if !CONFIG_LVGL_V9 || LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

void *lv_malloc_core(size_t size)
{
    void *ptr;

#if CONFIG_LVGL_MEM_USE_PSRAM
    ptr = psram_malloc(size);
    if (ptr == NULL)
    {
        BK_LOGE(TAG, "PSRAM malloc failed: size=%u free=%u min=%u\n",
                (unsigned)size,
                (unsigned)rtos_get_psram_free_heap_size(),
                (unsigned)rtos_get_psram_minimum_free_heap_size());
    }
#else
    ptr = hsram_malloc(size);
    if (ptr == NULL)
    {
        BK_LOGE(TAG, "HSRAM malloc failed: size=%u free=%u min=%u\n",
                (unsigned)size,
                (unsigned)rtos_get_hsram_free_heap_size(),
                (unsigned)rtos_get_hsram_minimum_free_heap_size());
    }
#endif

    return ptr;
}

void *lv_realloc_core(void *ptr, size_t size)
{
    void *new_ptr;

#if CONFIG_LVGL_MEM_USE_PSRAM
    new_ptr = psram_realloc(ptr, size);
    if (new_ptr == NULL && size != 0)
    {
        BK_LOGE(TAG, "PSRAM realloc failed: ptr=%p size=%u free=%u min=%u\n",
                ptr,
                (unsigned)size,
                (unsigned)rtos_get_psram_free_heap_size(),
                (unsigned)rtos_get_psram_minimum_free_heap_size());
    }
#else
    new_ptr = hsram_realloc(ptr, size);
    if (new_ptr == NULL && size != 0)
    {
        BK_LOGE(TAG, "HSRAM realloc failed: ptr=%p size=%u free=%u min=%u\n",
                ptr,
                (unsigned)size,
                (unsigned)rtos_get_hsram_free_heap_size(),
                (unsigned)rtos_get_hsram_minimum_free_heap_size());
    }
#endif

    return new_ptr;
}

void lv_free_core(void *ptr)
{
#if CONFIG_LVGL_MEM_USE_PSRAM
    psram_free(ptr);
#else
    hsram_free(ptr);
#endif
}

/*
 * The rest of the v9 lv_stdlib contract. v8 has no such API, so the v8 client
 * of the guard above takes only the three malloc/realloc/free functions and
 * stops here. All stubs: the underlying heap is the system one, which needs no
 * init and reports through its own rtos_get_*_free_heap_size().
 */
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

#endif /* CONFIG_LVGL_V9 */

#endif /* !CONFIG_LVGL_V9 || LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM */
