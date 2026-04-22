/**
 * pthread_rwlock compatibility for platforms without pthread_rwlock (e.g. m3 bk_rtos).
 * Provides POSIX reader-writer lock using pthread_mutex_t + pthread_cond_t.
 * Same API as ESP-IDF components/pthread/pthread_rwlock.c.
 */
#ifndef PTHREAD_RWLOCK_COMPAT_H
#define PTHREAD_RWLOCK_COMPAT_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	pthread_mutex_t  mutex;
	pthread_cond_t   cond;
	int              active_readers;
	int              active_writers;
	int              waiting_writers;
} usrsctp_rwlock_impl_t;

/* Expose as opaque pointer type so usrsctp can use pthread_rwlock_t as usual */
typedef usrsctp_rwlock_impl_t *pthread_rwlock_t;

typedef struct { int __dummy; } pthread_rwlockattr_t;

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

#ifdef __cplusplus
}
#endif

#endif /* PTHREAD_RWLOCK_COMPAT_H */
