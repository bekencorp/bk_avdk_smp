/**
 * Szymanski mutual exclusion lock - implementation.
 * Uses GCC atomic builtins (__atomic_load_n / __atomic_store_n), gnu99.
 */

#include "szymanski_lock.h"

#define SZYMANSKI_MAGIC 0x5A4E5A4EU

/**
 * If the platform guarantees atomic access to byte-sized data, the flag type can be changed to uint8_t to save space.
 * The current implementation uses uint32_t for reliable atomic operations. Adjust accordingly for resource-constrained systems.
 */
struct szymanski_lock_t {
    uint32_t magic;
    uint32_t flags[CONFIG_CPU_CNT];
};

static int flag_in_set(uint32_t f, uint32_t a, uint32_t b)
{
    return (f == a) || (f == b);
}

static int flag_in_set3(uint32_t f, uint32_t a, uint32_t b, uint32_t c)
{
    return (f == a) || (f == b) || (f == c);
}

szymanski_lock_t *szymanski_init(void *mem)
{
    if (mem == NULL)
        return NULL;
    szymanski_lock_t *self = (szymanski_lock_t *)mem;

    /* Already initialized (magic head present)? */
    if (__atomic_load_n(&self->magic, __ATOMIC_ACQUIRE) == SZYMANSKI_MAGIC)
        return (szymanski_lock_t *)mem;

    for (uint32_t i = 0; i < CONFIG_CPU_CNT; i++)
        __atomic_store_n(&self->flags[i], 0, __ATOMIC_RELEASE);
    __atomic_store_n(&self->magic, SZYMANSKI_MAGIC, __ATOMIC_RELEASE);
    return (szymanski_lock_t *)mem;
}

void szymanski_teardown(szymanski_lock_t *self)
{
    if (self == NULL)
        return;
    __atomic_store_n(&self->magic, 0, __ATOMIC_RELEASE);
}

int szymanski_lock(szymanski_lock_t *self, uint32_t id)
{
    if (self == NULL || id >= CONFIG_CPU_CNT)
        return -1;

    if (self->magic != SZYMANSKI_MAGIC) {
        szymanski_init(self);
    }

    /* flag[id] = 1 */
    __atomic_store_n(&self->flags[id], 1, __ATOMIC_SEQ_CST);

    /* await(all flag[j] in {0,1,2}) */
    for (;;) {
        int ok = 1;
        for (uint32_t j = 0; j < CONFIG_CPU_CNT; j++) {
            uint32_t f = __atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST);
            if (!flag_in_set3(f, 0, 1, 2)) {
                ok = 0;
                break;
            }
        }
        if (ok)
            break;
    }

    /* flag[id] = 3 */
    __atomic_store_n(&self->flags[id], 3, __ATOMIC_SEQ_CST);

    /* if any (other) flag[j] == 1: flag[id] = 2; await(any flag[j] == 4) */
    {
        int other_has_1 = 0;
        for (uint32_t j = 0; j < CONFIG_CPU_CNT; j++) {
            if (j == id)
                continue;
            if (__atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST) == 1) {
                other_has_1 = 1;
                break;
            }
        }
        if (other_has_1) {
            __atomic_store_n(&self->flags[id], 2, __ATOMIC_SEQ_CST);
            for (;;) {
                int seen_4 = 0;
                for (uint32_t j = 0; j < CONFIG_CPU_CNT; j++) {
                    if (j == id)
                        continue;
                    if (__atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST) == 4) {
                        seen_4 = 1;
                        break;
                    }
                }
                if (seen_4)
                    break;
            }
        }
    }

    /* flag[id] = 4 */
    __atomic_store_n(&self->flags[id], 4, __ATOMIC_SEQ_CST);

    /* await(all flag[0..id-1] in {0,1}) */
    for (;;) {
        int ok = 1;
        for (uint32_t j = 0; j < id; j++) {
            uint32_t f = __atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST);
            if (!flag_in_set(f, 0, 1)) {
                ok = 0;
                break;
            }
        }
        if (ok)
            break;
    }

    /* critical section entered */
    return 0;
}

int szymanski_unlock(szymanski_lock_t *self, uint32_t id)
{
    if (self == NULL || id >= CONFIG_CPU_CNT)
        return -1;
    /* await(all flag[id+1..n-1] in {0,1,4}) */
    for (;;) {
        int ok = 1;
        for (uint32_t j = id + 1; j < CONFIG_CPU_CNT; j++) {
            uint32_t f = __atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST);
            if (!flag_in_set3(f, 0, 1, 4)) {
                ok = 0;
                break;
            }
        }
        if (ok)
            break;
    }

    /* flag[id] = 0 */
    __atomic_store_n(&self->flags[id], 0, __ATOMIC_SEQ_CST);
    return 0;
}

int szymanski_available(szymanski_lock_t *self)
{
    if (self == NULL)
        return 0;
    for (uint32_t j = 0; j < CONFIG_CPU_CNT; j++) {
        if (__atomic_load_n(&self->flags[j], __ATOMIC_SEQ_CST) != 0)
            return 0;
    }
    return 1;
}
