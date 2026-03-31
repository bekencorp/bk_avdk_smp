/*
 * heap_types.h - Type definitions for heap abstraction layer
 *
 * This file contains type definitions that are shared between the abstraction
 * layer and the public API. It does not contain implementation-specific details.
 */

#ifndef HEAP_TYPES_H
#define HEAP_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include "heap_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Heap Statistics Structure (Public API)
 * ============================================================================ */

/**
 * @brief Heap statistics structure
 *
 * This structure is used by the public API to report heap statistics.
 */
typedef struct bk_heap_stats {
    size_t free_bytes;              /* Current free memory in bytes */
    size_t min_free_bytes;          /* Minimum free memory ever in bytes */
    size_t alloc_count;             /* Number of successful allocations */
    size_t free_count;              /* Number of successful frees */
    size_t largest_free_block;      /* Largest free block size in bytes */
    size_t smallest_free_block;     /* Smallest free block size in bytes */
    size_t free_block_count;        /* Number of free blocks */
} bk_heap_stats_t;

/* ============================================================================
 * Heap Management Structure (Abstraction Layer)
 * ============================================================================ */

/**
 * @brief Heap management structure
 *
 * This structure contains the information needed by the abstraction layer
 * to manage heap regions. Algorithm-specific data is stored in algorithm_data
 * pointer, which is managed by the implementation layer.
 *
 * Note: The abstraction layer directly accesses the fields in this structure
 * for performance reasons. These fields are stable and won't change with
 * different algorithm implementations.
 */
typedef struct heap_management {
    /* Validation and state */
    uint32_t magic;                    /* Magic number for validation */
    uint32_t is_inited;                 /* Initialization flag */

    /* Heap memory information */
    size_t heap_start_addr;             /* Heap starting address */
    size_t heap_len;                    /* Heap length in bytes */

    /* Algorithm interface */
    const HeapOps_t *ops;               /* Pointer to algorithm operations */
    void *algorithm_data;               /* Pointer to algorithm-specific data (managed by implementation layer) */

    /* Basic statistics (accessed directly by abstraction layer) */
    size_t xFreeBytesRemaining;         /* Current free memory in bytes */
    size_t xMinimumEverFreeBytesRemaining; /* Minimum free memory ever in bytes */
    size_t xNumberOfSuccessfulAllocations; /* Number of successful allocations */
    size_t xNumberOfSuccessfulFrees;   /* Number of successful frees */
} HeapMgmt_t;

#ifdef __cplusplus
}
#endif

#endif /* HEAP_TYPES_H */

