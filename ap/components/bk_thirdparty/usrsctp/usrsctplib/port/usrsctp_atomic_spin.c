/*
 * Match kvs_aws pic AtomicsGnu.h defaultAtomic*: disable interrupts + spinlock,
 * then RMW with __sync_synchronize() for ordering (same pattern as that file’s #else branch).
 */
#if defined(USRSCTP_ATOMIC_SPINLOCK_COMPAT)

#include <os/os.h>
#include "spinlock.h"
#include "usrsctp_atomic_spin.h"

static volatile spinlock_t s_usrsctp_atomic_lock = SPIN_LOCK_INIT;

static uint32_t usrsctp_atomic_enter(void)
{
	uint32_t flags = rtos_disable_int();

	spin_lock(&s_usrsctp_atomic_lock);
	return flags;
}

static void usrsctp_atomic_leave(uint32_t flags)
{
	spin_unlock(&s_usrsctp_atomic_lock);
	rtos_enable_int(flags);
}

int usrsctp_atomic_fetch_add_int(volatile int *p, int v)
{
	uint32_t f = usrsctp_atomic_enter();
	int old = *p;

	*p = old + v;
	__sync_synchronize();
	usrsctp_atomic_leave(f);
	return old;
}

void usrsctp_atomic_add_int(volatile int *p, int v)
{
	(void)usrsctp_atomic_fetch_add_int(p, v);
}

void usrsctp_atomic_subtract_int(volatile int *p, int v)
{
	(void)usrsctp_atomic_fetch_add_int(p, -v);
}

int usrsctp_atomic_cmpset_int(volatile int *dst, int exp, int src)
{
	uint32_t f = usrsctp_atomic_enter();
	int x = *dst;

	if (x == exp) {
		*dst = src;
		__sync_synchronize();
		usrsctp_atomic_leave(f);
		return 1;
	}
	usrsctp_atomic_leave(f);
	return 0;
}

#endif /* USRSCTP_ATOMIC_SPINLOCK_COMPAT */
