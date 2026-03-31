/*
 * heap_ops.h - Heap algorithm abstraction interface
 *
 * This file defines the abstract interface for heap management algorithms.
 * Different algorithms (heap_4_1, Best Fit, etc.) implement this interface.
 */

#ifndef HEAP_OPS_H
#define HEAP_OPS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct heap_management HeapMgmt_t;

/* ============================================================================
 * Heap Algorithm Operations Structure
 * ============================================================================ */

/**
 * @brief Heap algorithm operations structure
 *
 * This structure defines the interface that all heap algorithms must implement.
 * It uses function pointers to allow different algorithms to be used
 * interchangeably.
 */
typedef struct heap_operations {
    /**
     * @brief Initialize the heap
     *
     * @param self Pointer to heap management structure
     * @param start_addr Starting address of the heap memory
     * @param len Length of the heap memory in bytes
     * @return 0 on success, negative value on error
     */
    int (*init)(HeapMgmt_t *self, size_t start_addr, size_t len);

    /**
     * @brief Allocate memory
     *
     * @param self Pointer to heap management structure
     * @param size Size of memory to allocate in bytes
     * @return Pointer to allocated memory, or NULL on failure
     */
    void *(*malloc)(HeapMgmt_t *self, size_t size);

    /**
     * @brief Free allocated memory
     *
     * @param self Pointer to heap management structure
     * @param ptr Pointer to memory to free
     */
    void (*free)(HeapMgmt_t *self, void *ptr);

    /**
     * @brief Get current free memory size
     *
     * @param self Pointer to heap management structure
     * @return Current free memory size in bytes
     */
    size_t (*get_free_size)(HeapMgmt_t *self);

    /**
     * @brief Get minimum free memory size ever
     *
     * @param self Pointer to heap management structure
     * @return Minimum free memory size in bytes
     */
    size_t (*get_min_free_size)(HeapMgmt_t *self);

    /**
     * @brief Get heap statistics
     *
     * @param self Pointer to heap management structure
     * @param stats Pointer to statistics structure to fill
     * @return 0 on success, negative value on error
     */
    int (*get_stats)(HeapMgmt_t *self, void *stats);

    /**
     * @brief Get allocated memory size for a pointer
     *
     * @param self Pointer to heap management structure
     * @param ptr Pointer to allocated memory
     * @return Allocated size in bytes, or 0 if invalid pointer
     */
    size_t (*get_allocated_size)(HeapMgmt_t *self, void *ptr);

    /**
     * @brief Algorithm name (for debugging)
     */
    const char *name;
} HeapOps_t;

#ifdef __cplusplus
}
#endif

#endif /* HEAP_OPS_H */

