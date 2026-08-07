#include "os_wrapper/mutex.h"
#include <os/mem.h>
#include <os/os.h>

void *os_wrapper_mutex_create(void)
{
	beken_mutex_t *mutex = os_malloc(sizeof(beken_mutex_t));

	if (mutex == NULL) {
		return NULL;
	}

	if (rtos_init_mutex(mutex) != BK_OK) {
		os_free(mutex);
		return NULL;
	}

	return mutex;
}

uint32_t os_wrapper_mutex_acquire(void *handle, uint32_t timeout)
{
	beken_mutex_t *mutex = handle;

	if (mutex == NULL) {
		return OS_WRAPPER_ERROR;
	}

	if (timeout != OS_WRAPPER_WAIT_FOREVER) {
		return OS_WRAPPER_ERROR;
	}

	if (rtos_lock_mutex(mutex) != BK_OK) {
		return OS_WRAPPER_ERROR;
	}

	return OS_WRAPPER_SUCCESS;
}

uint32_t os_wrapper_mutex_release(void *handle)
{
	beken_mutex_t *mutex = handle;

	if (mutex == NULL) {
		return OS_WRAPPER_ERROR;
	}

	if (rtos_unlock_mutex(mutex) != BK_OK) {
		return OS_WRAPPER_ERROR;
	}

	return OS_WRAPPER_SUCCESS;
}

uint32_t os_wrapper_mutex_delete(void *handle)
{
	beken_mutex_t *mutex = handle;

	if (mutex == NULL) {
		return OS_WRAPPER_ERROR;
	}

	if (rtos_deinit_mutex(mutex) != BK_OK) {
		return OS_WRAPPER_ERROR;
	}

	os_free(mutex);
	return OS_WRAPPER_SUCCESS;
}
