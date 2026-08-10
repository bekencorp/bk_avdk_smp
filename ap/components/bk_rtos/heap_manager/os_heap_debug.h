#include "bk_list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEM_CHECK_TAG_LEN 4

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_MEM_DEBUG

typedef struct {
    struct list_head node;
    const char *funcName;
    const char *taskName;
    union {
        struct {
            uint16_t allocTime;
            uint16_t line;
        };
        uint32_t time_line;
    };
    uint32_t wantedSize;
} bk_heap_debug_info_t;


static inline bool bk_heap_debug_get_real_size(size_t size, size_t *real_size)
{
    const size_t overhead = sizeof(bk_heap_debug_info_t) + MEM_CHECK_TAG_LEN;

    if (size > SIZE_MAX - overhead) {
        return false;
    }
    *real_size = size + overhead;
    return true;
}

static inline void *bk_heap_debug_get_ptr(void *ptr)
{
    return (void *)((char *)ptr + sizeof(bk_heap_debug_info_t));
}

static inline void *bk_heap_debug_get_real_ptr(void *ptr)
{
    return (void *)((char *)ptr - sizeof(bk_heap_debug_info_t));
}

void bk_heap_debug_add_debug_info(struct list_head *list, void *ptr,
                        const char *func_name, uint16_t line, uint32_t size);

void bk_heap_debug_remove_debug_info(void *ptr);

void bk_heap_sram_debug_init(void);
void bk_heap_hsram_debug_init(void);
void bk_heap_psram_debug_init(void);
void bk_heap_psram_nocache_debug_init(void);

size_t os_heap_get_allocated_size(void *ptr);

void bk_heap_overflow_check(void *ptr);
void bk_heap_fill_overflow_tag(void *ptr);
int bk_heap_debug_dump_mem_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char *task, struct list_head *list);

#if CONFIG_HEAP_UAF_AUDIT_POISON
typedef void (*bk_heap_debug_free_func_t)(void *ptr);

void bk_heap_debug_record_free(void *ptr, const char *free_func, uint16_t free_line);
void bk_heap_debug_dump_free_history(uint32_t count);
void bk_heap_debug_trace_free_addr(uint32_t addr);
void bk_heap_debug_poison_after_free(void *ptr);

#if CONFIG_HEAP_UAF_QUARANTINE
void bk_heap_debug_quarantine_free(void *ptr, uint32_t size, bk_heap_debug_free_func_t free_func);
#endif
#endif
#else

static inline bool bk_heap_debug_get_real_size(size_t size, size_t *real_size)
{
    *real_size = size;
    return true;
}

static inline void *bk_heap_debug_get_ptr(void *ptr)
{
    return ptr;
}
#endif

#ifdef __cplusplus
}
#endif