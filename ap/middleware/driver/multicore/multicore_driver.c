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
#define AP_HOTPLUG_TIMEOUT_ONE_STEP     (1)
#define AP_HOTPLUG_TIMEOUT_ONE_STEP_US  (AP_HOTPLUG_TIMEOUT_ONE_STEP * 500) /* 1500us */
#define AP_HOTPLUG_TIMEOUT_ONE_STEP_MS  (AP_HOTPLUG_TIMEOUT_ONE_STEP)       /* 3ms */
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

static volatile uint32_t _cpu3_wants_offline = 0;
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

static inline void _cpu_hp_barrier(void)
{
	__asm volatile("dsb sy" ::: "memory");
	__asm volatile("isb sy" ::: "memory");
}

static inline void _cpu_hp_disable_local_irq(void)
{
	__asm volatile("cpsid i" ::: "memory");
}

static inline void _cpu_hp_enable_local_irq(void)
{
	__asm volatile("cpsie i" ::: "memory");
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
		current_step++;
	}

	return BK_OK;
}

///////////////////////////////////////////////////////////////////////////////

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

static bk_err_t _cpu_hp_offline_internal(uint32_t cpu_id)
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
	ret = _cpu_hp_lock_init();
	if (ret != BK_OK) {
		goto nolock_out;
	}

	rtos_lock_mutex(&_cpu_hp_lock);

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
	ret = crosscore_int_send_hotplug_stop(CPU3_CORE_ID);
	if (ret != BK_OK) {
		_cpu_hp_domain_set_dying(domain, cpu_id, 0);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		goto out;
	}
	ret = _cpu_hotplug_wait_ack(&_cpu3_offline_ack1, AP_HOTPLUG_TIMEOUT_STAPS);
	if (ret != BK_OK) {
		spin_lock_irqsave(&_cpu_hp_spin_lock, cpu_hp_irq_level);
		if (_cpu3_wants_offline == 1) {
			_cpu3_wants_offline = 0;
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
	rtos_unlock_mutex(&_cpu_hp_lock);
nolock_out:
	return ret;
}

static bk_err_t _cpu_hp_online_internal(uint32_t cpu_id)
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

	ret = _cpu_hp_lock_init();
	if (ret != BK_OK) {
		goto nolock_out;
	}

	smp_core = (BaseType_t)_cpu_hp_get_smp_core_id(domain, cpu_id);
	rtos_lock_mutex(&_cpu_hp_lock);

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
	} else {
		_cpu3_irq_route_backup();
		domain->cold_boot = 0;
	}
#endif
	vPortHotplugResetCoreState(smp_core);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_BOOT_PREPARE);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_RELEASE);
	_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_SECONDARY_BOOT);

	ret = bk_multicore_start(CPU3_CORE_ID);
	if (ret == BK_OK) {
		ret = _cpu_hotplug_wait_ack(&_cpu3_online_ack, AP_HOTPLUG_TIMEOUT_STAPS);
	}

	if (ret == BK_OK) {
#if CONFIG_TASK_WDT
		bk_task_wdt_set_feed_bits(smp_core, true);
#else
		(void)smp_core;
#endif
		_cpu_hp_domain_set_online(domain, cpu_id, 1);
		_cpu3_irq_route_restore();
		mbox0_init_on_current_core(CPU3_CORE_ID);
		_cpu_hp_domain_set_active(domain, cpu_id, 1);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
	} else {
		bk_multicore_stop(CPU3_CORE_ID);
		_cpu_hp_domain_set_active(domain, cpu_id, 0);
		_cpu_hp_domain_set_online(domain, cpu_id, 0);
		_cpu_hp_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
	}

out:
	rtos_unlock_mutex(&_cpu_hp_lock);
nolock_out:
	return ret;
}

uint32_t bk_cpu_hp_enter_primary(void)
{
	BaseType_t old_core_id = xTaskHotplugSetCurrentTaskCoreID(SMP_CORE0_ID);

	for (uint32_t i = 0; (i < AP_HOTPLUG_TIMEOUT_STAPS) &&
		(portGET_CORE_ID() != SMP_CORE0_ID); i++) {
		taskYIELD();
		rtos_delay_milliseconds(AP_HOTPLUG_TIMEOUT_ONE_STEP_MS);
	}

	return old_core_id;
}

void bk_cpu_hp_exit_primary(uint32_t old_core_id)
{
	(void)xTaskHotplugSetCurrentTaskCoreID(old_core_id);
}

bk_err_t bk_cpu_hp_offline(uint32_t cpu_id)
{
	bk_err_t ret = BK_FAIL;
	uint32_t old_core_id;
	uint32_t is_in_interrupt_context = platform_is_in_interrupt_context();

	if (is_in_interrupt_context == BK_FALSE)
		old_core_id = bk_cpu_hp_enter_primary();

	if (portGET_CORE_ID() == SMP_CORE0_ID)
		ret = _cpu_hp_offline_internal(cpu_id);
	else
		MULTICORE_LOGW("cpu%u offline must run on primary core, current SMP core=%d\r\n",
			cpu_id, portGET_CORE_ID());

	if (is_in_interrupt_context == BK_FALSE)
		bk_cpu_hp_exit_primary(old_core_id);

	return ret;
}

bk_err_t bk_cpu_hp_online(uint32_t cpu_id)
{
	bk_err_t ret = BK_FAIL;
	uint32_t old_core_id;
	uint32_t is_in_interrupt_context = platform_is_in_interrupt_context();

	if (is_in_interrupt_context == BK_FALSE)
		old_core_id = bk_cpu_hp_enter_primary();

	if (portGET_CORE_ID() == SMP_CORE0_ID)
		ret = _cpu_hp_online_internal(cpu_id);

	if (is_in_interrupt_context == BK_FALSE)
		bk_cpu_hp_exit_primary(old_core_id);

	return ret;
}

void multicore_stop_core1(void)
{
	(void)bk_cpu_hp_offline(CPU3_CORE_ID);
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

void bk_cpu_hp_idle_handler(void)
{
	cpu_hp_domain_t *domain = &_ap_domain;
	uint32_t cpu_id = CPU3_CORE_ID;

	if (portGET_CORE_ID() != SMP_CORE1_ID) {
		return;
	}

	_cpu_hp_disable_local_irq();
	spin_lock(&_cpu_hp_spin_lock);
	if (_cpu3_wants_offline == 0) {
		_cpu_hp_domain_set_active(domain, cpu_id, 1);
		spin_unlock(&_cpu_hp_spin_lock);
		_cpu_hp_enable_local_irq();
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

#if CONFIG_DCACHE
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

void bk_cpu_hp_core_online(void)
{
	if ((portGET_CORE_ID() == SMP_CORE1_ID) &&
	    (_ap_domain.cpu_state[CPU3_CORE_ID] == BK_CPU_HP_STATE_SECONDARY_BOOT)) {
		_cpu_hp_set_state(&_ap_domain, CPU3_CORE_ID, BK_CPU_HP_STATE_JOIN_SCHEDULER);
		_cpu3_online_ack = 1;
		_cpu_hp_barrier();
	}
	_cpu3_wants_offline = 0;
}

#endif /* CONFIG_CPU_HOTPLUG */
