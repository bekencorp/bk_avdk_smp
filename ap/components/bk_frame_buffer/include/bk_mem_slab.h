#pragma once

#include <components/bk_frame_buffer.h>

#define  ALIGN_BYTES  (64)

#if !defined(__ALIGN_DATA)
#define __ALIGN_DATA __attribute__((aligned(ALIGN_BYTES)))
#endif

#define SLAB_ALIGN_BYTES(number, alignment)    \
        (((number) + ((alignment) - 1)) & ~((alignment) - 1))

static inline uint32_t common_mod(uint32_t val, uint32_t div)
{
    BK_ASSERT(div);
    return ((val) % (div));
}

#define COMMON_MOD(val, div) common_mod(val, div)

struct __ALIGN_DATA fb_block_free
{
    /// Used to check if memory block has been corrupted or not
    uint32_t corrupt_check;
    /// Size of the current free block (including delimiter)
    uint32_t free_size;
    /// Next free block pointer
    struct fb_block_free *next;
    /// Previous free block pointer
    struct fb_block_free *previous;
#if MEM_SLAB_MEM_DEBUG
    uint32_t reserved[11];
    uint32_t head_end_check;
#else
    uint32_t reserved[12];
#endif
};

/// Used memory block delimiter structure (size must be word multiple)
typedef __ALIGN_DATA struct
{
    /// Used to check if memory block has been cor rupted or not
    uint32_t corrupt_check;
#if MEM_SLAB_MEM_DEBUG
    const char *func;
    uint32_t line;
#endif
    /// Size of the current used block (including delimiter)
    uint32_t size;
    uint32_t flag;
    uint32_t write_through_channel;
#if MEM_SLAB_MEM_DEBUG
    uint32_t user_size;
    uint32_t reserved[8];
    uint32_t head_end_check;
#else
    uint32_t reserved[12];
#endif
} fb_block_used;

typedef struct
{
    /// Root pointer = pointer to first element of heap linked lists
    struct fb_block_free *heap[MEM_SLAB_HEAP_MAX];
    /// Size of heaps
    uint32_t heap_size[MEM_SLAB_HEAP_MAX];
} fb_mem_heap_t;

// init psram mem to different heap block
void bk_mem_slab_init(void);

void bk_mem_slab_heap_init(uint8_t type, uint8_t *heap, uint32_t heap_size);

void bk_mem_slab_heap_resume(uint8_t type, uint8_t *heap, uint32_t heap_size);

#if MEM_SLAB_MEM_DEBUG
void *bk_mem_slab_malloc_debug(frame_buffer_heap_type_t type, uint32_t size, const char *func, uint32_t line);
#else
void *bk_mem_slab_malloc(frame_buffer_heap_type_t type, uint32_t size);
#endif

bk_err_t bk_mem_slab_set(void *mem_ptr, uint32_t flags);
void bk_mem_slab_free(void *mem_ptr);