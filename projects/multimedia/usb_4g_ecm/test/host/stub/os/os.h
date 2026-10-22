/* Host-side stubs: just enough of armino's os layer to run modem_at.c's parser
 * in a single thread. The semaphore take is where the simulated module speaks. */
#ifndef _STUB_OS_H_
#define _STUB_OS_H_

#ifdef MODEM_AT_THREADED_TEST
#include "threaded_os.h"
#else

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef int bk_err_t;
#define BK_OK   0
#define BK_FAIL (-1)

#define BEKEN_NO_WAIT 0

typedef void *beken_mutex_t;
typedef void *beken_semaphore_t;

/* Mutex: no contention in a single-threaded harness, but the -5 path needs
 * lock_mutex_timeout to actually fail when already held, so track depth. */
extern int stub_mutex_held;
extern int stub_sem_count;

static inline bk_err_t rtos_init_mutex(beken_mutex_t *m) {
    static int held[4]; static unsigned next;
    *m = &held[next++]; return BK_OK;
}
static inline bk_err_t rtos_deinit_mutex(beken_mutex_t *m) { *m = NULL; return BK_OK; }
static inline bk_err_t rtos_unlock_mutex(beken_mutex_t *m) { *(int *)*m = 0; return BK_OK; }
static inline bk_err_t rtos_lock_mutex_timeout(beken_mutex_t *m, uint32_t to) {
    (void)to;
    if (*(int *)*m) return BK_FAIL;
    *(int *)*m = 1; return BK_OK;
}
static inline bk_err_t rtos_lock_mutex(beken_mutex_t *m) {
    int ret = rtos_lock_mutex_timeout(m, 0);
    assert(ret == BK_OK); return ret;
}

static inline bk_err_t rtos_init_semaphore(beken_semaphore_t *s, int max) { (void)max; *s = (void *)1; stub_sem_count = 0; return BK_OK; }
static inline bk_err_t rtos_deinit_semaphore(beken_semaphore_t *s) { *s = NULL; return BK_OK; }
static inline bk_err_t rtos_set_semaphore(beken_semaphore_t *s) { (void)s; stub_sem_count++; return BK_OK; }

/* Defined by the test: feeds the scripted module output, then reports whether a
 * result code landed. This is what makes the harness single-threaded. */
bk_err_t rtos_get_semaphore(beken_semaphore_t *s, uint32_t timeout_ms);

#endif /* MODEM_AT_THREADED_TEST */
#endif
