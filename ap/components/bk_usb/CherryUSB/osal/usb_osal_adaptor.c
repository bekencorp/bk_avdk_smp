#include <os/os.h>
#include <os/mem.h>
#include <errno.h> /* -ETIMEDOUT / -EINVAL return codes (POSIX errno) */
#include "usb_osal.h"

typedef struct {
	beken_timer_t timer;
	struct usb_osal_timer *owner;
} bk_cherryusb_timer_t;

static void bk_cherryusb_timer_handler(void *arg)
{
	struct usb_osal_timer *timer = (struct usb_osal_timer *)arg;

	if (timer && timer->handler) {
		timer->handler(timer->argument);
	}
}

usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size, uint32_t prio,
					 usb_thread_entry_t entry, void *args)
{
	beken_thread_t thread = NULL;

	if (rtos_create_thread(&thread, prio, name, (beken_thread_function_t)entry, stack_size, args) != BK_OK) {
		return NULL;
	}

	return (usb_osal_thread_t)thread;
}

void usb_osal_thread_delete(usb_osal_thread_t thread)
{
	beken_thread_t beken_thread = (beken_thread_t)thread;

	if (thread == NULL) {
		(void)rtos_delete_thread(NULL);
		return;
	}

	(void)rtos_delete_thread(&beken_thread);
}

void usb_osal_thread_schedule_other(void)
{
	rtos_delay_milliseconds(1);
}

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count)
{
	beken_semaphore_t sem = NULL;
	uint32_t max_count = initial_count ? initial_count : 1;

	if (rtos_init_semaphore(&sem, max_count) != BK_OK) {
		return NULL;
	}

	while (initial_count--) {
		(void)rtos_set_semaphore(&sem);
	}

	return (usb_osal_sem_t)sem;
}

usb_osal_sem_t usb_osal_sem_create_counting(uint32_t max_count)
{
	beken_semaphore_t sem = NULL;

	if (rtos_init_semaphore(&sem, max_count ? max_count : 1) != BK_OK) {
		return NULL;
	}

	return (usb_osal_sem_t)sem;
}

void usb_osal_sem_delete(usb_osal_sem_t sem)
{
	beken_semaphore_t beken_sem = (beken_semaphore_t)sem;

	if (beken_sem) {
		rtos_deinit_semaphore(&beken_sem);
	}
}

int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout)
{
	uint32_t wait_ms = (timeout == USB_OSAL_WAITING_FOREVER) ? BEKEN_WAIT_FOREVER : timeout;
	beken_semaphore_t beken_sem = (beken_semaphore_t)sem;

	return (rtos_get_semaphore(&beken_sem, wait_ms) == BK_OK) ? 0 : -ETIMEDOUT;
}

int usb_osal_sem_give(usb_osal_sem_t sem)
{
	beken_semaphore_t beken_sem = (beken_semaphore_t)sem;

	return (rtos_set_semaphore(&beken_sem) == BK_OK) ? 0 : -EINVAL;
}

void usb_osal_sem_reset(usb_osal_sem_t sem)
{
	(void)sem;
}

usb_osal_mutex_t usb_osal_mutex_create(void)
{
	beken_mutex_t mutex = NULL;

	if (rtos_init_mutex(&mutex) != BK_OK) {
		return NULL;
	}

	return (usb_osal_mutex_t)mutex;
}

void usb_osal_mutex_delete(usb_osal_mutex_t mutex)
{
	beken_mutex_t beken_mutex = (beken_mutex_t)mutex;

	if (beken_mutex) {
		rtos_deinit_mutex(&beken_mutex);
	}
}

int usb_osal_mutex_take(usb_osal_mutex_t mutex)
{
	beken_mutex_t beken_mutex = (beken_mutex_t)mutex;

	return (rtos_lock_mutex(&beken_mutex) == BK_OK) ? 0 : -EINVAL;
}

int usb_osal_mutex_give(usb_osal_mutex_t mutex)
{
	beken_mutex_t beken_mutex = (beken_mutex_t)mutex;

	return (rtos_unlock_mutex(&beken_mutex) == BK_OK) ? 0 : -EINVAL;
}

usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs)
{
	beken_queue_t queue = NULL;

	if (rtos_init_queue(&queue, "usb_mq", sizeof(uintptr_t), max_msgs) != BK_OK) {
		return NULL;
	}

	return (usb_osal_mq_t)queue;
}

void usb_osal_mq_delete(usb_osal_mq_t mq)
{
	beken_queue_t queue = (beken_queue_t)mq;

	if (queue) {
		rtos_deinit_queue(&queue);
	}
}

int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr)
{
	beken_queue_t queue = (beken_queue_t)mq;

	return (rtos_push_to_queue(&queue, &addr, BEKEN_NO_WAIT) == BK_OK) ? 0 : -ETIMEDOUT;
}

int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout)
{
	uint32_t wait_ms = (timeout == USB_OSAL_WAITING_FOREVER) ? BEKEN_WAIT_FOREVER : timeout;
	beken_queue_t queue = (beken_queue_t)mq;

	return (rtos_pop_from_queue(&queue, addr, wait_ms) == BK_OK) ? 0 : -ETIMEDOUT;
}

struct usb_osal_timer *usb_osal_timer_create(const char *name, uint32_t timeout_ms,
					     usb_timer_handler_t handler, void *argument, bool is_period)
{
	struct usb_osal_timer *timer;
	bk_cherryusb_timer_t *bk_timer;

	(void)name;
	(void)is_period;

	timer = (struct usb_osal_timer *)os_malloc(sizeof(struct usb_osal_timer));
	if (!timer) {
		return NULL;
	}

	bk_timer = (bk_cherryusb_timer_t *)os_malloc(sizeof(bk_cherryusb_timer_t));
	if (!bk_timer) {
		os_free(timer);
		return NULL;
	}

	os_memset(timer, 0, sizeof(*timer));
	os_memset(bk_timer, 0, sizeof(*bk_timer));
	timer->handler = handler;
	timer->argument = argument;
	timer->is_period = is_period;
	timer->timeout_ms = timeout_ms;
	timer->timer = bk_timer;
	bk_timer->owner = timer;

	if (rtos_init_timer(&bk_timer->timer, timeout_ms, bk_cherryusb_timer_handler, timer) != BK_OK) {
		os_free(bk_timer);
		os_free(timer);
		return NULL;
	}

	return timer;
}

void usb_osal_timer_delete(struct usb_osal_timer *timer)
{
	bk_cherryusb_timer_t *bk_timer;

	if (!timer) {
		return;
	}

	bk_timer = (bk_cherryusb_timer_t *)timer->timer;
	if (bk_timer) {
		rtos_stop_timer(&bk_timer->timer);
		rtos_deinit_timer(&bk_timer->timer);
		os_free(bk_timer);
	}
	os_free(timer);
}

void usb_osal_timer_start(struct usb_osal_timer *timer)
{
	bk_cherryusb_timer_t *bk_timer;

	if (!timer || !timer->timer) {
		return;
	}

	bk_timer = (bk_cherryusb_timer_t *)timer->timer;
	(void)rtos_start_timer(&bk_timer->timer);
}

void usb_osal_timer_stop(struct usb_osal_timer *timer)
{
	bk_cherryusb_timer_t *bk_timer;

	if (!timer || !timer->timer) {
		return;
	}

	bk_timer = (bk_cherryusb_timer_t *)timer->timer;
	(void)rtos_stop_timer(&bk_timer->timer);
}

size_t usb_osal_enter_critical_section(void)
{
	return 0;
}

void usb_osal_leave_critical_section(size_t flag)
{
	(void)flag;
}

void usb_osal_msleep(uint32_t delay)
{
	rtos_delay_milliseconds(delay);
}

void *usb_osal_malloc(size_t size)
{
	return os_malloc(size);
}

void usb_osal_free(void *ptr)
{
	os_free(ptr);
}
