/*
 * bk_heap.h - Public API for RTOS heap management
 *
 * This is the main header file for users of the heap management system.
 * Include this file to use the heap management APIs.
 */

#ifndef BK_HEAP_H
#define BK_HEAP_H

#include <stddef.h>
#include <stdint.h>
#include "heap_types.h"
#include "heap_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/**
 * @brief Heap region ID type
 */
typedef int bk_heap_region_id_t;

/**
 * @brief Heap status type
 */
typedef int bk_heap_status_t;

/* ============================================================================
 * Constants
 * ============================================================================ */

/**
 * @brief Invalid region ID
 */
#define BK_HEAP_INVALID_REGION_ID        (-1)

/* ============================================================================
 * Status Codes
 * ============================================================================ */

/**
 * @brief Operation successful
 */
#define BK_HEAP_STATUS_OK                (0)

/**
 * @brief General error
 */
#define BK_HEAP_STATUS_ERROR             (-1)

/**
 * @brief Invalid parameter
 */
#define BK_HEAP_STATUS_INVALID_PARAM     (-2)

/**
 * @brief Invalid region ID
 */
#define BK_HEAP_STATUS_INVALID_REGION    (-3)

/**
 * @brief No memory available
 */
#define BK_HEAP_STATUS_NO_MEMORY         (-4)

/**
 * @brief Region slots full (cannot add more regions)
 */
#define BK_HEAP_STATUS_REGION_FULL       (-5)

/* ============================================================================
 * Statistics Structure
 * ============================================================================ */

/* Statistics structure is defined in heap_types.h */

/* ============================================================================
 * Region Management APIs
 * ============================================================================ */

/**
 * @brief Add a new heap region
 *
 * @param start_addr Starting address of the heap memory
 * @param len Length of the heap memory in bytes
 * @param ops Pointer to heap algorithm operations. Must not be NULL.
 * @param algorithm_data Pointer to algorithm-specific data. Must be allocated by the application
 *                       layer. For heap_4_1 algorithm, use sizeof(Heap_4_1_Data_t) to determine
 *                       the size. Can be NULL if the algorithm does not require algorithm_data.
 * @return Region ID on success, BK_HEAP_INVALID_REGION_ID on failure
 */
bk_heap_region_id_t bk_heap_add_region(size_t start_addr, size_t len, const HeapOps_t *ops, void *algorithm_data);

/**
 * @brief Delete a heap region
 *
 * @param region_id Region ID to delete
 * @return BK_HEAP_STATUS_OK on success, error code on failure
 */
bk_heap_status_t bk_heap_delete_region(bk_heap_region_id_t region_id);

/* ============================================================================
 * Memory Allocation APIs
 * ============================================================================ */

/**
 * @brief Allocate memory from a heap region
 *
 * @param region_id Region ID
 * @param size Size of memory to allocate in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *bk_heap_malloc(bk_heap_region_id_t region_id, size_t size);

/**
 * @brief Free allocated memory
 *
 * @param region_id Region ID
 * @param ptr Pointer to memory to free
 */
void bk_heap_free(bk_heap_region_id_t region_id, void *ptr);

/**
 * @brief Allocate and zero-initialize memory
 *
 * @param region_id Region ID
 * @param num Number of elements
 * @param size Size of each element in bytes
 * @return Pointer to allocated memory, or NULL on failure
 */
void *bk_heap_calloc(bk_heap_region_id_t region_id, size_t num, size_t size);

/**
 * @brief Reallocate memory
 *
 * @param region_id Region ID
 * @param ptr Pointer to previously allocated memory
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *bk_heap_realloc(bk_heap_region_id_t region_id, void *ptr, size_t size);

/* ============================================================================
 * Information Query APIs
 * ============================================================================ */

/**
 * @brief Get current free memory size
 *
 * @param region_id Region ID
 * @return Current free memory size in bytes
 */
size_t bk_heap_get_free_size(bk_heap_region_id_t region_id);

/**
 * @brief Get minimum free memory size ever
 *
 * @param region_id Region ID
 * @return Minimum free memory size in bytes
 */
size_t bk_heap_get_min_free_size(bk_heap_region_id_t region_id);

/**
 * @brief Get heap statistics
 *
 * @param region_id Region ID
 * @param stats Pointer to statistics structure to fill
 * @return BK_HEAP_STATUS_OK on success, error code on failure
 */
bk_heap_status_t bk_heap_get_stats(bk_heap_region_id_t region_id, bk_heap_stats_t *stats);

/**
 * @brief Get allocated memory size for a pointer
 *
 * @param region_id Region ID
 * @param ptr Pointer to allocated memory
 * @return Allocated size in bytes, or 0 if invalid pointer
 */
size_t bk_heap_get_allocated_size(bk_heap_region_id_t region_id, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* BK_HEAP_H */

