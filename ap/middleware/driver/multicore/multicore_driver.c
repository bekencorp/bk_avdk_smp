// Copyright 2020-2025 Beken
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
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cache.h"
#include "mbox0_drv.h"
#include "multicore_hal.h"
#include "multicore_driver.h"
#include "sys_ahbp_ll.h"
#include "bk_private/bk_wdt.h"

void bk_multicore_set_cpu_id(uint32_t cpu_id)
{
	multicore_hal_set_cpu_id(cpu_id);
}

uint32_t bk_multicore_get_cpu_id(void)
{
	return multicore_hal_get_cpu_id();
}

bk_err_t bk_multicore_start(uint32_t cpu_id)
{
	return multicore_hal_start(cpu_id);
}

bk_err_t bk_multicore_reset(uint32_t cpu_id)
{
	return multicore_hal_reset(cpu_id);
}

bk_err_t bk_multicore_stop(uint32_t cpu_id)
{
	return multicore_hal_stop(cpu_id);
}

#if CONFIG_CPU_HOTPLUG

#define AP_HOTPLUG_TIMEOUT_STAPS        (3)
#define AP_HOTPLUG_TIMEOUT_STAPS_ONLINE (20)
#define AP_HOTPLUG_TIMEOUT_ONE_STEP     (1)
#define AP_HOTPLUG_TIMEOUT_ONE_STEP_US  (AP_HOTPLUG_TIMEOUT_ONE_STEP * 500) /* 500us */
#define AP_HOTPLUG_TIMEOUT_ONE_STEP_MS  (AP_HOTPLUG_TIMEOUT_ONE_STEP)       /* 1ms */
#define AP_HOTPLUG_CPU3_ROUTE_REGS      (3)
#define AP_HOTPLUG_PRIMARY_ROUTE_REGS   (2)
#define AP_HOTPLUG_NVIC_WORDS           (4)
#define AP_HOTPLUG_SYSTICK_CTRL         (*(volatile uint32_t *)0xe000e010)
#define AP_HOTPLUG_SYSTICK_VAL          (*(volatile uint32_t *)0xe000e018)
#define AP_HOTPLUG_SCB_ICSR             (*(volatile uint32_t *)0xe000ed04)
#define AP_HOTPLUG_FPCCR                (*(volatile uint32_t *)0xe000ef34)
#define AP_HOTPLUG_NVIC_ICER_BASE       ((volatile uint32_t *)0xe000e180)
#define AP_HOTPLUG_NVIC_ICPR_BASE       ((volatile uint32_t *)0xe000e280)
#define AP_HOTPLUG_PENDSVCLR            (1UL << 27)
#define AP_HOTPLUG_PENDSTCLR            (1UL << 25)
#define AP_HOTPLUG_FPCCR_ASPEN          (1UL << 31)
#define AP_HOTPLUG_FPCCR_LSPEN          (1UL << 30)
#define BK_CPU_MASK(cpu)                BIT(cpu)

extern void bk_delay_us(UINT32 us);

typedef enum {
	BK_CPU_HP_DOMAIN_AP = 0,
} cpu_hp_domain_id_t;

typedef struct {
	cpu_hp_domain_id_t	id;
	uint32_t		possible_mask;
	uint32_t		primary_mask;
	uint32_t		hotplug_mask;
	uint32_t		online_mask;
	uint32_t		active_mask;
	uint32_t		dying_mask;
	uint32_t		offline_mask;
	uint32_t		primary_cpu;
	uint32_t		tick_owner_cpu;
	bk_cpu_hp_state_t	cpu_state[4];
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	uint32_t		cold_boot;
#endif
} cpu_hp_domain_t;

typedef struct {
	uint32_t valid;
	uint32_t primary_route[AP_HOTPLUG_PRIMARY_ROUTE_REGS];
	uint32_t target_route[AP_HOTPLUG_CPU3_ROUTE_REGS];
} ap_cpu3_irq_route_snapshot_t;

static beken_mutex_t _cpu_hp_lock;
static SPINLOCK_SECTION volatile spinlock_t _cpu_hp_spin_lock = SPIN_LOCK_INIT;

static volatile int32_t _cpu3_wants_offline = -1;
static volatile uint32_t _cpu3_offline_ack1;
static volatile uint32_t _cpu3_offline_ack2;
static volatile uint32_t _cpu3_offline_ack3;
static volatile uint32_t _cpu3_online_ack;
static ap_cpu3_irq_route_snapshot_t _cpu3_irq_route;

static cpu_hp_domain_t _ap_domain = {
	.id = BK_CPU_HP_DOMAIN_AP,
	.possible_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.primary_mask = BK_CPU_MASK(CPU2_CORE_ID),
	.hotplug_mask = BK_CPU_MASK(CPU3_CORE_ID),
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	/* Only CPU2 is online at power-on; CPU3 boots offline and is brought up
	 * later via bk_cpu_hp_online(CPU3_CORE_ID) / "cpu online 3". */
	.online_mask = BK_CPU_MASK(CPU2_CORE_ID),
	.active_mask = BK_CPU_MASK(CPU2_CORE_ID),
	.offline_mask = BK_CPU_MASK(CPU3_CORE_ID),
#else
	.online_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.active_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.offline_mask = 0,
#endif
	.primary_cpu = CPU2_CORE_ID,
	.tick_owner_cpu = CPU2_CORE_ID,
	.cpu_state = {
		[CPU2_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
		[CPU3_CORE_ID] = BK_CPU_HP_STATE_OFFLINE,
#else
		[CPU3_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
#endif
	},
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	.cold_boot = 1,
#endif
};

extern bk_err_t crosscore_int_send_hotplug_stop(int xCoreID);
extern void vPortHotplugResetCoreState(BaseType_t xCoreID);
extern void crosscore_int_reset_send(void);

static inline void _cpu_hp_barrier(void)
{
	__asm volatile("dsb sy" ::: "memory");
	__asm volatile("isb sy" ::: "memory");
}

static inline uint32_t _cpu_hp_disable_local_irq(void)
{
	return rtos_disable_int();
}

static inline void _cpu_hp_enable_local_irq(uint32_t irq_level)
{
	rtos_enable_int(irq_level);
}

static inline void _cpu_hp_wfi(void)
{
	__asm volatile("wfi" ::: "memory");
}

static bk_err_t _cpu_hp_lock_init(void)
{
	bk_err_t ret = BK_OK;
	uint32_t level = 0;

	level = rtos_enter_critical();
	
	if (_cpu_hp_lock == NULL) {
		ret = rtos_init_mutex(&_cpu_hp_lock);
	}

	rtos_exit_critical(level);

	return ret;
}

static inline cpu_hp_domain_t *_cpu_hp_domain(uint32_t cpu_id)
{
	if ((cpu_id == CPU2_CORE_ID) || (cpu_id == CPU3_CORE_ID)) {
		return &_ap_domain;
	}

	return NULL;
}

static inline uint32_t _cpu_hp_get_smp_core_id(
	const cpu_hp_domain_t *domain, uint32_t cpu_id)
{
	return cpu_id - domain->primary_cpu;
}

static inline void _cpu_hp_set_state(cpu_hp_domain_t *domain, uint32_t cpu_id,
	bk_cpu_hp_state_t state)
{
	domain->cpu_state[cpu_id] = state;
}

static void _cpu_hp_domain_set_active(cpu_hp_domain_t *domain, uint32_t cpu_id,
	uint32_t active)
{
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core = (BaseType_t)_cpu_hp_get_smp_core_id(domain, cpu_id);

	if (active) {
		domain->active_mask |= cpu_mask;
		vSetCoreActive(smp_core, pdTRUE);
	} else {
		domain->active_mask &= ~cpu_mask;
		vSetCoreActive(smp_core, pdFALSE);
	}
}

static void _cpu_hp_domain_set_online(cpu_hp_domain_t *domain, uint32_t cpu_id,
	uint32_t online)
{
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core = (BaseType_t)_cpu_hp_get_smp_core_id(domain, cpu_id);

	if (online) {
		domain->online_mask |= cpu_mask;
		domain->offline_mask &= ~cpu_mask;
		vSetCoreOnline(smp_core, pdTRUE);
	} else {
		domain->online_mask &= ~cpu_mask;
		domain->offline_mask |= cpu_mask;
		vSetCoreOnline(smp_core, pdFALSE);
	}
}

static void _cpu_hp_domain_set_dying(cpu_hp_domain_t *domain, uint32_t cpu_id,
	uint32_t dying)
{
	if (dying) {
		domain->dying_mask |= BK_CPU_MASK(cpu_id);
	} else {
		domain->dying_mask &= ~BK_CPU_MASK(cpu_id);
	}
}

static bk_err_t _cpu3_irq_route_backup(void)
{
	if (_cpu3_irq_route.valid == 0) {
		_cpu3_irq_route.primary_route[0] = sys_ahbp_ll_get_reg10_value();
		_cpu3_irq_route.primary_route[1] = sys_ahbp_ll_get_reg11_value();
		_cpu3_irq_route.target_route[0] = sys_ahbp_ll_get_reg12_value(); // TODO
		_cpu3_irq_route.target_route[1] = sys_ahbp_ll_get_reg13_value();
		_cpu3_irq_route.target_route[2] = sys_ahbp_ll_get_reg14_value();
		_cpu3_irq_route.valid = 1;
	}

	if (_cpu3_irq_route.target_route[2] != 0) {
		return BK_ERR_BUSY;
	}

	return BK_OK;
}

static bk_err_t _cpu3_irq_route_migrate_but_ipi(void)
{
	uint32_t stop_route = BIT(INT_SRC_MAILBOX) | BIT(INT_SRC_IPI);
	bk_err_t ret = _cpu3_irq_route_backup();

	if (ret != BK_OK) {
		return ret;
	}

	sys_ahbp_ll_set_reg10_value(_cpu3_irq_route.primary_route[0] |
		(_cpu3_irq_route.target_route[0] & ~stop_route));
	sys_ahbp_ll_set_reg11_value(_cpu3_irq_route.primary_route[1] |
		_cpu3_irq_route.target_route[1]);
	sys_ahbp_ll_set_reg12_value(_cpu3_irq_route.target_route[0] & stop_route);
	sys_ahbp_ll_set_reg13_value(0);
	sys_ahbp_ll_set_reg14_value(0);

	return BK_OK;
}

static void _cpu3_irq_route_mask_all(void)
{
	sys_ahbp_ll_set_reg12_value(0);
	sys_ahbp_ll_set_reg13_value(0);
	sys_ahbp_ll_set_reg14_value(0);
}

static void _cpu3_irq_route_restore(void)
{
	if (_cpu3_irq_route.valid) {
		sys_ahbp_ll_set_reg10_value(_cpu3_irq_route.primary_route[0]);
		sys_ahbp_ll_set_reg11_value(_cpu3_irq_route.primary_route[1]);
		sys_ahbp_ll_set_reg12_value(_cpu3_irq_route.target_route[0]);
		sys_ahbp_ll_set_reg13_value(_cpu3_irq_route.target_route[1]);
		sys_ahbp_ll_set_reg14_value(_cpu3_irq_route.target_route[2]);
		_cpu3_irq_route.valid = 0;
	}
}

static bk_err_t _cpu_hotplug_wait_ack(volatile uint32_t *ack, uint32_t timeout_steps)
{
	uint32_t current_step = 0;

	while (*ack == 0) {
		if (current_step >= timeout_steps) {
			return BK_ERR_TIMEOUT;
		}
		bk_delay_us(AP_HOTPLUG_TIMEOUT_ONE_STEP_US);
		current_step += AP_HOTPLUG_TIMEOUT_ONE_STEP;
	}

	return BK_OK;
}

uint32_t bk_cpu_hp_is_online(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	if (domain == NULL) {
		return 0;
	}

	return ((domain->online_mask & BK_CPU_MASK(cpu_id)) != 0) ? 1 : 0;
}

uint32_t bk_cpu_hp_is_active(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	if (domain == NULL) {
		return 0;
	}

	return ((domain->active_mask & BK_CPU_MASK(cpu_id)) != 0) ? 1 : 0;
}

bk_cpu_hp_state_t bk_cpu_hp_get_state(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? BK_CPU_HP_STATE_OFFLINE : domain->cpu_state[cpu_id];
}


static const char * const _cpu_hp_state_names[] = {
	[BK_CPU_HP_STATE_ONLINE] = "online",
	[BK_CPU_HP_STATE_OFFLINE_REQUESTED] = "offline-requested",
	[BK_CPU_HP_STATE_SCHEDULER_DRAINING] = "scheduler-draining",
	[BK_CPU_HP_STATE_IRQ_MIGRATING] = "irq-migrating",
	[BK_CPU_HP_STATE_QUIESCE] = "quiesce",
	[BK_CPU_HP_STATE_RESET_HOLD] = "reset-hold",
	[BK_CPU_HP_STATE_POWER_OFF] = "power-off",
	[BK_CPU_HP_STATE_OFFLINE] = "offline",
	[BK_CPU_HP_STATE_POWER_ON] = "power-on",
	[BK_CPU_HP_STATE_BOOT_PREPARE] = "boot-prepare",
	[BK_CPU_HP_STATE_RESET_RELEASE] = "reset-release",
	[BK_CPU_HP_STATE_SECONDARY_BOOT] = "secondary-boot",
	[BK_CPU_HP_STATE_JOIN_SCHEDULER] = "join-scheduler",
};
#define _CPU_HP_STATE_NAMES_COUNT \
	(sizeof(_cpu_hp_state_names) / sizeof(_cpu_hp_state_names[0]))

const char *bk_cpu_hp_get_state_name(uint32_t cpu_id)
{
	bk_cpu_hp_state_t state = bk_cpu_hp_get_state(cpu_id);

	if ((state >= _CPU_HP_STATE_NAMES_COUNT) ||
	    (_cpu_hp_state_names[state] == NULL)) {
		return "unknown";
	}

	return _cpu_hp_state_names[state];
}

uint32_t bk_cpu_hp_get_domain_possible_mask(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->possible_mask;
}

uint32_t bk_cpu_hp_get_domain_online_mask(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->online_mask;
}

uint32_t bk_cpu_hp_get_domain_active_mask(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->active_mask;
}

uint32_t bk_cpu_hp_get_domain_dying_mask(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->dying_mask;
}

uint32_t bk_cpu_hp_get_domain_offline_mask(uint32_t cpu_id)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->offline_mask;
}

static bk_err_t _cpu_hp_offline_internal(uint32_t cpu_id, uint32_t from_atomic)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core;
	bk_err_t ret;
	uint32_t cpu_hp_irq_level;

	if ((domain == NULL) || ((domain->hotplug_mask & cpu_mask) == 0)) {
		ret = BK_ERR_NOT_SUPPORT;
		goto nolock_out;
	}

	if (cpu_id != CPU3_CORE_ID) {
		ret = BK_ERR_NOT_SUPPORT;
		goto nolock_out;
	}

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		ret = BK_ERR_STATE;
		goto nolock_out;
	}

	smp_core = (BaseType_t)_cpu_hp_get_smp_core_id(domain, cpu_id);

	if (!from_atomic) {
		ret = _cpu_hp_lock_init();
		if (ret != BK_OK) {
			goto nolock_out;
		}
		rtos_lock_mutex(&_cpu_hp_lock);
	}

	if ((domain->online_mask & cpu_mask) == 0) {
		ret = BK_OK;
		goto out;
	}

	if (domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_ONLINE) {
		ret = BK_ERR_BUSY;
		goto out;
	}

	if (xTaskHasTasksPinnedToCore(smp_core) == pdTRUE) {
		ret = BK_ERR_BUSY;
		goto out;
	}

	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE_REQUESTED);
	_cpu_hp_domain_set_dying(domain, cpu_id, 1);
	_cpu3_offline_ack1 = 0;
	_cpu3_offline_ack2 = 0;
	_cpu3_offline_ack3 = 0;
	_cpu3_wants_offline = -1;
	ret = crosscore_int_send_hotplug_stop(CPU3_CORE_ID);
	if ((ret != BK_OK) && (ret != BK_ERR_IN_PROGRESS)) {
		_cpu_hp_domain_set_dying(domain, cpu_id, 0);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		goto out;
	}
	ret = _cpu_hotplug_wait_ack(&_cpu3_offline_ack1, AP_HOTPLUG_TIMEOUT_STAPS);
	if (ret != BK_OK) {
		spin_lock_irqsave(&_cpu_hp_spin_lock, cpu_hp_irq_level);
		if (_cpu3_wants_offline != 0) {
			_cpu3_wants_offline = -1;
		} else {
			spin_unlock_irqrestore(&_cpu_hp_spin_lock, cpu_hp_irq_level);
			goto continue_offline;
		}
		spin_unlock_irqrestore(&_cpu_hp_spin_lock, cpu_hp_irq_level);
		_cpu_hp_domain_set_dying(domain, cpu_id, 0);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		goto out;
	}

continue_offline:
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_IRQ_MIGRATING);
	_cpu3_irq_route_migrate_but_ipi();
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_QUIESCE);
	_cpu3_offline_ack2 = 1;
	_cpu_hp_barrier();

	while (_cpu3_offline_ack3 == 0);
#if CONFIG_TASK_WDT
	bk_task_wdt_set_feed_bits(smp_core, false);
#else
		(void)smp_core;
#endif
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_HOLD);
	ret = bk_multicore_stop(CPU3_CORE_ID);
	if (ret == BK_OK) {
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_OFF);
		_cpu_hp_domain_set_dying(domain, cpu_id, 0);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
	}

out:
	if (!from_atomic)
		rtos_unlock_mutex(&_cpu_hp_lock);
nolock_out:
	return ret;
}

static bk_err_t _cpu_hp_online_internal(uint32_t cpu_id, uint32_t from_atomic)
{
	cpu_hp_domain_t *domain = _cpu_hp_domain(cpu_id);
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	bk_err_t ret;
	BaseType_t smp_core;

	if ((domain == NULL) || ((domain->hotplug_mask & cpu_mask) == 0)) {
		ret = BK_ERR_NOT_SUPPORT;
		goto nolock_out;
	}

	if (cpu_id != CPU3_CORE_ID) {
		ret = BK_ERR_NOT_SUPPORT;
		goto nolock_out;
	}

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		ret = BK_ERR_STATE;
		goto nolock_out;
	}

	smp_core = (BaseType_t)_cpu_hp_get_smp_core_id(domain, cpu_id);

	if (!from_atomic) {
		ret = _cpu_hp_lock_init();
			if (ret != BK_OK) {
				goto nolock_out;
		}
		rtos_lock_mutex(&_cpu_hp_lock);
	}

	if (domain->cpu_state[cpu_id] == BK_CPU_HP_STATE_ONLINE) {
		ret = BK_OK;
		goto out;
	}

	if ((domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_OFFLINE) &&
	    (domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_RESET_HOLD)) {
		ret = BK_ERR_BUSY;
		goto out;
	}

	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_ON);
	_cpu3_online_ack = 0;
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	if (domain->cold_boot == 0) {
#endif
	_cpu3_irq_route_mask_all();
	vTaskHotplugResetIdleTaskContext(smp_core);
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	}
#endif
	vPortHotplugResetCoreState(smp_core);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_BOOT_PREPARE);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_RELEASE);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_SECONDARY_BOOT);

	ret = bk_multicore_start(CPU3_CORE_ID);
	if (ret == BK_OK)
		ret = _cpu_hotplug_wait_ack(&_cpu3_online_ack, AP_HOTPLUG_TIMEOUT_STAPS_ONLINE);
	else
		goto fail_online;

	if (ret == BK_OK) {
#if CONFIG_TASK_WDT
		bk_task_wdt_set_feed_bits(smp_core, true);
#else
		(void)smp_core;
#endif
		_cpu_hp_domain_set_online(domain, cpu_id, 1);
#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
		if (domain->cold_boot == 1)
			domain->cold_boot = 0;
		else
			_cpu3_irq_route_restore();
#else
		_cpu3_irq_route_restore();
#endif
		mbox0_init_on_current_core(CPU3_CORE_ID);
		_cpu_hp_domain_set_active(domain, cpu_id, 1);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		/* CPU3 is now fully online with its mailbox RX ready. Clear any
		 * outgoing doorbell that was lost while CPU3 was still joining, so a
		 * stuck "busy" flag can't swallow the next hotplug STOP. */
		crosscore_int_reset_send();
		goto out;
	}

fail_online:
	bk_multicore_stop(CPU3_CORE_ID);
	_cpu_hp_domain_set_active(domain, cpu_id, 0);
	_cpu_hp_domain_set_online(domain, cpu_id, 0);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);

out:
	if (!from_atomic)
		rtos_unlock_mutex(&_cpu_hp_lock);
nolock_out:
	return ret;
}

uint32_t bk_cpu_hp_enter_primary(void)
{
	BaseType_t old_core_id = xTaskHotplugSetCurrentTaskCoreID(SMP_CORE0_ID);

	for (uint32_t i = 0;
		(i < AP_HOTPLUG_TIMEOUT_STAPS) && (portGET_CORE_ID() != SMP_CORE0_ID);
		i += AP_HOTPLUG_TIMEOUT_ONE_STEP) {
		taskYIELD();
		rtos_delay_milliseconds(AP_HOTPLUG_TIMEOUT_ONE_STEP_MS);
	}

	return old_core_id;
}

void bk_cpu_hp_exit_primary(uint32_t old_core_id)
{
	(void)xTaskHotplugSetCurrentTaskCoreID(old_core_id);
}

static bk_err_t cpu_hp_offline_do(uint32_t cpu_id)
{
	bk_err_t ret = BK_FAIL;
	uint32_t old_core_id;
	uint32_t from_atomic;

	BK_ASSERT(platform_is_in_interrupt_context() == BK_FALSE);

	old_core_id = bk_cpu_hp_enter_primary();

	if (portGET_CORE_ID() == SMP_CORE0_ID) {
		from_atomic = platform_local_irq_disabled();
		ret = _cpu_hp_offline_internal(cpu_id, from_atomic);
	} else {
		MULTICORE_LOGW("cpu%u offline must run on primary core, current SMP core=%d\r\n",
			cpu_id, portGET_CORE_ID());
	}

	bk_cpu_hp_exit_primary(old_core_id);

	return ret;
}

static bk_err_t cpu_hp_online_do(uint32_t cpu_id)
{
	bk_err_t ret = BK_FAIL;
	uint32_t old_core_id;
	uint32_t from_atomic;

	BK_ASSERT(platform_is_in_interrupt_context() == BK_FALSE);

	old_core_id = bk_cpu_hp_enter_primary();

	if (portGET_CORE_ID() == SMP_CORE0_ID) {
		from_atomic = platform_local_irq_disabled();
		ret = _cpu_hp_online_internal(cpu_id, from_atomic);
	} else {
		MULTICORE_LOGW("cpu%u online must run on primary core, current SMP core=%d\r\n",
			cpu_id, portGET_CORE_ID());
	}

	bk_cpu_hp_exit_primary(old_core_id);

	return ret;
}

bk_err_t bk_cpu_hp_offline_direct(uint32_t cpu_id)
{
#if CONFIG_CPU_HP_VOTE
	MULTICORE_LOGW("cpu%u offline_direct: bypassing vote layer, vote tally may desync\r\n",
		cpu_id);
#endif
	return cpu_hp_offline_do(cpu_id);
}

bk_err_t bk_cpu_hp_online_direct(uint32_t cpu_id)
{
#if CONFIG_CPU_HP_VOTE
	MULTICORE_LOGW("cpu%u online_direct: bypassing vote layer, vote tally may desync\r\n",
		cpu_id);
#endif
	return cpu_hp_online_do(cpu_id);
}

bk_err_t bk_cpu_hp_offline(uint32_t cpu_id)
{
#if CONFIG_CPU_HP_VOTE
	MULTICORE_LOGW("cpu%u offline: CPU_HP_VOTE enabled, use bk_cpu_hp_vote_* (or _force)\r\n",
		cpu_id);
	return BK_ERR_NOT_SUPPORT;
#else
	return cpu_hp_offline_do(cpu_id);
#endif
}

bk_err_t bk_cpu_hp_online(uint32_t cpu_id)
{
#if CONFIG_CPU_HP_VOTE
	MULTICORE_LOGW("cpu%u online: CPU_HP_VOTE enabled, use bk_cpu_hp_vote_* (or _force)\r\n",
		cpu_id);
	return BK_ERR_NOT_SUPPORT;
#else
	return cpu_hp_online_do(cpu_id);
#endif
}

void multicore_stop_core1(void)
{
	(void)cpu_hp_offline_do(CPU3_CORE_ID);
}

void bk_cpu_hp_core_stop_hmb_isr(void)
{
	if (portGET_CORE_ID() != SMP_CORE1_ID) {
		return;
	}

	cpu_hp_domain_t *domain = &_ap_domain;
	uint32_t cpu_id = CPU3_CORE_ID;
	_cpu3_wants_offline = 1;
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_SCHEDULER_DRAINING);
	_cpu_hp_domain_set_active(domain, cpu_id, 0);
}

static void _cpu_hp_idle_handler_online(cpu_hp_domain_t *domain, uint32_t cpu_id)
{
	if ((portGET_CORE_ID() == SMP_CORE1_ID) &&
	    (_ap_domain.cpu_state[CPU3_CORE_ID] == BK_CPU_HP_STATE_SECONDARY_BOOT)) {
		_cpu_hp_set_state(&_ap_domain, CPU3_CORE_ID, BK_CPU_HP_STATE_JOIN_SCHEDULER);
		_cpu3_online_ack = 1;
		_cpu_hp_barrier();
	}
	_cpu3_wants_offline = -1;
}

static void _cpu_hp_idle_handler_offline(cpu_hp_domain_t *domain, uint32_t cpu_id)
{
	uint32_t irq_level;
	irq_level = _cpu_hp_disable_local_irq();
	spin_lock(&_cpu_hp_spin_lock);
	if (_cpu3_wants_offline != 1) {
		_cpu_hp_domain_set_active(domain, cpu_id, 1);
		spin_unlock(&_cpu_hp_spin_lock);
		_cpu_hp_enable_local_irq(irq_level);
		return;
	}

	_cpu3_wants_offline = 0;
	spin_unlock(&_cpu_hp_spin_lock);

	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_QUIESCE);
	_cpu3_offline_ack1 = 1;
	_cpu_hp_barrier();
	while (_cpu3_offline_ack2 == 0);
	
	AP_HOTPLUG_SYSTICK_CTRL = 0;
	AP_HOTPLUG_SYSTICK_VAL = 0;
	AP_HOTPLUG_SCB_ICSR = AP_HOTPLUG_PENDSVCLR | AP_HOTPLUG_PENDSTCLR;
	AP_HOTPLUG_FPCCR &= ~(AP_HOTPLUG_FPCCR_ASPEN | AP_HOTPLUG_FPCCR_LSPEN);

	for (uint32_t i = 0; i < AP_HOTPLUG_NVIC_WORDS; i++) {
		AP_HOTPLUG_NVIC_ICER_BASE[i] = 0xffffffff;
		AP_HOTPLUG_NVIC_ICPR_BASE[i] = 0xffffffff;
	}

#if CONFIG_CACHE_MAINTENANCE
	flush_all_dcache();
#endif
	vTaskHotplugClearCurrentTCB(SMP_CORE1_ID);
	_cpu3_irq_route_mask_all();
	_cpu_hp_domain_set_online(domain, cpu_id, 0);
	_cpu3_offline_ack3 = 1;
	_cpu_hp_barrier();

	while (1) {
		_cpu_hp_wfi();
	}
}

void bk_cpu_hp_idle_handler(void)
{
	cpu_hp_domain_t *domain = &_ap_domain;
	uint32_t cpu_id = CPU3_CORE_ID;

	if (portGET_CORE_ID() != SMP_CORE1_ID) {
		return;
	}

	if (domain->offline_mask & BK_CPU_MASK(cpu_id))
		_cpu_hp_idle_handler_online(domain, cpu_id);
	else
		_cpu_hp_idle_handler_offline(domain, cpu_id);
}

#if CONFIG_CPU_HP_GOVERNOR

/* Idle run-time counter (per core) lives in the FreeRTOS task additions and is
 * only declared inside tasks.c, so we re-declare the prototype here. It is built
 * because configGENERATE_RUN_TIME_STATS && INCLUDE_xTaskGetIdleTaskHandle. */
extern configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounterForCore(BaseType_t xCoreID);

/* -------- Tunables (Kconfig with compile-time fallbacks) ------------------ */

#define CPU_HP_GOV_PERIOD_MS       (CONFIG_CPU_HP_GOVERNOR_PERIOD_MS)
#define CPU_HP_GOV_UP_TH           (CONFIG_CPU_HP_GOVERNOR_UP_THRESHOLD)
#define CPU_HP_GOV_DOWN_TH         (CONFIG_CPU_HP_GOVERNOR_DOWN_THRESHOLD)
#define CPU_HP_GOV_UP_DEBOUNCE     (CONFIG_CPU_HP_GOVERNOR_UP_DEBOUNCE)
#define CPU_HP_GOV_DOWN_DEBOUNCE   (CONFIG_CPU_HP_GOVERNOR_DOWN_DEBOUNCE)
#define CPU_HP_GOV_COOLDOWN_MS     (CONFIG_CPU_HP_GOVERNOR_COOLDOWN_MS)

#define CPU_HP_GOV_LOAD_EMA_DIV    (CONFIG_CPU_HP_GOVERNOR_LOAD_EMA_DIV)
#define CPU_HP_GOV_LOAD_EMA_WEIGHT (CPU_HP_GOV_LOAD_EMA_DIV - 1U)

#define CPU_HP_GOV_TASK_PRIO       (BEKEN_DEFAULT_WORKER_PRIORITY)
#define CPU_HP_GOV_TASK_STACK      (2048)

/* SMP core id (0..CONFIG_SMP_CORE_CNT) -> global CPU id for the hotplug API.
 * On the AP domain CPU2 is smp core 0 and CPU3 is smp core 1, so the global id
 * is the smp core id plus CONFIG_CPU_ID_OFFSET. */
#define CPU_HP_GOV_SMP_TO_CPU(smp) ((smp) + CONFIG_CPU_ID_OFFSET)

typedef struct {
	beken_thread_t  thread;
	volatile uint32_t enabled;
	volatile uint32_t running;

	uint32_t idle_prev[CONFIG_SMP_CORE_CNT]; /* last idle counter per smp core */
	uint32_t tick_prev;			 /* last sample time (run-time counter base) */
	uint32_t load[CONFIG_SMP_CORE_CNT];	 /* smoothed busy% per smp core */

	uint32_t up_cnt;
	uint32_t down_cnt;
	uint32_t cooldown_left_ms; /* remaining cooldown after a transition */

	uint32_t online_cnt;
	uint32_t offline_cnt;
} cpu_hp_gov_t;

static cpu_hp_gov_t s_gov;

/* The run-time stats time base is bk_get_tick() truncated to 32 bits (see
 * portGET_RUN_TIME_COUNTER_VALUE in FreeRTOSConfig.h), so use the same width so
 * idle-counter deltas and elapsed-time deltas are in identical units. */
static inline uint32_t cpu_hp_gov_now(void)
{
	return (uint32_t)bk_get_tick();
}

static void cpu_hp_gov_reset_window(cpu_hp_gov_t *g)
{
	uint32_t c_core_id = SMP_CORE0_ID;
	g->idle_prev[c_core_id] = ulTaskGetIdleRunTimeCounterForCore(c_core_id);
	for (c_core_id++; c_core_id < CONFIG_SMP_CORE_CNT; c_core_id++) {
		g->idle_prev[c_core_id] = bk_cpu_hp_is_online(CPU_HP_GOV_SMP_TO_CPU(c_core_id)) ?
			ulTaskGetIdleRunTimeCounterForCore(c_core_id) : 0;
	}
	g->tick_prev = cpu_hp_gov_now();
	g->up_cnt = 0;
	g->down_cnt = 0;
}

static void cpu_hp_gov_sample(cpu_hp_gov_t *g)
{
	uint32_t now = cpu_hp_gov_now();
	uint32_t span = now - g->tick_prev;   /* unsigned: wrap-safe */

	if (span == 0) {
		return;
	}

	for (uint32_t c = SMP_CORE0_ID; c < CONFIG_SMP_CORE_CNT; c++) {
		uint32_t busy;

		if (!bk_cpu_hp_is_online(CPU_HP_GOV_SMP_TO_CPU(c))) {
			/* Offline core consumes no capacity. */
			busy = 0;
			g->idle_prev[c] = 0;
		} else {
			uint32_t idle = ulTaskGetIdleRunTimeCounterForCore((BaseType_t)c);
			uint32_t didle = idle - g->idle_prev[c];   /* wrap-safe */
			uint32_t ipct;

			g->idle_prev[c] = idle;
			ipct = (didle >= span) ? 100 : ((didle * 100) / span);
			busy = 100 - ipct;
		}

		/* First-order EMA to damp transient spikes. */
		g->load[c] = ((g->load[c] * CPU_HP_GOV_LOAD_EMA_WEIGHT) + busy) / CPU_HP_GOV_LOAD_EMA_DIV;
	}

	g->tick_prev = now;
}

static void cpu_hp_gov_try_online(cpu_hp_gov_t *g)
{
	if (g->load[SMP_CORE0_ID] > CPU_HP_GOV_UP_TH) {
		g->up_cnt++;
	} else {
		g->up_cnt = 0;
	}

	if (g->up_cnt < CPU_HP_GOV_UP_DEBOUNCE) {
		return;
	}

	if (cpu_hp_online_do(CPU3_CORE_ID) == BK_OK) {
		g->online_cnt++;
		g->cooldown_left_ms = CPU_HP_GOV_COOLDOWN_MS;
		cpu_hp_gov_reset_window(g);
		MULTICORE_LOGI("governor: online cpu3 (load0=%u%%)\r\n", g->load[SMP_CORE0_ID]);
	} else {
		/* Could not bring it up now; back off and retry next window. */
		g->up_cnt = 0;
	}
}

static void cpu_hp_gov_try_offline(cpu_hp_gov_t *g)
{
	uint32_t combined = g->load[SMP_CORE0_ID] + g->load[SMP_CORE1_ID];

	if (combined < CPU_HP_GOV_DOWN_TH) {
		g->down_cnt++;
	} else {
		g->down_cnt = 0;
	}

	if (g->down_cnt < CPU_HP_GOV_DOWN_DEBOUNCE) {
		return;
	}

	bk_err_t ret = cpu_hp_offline_do(CPU3_CORE_ID);
	if (ret == BK_OK) {
		g->offline_cnt++;
		g->cooldown_left_ms = CPU_HP_GOV_COOLDOWN_MS;
		cpu_hp_gov_reset_window(g);
		MULTICORE_LOGI("governor: offline cpu3 (load0+1=%u%%)\r\n", combined);
	} else {
		/* BK_ERR_BUSY: core still has pinned tasks / is mid-transition.
		 * Drop the counter and re-evaluate later. */
		g->down_cnt = 0;
	}
}

static void cpu_hp_gov_task(void *arg)
{
	cpu_hp_gov_t *g = &s_gov;

	(void)arg;

#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	if (!bk_cpu_hp_is_online(CPU3_CORE_ID) &&
	    (xTaskHasTasksPinnedToCore(SMP_CORE1_ID) == pdTRUE)) {
		if (cpu_hp_online_do(CPU3_CORE_ID) == BK_OK) {
			g->online_cnt++;
			g->cooldown_left_ms = CPU_HP_GOV_COOLDOWN_MS;
			MULTICORE_LOGI("governor: online cpu3 (pinned tasks present at boot)\r\n");
		} else {
			MULTICORE_LOGW("governor: online cpu3 for pinned tasks failed\r\n");
		}
	}
#endif

	cpu_hp_gov_reset_window(g);

	while (g->running) {
		rtos_delay_milliseconds(CPU_HP_GOV_PERIOD_MS);

		cpu_hp_gov_sample(g);

		if (g->cooldown_left_ms > 0) {
			g->cooldown_left_ms = (g->cooldown_left_ms > CPU_HP_GOV_PERIOD_MS) ?
				(g->cooldown_left_ms - CPU_HP_GOV_PERIOD_MS) : 0;
			continue;
		}

		if (!g->enabled) {
			continue;
		}

		if (!bk_cpu_hp_is_online(CPU3_CORE_ID))
			cpu_hp_gov_try_online(g);
		else
			cpu_hp_gov_try_offline(g);
	}

	g->thread = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t bk_cpu_hp_governor_init(void)
{
	bk_err_t ret;

	if (s_gov.thread != NULL) {
		return BK_OK;
	}

	os_memset(&s_gov, 0, sizeof(s_gov));
#if CONFIG_CPU_HP_GOVERNOR_AUTOSTART
	s_gov.enabled = 1;
#endif
	s_gov.running = 1;

	ret = rtos_create_thread(&s_gov.thread, CPU_HP_GOV_TASK_PRIO, "cpu_gov",
		cpu_hp_gov_task, CPU_HP_GOV_TASK_STACK, NULL);
	if (ret != BK_OK) {
		s_gov.running = 0;
		s_gov.thread = NULL;
		MULTICORE_LOGE("governor: create task failed %d\r\n", ret);
	}

	return ret;
}

bk_err_t bk_cpu_hp_governor_start(void)
{
	if (s_gov.thread == NULL) {
		return BK_ERR_STATE;
	}

	s_gov.enabled = 1;
	return BK_OK;
}

bk_err_t bk_cpu_hp_governor_stop(void)
{
	s_gov.enabled = 0;
	return BK_OK;
}

void bk_cpu_hp_governor_get_status(bk_cpu_hp_governor_status_t *status)
{
	if (status == NULL) {
		return;
	}

	status->enabled     = s_gov.enabled;
	status->load0       = s_gov.load[SMP_CORE0_ID];
	status->load1       = s_gov.load[SMP_CORE1_ID];
	status->cpu1_online = bk_cpu_hp_is_online(CPU3_CORE_ID);
	status->up_cnt      = s_gov.up_cnt;
	status->down_cnt    = s_gov.down_cnt;
	status->online_cnt  = s_gov.online_cnt;
	status->offline_cnt = s_gov.offline_cnt;
}

#endif /* CONFIG_CPU_HP_GOVERNOR */

#if CONFIG_CPU_HP_VOTE

#define CPU_HP_VOTE_MAX_VOTERS          (CONFIG_CPU_HP_VOTE_MAX_VOTERS)
#define CPU_HP_VOTE_BITMAP_WORDS        ((CPU_HP_VOTE_MAX_VOTERS + 31u) / 32u)
#define CPU_HP_VOTE_TARGET_CPU          (CPU3_CORE_ID)  /* only CPU3 is hotpluggable */

struct cpu_hp_voter {
	uint32_t bit;				/* this voter's ticket index [0, MAX) */
#if CONFIG_CPU_HP_VOTE_DUMP
	struct cpu_hp_voter *next;		/* registry list link */
#endif
#if CONFIG_CPU_HP_VOTE_NAME
	char name[CONFIG_CPU_HP_VOTE_NAME_LEN];	/* diagnostic / lookup label */
#endif
};

static uint32_t s_alloc_mask[CPU_HP_VOTE_BITMAP_WORDS];	/* shadow reg: bit set == ticket in use */
static uint32_t s_online_mask[CPU_HP_VOTE_BITMAP_WORDS];	/* online-voting ticket bits */
#if CONFIG_CPU_HP_VOTE_DUMP
static struct cpu_hp_voter *s_voter_list;		/* registry of live voters */
#endif

#if CONFIG_CPU_HP_VOTE_STATIC
static struct cpu_hp_voter s_voters[CPU_HP_VOTE_MAX_VOTERS];	/* static handle pool (no malloc); slot index == ticket bit */
#endif

static uint32_t s_online_count;	/* voters voting online (popcount s_online_mask) */
static uint32_t s_voter_count;	/* registered voters (popcount s_alloc_mask) */

static beken_mutex_t s_vote_lock;

static bk_err_t cpu_hp_vote_lock_init(void)
{
	bk_err_t ret = BK_OK;
	uint32_t level;

	level = rtos_enter_critical();
	if (s_vote_lock == NULL) {
		ret = rtos_init_mutex(&s_vote_lock);
	}
	rtos_exit_critical(level);

	return ret;
}

static inline uint32_t cpu_hp_vote_bit_test(const uint32_t *mask, uint32_t bit)
{
	return (mask[bit >> 5] >> (bit & 31u)) & 1u;
}

static inline void cpu_hp_vote_bit_set(uint32_t *mask, uint32_t bit)
{
	mask[bit >> 5] |= (1u << (bit & 31u));
}

static inline void cpu_hp_vote_bit_clear(uint32_t *mask, uint32_t bit)
{
	mask[bit >> 5] &= ~(1u << (bit & 31u));
}

static uint32_t cpu_hp_vote_is_valid_locked(const cpu_hp_voter_handle_t voter)
{
	if ((voter == NULL) || (voter->bit >= CPU_HP_VOTE_MAX_VOTERS))
		return 0;

	return cpu_hp_vote_bit_test(s_alloc_mask, voter->bit);
}

/**
 * @brief Change @voter's vote and drive the hotplug core only when s_online_count crosses zero (0->1 online, 1->0 offline). The no-change guard keeps repeated votes idempotent. Returns the hotplug result on a transition, else BK_OK.
 * 
 * @param voter voter handle
 * @param want_online 1 to vote online, 0 to vote offline
 * @return bk_err_t BK_OK if the vote is applied successfully, otherwise an error code.
 */
static bk_err_t cpu_hp_vote_apply_locked(cpu_hp_voter_handle_t voter, uint32_t want_online)
{
	bk_err_t ret = BK_OK;

	if (cpu_hp_vote_bit_test(s_online_mask, voter->bit) == want_online)
		return ret;

	if (want_online) {
		if (!bk_cpu_hp_is_online(CPU_HP_VOTE_TARGET_CPU)) {
			ret = cpu_hp_online_do(CPU_HP_VOTE_TARGET_CPU);
			MULTICORE_LOGI("cpu_hp_vote: try to online cpu%u (voters=%u) ret=%d\r\n",
				CPU_HP_VOTE_TARGET_CPU, s_online_count + 1, ret);
			if (ret != BK_OK)
				return ret;
		}
		cpu_hp_vote_bit_set(s_online_mask, voter->bit);
		s_online_count++;
	} else {
		if (s_online_count == 1) {
			ret = cpu_hp_offline_do(CPU_HP_VOTE_TARGET_CPU);
			MULTICORE_LOGI("cpu_hp_vote: try to offline cpu%u (all released) ret=%d\r\n",
				CPU_HP_VOTE_TARGET_CPU, ret);
			if (ret != BK_OK)
				return ret;
		}
		cpu_hp_vote_bit_clear(s_online_mask, voter->bit);
		s_online_count--;
	}

	return ret;
}

/**
 * @brief Grab a free ticket bit and allocate its handle. Caller holds
 * s_vote_lock. NULL if the ticket pool is full or allocation fails.
 */
static cpu_hp_voter_handle_t cpu_hp_vote_alloc_locked(const char *name)
{
	uint32_t bit = CPU_HP_VOTE_MAX_VOTERS;
	cpu_hp_voter_handle_t voter;

	for (uint32_t i = 0; i < CPU_HP_VOTE_MAX_VOTERS; i++) {
		if (!cpu_hp_vote_bit_test(s_alloc_mask, i)) {
			bit = i;
			break;
		}
	}
	if (bit == CPU_HP_VOTE_MAX_VOTERS)
		return NULL;

#if CONFIG_CPU_HP_VOTE_STATIC
	/* static pool: slot index maps 1:1 to the ticket bit */
	voter = &s_voters[bit];
	os_memset(voter, 0, sizeof(*voter));
#else
	voter = (cpu_hp_voter_handle_t)os_zalloc(sizeof(*voter));
	if (voter == NULL)
		return NULL;
#endif

	voter->bit = bit;
#if CONFIG_CPU_HP_VOTE_NAME
	if (name != NULL) {
		os_strncpy(voter->name, name, CONFIG_CPU_HP_VOTE_NAME_LEN - 1);
		voter->name[CONFIG_CPU_HP_VOTE_NAME_LEN - 1] = '\0';
	}
#else
	(void)name;
#endif

	cpu_hp_vote_bit_set(s_alloc_mask, bit);
	s_voter_count++;

	/* adopt the current core state so s_online_count tracks reality */
	if (bk_cpu_hp_is_online(CPU_HP_VOTE_TARGET_CPU)) {
		cpu_hp_vote_bit_set(s_online_mask, bit);
		s_online_count++;
	}

#if CONFIG_CPU_HP_VOTE_DUMP
	voter->next = s_voter_list;
	s_voter_list = voter;
#endif

	return voter;
}

/**
 * @brief Release a voter: unlink it, clear its ticket bits, and release the
 * handle (heap free in dynamic mode; nothing to free for the static pool).
 * Caller holds s_vote_lock.
 */
static void cpu_hp_vote_free_locked(cpu_hp_voter_handle_t voter)
{
#if CONFIG_CPU_HP_VOTE_DUMP
	cpu_hp_voter_handle_t *pp = &s_voter_list;

	while ((*pp != NULL) && (*pp != voter))
		pp = &(*pp)->next;
	if (*pp == voter)
		*pp = voter->next;
#endif

	/* online bit is normally cleared by apply(offline) before free; be defensive */
	if (cpu_hp_vote_bit_test(s_online_mask, voter->bit)) {
		cpu_hp_vote_bit_clear(s_online_mask, voter->bit);
		if (s_online_count != 0)
			s_online_count--;
	}
	cpu_hp_vote_bit_clear(s_alloc_mask, voter->bit);
	s_voter_count--;

#if !CONFIG_CPU_HP_VOTE_STATIC
	os_free(voter);
#endif
}

cpu_hp_voter_handle_t bk_cpu_hp_vote_register(const char *name)
{
	cpu_hp_voter_handle_t voter;

	if (name == NULL)
		return NULL;

	if (cpu_hp_vote_lock_init() != BK_OK) {
		MULTICORE_LOGE("cpu_hp_vote: lock init failed\r\n");
		return NULL;
	}

	rtos_lock_mutex(&s_vote_lock);
	voter = cpu_hp_vote_alloc_locked(name);
	rtos_unlock_mutex(&s_vote_lock);

	if (voter == NULL) {
		MULTICORE_LOGE("cpu_hp_vote: table full (max=%u)\r\n", CPU_HP_VOTE_MAX_VOTERS);
	} else {
		MULTICORE_LOGI("cpu_hp_vote: register '%s'\r\n", name);
	}

	return voter;
}

bk_err_t bk_cpu_hp_vote_unregister(cpu_hp_voter_handle_t voter)
{
	bk_err_t ret;

	/* A NULL handle is always a parameter error, regardless of init state. */
	if (voter == NULL) {
		return BK_ERR_PARAM;
	}

	if (s_vote_lock == NULL) {
		return BK_ERR_NOT_INIT;
	}

	rtos_lock_mutex(&s_vote_lock);

	if (!cpu_hp_vote_is_valid_locked(voter)) {
		rtos_unlock_mutex(&s_vote_lock);
		return BK_ERR_PARAM;
	}

	/* unregister == vote offline, then free the slot */
	ret = cpu_hp_vote_apply_locked(voter, 0);
	cpu_hp_vote_free_locked(voter);

	rtos_unlock_mutex(&s_vote_lock);

	return ret;
}

#if CONFIG_CPU_HP_VOTE_FIND
cpu_hp_voter_handle_t bk_cpu_hp_vote_find(const char *name)
{
	cpu_hp_voter_handle_t voter = NULL;

	if ((s_vote_lock == NULL) || (name == NULL)) {
		return NULL;
	}

	rtos_lock_mutex(&s_vote_lock);
	for (cpu_hp_voter_handle_t v = s_voter_list; v != NULL; v = v->next) {
		if (os_strcmp(v->name, name) == 0) {
			voter = v;
			break;
		}
	}
	rtos_unlock_mutex(&s_vote_lock);

	return voter;
}
#endif /* CONFIG_CPU_HP_VOTE_FIND */

bk_err_t bk_cpu_hp_vote_online(cpu_hp_voter_handle_t voter)
{
	bk_err_t ret;

	if (voter == NULL)
		return BK_ERR_PARAM;

	if (s_vote_lock == NULL)
		return BK_ERR_NOT_INIT;

	rtos_lock_mutex(&s_vote_lock);

	if (!cpu_hp_vote_is_valid_locked(voter)) {
		rtos_unlock_mutex(&s_vote_lock);
		return BK_ERR_PARAM;
	}

	ret = cpu_hp_vote_apply_locked(voter, 1);

	rtos_unlock_mutex(&s_vote_lock);

	return ret;
}

bk_err_t bk_cpu_hp_vote_offline(cpu_hp_voter_handle_t voter)
{
	bk_err_t ret;

	if (voter == NULL)
		return BK_ERR_PARAM;

	if (s_vote_lock == NULL)
		return BK_ERR_NOT_INIT;

	rtos_lock_mutex(&s_vote_lock);

	if (!cpu_hp_vote_is_valid_locked(voter)) {
		rtos_unlock_mutex(&s_vote_lock);
		return BK_ERR_PARAM;
	}

	ret = cpu_hp_vote_apply_locked(voter, 0);

	rtos_unlock_mutex(&s_vote_lock);

	return ret;
}

bk_err_t bk_cpu_hp_vote_get_wanted(cpu_hp_voter_handle_t voter, uint32_t *wanted)
{
	bk_err_t ret = BK_ERR_PARAM;

	if ((voter == NULL) || (wanted == NULL))
		return BK_ERR_PARAM;

	if (s_vote_lock == NULL)
		return BK_ERR_NOT_INIT;

	rtos_lock_mutex(&s_vote_lock);
	if (cpu_hp_vote_is_valid_locked(voter)) {
		*wanted = cpu_hp_vote_bit_test(s_online_mask, voter->bit);
		ret = BK_OK;
	}
	rtos_unlock_mutex(&s_vote_lock);

	return ret;
}

uint32_t bk_cpu_hp_vote_get_online_count(void)
{
	return s_online_count;
}

uint32_t bk_cpu_hp_vote_get_voter_count(void)
{
	return s_voter_count;
}

#if CONFIG_CPU_HP_VOTE_DUMP
void bk_cpu_hp_vote_dump(void)
{
	if (s_vote_lock == NULL) {
		MULTICORE_LOGI("cpu_hp_vote: not initialized\r\n");
		return;
	}

	rtos_lock_mutex(&s_vote_lock);

	MULTICORE_LOGI("cpu_hp_vote: target=cpu%u online=%s online_votes=%u voters=%u\r\n",
		CPU_HP_VOTE_TARGET_CPU,
		bk_cpu_hp_is_online(CPU_HP_VOTE_TARGET_CPU) ? "on" : "off",
		s_online_count, s_voter_count);
	for (cpu_hp_voter_handle_t v = s_voter_list; v != NULL; v = v->next) {
#if CONFIG_CPU_HP_VOTE_NAME
		MULTICORE_LOGI(" voter[%u] '%s' vote=%s\r\n",
			v->bit, v->name,
			cpu_hp_vote_bit_test(s_online_mask, v->bit) ? "on" : "off");
#else
		MULTICORE_LOGI(" voter[%u] vote=%s\r\n",
			v->bit,
			cpu_hp_vote_bit_test(s_online_mask, v->bit) ? "on" : "off");
#endif
	}

	rtos_unlock_mutex(&s_vote_lock);
}
#endif /* CONFIG_CPU_HP_VOTE_DUMP */

#endif /* CONFIG_CPU_HP_VOTE */

#endif /* CONFIG_CPU_HOTPLUG */
