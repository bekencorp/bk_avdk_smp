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

#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "spinlock.h"
#include "cmsis_gcc.h"

#if CONFIG_SOC_SMP
#include "FreeRTOS.h"

#endif

#if CONFIG_SPINLOCK_DEBUG
#include "task.h"
#endif


#define arch_int_disable	rtos_disable_int
#define arch_int_restore	rtos_enable_int

#if CONFIG_SOC_SMP
static inline void spinlock_take(volatile spinlock_t *lock)
{
	uint32_t core_id;
	int status;

	core_id = (uint32_t)portGET_CORE_ID();

	if(core_id == lock->owner)
	{
		BK_ASSERT(lock->count > 0 && lock->count < 0xFF);
		lock->count++;
		return;
	}

	do
	{
		while (__LDAEX(&lock->owner) != SPIN_LOCK_FREE)
		{
			__CLREX();
			__WFE();
		}

		BK_ASSERT((core_id == 0) || (core_id == 1));
		status = __STREXW(core_id, &lock->owner);
	} while (status != 0);

	__DMB();
	lock->core_id = core_id;
	lock->count = 1;
}

static inline void spinlock_give(volatile spinlock_t *lock)
{
	uint32_t core_id;

	core_id = (uint32_t)portGET_CORE_ID();

	if(core_id != lock->owner)
	{
		BK_ASSERT(0);
		return;
	}

	if(lock->count == 0)
	{
		BK_ASSERT(0);
		return;
	}

	lock->count--;

	if(lock->count == 0)
	{
		lock->core_id = SPINLOCK_CORE_ID_UNINITILIZE;
		__DMB();
		__STL(SPIN_LOCK_FREE, &lock->owner);
		__DSB();
		__SEV();
	}
}
#endif

void spinlock_init(spinlock_t *slock)
{
#if CONFIG_SPINLOCK_DEBUG
    beken_thread_t  Current_TCB = NULL;
    Current_TCB = (beken_thread_t)xTaskGetCurrentTaskHandle();
    slock->taskTCBPointer = (uint32_t )Current_TCB;
#endif
	slock->owner = SPIN_LOCK_FREE;
	slock->count = 0;
	slock->core_id = SPINLOCK_CORE_ID_UNINITILIZE;
}

uint32_t spinlock_acquire(volatile spinlock_t *slock, int32_t timeout)
{
	uint32_t flag = arch_int_disable();
	(void)timeout;

#if CONFIG_SOC_SMP
	spinlock_take(slock);
#endif
	arch_int_restore(flag);

	return flag;
}

void spinlock_release(volatile spinlock_t *slock, uint32_t flag2)
{
	uint32_t flag = arch_int_disable();
	(void)flag2;

#if CONFIG_SOC_SMP
	spinlock_give(slock);
#endif
	arch_int_restore(flag);
}

#if (CONFIG_SOC_SMP)

void spin_lock(volatile spinlock_t *lock)
{
	spinlock_take(lock);
}

void spin_unlock(volatile spinlock_t *lock)
{
	spinlock_give(lock);
}

uint32_t _spin_lock_irqsave(volatile spinlock_t *lock)
{
	unsigned long flags = rtos_disable_int();
	spin_lock(lock);

	return flags;
}

void _spin_unlock_irqrestore(volatile spinlock_t *lock, uint32_t flags)
{
	spin_unlock(lock);
	rtos_enable_int(flags);
}

/*
 * SPINLOCK:
 * NOTES:
 * There are two exclusive mointors in SRAM PORT, if two CPU-COREs exclusive access SRAM at the same time,
 * the third device(i.e:DMA, Audio, ...) writes data to SRAM will be failed without any indications.
 * So SPINLOCK memory are allocated in special section which will not conflicts with other devices.
 * If static allocates spinlock, please sets the spinlock in section of "sram_spinlock_section"
 */
#if CONFIG_SPINLOCK_SECTION
SPINLOCK_SECTION spinlock_t s_spinlock_memlock = SPIN_LOCK_INIT;
#ifndef CONFIG_SPINLOCK_DYNAMIC_CNT
#define CONFIG_SPINLOCK_DYNAMIC_CNT (128)
#endif
SPINLOCK_SECTION spinlock_t s_spinlock_mem[CONFIG_SPINLOCK_DYNAMIC_CNT];
#define SPINLOCK_GROUPS_CNT ((CONFIG_SPINLOCK_DYNAMIC_CNT+31)/32)
static uint32_t s_mem_manage_bits[SPINLOCK_GROUPS_CNT];

spinlock_t *spinlock_mem_dynamic_alloc()
{
	uint32_t i, j, k;
	spinlock_t *lock_p = NULL;
	uint32_t int_level = rtos_disable_int();
	spin_lock(&s_spinlock_memlock);

	for(k = 0; k < CONFIG_SPINLOCK_DYNAMIC_CNT; k++)
	{
		i = k / 32;
		j = k - (i * 32);
		if(s_mem_manage_bits[i] & (0x1 << j))
		{
			continue;
		}
		s_mem_manage_bits[i] |= 0x1 << j;
		spinlock_init(&s_spinlock_mem[k]);
		lock_p = &s_spinlock_mem[k];
		break;
	}

	if(lock_p == NULL)
	{
		BK_LOGD(NULL, "%s fail:spinlock doesn't free? or increase CONFIG_SPINLOCK_DYNAMIC_CNT\r\n", __func__);
		BK_ASSERT(0);	//please check whether some spinlock doesn't free, or increases CONFIG_SPINLOCK_DYNAMIC_CNT value
	}

	spin_unlock(&s_spinlock_memlock);
	rtos_enable_int(int_level);

	return lock_p;
}

bk_err_t spinlock_mem_dynamic_free(spinlock_t *slock)
{
	uint32_t int_level;
	uint32_t i, j, k;
	if((slock) &&
		((uint32_t)slock >= (uint32_t)&s_spinlock_mem[0]) &&
		((uint32_t)slock < (uint32_t)&s_spinlock_mem[CONFIG_SPINLOCK_DYNAMIC_CNT]))
	{
		k = slock - &s_spinlock_mem[0];
		i = k/32;
		j = k - (i*32);
	}
	else
	{
		BK_LOGD(NULL, "%s:free ptr=0x%x fail\r\n", __func__, slock);
		return BK_FAIL;
	}

	int_level = rtos_disable_int();
	spin_lock(&s_spinlock_memlock);
#if CONFIG_SPINLOCK_DEBUG
    slock->taskTCBPointer = 0;
#endif
    s_mem_manage_bits[i] &= ~(0x1<<j);
	spin_unlock(&s_spinlock_memlock);
	rtos_enable_int(int_level);


	return BK_OK;
}

#endif


#endif // CONFIG_SOC_SMP
// eof

