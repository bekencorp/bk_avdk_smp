/*
 * bk_heap.c - Multi-region heap management implementation
 *
 * This file implements the multi-region heap management layer.
 * It manages multiple independent heap regions and delegates
 * actual memory operations to algorithm implementations.
 */

#include "bk_heap.h"
#include "heap_ops.h"
#include "heap_types.h"
#include "bk_heap_config.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ============================================================================
 * Algorithm Selection
 * ============================================================================ */

/* Note: Algorithm selection is now explicit - users must provide HeapOps_t *
 * when calling bk_heap_add_region(). This removes the dependency on
 * implementation-specific details from the abstraction layer.
 */

/* ============================================================================
 * Region Management
 * ============================================================================ */

/* Region management array */
static HeapMgmt_t g_heap_regions[BK_HEAP_MAX_REGION] = {{0}};

/* Magic number for validation */
#define BK_HEAP_MAGIC       (0x48454150)  /* "HEAP" */

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/**
 * @brief Get a free heap slot
 *
 * @return Pointer to free heap slot, or NULL if no free slot
 */
static HeapMgmt_t *get_free_heap_slot(void)
{
    uint32_t i;
    HeapMgmt_t *heap;

    for (i = 0; i < BK_HEAP_MAX_REGION; i++) {
        heap = &g_heap_regions[i];
        if (heap->is_inited == 0) {
            return heap;
        }
    }

    return NULL;
}

/**
 * @brief Get heap by region ID
 *
 * @param region_id Region ID
 * @return Pointer to heap management structure, or NULL if invalid
 */
static HeapMgmt_t *get_heap_by_id(bk_heap_region_id_t region_id)
{
    if (region_id < 0 || region_id >= BK_HEAP_MAX_REGION) {
        return NULL;
    }

    return &g_heap_regions[region_id];
}

/**
 * @brief Get region ID from heap pointer
 *
 * @param heap Heap management structure pointer
 * @return Region ID, or BK_HEAP_INVALID_REGION_ID if not found
 */
static bk_heap_region_id_t get_region_id(HeapMgmt_t *heap)
{
    uint32_t i;

    if (heap == NULL) {
        return BK_HEAP_INVALID_REGION_ID;
    }

    for (i = 0; i < BK_HEAP_MAX_REGION; i++) {
        if (&g_heap_regions[i] == heap) {
            return (bk_heap_region_id_t)i;
        }
    }

    return BK_HEAP_INVALID_REGION_ID;
}

/**
 * @brief Validate region ID
 *
 * @param region_id Region ID to validate
 * @return Pointer to heap management structure, or NULL if invalid
 */
static HeapMgmt_t *validate_region_id(bk_heap_region_id_t region_id)
{
    HeapMgmt_t *heap;

    if (region_id == BK_HEAP_INVALID_REGION_ID) {
        return NULL;
    }

    heap = get_heap_by_id(region_id);
    if (heap == NULL || heap->is_inited == 0) {
        return NULL;
    }

    /* Validate magic number */
    if (heap->magic != BK_HEAP_MAGIC) {
        return NULL;
    }

    return heap;
}

/* ============================================================================
 * Region Management APIs
 * ============================================================================ */

bk_heap_region_id_t bk_heap_add_region(size_t start_addr, size_t len, const HeapOps_t *ops, void *algorithm_data)
{
    HeapMgmt_t *heap;
    int ret;

    /* Check parameters */
    if (len == 0) {
        return BK_HEAP_INVALID_REGION_ID;
    }

    /* ops parameter must not be NULL */
    if (ops == NULL) {
        return BK_HEAP_INVALID_REGION_ID;
    }

    /* Get a free heap slot */
    heap = get_free_heap_slot();
    if (heap == NULL) {
        return BK_HEAP_INVALID_REGION_ID;
    }

    /* Initialize heap structure */
    memset(heap, 0, sizeof(HeapMgmt_t));
    heap->magic = BK_HEAP_MAGIC;
    heap->ops = ops;
    heap->algorithm_data = algorithm_data;

    /* Call algorithm initialization */
    ret = heap->ops->init(heap, start_addr, len);
    if (ret != BK_HEAP_STATUS_OK) {
        return BK_HEAP_INVALID_REGION_ID;
    }

    /* Mark as initialized */
    heap->is_inited = 1;

    return get_region_id(heap);
}

bk_heap_status_t bk_heap_delete_region(bk_heap_region_id_t region_id)
{
    HeapMgmt_t *heap;

    heap = validate_region_id(region_id);
    if (heap == NULL) {
        return BK_HEAP_STATUS_INVALID_REGION;
    }

    /* Clear heap structure */
    memset(heap, 0, sizeof(HeapMgmt_t));

    return BK_HEAP_STATUS_OK;
}

/* ============================================================================
 * Memory Allocation APIs
 * ============================================================================ */

void *bk_heap_malloc(bk_heap_region_id_t region_id, size_t size)
{
    HeapMgmt_t *heap;

    if (size == 0) {
        return NULL;
    }

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return NULL;
    }

    return heap->ops->malloc(heap, size);
}

void bk_heap_free(bk_heap_region_id_t region_id, void *ptr)
{
    HeapMgmt_t *heap;

    if (ptr == NULL) {
        return;
    }

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return;
    }

    heap->ops->free(heap, ptr);
}

void *bk_heap_calloc(bk_heap_region_id_t region_id, size_t num, size_t size)
{
    void *ptr;
    size_t total;

    /* Check for multiplication overflow */
    if (num == 0 || size == 0) {
        return NULL;
    }

    total = num * size;
    if ((size != 0) && ((total / size) != num)) {
        return NULL;  /* Overflow */
    }

    ptr = bk_heap_malloc(region_id, total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }

    return ptr;
}

void *bk_heap_realloc(bk_heap_region_id_t region_id, void *ptr, size_t size)
{
    HeapMgmt_t *heap;
    void *new_ptr;
    size_t old_size;
    size_t copy_size;

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return NULL;
    }

    /* If ptr is NULL, equivalent to malloc */
    if (ptr == NULL) {
        return bk_heap_malloc(region_id, size);
    }

    /* If size is 0, equivalent to free and return NULL */
    if (size == 0) {
        bk_heap_free(region_id, ptr);
        return NULL;
    }

    /* Get old size */
    old_size = heap->ops->get_allocated_size(heap, ptr);
    if (old_size == 0) {
        /* Invalid pointer */
        return NULL;
    }

    /* If new size equals old size, return same pointer */
    if (size == old_size) {
        return ptr;
    }

    /* Allocate new block */
    new_ptr = bk_heap_malloc(region_id, size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* Copy data from old block to new block */
    copy_size = (size < old_size) ? size : old_size;
    memcpy(new_ptr, ptr, copy_size);

    /* Free old block */
    bk_heap_free(region_id, ptr);

    return new_ptr;
}

/* ============================================================================
 * Information Query APIs
 * ============================================================================ */

size_t bk_heap_get_free_size(bk_heap_region_id_t region_id)
{
    HeapMgmt_t *heap;

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return 0;
    }

    return heap->ops->get_free_size(heap);
}

size_t bk_heap_get_min_free_size(bk_heap_region_id_t region_id)
{
    HeapMgmt_t *heap;

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return 0;
    }

    return heap->ops->get_min_free_size(heap);
}

bk_heap_status_t bk_heap_get_stats(bk_heap_region_id_t region_id, bk_heap_stats_t *stats)
{
    HeapMgmt_t *heap;

    if (stats == NULL) {
        return BK_HEAP_STATUS_INVALID_PARAM;
    }

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return BK_HEAP_STATUS_INVALID_REGION;
    }

    return heap->ops->get_stats(heap, stats);
}

size_t bk_heap_get_allocated_size(bk_heap_region_id_t region_id, void *ptr)
{
    HeapMgmt_t *heap;

    if (ptr == NULL) {
        return 0;
    }

    heap = validate_region_id(region_id);
    if (heap == NULL || heap->ops == NULL) {
        return 0;
    }

    return heap->ops->get_allocated_size(heap, ptr);
}

