#include <os/mem.h>
#include <os/os.h>
#include "os_heap.h"
#include "bk_heap/port/port_heap.h"
#include "os_heap_debug.h"

#define IF_NULL_RETURN_NULL(ptr) do { if (ptr == NULL) { return NULL; } } while (0)

static void check_heap_risk_release(const char *risk_type)
{
    if (port_heap_in_risk_state()) {
        BK_DUMP_OUT("Error: %s risk.\r\n", risk_type);
        BK_ASSERT(0);
    }
}

static void check_heap_risk_debug(const char *func_name, int line, const char *risk_type)
{
    if (port_heap_in_risk_state()) {
        BK_DUMP_OUT("Error: [%s] line(%d). %s risk.\r\n", func_name, line, risk_type);
        BK_ASSERT(0);
    }
}

#if CONFIG_MEM_DEBUG
size_t os_heap_get_allocated_size(void *ptr)
{
    if (ptr_is_sram_heap(ptr)) {
        return sram_get_allocated_size(ptr);
    }
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        return psram_get_allocated_size(ptr);
    }
    #endif
    #ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    else if (ptr_is_hsram_heap(ptr)) {
        return hsram_get_allocated_size(ptr);
    }
    #endif
    else {
        BK_ASSERT(0);
    }
    return 0;
}
#endif

typedef struct bk_heap_t {
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
#if CONFIG_MEM_DEBUG
    struct list_head *used_list;
#endif
} bk_heap_t;

static void *bk_heap_malloc_impl(const bk_heap_t *self, const char *func_name, int line, size_t size, int need_zero)
{
    check_heap_risk_debug(func_name, line, "malloc");
    size_t real_size = bk_heap_debug_get_real_size(size);
    void *ptr = self->malloc(real_size);
    IF_NULL_RETURN_NULL(ptr);
    if (need_zero) {
        os_memset(ptr, 0, real_size);
    }
#if CONFIG_MEM_DEBUG
    bk_heap_debug_add_debug_info(self->used_list, ptr, func_name, line, size);
    bk_heap_fill_overflow_tag(ptr);
#endif
    return bk_heap_debug_get_ptr(ptr);
}

static void bk_heap_free_impl(const bk_heap_t *self, const char *func_name, int line, void *ptr)
{
    check_heap_risk_debug(func_name, line, "free");
#if CONFIG_MEM_DEBUG
    ptr = bk_heap_debug_get_real_ptr(ptr);
    bk_heap_debug_remove_debug_info(ptr);
    bk_heap_overflow_check(ptr);
#endif
    self->free(ptr);
}
/* =========================== OS API =========================== */

void os_free_debug(const char *func_name, int line, void *ptr)
{
    if (ptr_is_sram_heap(ptr)) {
        sram_free_debug(func_name, line, ptr);
    }
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        psram_free_debug(func_name, line, ptr);
    }
    #endif
    #ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    else if (ptr_is_hsram_heap(ptr)) {
        hsram_free_debug(func_name, line, ptr);
    }
    #endif
    else {
        BK_ASSERT(0);
    }
}

void os_free_release(void *ptr)
{
    if (ptr_is_sram_heap(ptr)) {
        sram_free_release(ptr);
    }
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        psram_free_release(ptr);
    }
    #endif
    #ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    else if (ptr_is_hsram_heap(ptr)) {
        hsram_free_release(ptr);
    }
    #endif
    else {
        BK_ASSERT(0);
    }
}

/* Default OS heap type selection (Kconfig: OS_HEAP_TYPE) */
#if defined(CONFIG_OS_HEAP_USE_HSRAM)
    void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero) __attribute__((alias("hsram_malloc_debug")));
    void *os_malloc_release(size_t size) __attribute__((alias("hsram_malloc_release")));
    void *os_zalloc_release(size_t size) __attribute__((alias("hsram_zalloc_release")));
    #if !defined(CONFIG_AP_HSRAM_HEAP_ADDR)
        #error "Require CONFIG_AP_HSRAM_HEAP_ADDR to be defined"
    #endif
#elif defined(CONFIG_OS_HEAP_USE_PSRAM)
    void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero) __attribute__((alias("psram_malloc_debug")));
    void *os_malloc_release(size_t size) __attribute__((alias("psram_malloc_release")));
    void *os_zalloc_release(size_t size) __attribute__((alias("psram_zalloc_release")));
    #if !defined(CONFIG_AP_PSRAM_HEAP_ADDR)
        #error "Require CONFIG_AP_PSRAM_HEAP_ADDR to be defined"
    #endif
#else /* default: SRAM */
    void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero) __attribute__((alias("sram_malloc_debug")));
    void *os_malloc_release(size_t size) __attribute__((alias("sram_malloc_release")));
    void *os_zalloc_release(size_t size) __attribute__((alias("sram_zalloc_release")));
#endif

void *os_realloc_debug(const char *func_name, int line, void *ptr, size_t size, int need_zero)
{
    void *tmp;
  
    tmp = (void *)os_malloc_debug(func_name, line, size, need_zero);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        os_free_debug(func_name, line, ptr);
    }

    return tmp;
}

void *os_realloc_release(void *ptr, size_t size)
{
    void *tmp;

    tmp = (void *)os_malloc_release(size);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        os_free_release(ptr);
    }

    return tmp;
}

/* =========================== SRAM HEAP =========================== */
#if CONFIG_MEM_DEBUG
static struct list_head s_sram_used;
void bk_heap_sram_debug_init(void)
{
    INIT_LIST_HEAD(&s_sram_used);
}
#endif

static const bk_heap_t bk_heap_sram = {
    .malloc = sram_malloc_impl,
    .free = sram_free_impl,
#if CONFIG_MEM_DEBUG
    .used_list = &s_sram_used,
#endif
};

void *sram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
    return bk_heap_malloc_impl(&bk_heap_sram, func_name, line, size, need_zero);
}

void sram_free_debug(const char *func_name, int line, void *ptr)
{
    bk_heap_free_impl(&bk_heap_sram, func_name, line, ptr);
}

void *sram_malloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_sram, "NULL", 0, size, 0);
}

void sram_free_release(void *ptr)
{
    bk_heap_free_impl(&bk_heap_sram, "NULL", 0, ptr);
}

void *sram_zalloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_sram, "NULL", 0, size, 1);
}
/* =========================== HSRAM HEAP =========================== */
#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
#if CONFIG_MEM_DEBUG
static struct list_head s_hsram_used;
void bk_heap_hsram_debug_init(void)
{
    INIT_LIST_HEAD(&s_hsram_used);
}
#endif

static const bk_heap_t bk_heap_hsram = {
    .malloc = hsram_malloc_impl,
    .free = hsram_free_impl,
#if CONFIG_MEM_DEBUG
    .used_list = &s_hsram_used,
#endif
};

void *hsram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
    return bk_heap_malloc_impl(&bk_heap_hsram, func_name, line, size, need_zero);
}

void hsram_free_debug(const char *func_name, int line, void *ptr)
{
    bk_heap_free_impl(&bk_heap_hsram, func_name, line, ptr);
}

void *hsram_malloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_hsram, "NULL", 0, size, 0);
}

void hsram_free_release(void *ptr)
{
    bk_heap_free_impl(&bk_heap_hsram, "NULL", 0, ptr);
}

void *hsram_zalloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_hsram, "NULL", 0, size, 1);
}

void *hsram_realloc_debug(const char *func_name, int line, void *ptr, size_t size, int need_zero)
{
    void *tmp;
  
    tmp = (void *)hsram_malloc_debug(func_name, line, size, need_zero);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        hsram_free_debug(func_name, line, ptr);
    }

    return tmp;
}

void *hsram_realloc_release(void *ptr, size_t size)
{
    void *tmp;

    tmp = (void *)hsram_malloc_release(size);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        hsram_free_release(ptr);
    }

    return tmp;
}
#endif
/* =========================== PSRAM HEAP =========================== */
#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
#if CONFIG_MEM_DEBUG
static struct list_head s_psram_used;
void bk_heap_psram_debug_init(void)
{
    INIT_LIST_HEAD(&s_psram_used);
}
#endif

static const bk_heap_t bk_heap_psram = {
    .malloc = psram_malloc_impl,
    .free = psram_free_impl,
#if CONFIG_MEM_DEBUG
    .used_list = &s_psram_used,
#endif
};

void *psram_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
    return bk_heap_malloc_impl(&bk_heap_psram, func_name, line, size, need_zero);
}

void psram_free_debug(const char *func_name, int line, void *ptr)
{
    bk_heap_free_impl(&bk_heap_psram, func_name, line, ptr);
}

void *psram_malloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_psram, "NULL", 0, size, 0);
}

void psram_free_release(void *ptr)
{
    bk_heap_free_impl(&bk_heap_psram, "NULL", 0, ptr);
}

void *psram_realloc_debug(const char *func_name, int line, void *ptr, size_t size)
{
    void *tmp;

    tmp = (void *)psram_malloc_debug(func_name, line, size, 0);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        psram_free_debug(func_name, line, ptr);
    }

    return tmp;
}

void *psram_realloc_release(void *ptr, size_t size)
{
    void *tmp;

    tmp = (void *)psram_malloc_release(size);
    if (tmp && ptr) {
        os_memcpy(tmp, ptr, size);
        psram_free_release(ptr);
    }

    return tmp;
}

void *psram_zalloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_psram, "NULL", 0, size, 1);
}
#endif

#if CONFIG_MEM_DEBUG
void os_dump_memory_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char* task)
{
    BK_DUMP_OUT(">>>>> sram heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(start_tick, ticks_since_malloc, task, &s_sram_used) != 0) {
        BK_DUMP_OUT("sram heap list is empty.\r\n");
    }
    #ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    BK_DUMP_OUT(">>>>> hsram heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(start_tick, ticks_since_malloc, task, &s_hsram_used) != 0) {
        BK_DUMP_OUT("hsram heap list is empty.\r\n");
    }
    #endif
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    BK_DUMP_OUT(">>>>> psram heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(start_tick, ticks_since_malloc, task, &s_psram_used) != 0) {
        BK_DUMP_OUT("psram heap list is empty.\r\n");
    }
    #endif
}
#endif

#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
void bk_psram_heap_get_used_state(void)
{
    uint32_t count = bk_psram_heap_get_used_count();
    BK_DUMP_OUT("Psram heap used count is %d.\n", count);
    if (0 == count) {
        return;
    }
    BK_DUMP_OUT(">>>>> psram heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(0, 0, NULL, &s_psram_used) != 0) {
        BK_DUMP_OUT("psram heap list is empty.\r\n");
    }
}
#endif
