/*
 * test_bk_heap.cpp - Unit tests for multi-region heap management
 *
 * This file contains unit tests for the bk_heap API layer.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "bk_heap.h"
#include "heap_ops.h"
#include "heap_4_1.h"
#include "bk_heap_config.h"
#include "mock_port_heap.h"
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

/* Static memory pool for testing */
static uint8_t test_heap_pool[256 * 1024] __attribute__((aligned(8)));

/* Helper function to allocate algorithm_data */
static void *allocate_algorithm_data(const HeapOps_t *ops) {
    if (ops == NULL) {
        return NULL;
    }
    /* Use sizeof(Heap_4_1_Data_t) directly since it's now in the header */
    size_t size = sizeof(Heap_4_1_Data_t);
    if (size == 0) {
        return NULL;
    }
    void *data = malloc(size);
    if (data != NULL) {
        memset(data, 0, size);
    }
    return data;
}

class BkHeapTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test preparation - use static memory pool
        test_heap_addr = (size_t)test_heap_pool;
        test_heap_size = 64 * 1024;  // 64KB
        
        // Reset mock counters
        mock_reset_counters();
    }

    void TearDown() override {
        // Clean up all regions created during tests
        // This ensures test isolation - each test starts with a clean state
        for (int i = 0; i < BK_HEAP_MAX_REGION; i++) {
            bk_heap_delete_region(i);
        }
    }

    size_t test_heap_addr;
    size_t test_heap_size;
};

/* ============================================================================
 * Region Management Tests
 * ============================================================================ */

/* Test adding a region */
TEST_F(BkHeapTest, AddRegion) {
    bk_heap_region_id_t region_id;
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    EXPECT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    /* Verify region was added */
    size_t free_size = bk_heap_get_free_size(region_id);
    EXPECT_GT(free_size, 0);
    
    free(algorithm_data);
}

/* Test adding multiple regions */
TEST_F(BkHeapTest, AddMultipleRegions) {
    static uint8_t pool1[32 * 1024] __attribute__((aligned(8)));
    static uint8_t pool2[64 * 1024] __attribute__((aligned(8)));
    static uint8_t pool3[128 * 1024] __attribute__((aligned(8)));
    bk_heap_region_id_t region1, region2, region3;
    void *algorithm_data1 = allocate_algorithm_data(&heap_4_1_ops);
    void *algorithm_data2 = allocate_algorithm_data(&heap_4_1_ops);
    void *algorithm_data3 = allocate_algorithm_data(&heap_4_1_ops);
    
    region1 = bk_heap_add_region((size_t)pool1, sizeof(pool1), &heap_4_1_ops, algorithm_data1);
    region2 = bk_heap_add_region((size_t)pool2, sizeof(pool2), &heap_4_1_ops, algorithm_data2);
    region3 = bk_heap_add_region((size_t)pool3, sizeof(pool3), &heap_4_1_ops, algorithm_data3);
    
    EXPECT_NE(region1, BK_HEAP_INVALID_REGION_ID);
    EXPECT_NE(region2, BK_HEAP_INVALID_REGION_ID);
    EXPECT_NE(region3, BK_HEAP_INVALID_REGION_ID);
    
    /* Verify all regions are independent */
    EXPECT_NE(region1, region2);
    EXPECT_NE(region2, region3);
    EXPECT_NE(region1, region3);
}

/* Test adding region with invalid parameters */
TEST_F(BkHeapTest, AddRegionInvalidParams) {
    bk_heap_region_id_t region_id;
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    
    /* Test with zero length */
    region_id = bk_heap_add_region(test_heap_addr, 0, &heap_4_1_ops, algorithm_data);
    EXPECT_EQ(region_id, BK_HEAP_INVALID_REGION_ID);
    
    free(algorithm_data);
}

/* Test deleting a region */
TEST_F(BkHeapTest, DeleteRegion) {
    bk_heap_region_id_t region_id;
    bk_heap_status_t status;
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    status = bk_heap_delete_region(region_id);
    EXPECT_EQ(status, BK_HEAP_STATUS_OK);
    
    /* Verify region is deleted */
    size_t free_size = bk_heap_get_free_size(region_id);
    EXPECT_EQ(free_size, 0);
}

/* Test deleting invalid region */
TEST_F(BkHeapTest, DeleteInvalidRegion) {
    bk_heap_status_t status;
    
    status = bk_heap_delete_region(BK_HEAP_INVALID_REGION_ID);
    EXPECT_EQ(status, BK_HEAP_STATUS_INVALID_REGION);
    
    status = bk_heap_delete_region(999);  /* Invalid ID */
    EXPECT_EQ(status, BK_HEAP_STATUS_INVALID_REGION);
}

/* ============================================================================
 * Memory Allocation Tests
 * ============================================================================ */

/* Test basic malloc */
TEST_F(BkHeapTest, Malloc) {
    bk_heap_region_id_t region_id;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_malloc(region_id, 1024);
    ASSERT_NE(ptr, nullptr);
    
    /* Verify memory is writable */
    memset(ptr, 0xAA, 1024);
    EXPECT_EQ(*((uint8_t*)ptr), 0xAA);
}

/* Test malloc with zero size */
TEST_F(BkHeapTest, MallocZeroSize) {
    bk_heap_region_id_t region_id;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_malloc(region_id, 0);
    EXPECT_EQ(ptr, nullptr);
}

/* Test malloc with invalid region */
TEST_F(BkHeapTest, MallocInvalidRegion) {
    void *ptr;
    
    ptr = bk_heap_malloc(BK_HEAP_INVALID_REGION_ID, 1024);
    EXPECT_EQ(ptr, nullptr);
}

/* Test free */
TEST_F(BkHeapTest, Free) {
    bk_heap_region_id_t region_id;
    void *ptr;
    size_t free_before, free_after_malloc, free_after;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    free_before = bk_heap_get_free_size(region_id);
    ptr = bk_heap_malloc(region_id, 1024);
    ASSERT_NE(ptr, nullptr);
    
    free_after_malloc = bk_heap_get_free_size(region_id);
    /* Verify memory was allocated (free size decreased) */
    EXPECT_LT(free_after_malloc, free_before);
    
    bk_heap_free(region_id, ptr);
    free_after = bk_heap_get_free_size(region_id);
    
    /* Verify memory was freed (free size increased back) */
    EXPECT_GT(free_after, free_after_malloc);
    EXPECT_EQ(free_after, free_before);
}

/* Test free with NULL pointer */
TEST_F(BkHeapTest, FreeNullPointer) {
    bk_heap_region_id_t region_id;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    /* Should not crash */
    bk_heap_free(region_id, nullptr);
}

/* Test calloc */
TEST_F(BkHeapTest, Calloc) {
    bk_heap_region_id_t region_id;
    void *ptr;
    uint8_t *p;
    int i;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_calloc(region_id, 10, sizeof(int));
    ASSERT_NE(ptr, nullptr);
    
    /* Verify memory is zero-initialized */
    p = (uint8_t *)ptr;
    for (i = 0; i < 10 * sizeof(int); i++) {
        EXPECT_EQ(p[i], 0);
    }
}

/* Test calloc with zero parameters */
TEST_F(BkHeapTest, CallocZeroParams) {
    bk_heap_region_id_t region_id;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_calloc(region_id, 0, sizeof(int));
    EXPECT_EQ(ptr, nullptr);
    
    ptr = bk_heap_calloc(region_id, 10, 0);
    EXPECT_EQ(ptr, nullptr);
}

/* Test realloc */
TEST_F(BkHeapTest, Realloc) {
    bk_heap_region_id_t region_id;
    void *ptr1, *ptr2;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr1 = bk_heap_malloc(region_id, 100);
    ASSERT_NE(ptr1, nullptr);
    
    /* Write some data */
    memset(ptr1, 0xAA, 100);
    
    ptr2 = bk_heap_realloc(region_id, ptr1, 200);
    ASSERT_NE(ptr2, nullptr);
    
    /* Verify data was copied */
    EXPECT_EQ(*((uint8_t*)ptr2), 0xAA);
}

/* Test realloc with NULL pointer (should behave like malloc) */
TEST_F(BkHeapTest, ReallocNullPointer) {
    bk_heap_region_id_t region_id;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_realloc(region_id, nullptr, 1024);
    EXPECT_NE(ptr, nullptr);
    
    if (ptr != nullptr) {
        bk_heap_free(region_id, ptr);
    }
}

/* Test realloc with zero size (should behave like free) */
TEST_F(BkHeapTest, ReallocZeroSize) {
    bk_heap_region_id_t region_id;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_malloc(region_id, 1024);
    ASSERT_NE(ptr, nullptr);
    
    ptr = bk_heap_realloc(region_id, ptr, 0);
    EXPECT_EQ(ptr, nullptr);
}

/* ============================================================================
 * Information Query Tests
 * ============================================================================ */

/* Test get free size */
TEST_F(BkHeapTest, GetFreeSize) {
    bk_heap_region_id_t region_id;
    size_t free_size;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    free_size = bk_heap_get_free_size(region_id);
    EXPECT_GT(free_size, 0);
}

/* Test get min free size */
TEST_F(BkHeapTest, GetMinFreeSize) {
    bk_heap_region_id_t region_id;
    void *ptr;
    size_t min_free_before, min_free_after;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    min_free_before = bk_heap_get_min_free_size(region_id);
    ptr = bk_heap_malloc(region_id, test_heap_size / 2);
    ASSERT_NE(ptr, nullptr);
    
    min_free_after = bk_heap_get_min_free_size(region_id);
    EXPECT_LE(min_free_after, min_free_before);
}

/* Test get stats */
TEST_F(BkHeapTest, GetStats) {
    bk_heap_region_id_t region_id;
    bk_heap_stats_t stats;
    void *ptr;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    /* Get initial statistics */
    EXPECT_EQ(bk_heap_get_stats(region_id, &stats), BK_HEAP_STATUS_OK);
    EXPECT_GT(stats.free_bytes, 0);
    EXPECT_EQ(stats.alloc_count, 0);
    EXPECT_EQ(stats.free_count, 0);
    
    /* Allocate and check statistics */
    ptr = bk_heap_malloc(region_id, 1024);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(bk_heap_get_stats(region_id, &stats), BK_HEAP_STATUS_OK);
    EXPECT_EQ(stats.alloc_count, 1);
    
    /* Free and check statistics */
    bk_heap_free(region_id, ptr);
    EXPECT_EQ(bk_heap_get_stats(region_id, &stats), BK_HEAP_STATUS_OK);
    EXPECT_EQ(stats.free_count, 1);
}

/* Test get allocated size */
TEST_F(BkHeapTest, GetAllocatedSize) {
    bk_heap_region_id_t region_id;
    void *ptr;
    size_t allocated_size;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    ptr = bk_heap_malloc(region_id, 1024);
    ASSERT_NE(ptr, nullptr);
    
    allocated_size = bk_heap_get_allocated_size(region_id, ptr);
    EXPECT_GE(allocated_size, 1024);
    
    bk_heap_free(region_id, ptr);
    
    /* After free, should return 0 */
    allocated_size = bk_heap_get_allocated_size(region_id, ptr);
    EXPECT_EQ(allocated_size, 0);
}

/* ============================================================================
 * Multi-Region Tests
 * ============================================================================ */

/* Test multiple regions are independent */
TEST_F(BkHeapTest, MultiRegionIndependent) {
    static uint8_t pool1[32 * 1024] __attribute__((aligned(8)));
    static uint8_t pool2[64 * 1024] __attribute__((aligned(8)));
    bk_heap_region_id_t region1, region2;
    void *ptr1, *ptr2;
    
    void *algorithm_data1 = allocate_algorithm_data(&heap_4_1_ops);
    void *algorithm_data2 = allocate_algorithm_data(&heap_4_1_ops);
    region1 = bk_heap_add_region((size_t)pool1, sizeof(pool1), &heap_4_1_ops, algorithm_data1);
    region2 = bk_heap_add_region((size_t)pool2, sizeof(pool2), &heap_4_1_ops, algorithm_data2);
    
    ASSERT_NE(region1, BK_HEAP_INVALID_REGION_ID);
    ASSERT_NE(region2, BK_HEAP_INVALID_REGION_ID);
    
    ptr1 = bk_heap_malloc(region1, 1024);
    ptr2 = bk_heap_malloc(region2, 2048);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    
    /* Verify different regions are independent */
    EXPECT_NE(ptr1, ptr2);
    
    /* Free from different regions */
    bk_heap_free(region1, ptr1);
    bk_heap_free(region2, ptr2);
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

/* Test error handling with invalid region ID */
TEST_F(BkHeapTest, ErrorHandlingInvalidRegion) {
    void *ptr;
    
    /* Invalid region ID */
    ptr = bk_heap_malloc(BK_HEAP_INVALID_REGION_ID, 1024);
    EXPECT_EQ(ptr, nullptr);
    
    bk_heap_free(BK_HEAP_INVALID_REGION_ID, nullptr);
    
    size_t free_size = bk_heap_get_free_size(BK_HEAP_INVALID_REGION_ID);
    EXPECT_EQ(free_size, 0);
}

/* Test error handling with NULL stats pointer */
TEST_F(BkHeapTest, ErrorHandlingNullStats) {
    bk_heap_region_id_t region_id;
    bk_heap_status_t status;
    
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    ASSERT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    status = bk_heap_get_stats(region_id, nullptr);
    EXPECT_EQ(status, BK_HEAP_STATUS_INVALID_PARAM);
}

/* Test adding region when all slots are full */
TEST_F(BkHeapTest, AddRegionWhenFull) {
    static uint8_t pools[BK_HEAP_MAX_REGION][32 * 1024] __attribute__((aligned(8)));
    bk_heap_region_id_t region_ids[BK_HEAP_MAX_REGION];
    bk_heap_region_id_t extra_region;
    int i;
    
    /* Fill all available regions */
    for (i = 0; i < BK_HEAP_MAX_REGION; i++) {
        void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
        region_ids[i] = bk_heap_add_region((size_t)pools[i], sizeof(pools[i]), &heap_4_1_ops, algorithm_data);
        EXPECT_NE(region_ids[i], BK_HEAP_INVALID_REGION_ID);
    }
    
    /* Try to add one more region - should fail */
    void *extra_algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    extra_region = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, extra_algorithm_data);
    EXPECT_EQ(extra_region, BK_HEAP_INVALID_REGION_ID);
    free(extra_algorithm_data);
    
    /* Delete one region and try again - should succeed */
    bk_heap_delete_region(region_ids[0]);
    extra_algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    extra_region = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, extra_algorithm_data);
    EXPECT_NE(extra_region, BK_HEAP_INVALID_REGION_ID);
    free(extra_algorithm_data);
}

/* Test adding region with custom algorithm operations */
TEST_F(BkHeapTest, AddRegionWithCustomOps) {
    extern const HeapOps_t heap_4_1_ops;
    bk_heap_region_id_t region_id;
    void *ptr;
    
    /* Add region with explicit heap_4_1 algorithm */
    void *algorithm_data = allocate_algorithm_data(&heap_4_1_ops);
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, &heap_4_1_ops, algorithm_data);
    EXPECT_NE(region_id, BK_HEAP_INVALID_REGION_ID);
    
    /* Verify region works correctly */
    ptr = bk_heap_malloc(region_id, 1024);
    EXPECT_NE(ptr, nullptr);
    
    if (ptr != nullptr) {
        bk_heap_free(region_id, ptr);
    }
}

/* Test adding region with NULL ops (should fail) */
TEST_F(BkHeapTest, AddRegionWithNullOps) {
    bk_heap_region_id_t region_id;
    
    /* Add region with NULL ops - should fail */
    region_id = bk_heap_add_region(test_heap_addr, test_heap_size, NULL, NULL);
    EXPECT_EQ(region_id, BK_HEAP_INVALID_REGION_ID);
}

