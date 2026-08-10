#include <os/mem.h>
#include <os/os.h>
#include "os_heap.h"
#include "bk_heap/port/port_heap.h"
#include "os_heap_debug.h"
#include "sys_sw_regs.h"
#include <soc/soc.h>

extern unsigned char _heap_start;
#define SRAM_HEAP_START_ADDRESS ((uint32_t)&_heap_start)

#define IF_NULL_RETURN_NULL(ptr) do { if (ptr == NULL) { return NULL; } } while (0)

static void bk_heap_track_ap_heap_window(void *ptr, size_t size)
{
    volatile sys_sw_regs_t *sys_sw_regs;
    volatile ap_heap_dump_info_t *slot;
    uint32_t pool_base;
    uint32_t alloc_end;
    bk_sys_sw_regs_ap_heap_id_t heap_id;

    if ((ptr == NULL) || (size == 0U)) {
        return;
    }

    alloc_end = (uint32_t)ptr + (uint32_t)size;

    if (ptr_is_sram_heap(ptr)) {
        heap_id = BK_SYS_SW_REGS_AP_HEAP_SRAM;
        pool_base = SRAM_HEAP_START_ADDRESS;
    }
#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        heap_id = BK_SYS_SW_REGS_AP_HEAP_PSRAM;
        pool_base = CONFIG_AP_PSRAM_HEAP_ADDR;
    }
#endif
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    else if (ptr_is_psram_nocache_heap(ptr)) {
        return;
    }
#endif
#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    else if (ptr_is_hsram_heap(ptr)) {
        heap_id = BK_SYS_SW_REGS_AP_HEAP_HSRAM;
        pool_base = (uint32_t)SOC_SRAM_CPU_ADDR(CONFIG_AP_HSRAM_HEAP_ADDR);
    }
#endif
    else {
        return;
    }

    sys_sw_regs = bk_sys_sw_regs_ptr();
    slot = &sys_sw_regs->ap_heap_dump[heap_id];

    port_heap_enter_critical();
    if ((slot->valid != BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID) || (slot->pool_base != pool_base)) {
        slot->valid = 0U;
        slot->pool_base = pool_base;
        slot->max_alloc_end = alloc_end;
        slot->valid = BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID;
    } else if (alloc_end > slot->max_alloc_end) {
        slot->max_alloc_end = alloc_end;
    }
    port_heap_exit_critical();
}

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
    #if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    else if (ptr_is_psram_nocache_heap(ptr)) {
        return psram_nocache_get_allocated_size(ptr);
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

typedef void *(*bk_heap_debug_alloc_t)(const char *, int, size_t, int);
typedef void (*bk_heap_debug_free_t)(const char *, int, void *);
typedef void *(*bk_heap_release_alloc_t)(size_t);
typedef void (*bk_heap_release_free_t)(void *);
typedef size_t (*bk_heap_get_size_t)(void *);

#if defined(CONFIG_OS_HEAP_USE_PSRAM)
static bool s_os_heap_psram_ready;

void os_heap_enable_psram_default(void)
{
    __atomic_store_n(&s_os_heap_psram_ready, true, __ATOMIC_RELEASE);
}

static bool os_heap_psram_is_ready(void)
{
    return __atomic_load_n(&s_os_heap_psram_ready, __ATOMIC_ACQUIRE);
}
#endif

#if CONFIG_MEM_DEBUG
static void *bk_heap_realloc_debug_impl(bk_heap_debug_alloc_t alloc_func,
    bk_heap_debug_free_t free_func, bk_heap_get_size_t get_size_func,
    const char *func_name, int line, void *ptr, size_t size, int need_zero)
{
    bk_heap_debug_info_t *info;
    size_t allocated_size;
    size_t user_capacity;
    void *real_ptr;
    void *tmp;

    if (size == 0) {
        if (ptr != NULL) {
            free_func(func_name, line, ptr);
        }
        return NULL;
    }
    if (ptr == NULL) {
        return alloc_func(func_name, line, size, need_zero);
    }

    real_ptr = bk_heap_debug_get_real_ptr(ptr);
    info = (bk_heap_debug_info_t *)real_ptr;
    allocated_size = get_size_func(real_ptr);
    if (allocated_size < sizeof(*info) + MEM_CHECK_TAG_LEN) {
        return NULL;
    }
    user_capacity = allocated_size - sizeof(*info) - MEM_CHECK_TAG_LEN;
    if (info->wantedSize > user_capacity) {
        return NULL;
    }

    tmp = alloc_func(func_name, line, size, need_zero);
    if (tmp == NULL) {
        return NULL;
    }
    os_memcpy(tmp, ptr, size < info->wantedSize ? size : info->wantedSize);
    free_func(func_name, line, ptr);
    return tmp;
}
#endif

static void *bk_heap_realloc_release_impl(bk_heap_release_alloc_t alloc_func,
    bk_heap_release_free_t free_func, bk_heap_get_size_t get_size_func,
    void *ptr, size_t size)
{
    size_t old_size;
    void *tmp;

    if (size == 0) {
        if (ptr != NULL) {
            free_func(ptr);
        }
        return NULL;
    }
    if (ptr == NULL) {
        return alloc_func(size);
    }

    old_size = get_size_func(ptr);
    if (old_size == 0) {
        return NULL;
    }
    tmp = alloc_func(size);
    if (tmp == NULL) {
        return NULL;
    }
    os_memcpy(tmp, ptr, size < old_size ? size : old_size);
    free_func(ptr);
    return tmp;
}

static void *bk_heap_malloc_impl(const bk_heap_t *self, const char *func_name, int line, size_t size, int need_zero)
{
    size_t real_size;

    check_heap_risk_debug(func_name, line, "malloc");
    if (!bk_heap_debug_get_real_size(size, &real_size)) {
        return NULL;
    }
    void *ptr = self->malloc(real_size);
    IF_NULL_RETURN_NULL(ptr);
    if (need_zero) {
        os_memset(ptr, 0, real_size);
    }
#if CONFIG_MEM_DEBUG
    bk_heap_debug_add_debug_info(self->used_list, ptr, func_name, line, size);
    bk_heap_fill_overflow_tag(ptr);
#endif
    bk_heap_track_ap_heap_window(ptr, real_size);
    return bk_heap_debug_get_ptr(ptr);
}

static void bk_heap_free_impl(const bk_heap_t *self, const char *func_name, int line, void *ptr)
{
    check_heap_risk_debug(func_name, line, "free");
#if CONFIG_MEM_DEBUG
    ptr = bk_heap_debug_get_real_ptr(ptr);
    bk_heap_debug_remove_debug_info(ptr);
    bk_heap_overflow_check(ptr);
#if CONFIG_HEAP_UAF_AUDIT_POISON
    bk_heap_debug_record_free(ptr, func_name, (uint16_t)line);
    bk_heap_debug_poison_after_free(ptr);
#endif
#endif
#if CONFIG_MEM_DEBUG && CONFIG_HEAP_UAF_QUARANTINE
    bk_heap_debug_quarantine_free(ptr, (uint32_t)os_heap_get_allocated_size(ptr), self->free);
#else
    self->free(ptr);
#endif
}
/* =========================== OS API =========================== */

void os_free_debug(const char *func_name, int line, void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    if (ptr_is_sram_heap(ptr)) {
        sram_free_debug(func_name, line, ptr);
    }
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        psram_free_debug(func_name, line, ptr);
    }
    #endif
    #if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    else if (ptr_is_psram_nocache_heap(ptr)) {
        psram_nocache_free_debug(func_name, line, ptr);
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
    if (ptr == NULL) {
        return;
    }
    if (ptr_is_sram_heap(ptr)) {
        sram_free_release(ptr);
    }
    #ifdef CONFIG_AP_PSRAM_HEAP_ADDR
    else if (ptr_is_psram_heap(ptr)) {
        psram_free_release(ptr);
    }
    #endif
    #if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    else if (ptr_is_psram_nocache_heap(ptr)) {
        psram_nocache_free_release(ptr);
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
    #if !defined(CONFIG_AP_PSRAM_HEAP_ADDR)
        #error "Require CONFIG_AP_PSRAM_HEAP_ADDR to be defined"
    #endif

void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
    if (!os_heap_psram_is_ready()) {
        return sram_malloc_debug(func_name, line, size, need_zero);
    }
    return psram_malloc_debug(func_name, line, size, need_zero);
}

void *os_malloc_release(size_t size)
{
    if (!os_heap_psram_is_ready()) {
        return sram_malloc_release(size);
    }
    return psram_malloc_release(size);
}

void *os_zalloc_release(size_t size)
{
    if (!os_heap_psram_is_ready()) {
        return sram_zalloc_release(size);
    }
    return psram_zalloc_release(size);
}
#else /* default: SRAM */
    void *os_malloc_debug(const char *func_name, int line, size_t size, int need_zero) __attribute__((alias("sram_malloc_debug")));
    void *os_malloc_release(size_t size) __attribute__((alias("sram_malloc_release")));
    void *os_zalloc_release(size_t size) __attribute__((alias("sram_zalloc_release")));
#endif

void *os_realloc_debug(const char *func_name, int line, void *ptr, size_t size, int need_zero)
{
#if CONFIG_MEM_DEBUG
#if defined(CONFIG_OS_HEAP_USE_HSRAM)
    bk_heap_get_size_t get_size_func = hsram_get_allocated_size;
#elif defined(CONFIG_OS_HEAP_USE_PSRAM)
    bk_heap_get_size_t get_size_func = os_heap_get_allocated_size;
#else
    bk_heap_get_size_t get_size_func = sram_get_allocated_size;
#endif
    return bk_heap_realloc_debug_impl(os_malloc_debug, os_free_debug,
                                      get_size_func, func_name, line,
                                      ptr, size, need_zero);
#else
    return os_realloc_release(ptr, size);
#endif
}

void *os_realloc_release(void *ptr, size_t size)
{
#if defined(CONFIG_OS_HEAP_USE_HSRAM)
    return bk_heap_realloc_release_impl(hsram_malloc_release, hsram_free_release,
                                        hsram_get_allocated_size, ptr, size);
#elif defined(CONFIG_OS_HEAP_USE_PSRAM)
    if ((ptr == NULL) || ptr_is_psram_heap(ptr)) {
        return bk_heap_realloc_release_impl(os_malloc_release, os_free_release,
                                            psram_get_allocated_size, ptr, size);
    } else if (ptr_is_sram_heap(ptr)) {
        return bk_heap_realloc_release_impl(os_malloc_release, os_free_release,
                                            sram_get_allocated_size, ptr, size);
    }
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    else if (ptr_is_psram_nocache_heap(ptr)) {
        /* Keep non-cacheable buffers (e.g. DMA) inside the nocache heap; do not
         * migrate them into cached PSRAM, which would break cache coherency. */
        return bk_heap_realloc_release_impl(psram_nocache_malloc_release,
                                            psram_nocache_free_release,
                                            psram_nocache_get_allocated_size,
                                            ptr, size);
    }
#endif
#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
    else if (ptr_is_hsram_heap(ptr)) {
        return hsram_realloc_release(ptr, size);
    }
#endif
    BK_ASSERT(0);
    return NULL;
#else
    return bk_heap_realloc_release_impl(sram_malloc_release, sram_free_release,
                                        sram_get_allocated_size, ptr, size);
#endif
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
#if CONFIG_MEM_DEBUG
    return bk_heap_realloc_debug_impl(hsram_malloc_debug, hsram_free_debug,
                                      hsram_get_allocated_size, func_name, line,
                                      ptr, size, need_zero);
#else
    return hsram_realloc_release(ptr, size);
#endif
}

void *hsram_realloc_release(void *ptr, size_t size)
{
    return bk_heap_realloc_release_impl(hsram_malloc_release, hsram_free_release,
                                        hsram_get_allocated_size, ptr, size);
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
#if CONFIG_MEM_DEBUG
    return bk_heap_realloc_debug_impl(psram_malloc_debug, psram_free_debug,
                                      psram_get_allocated_size, func_name, line,
                                      ptr, size, 0);
#else
    return psram_realloc_release(ptr, size);
#endif
}

void *psram_realloc_release(void *ptr, size_t size)
{
    return bk_heap_realloc_release_impl(psram_malloc_release, psram_free_release,
                                        psram_get_allocated_size, ptr, size);
}

void *psram_zalloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_psram, "NULL", 0, size, 1);
}
#endif

/* =========================== PSRAM NOCACHE HEAP =========================== */
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
#if CONFIG_MEM_DEBUG
static struct list_head s_psram_nocache_used;
void bk_heap_psram_nocache_debug_init(void)
{
    INIT_LIST_HEAD(&s_psram_nocache_used);
}
#endif

static const bk_heap_t bk_heap_psram_nocache = {
    .malloc = psram_nocache_malloc_impl,
    .free = psram_nocache_free_impl,
#if CONFIG_MEM_DEBUG
    .used_list = &s_psram_nocache_used,
#endif
};

void *psram_nocache_malloc_debug(const char *func_name, int line, size_t size, int need_zero)
{
    return bk_heap_malloc_impl(&bk_heap_psram_nocache, func_name, line, size, need_zero);
}

void psram_nocache_free_debug(const char *func_name, int line, void *ptr)
{
    bk_heap_free_impl(&bk_heap_psram_nocache, func_name, line, ptr);
}

void *psram_nocache_malloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_psram_nocache, "NULL", 0, size, 0);
}

void psram_nocache_free_release(void *ptr)
{
    bk_heap_free_impl(&bk_heap_psram_nocache, "NULL", 0, ptr);
}

void *psram_nocache_zalloc_release(size_t size)
{
    return bk_heap_malloc_impl(&bk_heap_psram_nocache, "NULL", 0, size, 1);
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
    #if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    BK_DUMP_OUT(">>>>> psram nocache heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(start_tick, ticks_since_malloc, task, &s_psram_nocache_used) != 0) {
        BK_DUMP_OUT("psram nocache heap list is empty.\r\n");
    }
    #endif
}

#if CONFIG_HEAP_UAF_AUDIT_POISON
void os_dump_heap_free_history(uint32_t count)
{
    bk_heap_debug_dump_free_history(count);
}

void os_trace_heap_free_addr(uint32_t addr)
{
    bk_heap_debug_trace_free_addr(addr);
}
#endif
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
#if defined(CONFIG_AP_PSRAM_NOCACHE_HEAP_ADDR) && (CONFIG_AP_PSRAM_NOCACHE_HEAP_SIZE > 0)
    BK_DUMP_OUT(">>>>> psram nocache heap dump memory stats.\r\n");
    if (bk_heap_debug_dump_mem_stats(0, 0, NULL, &s_psram_nocache_used) != 0) {
        BK_DUMP_OUT("psram nocache heap list is empty.\r\n");
    }
#endif
}
#endif
