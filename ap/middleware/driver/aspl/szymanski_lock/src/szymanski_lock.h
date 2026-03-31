/**
 * Szymanski mutual exclusion lock for multi-core MCU.
 * Shared memory only; no allocation (init/teardown on user-provided buffer).
 */

#ifndef SZYMANSKI_LOCK_H
#define SZYMANSKI_LOCK_H

#include <stdint.h>
#include <stddef.h>

/** Number of CPUs/threads (override before including if needed). */
#ifndef CONFIG_CPU_CNT
#define CONFIG_CPU_CNT 4
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct szymanski_lock_t;
typedef struct szymanski_lock_t szymanski_lock_t;

/** Initialize lock in user-provided shared memory. If magic already set, returns handle as-is. */
szymanski_lock_t *szymanski_init(void *mem);

/** Clear state and magic; memory is not freed (user-managed). */
void szymanski_teardown(szymanski_lock_t *self);

/** Acquire lock; id is this core's runtime id in [0, CONFIG_CPU_CNT). Returns 0 on success, non-zero on failure (null self or invalid id). */
int szymanski_lock(szymanski_lock_t *self, uint32_t id);

/** Release lock. Returns 0 on success, non-zero on failure (null self or invalid id). */
int szymanski_unlock(szymanski_lock_t *self, uint32_t id);

/** Returns 1 if lock is available (all flags 0), 0 if any flag is non-zero. Does not acquire. */
int szymanski_available(szymanski_lock_t *self);

#ifdef __cplusplus
}
#endif

#endif /* SZYMANSKI_LOCK_H */
