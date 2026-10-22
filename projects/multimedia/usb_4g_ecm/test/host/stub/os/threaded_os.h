/* Real host synchronization for the AT concurrency regression. */
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

typedef int bk_err_t;
#define BK_OK 0
#define BK_FAIL (-1)
#define BEKEN_NO_WAIT 0

typedef pthread_mutex_t *beken_mutex_t;
typedef struct { pthread_mutex_t lock; pthread_cond_t cv; unsigned count; } *beken_semaphore_t;
static inline int rtos_init_mutex(beken_mutex_t *m) {
    *m = malloc(sizeof(**m));
    return *m && !pthread_mutex_init(*m, NULL) ? BK_OK : BK_FAIL;
}
static inline int rtos_deinit_mutex(beken_mutex_t *m) {
    pthread_mutex_destroy(*m); free(*m); *m = NULL; return BK_OK;
}
static inline int rtos_lock_mutex(beken_mutex_t *m) { return pthread_mutex_lock(*m); }
static inline int rtos_unlock_mutex(beken_mutex_t *m) { return pthread_mutex_unlock(*m); }
static inline int rtos_lock_mutex_timeout(beken_mutex_t *m, uint32_t ms) {
    return ms ? pthread_mutex_lock(*m) : pthread_mutex_trylock(*m);
}
static inline int rtos_init_semaphore(beken_semaphore_t *s, int max) {
    (void)max; *s = calloc(1, sizeof(**s));
    if (!*s) return BK_FAIL;
    pthread_mutex_init(&(*s)->lock, NULL); pthread_cond_init(&(*s)->cv, NULL); return BK_OK;
}
static inline int rtos_set_semaphore(beken_semaphore_t *s) {
    pthread_mutex_lock(&(*s)->lock); (*s)->count = 1;
    pthread_cond_signal(&(*s)->cv); pthread_mutex_unlock(&(*s)->lock); return BK_OK;
}
static inline int rtos_get_semaphore(beken_semaphore_t *s, uint32_t ms) {
    struct timespec end; int err = 0;
    clock_gettime(CLOCK_REALTIME, &end);
    end.tv_sec += ms / 1000; end.tv_nsec += (long)(ms % 1000) * 1000000;
    if (end.tv_nsec >= 1000000000) { end.tv_sec++; end.tv_nsec -= 1000000000; }
    pthread_mutex_lock(&(*s)->lock);
    while (!(*s)->count && !err && ms) err = pthread_cond_timedwait(&(*s)->cv, &(*s)->lock, &end);
    int ret = (*s)->count ? BK_OK : BK_FAIL;
    if ((*s)->count) (*s)->count--;
    pthread_mutex_unlock(&(*s)->lock); return ret;
}
static inline int rtos_deinit_semaphore(beken_semaphore_t *s) {
    pthread_cond_destroy(&(*s)->cv); pthread_mutex_destroy(&(*s)->lock);
    free(*s); *s = NULL; return BK_OK;
}
