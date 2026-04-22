#ifndef __UTILS_ATOMICS_GNU__
#define __UTILS_ATOMICS_GNU__

#ifdef __cplusplus
extern "C" {
#endif

#include "Include_i.h"
#if 1
#include <stdint.h>
#include <os/os.h>
#include "spinlock.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc11-extensions"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if 1
static SPINLOCK_SECTION volatile spinlock_t kvs_aws_atomic_spin_lock = SPIN_LOCK_INIT;
uint32_t kvs_aws_atomic_seq_lock( void )
{
    uint32_t flags = rtos_disable_int();
    spin_lock(&kvs_aws_atomic_spin_lock);
    return flags;
}

void kvs_aws_atomic_seq_unlock( uint32_t state )
{
    spin_unlock(&kvs_aws_atomic_spin_lock);
    rtos_enable_int(state);
}
#endif

static inline SIZE_T defaultAtomicLoad(volatile SIZE_T* pAtomic)
{
#if 0
    return __atomic_load_n(pAtomic, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(const volatile SIZE_T *) pAtomic;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline VOID defaultAtomicStore(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    __atomic_store_n(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    *(volatile SIZE_T *) pAtomic = var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
#endif
}

static inline SIZE_T defaultAtomicExchange(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_exchange_n(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline BOOL defaultAtomicCompareExchange(volatile SIZE_T* pAtomic, SIZE_T* pExpected, SIZE_T desired)
{
#if 0
    return __atomic_compare_exchange_n(pAtomic, pExpected, desired, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    if (x == *pExpected) {
        *(volatile SIZE_T *) pAtomic = desired;
        __sync_synchronize();
        kvs_aws_atomic_seq_unlock(f);
        return TRUE;
    }
    *pExpected = x;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return FALSE;
#endif
}

static inline SIZE_T defaultAtomicIncrement(volatile SIZE_T* pAtomic)
{
#if 0
    return __atomic_fetch_add(pAtomic, 1, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x + 1;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicDecrement(volatile SIZE_T* pAtomic)
{
#if 0
    return __atomic_fetch_sub(pAtomic, 1, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x - 1;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicAdd(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_fetch_add(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x + var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicSubtract(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_fetch_sub(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x - var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicAnd(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_fetch_and(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x & var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicOr(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_fetch_or(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x | var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

static inline SIZE_T defaultAtomicXor(volatile SIZE_T* pAtomic, SIZE_T var)
{
#if 0
    return __atomic_fetch_xor(pAtomic, var, __ATOMIC_SEQ_CST);
#else
    uint32_t f = kvs_aws_atomic_seq_lock();
    SIZE_T x = *(volatile SIZE_T *) pAtomic;
    *(volatile SIZE_T *) pAtomic = x ^ var;
    __sync_synchronize();
    kvs_aws_atomic_seq_unlock(f);
    return x;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* __UTILS_ATOMICS_GNU__ */
