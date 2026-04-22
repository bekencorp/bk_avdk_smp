/**
 * pthread_rwlock implementation for platforms without it (m3 bk_rtos).
 * Uses pthread_mutex_t + pthread_cond_t; logic mirrors ESP-IDF pthread_rwlock.c.
 */
#include "port/pthread_rwlock_compat.h"
#include <os/mem.h>
#include <stdlib.h>
#include <errno.h>

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	usrsctp_rwlock_impl_t *impl;

	(void)attr;
	if (!rwlock)
		return EINVAL;

	impl = (usrsctp_rwlock_impl_t *)os_malloc(sizeof(usrsctp_rwlock_impl_t));
	if (!impl)
		return ENOMEM;

	if (pthread_mutex_init(&impl->mutex, NULL) != 0) {
		os_free(impl);
		return ENOMEM;
	}
	if (pthread_cond_init(&impl->cond, NULL) != 0) {
		pthread_mutex_destroy(&impl->mutex);
		os_free(impl);
		return ENOMEM;
	}

	impl->active_readers = 0;
	impl->active_writers = 0;
	impl->waiting_writers = 0;
	*rwlock = impl;
	return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;

	if (!rwlock || !*rwlock)
		return EINVAL;

	impl = *rwlock;
	pthread_cond_destroy(&impl->cond);
	pthread_mutex_destroy(&impl->mutex);
	os_free(impl);
	*rwlock = NULL;
	return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;

	if (!rwlock || !*rwlock)
		return EINVAL;
	impl = *rwlock;

	if (pthread_mutex_lock(&impl->mutex) != 0)
		return EINVAL;

	while (impl->active_writers > 0)
		pthread_cond_wait(&impl->cond, &impl->mutex);
	impl->active_readers++;

	pthread_mutex_unlock(&impl->mutex);
	return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;
	int ret = 0;

	if (!rwlock || !*rwlock)
		return EINVAL;
	impl = *rwlock;

	if (pthread_mutex_lock(&impl->mutex) != 0)
		return EINVAL;

	if (impl->active_writers > 0)
		ret = EBUSY;
	else
		impl->active_readers++;

	pthread_mutex_unlock(&impl->mutex);
	return ret;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;

	if (!rwlock || !*rwlock)
		return EINVAL;
	impl = *rwlock;

	if (pthread_mutex_lock(&impl->mutex) != 0)
		return EINVAL;

	impl->waiting_writers++;
	while (impl->active_readers > 0 || impl->active_writers > 0)
		pthread_cond_wait(&impl->cond, &impl->mutex);
	impl->waiting_writers--;
	impl->active_writers++;

	pthread_mutex_unlock(&impl->mutex);
	return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;
	int ret = 0;

	if (!rwlock || !*rwlock)
		return EINVAL;
	impl = *rwlock;

	if (pthread_mutex_lock(&impl->mutex) != 0)
		return EINVAL;

	if (impl->active_readers > 0 || impl->active_writers > 0 || impl->waiting_writers > 0)
		ret = EBUSY;
	else
		impl->active_writers++;

	pthread_mutex_unlock(&impl->mutex);
	return ret;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	usrsctp_rwlock_impl_t *impl;

	if (!rwlock || !*rwlock)
		return EINVAL;
	impl = *rwlock;

	if (pthread_mutex_lock(&impl->mutex) != 0)
		return EINVAL;

	if (impl->active_readers > 0) {
		impl->active_readers--;
		if (impl->active_readers == 0)
			pthread_cond_broadcast(&impl->cond);
	} else {
		impl->active_writers = 0;
		pthread_cond_broadcast(&impl->cond);
	}

	pthread_mutex_unlock(&impl->mutex);
	return 0;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
	(void)attr;
	return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
	(void)attr;
	return 0;
}
