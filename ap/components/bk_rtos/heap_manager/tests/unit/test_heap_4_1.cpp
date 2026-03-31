/*
 * test_heap_4_1.cpp - Unit tests for heap_4_1 algorithm implementation
 *
 * This file contains unit tests for the heap_4_1 algorithm.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "bk_heap.h"
#include "heap_4_1.h"
#include "heap_ops.h"
#include "heap_types.h"
#include "bk_heap_config.h"
#include "mock_port_heap.h"
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

/* Static memory pool for testing */
static uint8_t test_heap_pool[256 * 1024] __attribute__((aligned(8)));

class Heap4_1Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Use static memory pool
        test_heap_addr = (size_t)test_heap_pool;
        test_heap_size = 64 * 1024;  // 64KB
        
        /* Initialize heap structure */
        memset(&heap, 0, sizeof(heap));
        heap.ops = &heap_4_1_ops;
        
        /* Allocate algorithm-specific data */
        /* Use sizeof(Heap_4_1_Data_t) directly since it's now in the header */
        heap.algorithm_data = malloc(sizeof(Heap_4_1_Data_t));
        if (heap.algorithm_data != NULL) {
            memset(heap.algorithm_data, 0, sizeof(Heap_4_1_Data_t));
        }
        
        /* Reset mock counters */
        mock_reset_counters();
    }

    void TearDown() override {
        /* Cleanup if needed */
    }

    HeapMgmt_t heap;
    size_t test_heap_addr;
    size_t test_heap_size;
};

/* ============================================================================
 * Algorithm Initialization Tests
 * ============================================================================ */

/* Test algorithm initialization */
TEST_F(Heap4_1Test, AlgorithmInit) {
    int ret;
    
    ret = heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    EXPECT_EQ(ret, BK_HEAP_STATUS_OK);
    EXPECT_NE(heap.algorithm_data, nullptr);
    /* Note: is_inited is managed by abstraction layer, not algorithm implementation */
    EXPECT_GT(heap.xFreeBytesRemaining, 0);
}

/* Test algorithm initialization with invalid parameters */
TEST_F(Heap4_1Test, AlgorithmInitInvalidParams) {
    int ret;
    
    /* NULL heap pointer */
    ret = heap.ops->init(nullptr, test_heap_addr, test_heap_size);
    EXPECT_NE(ret, BK_HEAP_STATUS_OK);
    
    /* Zero length */
    ret = heap.ops->init(&heap, test_heap_addr, 0);
    EXPECT_NE(ret, BK_HEAP_STATUS_OK);
}

/* ============================================================================
 * Algorithm Allocation Tests
 * ============================================================================ */

/* Test algorithm malloc */
TEST_F(Heap4_1Test, AlgorithmMalloc) {
    void *ptr;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    ptr = heap.ops->malloc(&heap, 1024);
    EXPECT_NE(ptr, nullptr);
    
    /* Verify alignment */
    EXPECT_EQ(((size_t)ptr) & 0x7, 0);
}

/* Test algorithm malloc with zero size */
TEST_F(Heap4_1Test, AlgorithmMallocZeroSize) {
    void *ptr;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    ptr = heap.ops->malloc(&heap, 0);
    EXPECT_EQ(ptr, nullptr);
}

/* Test algorithm malloc with large size */
TEST_F(Heap4_1Test, AlgorithmMallocLargeSize) {
    void *ptr;
    size_t free_size;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    free_size = heap.ops->get_free_size(&heap);
    ptr = heap.ops->malloc(&heap, free_size + 1);
    EXPECT_EQ(ptr, nullptr);
}

/* ============================================================================
 * Algorithm Free Tests
 * ============================================================================ */

/* Test algorithm free */
TEST_F(Heap4_1Test, AlgorithmFree) {
    void *ptr;
    size_t free_before, free_after;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    free_before = heap.ops->get_free_size(&heap);
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    
    heap.ops->free(&heap, ptr);
    free_after = heap.ops->get_free_size(&heap);
    
    EXPECT_EQ(free_before, free_after);
}

/* Test algorithm free with NULL pointer */
TEST_F(Heap4_1Test, AlgorithmFreeNullPointer) {
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    /* Should not crash */
    heap.ops->free(&heap, nullptr);
}

/* ============================================================================
 * Block Split and Merge Tests
 * ============================================================================ */

/* Test block split */
TEST_F(Heap4_1Test, BlockSplit) {
    void *ptr1, *ptr2;
    size_t free_before, free_after;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    free_before = heap.ops->get_free_size(&heap);
    
    /* Allocate large block, should split */
    ptr1 = heap.ops->malloc(&heap, 10 * 1024);
    ASSERT_NE(ptr1, nullptr);
    
    free_after = heap.ops->get_free_size(&heap);
    EXPECT_LT(free_after, free_before);
    
    /* Allocate again, verify split block is usable */
    ptr2 = heap.ops->malloc(&heap, 5 * 1024);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
}

/* Test block merge */
TEST_F(Heap4_1Test, BlockMerge) {
    void *ptr1, *ptr2;
    size_t free_before, free_after1, free_after2;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    free_before = heap.ops->get_free_size(&heap);
    
    /* Allocate two adjacent blocks */
    ptr1 = heap.ops->malloc(&heap, 1024);
    ptr2 = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    free_after1 = heap.ops->get_free_size(&heap);
    
    /* Free both blocks, should merge */
    heap.ops->free(&heap, ptr1);
    heap.ops->free(&heap, ptr2);
    
    free_after2 = heap.ops->get_free_size(&heap);
    
    /* Verify merge - memory should be restored */
    EXPECT_EQ(free_before, free_after2);
}

/* ============================================================================
 * Memory Alignment Tests
 * ============================================================================ */

/* Test memory alignment */
TEST_F(Heap4_1Test, MemoryAlignment) {
    void *ptr;
    size_t size;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    /* Allocate different sizes, verify alignment */
    for (size = 1; size < 1024; size += 7) {
        ptr = heap.ops->malloc(&heap, size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate size " << size;
        EXPECT_EQ(((size_t)ptr) & 0x7, 0) << "Memory not aligned for size " << size;
        heap.ops->free(&heap, ptr);
    }
}

/* ============================================================================
 * Statistics Tests
 * ============================================================================ */

/* Test get free size */
TEST_F(Heap4_1Test, GetFreeSize) {
    void *ptr;
    size_t free_before, free_after;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    free_before = heap.ops->get_free_size(&heap);
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    
    free_after = heap.ops->get_free_size(&heap);
    EXPECT_LT(free_after, free_before);
}

/* Test get min free size */
TEST_F(Heap4_1Test, GetMinFreeSize) {
    void *ptr;
    size_t min_free_before, min_free_after;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    min_free_before = heap.ops->get_min_free_size(&heap);
    ptr = heap.ops->malloc(&heap, test_heap_size / 2);
    ASSERT_NE(ptr, nullptr);
    
    min_free_after = heap.ops->get_min_free_size(&heap);
    EXPECT_LE(min_free_after, min_free_before);
}

/* Test get stats */
TEST_F(Heap4_1Test, GetStats) {
    bk_heap_stats_t stats;
    void *ptr;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    /* Get initial statistics */
    EXPECT_EQ(heap.ops->get_stats(&heap, &stats), BK_HEAP_STATUS_OK);
    EXPECT_GT(stats.free_bytes, 0);
    EXPECT_EQ(stats.alloc_count, 0);
    EXPECT_EQ(stats.free_count, 0);
    
    /* Allocate and check */
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(heap.ops->get_stats(&heap, &stats), BK_HEAP_STATUS_OK);
    EXPECT_GT(stats.alloc_count, 0);
    
    /* Free and check */
    heap.ops->free(&heap, ptr);
    EXPECT_EQ(heap.ops->get_stats(&heap, &stats), BK_HEAP_STATUS_OK);
    EXPECT_GT(stats.free_count, 0);
}

/* Test get allocated size */
TEST_F(Heap4_1Test, GetAllocatedSize) {
    void *ptr;
    size_t allocated_size;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    
    allocated_size = heap.ops->get_allocated_size(&heap, ptr);
    EXPECT_GE(allocated_size, 1024);
    
    heap.ops->free(&heap, ptr);
    
    /* After free, should return 0 */
    allocated_size = heap.ops->get_allocated_size(&heap, ptr);
    EXPECT_EQ(allocated_size, 0);
}

/* ============================================================================
 * Thread Safety Tests (Mock Verification)
 * ============================================================================ */

/* Test thread safety calls */
TEST_F(Heap4_1Test, ThreadSafetyCalls) {
    void *ptr;
    int suspend_count_before, suspend_count_after;
    int resume_count_before, resume_count_after;
    
    heap.ops->init(&heap, test_heap_addr, test_heap_size);
    
    suspend_count_before = mock_get_enter_critical_count();
    resume_count_before = mock_get_exit_critical_count();
    
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    
    suspend_count_after = mock_get_enter_critical_count();
    resume_count_after = mock_get_exit_critical_count();
    
    /* Verify suspend/resume were called */
    EXPECT_GT(suspend_count_after, suspend_count_before);
    EXPECT_GT(resume_count_after, resume_count_before);
    EXPECT_EQ(suspend_count_after, resume_count_after);
    
    heap.ops->free(&heap, ptr);
    
    /* Verify suspend/resume were called again */
    EXPECT_GT(mock_get_enter_critical_count(), suspend_count_after);
    EXPECT_GT(mock_get_exit_critical_count(), resume_count_after);
}

/* ============================================================================
 * Algorithm Data Management Tests
 * ============================================================================ */

/* Test Heap_4_1_Data_t structure size */
TEST_F(Heap4_1Test, GetAlgorithmDataValid) {
    size_t data_size;
    void *data0, *data1, *data2, *data3;
    
    /* Test sizeof(Heap_4_1_Data_t) - now available in header */
    data_size = sizeof(Heap_4_1_Data_t);
    EXPECT_GT(data_size, 0);
    
    /* Allocate data for multiple regions */
    data0 = malloc(data_size);
    data1 = malloc(data_size);
    data2 = malloc(data_size);
    data3 = malloc(data_size);
    
    EXPECT_NE(data0, nullptr);
    EXPECT_NE(data1, nullptr);
    EXPECT_NE(data2, nullptr);
    EXPECT_NE(data3, nullptr);
    
    /* Verify each region gets a unique data pointer */
    EXPECT_NE(data0, data1);
    EXPECT_NE(data1, data2);
    EXPECT_NE(data2, data3);
    EXPECT_NE(data0, data3);
    
    free(data0);
    free(data1);
    free(data2);
    free(data3);
}

/* Test Heap_4_1_Data_t structure size */
TEST_F(Heap4_1Test, GetAlgorithmDataInvalid) {
    size_t data_size;
    
    /* Test sizeof(Heap_4_1_Data_t) - now available in header */
    data_size = sizeof(Heap_4_1_Data_t);
    EXPECT_GT(data_size, 0);
    EXPECT_LE(data_size, 1024);  /* Reasonable upper bound */
}

/* Test algorithm operations with NULL algorithm_data */
TEST_F(Heap4_1Test, AlgorithmOpsWithNullData) {
    int ret;
    void *ptr;
    
    /* Set algorithm_data to NULL */
    heap.algorithm_data = nullptr;
    
    /* Test init with NULL algorithm_data */
    ret = heap.ops->init(&heap, test_heap_addr, test_heap_size);
    EXPECT_NE(ret, BK_HEAP_STATUS_OK);
    
    /* Initialize properly first */
    /* Use sizeof(Heap_4_1_Data_t) directly since it's now in the header */
    heap.algorithm_data = malloc(sizeof(Heap_4_1_Data_t));
    memset(heap.algorithm_data, 0, sizeof(Heap_4_1_Data_t));
    ret = heap.ops->init(&heap, test_heap_addr, test_heap_size);
    ASSERT_EQ(ret, BK_HEAP_STATUS_OK);
    
    /* Allocate some memory */
    ptr = heap.ops->malloc(&heap, 1024);
    ASSERT_NE(ptr, nullptr);
    
    /* Set algorithm_data to NULL and try operations */
    heap.algorithm_data = nullptr;
    
    /* malloc should fail */
    ptr = heap.ops->malloc(&heap, 1024);
    EXPECT_EQ(ptr, nullptr);
    
    /* Note: get_free_size may still return cached value from heap structure,
     * but malloc should fail when algorithm_data is NULL */
}

