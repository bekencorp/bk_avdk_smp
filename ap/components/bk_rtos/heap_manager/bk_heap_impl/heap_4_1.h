/*
 * heap_4_1.h - heap_4_1 heap algorithm implementation
 *
 * This file implements the heap_4_1 memory allocation algorithm.
 * It is based on the heap_4 algorithm from FreeRTOS.
 */

#ifndef HEAP_4_1_H
#define HEAP_4_1_H

#include "heap_ops.h"
#include "heap_types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Memory Block Link Structure
 * ============================================================================ */

/**
 * @brief Free block link structure
 *
 * This structure is placed at the beginning of each free memory block.
 * The highest bit of xBlockSize is used to mark if the block is allocated.
 * This is used by algorithms that manage free blocks in a linked list.
 */
typedef struct heap_block_link {
    struct heap_block_link *pxNextFreeBlock;  /* Next free block in the list */
    size_t xBlockSize;                         /* Block size (highest bit = allocated flag) */
} HeapBlockLink_t;

/* ============================================================================
 * Helper Macros
 * ============================================================================ */

/**
 * @brief Check if block is allocated
 */
#define HEAP_BLOCK_IS_ALLOCATED(block_size, allocated_bit) \
    (((block_size) & (allocated_bit)) != 0)

/**
 * @brief Mark block as allocated
 */
#define HEAP_BLOCK_MARK_ALLOCATED(block_size, allocated_bit) \
    ((block_size) |= (allocated_bit))

/**
 * @brief Mark block as free
 */
#define HEAP_BLOCK_MARK_FREE(block_size, allocated_bit) \
    ((block_size) &= ~(allocated_bit))

/**
 * @brief Get block size without allocation flag
 */
#define HEAP_BLOCK_GET_SIZE(block_size, allocated_bit) \
    ((block_size) & ~(allocated_bit))

/* ============================================================================
 * heap_4_1 Algorithm Specific Data Structure
 * ============================================================================ */

/**
 * @brief heap_4_1 algorithm specific data structure
 *
 * This structure contains algorithm-specific data that needs to be allocated
 * by the application layer. The application layer should allocate this structure
 * statically and pass it to bk_heap_add_region().
 */
typedef struct {
    HeapBlockLink_t xStart;        /* Free block list head */
    HeapBlockLink_t *pxEnd;        /* Free block list end marker */
    size_t xBlockAllocatedBit;     /* Bit mask to mark allocated blocks */
} Heap_4_1_Data_t;

/* ============================================================================
 * heap_4_1 Algorithm Operations
 * ============================================================================ */

/**
 * @brief heap_4_1 algorithm operations structure
 *
 * This structure exports the heap_4_1 algorithm implementation
 * through the HeapOps_t interface.
 */
extern const HeapOps_t heap_4_1_ops;


#ifdef __cplusplus
}
#endif

#endif /* HEAP_4_1_H */

