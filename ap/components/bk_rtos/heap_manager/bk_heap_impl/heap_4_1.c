/*
 * heap_4_1.c - heap_4_1 heap algorithm implementation
 *
 * This file implements the heap_4_1 memory allocation algorithm.
 * It is based on the heap_4 algorithm from FreeRTOS.
 */

#include "heap_4_1.h"
#include "heap_types.h"
#include "bk_heap_config.h"
#include "port_heap.h"
#include "bk_heap.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

/* ============================================================================
 * Constants and Macros
 * ============================================================================ */

/* Block sizes must not get too small */
#define HEAP_MINIMUM_BLOCK_SIZE    ((sizeof(HeapBlockLink_t) << 1))

/* Assumes 8bit bytes */
#define HEAP_BITS_PER_BYTE         (8)

/* The size of the structure placed at the beginning of each allocated memory
 * block must be correctly byte aligned. */
static const size_t xHeapStructSize = (sizeof(HeapBlockLink_t) + 
    ((size_t)(BK_HEAP_BYTE_ALIGNMENT - 1))) & 
    ~((size_t)BK_HEAP_BYTE_ALIGNMENT_MASK);

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

/**
 * @brief Initialize the heap
 *
 * @param data heap_4_1 algorithm data
 * @param self Heap management structure
 * @param start_addr Starting address of heap memory
 * @param len Length of heap memory
 */
static void prvHeapInit(Heap_4_1_Data_t *data, HeapMgmt_t *self, 
                        size_t start_addr, size_t len);

/**
 * @brief Insert a block into the free list (with automatic merging)
 *
 * @param data heap_4_1 algorithm data
 * @param pxBlockToInsert Block to insert
 */
static void prvInsertBlockIntoFreeList(Heap_4_1_Data_t *data, 
                                       HeapBlockLink_t *pxBlockToInsert);

/* ============================================================================
 * Heap Operations Implementation
 * ============================================================================ */

/**
 * @brief Initialize the heap
 */
static int heap_4_1_init(HeapMgmt_t *self, size_t start_addr, size_t len)
{
    Heap_4_1_Data_t *data;
    HeapBlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize = len;

    /* Check parameters */
    if (self == NULL || len == 0) {
        return BK_HEAP_STATUS_INVALID_PARAM;
    }

    /* Get heap_4_1 data from algorithm_data (set by abstraction layer) */
    if (self->algorithm_data == NULL) {
        return BK_HEAP_STATUS_ERROR;
    }
    data = (Heap_4_1_Data_t *)self->algorithm_data;
    port_heap_memset(data, 0, sizeof(Heap_4_1_Data_t));

    /* Ensure the heap starts on a correctly aligned boundary */
    uxAddress = start_addr;
    if ((uxAddress & BK_HEAP_BYTE_ALIGNMENT_MASK) != 0) {
        uxAddress += (BK_HEAP_BYTE_ALIGNMENT - 1);
        uxAddress &= ~((size_t)BK_HEAP_BYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= uxAddress - start_addr;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free blocks */
    data->xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
    data->xStart.xBlockSize = (size_t)0;

    /* pxEnd is used to mark the end of the list of free blocks */
    uxAddress = ((size_t)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~((size_t)BK_HEAP_BYTE_ALIGNMENT_MASK);
    data->pxEnd = (void *)uxAddress;
    data->pxEnd->xBlockSize = 0;
    data->pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that covers the entire heap space */
    pxFirstFreeBlock = (void *)pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = data->pxEnd;

    /* Work out the position of the top bit in a size_t variable */
    data->xBlockAllocatedBit = ((size_t)1) << 
        ((sizeof(size_t) * HEAP_BITS_PER_BYTE) - 1);

    /* Initialize statistics */
    self->xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    self->xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    self->xNumberOfSuccessfulAllocations = 0;
    self->xNumberOfSuccessfulFrees = 0;

    /* Save heap information */
    self->heap_start_addr = uxAddress;
    self->heap_len = xTotalHeapSize;
    /* Note: algorithm_data is already set by abstraction layer before calling init */

    return BK_HEAP_STATUS_OK;
}

/**
 * @brief Allocate memory using heap_4_1 algorithm
 */
static void *heap_4_1_malloc(HeapMgmt_t *self, size_t size)
{
    Heap_4_1_Data_t *data;
    HeapBlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
    void *pvReturn = NULL;
    size_t xWantedSize = size;

    if (self == NULL || self->algorithm_data == NULL) {
        return NULL;
    }

    data = (Heap_4_1_Data_t *)self->algorithm_data;

    port_heap_enter_critical();
    {
        /* Check the requested block size is not so large that the top bit is set */
        if ((xWantedSize & data->xBlockAllocatedBit) == 0) {
            /* The wanted size must be increased to contain a HeapBlockLink_t structure */
            if ((xWantedSize > 0) && 
                ((xWantedSize + xHeapStructSize) > xWantedSize)) {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned */
                if ((xWantedSize & BK_HEAP_BYTE_ALIGNMENT_MASK) != 0x00) {
                    /* Byte alignment required. Check for overflow */
                    if ((xWantedSize + (BK_HEAP_BYTE_ALIGNMENT - 
                         (xWantedSize & BK_HEAP_BYTE_ALIGNMENT_MASK))) > xWantedSize) {
                        xWantedSize += (BK_HEAP_BYTE_ALIGNMENT - 
                                       (xWantedSize & BK_HEAP_BYTE_ALIGNMENT_MASK));
                        PORT_HEAP_ASSERT((xWantedSize & BK_HEAP_BYTE_ALIGNMENT_MASK) == 0);
                    } else {
                        xWantedSize = 0;
                    }
                }
            } else {
                xWantedSize = 0;
            }
        } else {
            xWantedSize = 0;
        }

        if ((xWantedSize > 0) && (xWantedSize <= self->xFreeBytesRemaining)) {
            /* Traverse the list from the start (lowest address) block until
             * one of adequate size is found (heap_4_1) */
            pxPreviousBlock = &(data->xStart);
            pxBlock = data->xStart.pxNextFreeBlock;

            while ((pxBlock->xBlockSize < xWantedSize) && 
                   (pxBlock->pxNextFreeBlock != NULL)) {
                pxPreviousBlock = pxBlock;
                pxBlock = pxBlock->pxNextFreeBlock;
            }

            /* If the end marker was not reached then a block of adequate size was found */
            if (pxBlock != data->pxEnd) {
                /* Return the memory space - jumping over the HeapBlockLink_t structure */
                pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) + 
                                   xHeapStructSize);

                /* This block is being returned so must be taken out of the free list */
                pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                /* If the block is larger than required it can be split into two */
                if ((pxBlock->xBlockSize - xWantedSize) > HEAP_MINIMUM_BLOCK_SIZE) {
                    /* Create a new block following the number of bytes requested */
                    pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
                    PORT_HEAP_ASSERT((((size_t)pxNewBlockLink) & 
                                     BK_HEAP_BYTE_ALIGNMENT_MASK) == 0);

                    /* Calculate the sizes of two blocks split from the single block */
                    pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                    pxBlock->xBlockSize = xWantedSize;

                    /* Insert the new block into the list of free blocks */
                    prvInsertBlockIntoFreeList(data, pxNewBlockLink);
                }

                /* Update statistics */
                self->xFreeBytesRemaining -= pxBlock->xBlockSize;
                if (self->xFreeBytesRemaining < self->xMinimumEverFreeBytesRemaining) {
                    self->xMinimumEverFreeBytesRemaining = self->xFreeBytesRemaining;
                }

                /* The block is being returned - it is allocated and owned by the application */
                pxBlock->xBlockSize |= data->xBlockAllocatedBit;
                pxBlock->pxNextFreeBlock = NULL;
                self->xNumberOfSuccessfulAllocations++;
            }
        }
    }
    port_heap_exit_critical();

    PORT_HEAP_ASSERT((((size_t)pvReturn) & (size_t)BK_HEAP_BYTE_ALIGNMENT_MASK) == 0);
    return pvReturn;
}

/**
 * @brief Free allocated memory
 */
static void heap_4_1_free(HeapMgmt_t *self, void *ptr)
{
    Heap_4_1_Data_t *data;
    uint8_t *puc;
    HeapBlockLink_t *pxLink;

    if (self == NULL || self->algorithm_data == NULL || ptr == NULL) {
        return;
    }

    data = (Heap_4_1_Data_t *)self->algorithm_data;

    /* The memory being freed will have a HeapBlockLink_t structure immediately before it */
    puc = (uint8_t *)ptr;
    puc -= xHeapStructSize;
    pxLink = (void *)puc;

    /* Check the block is actually allocated */
    if ((pxLink->xBlockSize & data->xBlockAllocatedBit) != 0) {
        if (pxLink->pxNextFreeBlock == NULL) {
            /* The block is being returned to the heap - it is no longer allocated */
            pxLink->xBlockSize &= ~data->xBlockAllocatedBit;

            port_heap_enter_critical();
            {
                /* Add this block to the list of free blocks */
                self->xFreeBytesRemaining += pxLink->xBlockSize;
                prvInsertBlockIntoFreeList(data, pxLink);
                self->xNumberOfSuccessfulFrees++;
            }
            port_heap_exit_critical();
        }
    }
}

/**
 * @brief Get current free memory size
 */
static size_t heap_4_1_get_free_size(HeapMgmt_t *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->xFreeBytesRemaining;
}

/**
 * @brief Get minimum free memory size ever
 */
static size_t heap_4_1_get_min_free_size(HeapMgmt_t *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->xMinimumEverFreeBytesRemaining;
}

/**
 * @brief Get heap statistics
 */
static int heap_4_1_get_stats(HeapMgmt_t *self, void *stats)
{
    Heap_4_1_Data_t *data;
    HeapBlockLink_t *pxBlock;
    bk_heap_stats_t *pStats;
    size_t xBlocks = 0;
    size_t xMaxSize = 0;
    size_t xMinSize = SIZE_MAX;

    if (self == NULL || self->algorithm_data == NULL || stats == NULL) {
        return BK_HEAP_STATUS_INVALID_PARAM;
    }

    data = (Heap_4_1_Data_t *)self->algorithm_data;
    pStats = (bk_heap_stats_t *)stats;

    port_heap_enter_critical();
    {
        pxBlock = data->xStart.pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised */
        if (pxBlock != NULL) {
            do {
                /* Increment the number of blocks and record the largest/smallest block */
                xBlocks++;

                if (pxBlock->xBlockSize > xMaxSize) {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if (pxBlock->xBlockSize < xMinSize) {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain */
                pxBlock = pxBlock->pxNextFreeBlock;
            } while (pxBlock != data->pxEnd);
        }
    }
    port_heap_exit_critical();

    /* Fill statistics structure */
    pStats->free_bytes = self->xFreeBytesRemaining;
    pStats->min_free_bytes = self->xMinimumEverFreeBytesRemaining;
    pStats->alloc_count = self->xNumberOfSuccessfulAllocations;
    pStats->free_count = self->xNumberOfSuccessfulFrees;
    pStats->largest_free_block = xMaxSize;
    pStats->smallest_free_block = (xMinSize == SIZE_MAX) ? 0 : xMinSize;
    pStats->free_block_count = xBlocks;

    return BK_HEAP_STATUS_OK;
}

/**
 * @brief Get allocated memory size for a pointer
 */
static size_t heap_4_1_get_allocated_size(HeapMgmt_t *self, void *ptr)
{
    Heap_4_1_Data_t *data;
    uint8_t *puc;
    HeapBlockLink_t *pxLink;
    size_t sz = 0;

    if (self == NULL || self->algorithm_data == NULL || ptr == NULL) {
        return 0;
    }

    data = (Heap_4_1_Data_t *)self->algorithm_data;

    /* The memory will have a HeapBlockLink_t structure immediately before it */
    puc = (uint8_t *)ptr;
    puc -= xHeapStructSize;
    pxLink = (void *)puc;

    /* Check if the block is actually allocated */
    if ((pxLink->xBlockSize & data->xBlockAllocatedBit) != 0) {
        /* Return the size without the allocation bit and metadata */
        sz = HEAP_BLOCK_GET_SIZE(pxLink->xBlockSize, data->xBlockAllocatedBit);
        if (sz > xHeapStructSize) {
            sz -= xHeapStructSize;
        } else {
            sz = 0;
        }
    }

    return sz;
}

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/**
 * @brief Insert a block into the free list (with automatic merging)
 */
static void prvInsertBlockIntoFreeList(Heap_4_1_Data_t *data, 
                                       HeapBlockLink_t *pxBlockToInsert)
{
    HeapBlockLink_t *pxIterator;
    uint8_t *puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted */
    for (pxIterator = &(data->xStart); 
         pxIterator->pxNextFreeBlock < pxBlockToInsert; 
         pxIterator = pxIterator->pxNextFreeBlock) {
        /* Nothing to do here, just iterate to the right position */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxIterator;

    if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert) {
        /* Merge with the previous block */
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = (uint8_t *)pxBlockToInsert;

    if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock) {
        if (pxIterator->pxNextFreeBlock != data->pxEnd) {
            /* Form one big block from the two blocks */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        } else {
            pxBlockToInsert->pxNextFreeBlock = data->pxEnd;
        }
    } else {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gap, so was merged with the block
     * before and the block after, then its pxNextFreeBlock pointer will have
     * already been set */
    if (pxIterator != pxBlockToInsert) {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
}

/* ============================================================================
 * Export Algorithm Operations
 * ============================================================================ */

const HeapOps_t heap_4_1_ops = {
    .init = heap_4_1_init,
    .malloc = heap_4_1_malloc,
    .free = heap_4_1_free,
    .get_free_size = heap_4_1_get_free_size,
    .get_min_free_size = heap_4_1_get_min_free_size,
    .get_stats = heap_4_1_get_stats,
    .get_allocated_size = heap_4_1_get_allocated_size,
    .name = "heap_4_1"
};

