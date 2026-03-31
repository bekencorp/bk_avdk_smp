#include "bk_list.h"
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


static inline uint32_t bk_heap_debug_get_real_size(uint32_t size)
{
    return size + sizeof(bk_heap_debug_info_t) + MEM_CHECK_TAG_LEN;
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

size_t sram_get_allocated_size(void *ptr);
size_t psram_get_allocated_size(void *ptr);
size_t hsram_get_allocated_size(void *ptr);

size_t os_heap_get_allocated_size(void *ptr);

void bk_heap_overflow_check(void *ptr);
void bk_heap_fill_overflow_tag(void *ptr);
int bk_heap_debug_dump_mem_stats(uint32_t start_tick, uint32_t ticks_since_malloc, const char *task, struct list_head *list);
#else

static inline uint32_t bk_heap_debug_get_real_size(uint32_t size)
{
    return size;
}

static inline void *bk_heap_debug_get_ptr(void *ptr)
{
    return ptr;
}
#endif

#ifdef __cplusplus
}
#endif