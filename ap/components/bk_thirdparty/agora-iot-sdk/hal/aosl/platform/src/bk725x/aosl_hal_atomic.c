#include <stdbool.h>
#include <stdint.h>

#include <hal/aosl_hal_atomic.h>
#include <os/os.h>
#include "spinlock.h"
/*
 * BK7259 (and similar) PSRAM is not on the local bus that supports ARMv8-M
 * LDREX/STREX exclusive monitors reliably. GCC __atomic_* on intptr_t may
 * lower to exclusive primitives for SRAM/TCM but must not be used for objects
 * in PSRAM.
 *
 * Strategy:
 *   - Addresses inside [CONFIG_PSRAM_BASE, CONFIG_PSRAM_BASE+CAPACITY): critical
 *     section + plain volatile load/store + __sync_synchronize() for seq_cst.
 *   - Otherwise: __atomic_* (LDREX/STREX on internal SRAM / HSRAM as usual).
 *
 * ram_regions.h is typically force-included by the AP build; keep a fallback
 * so standalone AOSL builds still compile.
 */
// #if !defined(CONFIG_PSRAM_BASE) || !defined(CONFIG_PSRAM_CAPACITY)
// #ifndef CONFIG_PSRAM_BASE
// #define CONFIG_PSRAM_BASE 0x60000000u
// #endif
// #ifndef CONFIG_PSRAM_CAPACITY
// #define CONFIG_PSRAM_CAPACITY 0x00800000u
// #endif
// #endif

// static inline bool aosl_atomic_ptr_in_psram(const void *p)
// {
// 	uintptr_t a = (uintptr_t)p;
// 	uintptr_t base = (uintptr_t)CONFIG_PSRAM_BASE;
// 	uintptr_t end = base + (uintptr_t)CONFIG_PSRAM_CAPACITY;

// 	return (a >= base) && (a < end);
// }

static SPINLOCK_SECTION volatile spinlock_t aosl_atomic_spin_lock = SPIN_LOCK_INIT;
uint32_t aosl_atomic_seq_lock( void )
{
	uint32_t flags = rtos_disable_int();
	spin_lock(&aosl_atomic_spin_lock);
	return flags;
}

void aosl_atomic_seq_unlock( uint32_t state )
{
	spin_unlock(&aosl_atomic_spin_lock);
	rtos_enable_int(state);
}


intptr_t aosl_hal_atomic_read(const intptr_t *v)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t x = *(const volatile intptr_t *)v;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return x;
	// }
	// return __atomic_load_n((volatile intptr_t *)v, __ATOMIC_SEQ_CST);
}

void aosl_hal_atomic_set(intptr_t *v, intptr_t i)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		*(volatile intptr_t *)v = i;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return;
	// }
	// __atomic_store_n((volatile intptr_t *)v, i, __ATOMIC_SEQ_CST);
}

intptr_t aosl_hal_atomic_inc(intptr_t *v)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t prev = *(volatile intptr_t *)v;
		*(volatile intptr_t *)v = prev + 1;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return prev;
	// }
	// return __atomic_fetch_add((volatile intptr_t *)v, 1, __ATOMIC_SEQ_CST);
}

intptr_t aosl_hal_atomic_dec(intptr_t *v)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t prev = *(volatile intptr_t *)v;
		*(volatile intptr_t *)v = prev - 1;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return prev;
	// }
	// return __atomic_fetch_sub((volatile intptr_t *)v, 1, __ATOMIC_SEQ_CST);
}

intptr_t aosl_hal_atomic_add(intptr_t i, intptr_t *v)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t prev = *(volatile intptr_t *)v;
		intptr_t next = prev + i;
		*(volatile intptr_t *)v = next;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return next;
	// }
	// return __atomic_add_fetch((volatile intptr_t *)v, i, __ATOMIC_SEQ_CST);
}

intptr_t aosl_hal_atomic_sub(intptr_t i, intptr_t *v)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t prev = *(volatile intptr_t *)v;
		intptr_t next = prev - i;
		*(volatile intptr_t *)v = next;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return next;
	// }
	// return __atomic_sub_fetch((volatile intptr_t *)v, i, __ATOMIC_SEQ_CST);
}

intptr_t aosl_hal_atomic_cmpxchg(intptr_t *v, intptr_t old, intptr_t new)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t cur = *(volatile intptr_t *)v;
		if (cur == old) {
			*(volatile intptr_t *)v = new;
		}
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return cur;
	//  }
	// if (__atomic_compare_exchange_n((volatile intptr_t *)v, &old, new, false,
	// 			      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
	// 	return old;
	// } else {
	// 	return old;
	// }
}

intptr_t aosl_hal_atomic_xchg(intptr_t *v, intptr_t new)
{
	// if (aosl_atomic_ptr_in_psram(v)) {
		uint32_t f = aosl_atomic_seq_lock();
		intptr_t prev = *(volatile intptr_t *)v;
		*(volatile intptr_t *)v = new;
		__sync_synchronize();
		aosl_atomic_seq_unlock(f);
		return prev;
	// }
	// return __atomic_exchange_n((volatile intptr_t *)v, new, __ATOMIC_SEQ_CST);
}

void aosl_hal_mb(void)
{
	__sync_synchronize();
}

void aosl_hal_rmb(void)
{
	__sync_synchronize();
}

void aosl_hal_wmb(void)
{
	__sync_synchronize();
}
