// Copyright 2020-2022 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _spinlock_h_
#define _spinlock_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_typedef.h>
typedef struct {
#if CONFIG_SPINLOCK_DEBUG
    uint32_t taskTCBPointer;
#endif
	uint32_t  owner; 
	uint32_t  count;	
    uint32_t  core_id;
} spinlock_t;

#define SPINLOCK_WAIT_FOREVER  (-1)
#define SPINLOCK_NO_WAIT        0

#define SPIN_LOCK_FREE      (0xF2EEF2EE)
#define SPINLOCK_CORE_ID_UNINITILIZE (0xF2EE)
#if CONFIG_SPINLOCK_DEBUG
// #define SPIN_LOCK_INIT   {.task_number=0xbeef, .task_affinity=0xad, .task_priority=0xde, .owner = SPIN_LOCK_FREE, .count = 0}
// #define SPIN_LOCK_ACQUIRE_INIT   {.task_number=0xbeef, .task_affinity=0xad, .task_priority=0xde, .owner = 0, .count = 0,.core_id=SPINLOCK_CORE_ID_UNINITILIZE}
#define SPIN_LOCK_INIT   {.taskTCBPointer = 0xdeadbeef, .owner = SPIN_LOCK_FREE, .count = 0, .core_id = SPINLOCK_CORE_ID_UNINITILIZE}
#else
#define SPIN_LOCK_INIT   { .owner = SPIN_LOCK_FREE, .count = 0, .core_id = SPINLOCK_CORE_ID_UNINITILIZE}
#endif
#define SPIN_LOCK_ACQUIRE_INIT   SPIN_LOCK_INIT

//adapte for freertos smp v2p0
#define SPINLOCK_FREE (SPIN_LOCK_FREE)         //0xB33FFFFF
#define SPINLOCK_INITIALIZER   SPIN_LOCK_INIT
#define SPINLOCK_ACQUIRE_INITIALIZER   SPIN_LOCK_ACQUIRE_INIT

// spinlock_acquire/release and spin_lock/unlock share the same owner/free convention.
void spinlock_init(spinlock_t *slock);
uint32_t spinlock_acquire(volatile spinlock_t *slock, int32_t timeout);	//@cyg:TODO:timeout doesn't support until-now.
void spinlock_release(volatile spinlock_t *slock, uint32_t flag);

/* similiar with linux spin lock API */
/* NOTE: !!! lock must located in shared memory */
void spin_lock_init(spinlock_t *lock);
void spin_lock(volatile spinlock_t *lock);
void spin_unlock(volatile spinlock_t *lock);

/* spin_trylock: 0: spin lock failed, 1: spin lock success  */
int spin_trylock(volatile spinlock_t *lock);

uint32_t _spin_lock_irqsave(volatile spinlock_t *lock);
void _spin_unlock_irqrestore(volatile spinlock_t *lock, uint32_t flags);

#define spin_lock_irqsave(lock, flags)      \
	do {				                    \
		flags = _spin_lock_irqsave(lock);	\
	} while (0)

#define spin_unlock_irqrestore(lock, flags)		\
	do {							            \
		_spin_unlock_irqrestore(lock, flags);	\
	} while (0)

#ifdef CONFIG_SPIRAM_WORKAROUND_NEED_VOLATILE_SPINLOCK
#define NEED_VOLATILE_MUX volatile
#else
#define NEED_VOLATILE_MUX
#endif


	/**
	 * @brief Initialize a lock to its default state - unlocked
	 * @param lock - spinlock object to initialize
	 */
	static inline void __attribute__((always_inline)) spinlock_initialize(spinlock_t *lock)
	{
		spinlock_init(lock);
	}

#if CONFIG_SPINLOCK_SECTION
#define SPINLOCK_SECTION __attribute__((used, section(".sram_spinlock_section"))) 
spinlock_t *spinlock_mem_dynamic_alloc(void);
bk_err_t spinlock_mem_dynamic_free(spinlock_t *slock);
#else
#define SPINLOCK_SECTION
#endif

#ifdef __cplusplus
}
#endif

#endif /* _spinlock_h_ */

