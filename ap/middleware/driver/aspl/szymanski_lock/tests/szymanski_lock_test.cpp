/**
 * Tests for Szymanski lock (init on user buffer, lock/unlock).
 */
#include "szymanski_lock.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace {

/** Lock buffer size: magic (uint32_t) + CONFIG_CPU_CNT flags (each uint8_t).
 *  Oversized intentionally so the buffer is always sufficient. */
static const size_t SZYMANSKI_BUF_SIZE =
    sizeof(uint32_t) * (1 + CONFIG_CPU_CNT);

TEST(SzymanskiLock, InitNullMem) {
    EXPECT_EQ(szymanski_init(nullptr), nullptr);
}


TEST(SzymanskiLock, SingleThreadLockUnlock) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(szymanski_lock(lock, 0), 0);
        EXPECT_EQ(szymanski_unlock(lock, 0), 0);
    }
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, MagicHeadIdempotentInit) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock1 = szymanski_init(buf);
    ASSERT_NE(lock1, nullptr);
    szymanski_lock_t * lock2 = szymanski_init(buf);
    EXPECT_EQ(lock1, lock2);
    szymanski_teardown(lock1);
}

TEST(SzymanskiLock, MultiThreadMutualExclusion) {
    const uint32_t N = 4;
    const int iterations = 5000;
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);

    unsigned long counter = 0;
    std::vector<std::thread> threads;
    for (uint32_t id = 0; id < N; id++) {
        threads.emplace_back([lock, id, iterations, &counter]() {
            for (int i = 0; i < iterations; i++) {
                ASSERT_EQ(szymanski_lock(lock, id), 0);
                counter++;
                ASSERT_EQ(szymanski_unlock(lock, id), 0);
            }
        });
    }
    for (auto &t : threads)
        t.join();

    EXPECT_EQ(counter, static_cast<unsigned long>(N) * iterations);
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, AvailableWhenFree_ReturnsOne) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    EXPECT_EQ(szymanski_available(lock), 1);
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, AvailableWhenHeld_ReturnsZero) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    EXPECT_EQ(szymanski_lock(lock, 0), 0);
    EXPECT_EQ(szymanski_available(lock), 0);
    EXPECT_EQ(szymanski_unlock(lock, 0), 0);
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, AvailableAfterUnlock_ReturnsOne) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    EXPECT_EQ(szymanski_lock(lock, 0), 0);
    EXPECT_EQ(szymanski_unlock(lock, 0), 0);
    EXPECT_EQ(szymanski_available(lock), 1);
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, LockInvalidId_ReturnsZero) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    EXPECT_NE(szymanski_lock(lock, CONFIG_CPU_CNT), 0);
    EXPECT_NE(szymanski_lock(nullptr, 0), 0);
    szymanski_teardown(lock);
}

TEST(SzymanskiLock, UnlockInvalidId_ReturnsZero) {
    unsigned char buf[SZYMANSKI_BUF_SIZE] = {0};
    szymanski_lock_t * lock = szymanski_init(buf);
    ASSERT_NE(lock, nullptr);
    EXPECT_NE(szymanski_unlock(lock, CONFIG_CPU_CNT), 0);
    EXPECT_NE(szymanski_unlock(nullptr, 0), 0);
    szymanski_teardown(lock);
}

} // namespace
