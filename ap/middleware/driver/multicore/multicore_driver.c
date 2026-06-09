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

#if CONFIG_SOC_SMP
#define AP_HOTPLUG_TIMEOUT_MS        (100)
#define AP_HOTPLUG_CPU3_ROUTE_REGS   (3)
#define AP_HOTPLUG_PRIMARY_ROUTE_REGS (2)
#define AP_HOTPLUG_NVIC_WORDS        (4)
#define AP_HOTPLUG_SYSTICK_CTRL      (*(volatile uint32_t *)0xe000e010)
#define AP_HOTPLUG_SYSTICK_VAL       (*(volatile uint32_t *)0xe000e018)
#define AP_HOTPLUG_SCB_ICSR          (*(volatile uint32_t *)0xe000ed04)
#define AP_HOTPLUG_FPCCR             (*(volatile uint32_t *)0xe000ef34)
#define AP_HOTPLUG_NVIC_ICER_BASE    ((volatile uint32_t *)0xe000e180)
#define AP_HOTPLUG_NVIC_ICPR_BASE    ((volatile uint32_t *)0xe000e280)
#define AP_HOTPLUG_PENDSVCLR         (1UL << 27)
#define AP_HOTPLUG_PENDSTCLR         (1UL << 25)
#define AP_HOTPLUG_FPCCR_ASPEN       (1UL << 31)
#define AP_HOTPLUG_FPCCR_LSPEN       (1UL << 30)
#define BK_CPU_MASK(cpu)             BIT(cpu)

typedef enum {
	BK_SMP_DOMAIN_CP = 0,
	BK_SMP_DOMAIN_AP,
} bk_smp_domain_id_t;

typedef struct {
	bk_smp_domain_id_t id;
	uint32_t possible_mask;
	uint32_t primary_mask;
	uint32_t hotplug_mask;
	uint32_t online_mask;
	uint32_t active_mask;
	uint32_t dying_mask;
	uint32_t offline_mask;
	uint32_t primary_cpu;
	uint32_t tick_owner_cpu;
	bk_cpu_hotplug_state_t cpu_state[4];
} bk_smp_domain_t;

typedef struct {
	uint32_t valid;
	uint32_t primary_route[AP_HOTPLUG_PRIMARY_ROUTE_REGS];
	uint32_t target_route[AP_HOTPLUG_CPU3_ROUTE_REGS];
} ap_cpu3_irq_route_snapshot_t;

static beken_mutex_t s_ap_cpu_hotplug_lock;
static volatile uint32_t s_cpu3_offline_ack;
static volatile uint32_t s_cpu3_online_ack;
static ap_cpu3_irq_route_snapshot_t s_cpu3_irq_route;
static bk_smp_domain_t s_cp_domain = {
	.id = BK_SMP_DOMAIN_CP,
	.possible_mask = BK_CPU_MASK(CPU0_CORE_ID) | BK_CPU_MASK(CPU1_CORE_ID),
	.primary_mask = BK_CPU_MASK(CPU0_CORE_ID),
	.hotplug_mask = BK_CPU_MASK(CPU1_CORE_ID),
	.online_mask = BK_CPU_MASK(CPU0_CORE_ID) | BK_CPU_MASK(CPU1_CORE_ID),
	.active_mask = BK_CPU_MASK(CPU0_CORE_ID) | BK_CPU_MASK(CPU1_CORE_ID),
	.offline_mask = 0,
	.primary_cpu = CPU0_CORE_ID,
	.tick_owner_cpu = CPU0_CORE_ID,
	.cpu_state = {
		[CPU0_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
		[CPU1_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
	},
};
static bk_smp_domain_t s_ap_domain = {
	.id = BK_SMP_DOMAIN_AP,
	.possible_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.primary_mask = BK_CPU_MASK(CPU2_CORE_ID),
	.hotplug_mask = BK_CPU_MASK(CPU3_CORE_ID),
	.online_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.active_mask = BK_CPU_MASK(CPU2_CORE_ID) | BK_CPU_MASK(CPU3_CORE_ID),
	.offline_mask = 0,
	.primary_cpu = CPU2_CORE_ID,
	.tick_owner_cpu = CPU2_CORE_ID,
	.cpu_state = {
		[CPU2_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
		[CPU3_CORE_ID] = BK_CPU_HP_STATE_ONLINE,
	},
};

extern bk_err_t crosscore_int_send_hotplug_stop(int xCoreID);
extern void vPortHotplugResetCoreState(BaseType_t xCoreID);

static void ap_cpu_hotplug_barrier(void)
{
	__asm volatile("dsb sy" ::: "memory");
	__asm volatile("isb sy" ::: "memory");
}

static void ap_cpu_hotplug_disable_local_irq(void)
{
	__asm volatile("cpsid i" ::: "memory");
}

static void ap_cpu_hotplug_wfi(void)
{
	__asm volatile("wfi" ::: "memory");
}

static bk_err_t ap_cpu_hotplug_lock_init(void)
{
	if (s_ap_cpu_hotplug_lock == NULL) {
		return rtos_init_mutex(&s_ap_cpu_hotplug_lock);
	}

	return BK_OK;
}

static bk_smp_domain_t *bk_cpu_hotplug_domain(uint32_t cpu_id)
{
	if (cpu_id <= CPU1_CORE_ID) {
		return &s_cp_domain;
	}

	if ((cpu_id == CPU2_CORE_ID) || (cpu_id == CPU3_CORE_ID)) {
		return &s_ap_domain;
	}

	return NULL;
}

static uint32_t bk_cpu_hotplug_smp_core(const bk_smp_domain_t *domain, uint32_t cpu_id)
{
	return cpu_id - domain->primary_cpu;
}

static void bk_cpu_hotplug_set_state(bk_smp_domain_t *domain, uint32_t cpu_id,
	bk_cpu_hotplug_state_t state)
{
	domain->cpu_state[cpu_id] = state;
}

static void bk_cpu_hotplug_set_active(bk_smp_domain_t *domain, uint32_t cpu_id,
	uint32_t active)
{
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core = (BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id);

	if (active) {
		domain->active_mask |= cpu_mask;
		vSetCoreActive(smp_core, pdTRUE);
	} else {
		domain->active_mask &= ~cpu_mask;
		vSetCoreActive(smp_core, pdFALSE);
	}
}

static void bk_cpu_hotplug_set_online(bk_smp_domain_t *domain, uint32_t cpu_id,
	uint32_t online)
{
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core = (BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id);

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

static void bk_cpu_hotplug_set_dying(bk_smp_domain_t *domain, uint32_t cpu_id,
	uint32_t dying)
{
	if (dying) {
		domain->dying_mask |= BK_CPU_MASK(cpu_id);
	} else {
		domain->dying_mask &= ~BK_CPU_MASK(cpu_id);
	}
}

static bk_err_t ap_cpu3_irq_route_backup(void)
{
	if (s_cpu3_irq_route.valid == 0) {
		s_cpu3_irq_route.primary_route[0] = sys_ahbp_ll_get_reg10_value();
		s_cpu3_irq_route.primary_route[1] = sys_ahbp_ll_get_reg11_value();
		s_cpu3_irq_route.target_route[0] = sys_ahbp_ll_get_reg12_value();
		s_cpu3_irq_route.target_route[1] = sys_ahbp_ll_get_reg13_value();
		s_cpu3_irq_route.target_route[2] = sys_ahbp_ll_get_reg14_value();
		s_cpu3_irq_route.valid = 1;
	}

	if (s_cpu3_irq_route.target_route[2] != 0) {
		return BK_ERR_BUSY;
	}

	return BK_OK;
}

static bk_err_t ap_cpu3_irq_route_migrate_for_stop_ipi(void)
{
	uint32_t stop_route = BIT(INT_SRC_MAILBOX) | BIT(INT_SRC_IPI);
	bk_err_t ret = ap_cpu3_irq_route_backup();

	if (ret != BK_OK) {
		return ret;
	}

	sys_ahbp_ll_set_reg10_value(s_cpu3_irq_route.primary_route[0] |
		(s_cpu3_irq_route.target_route[0] & ~stop_route));
	sys_ahbp_ll_set_reg11_value(s_cpu3_irq_route.primary_route[1] |
		s_cpu3_irq_route.target_route[1]);
	sys_ahbp_ll_set_reg12_value(s_cpu3_irq_route.target_route[0] & stop_route);
	sys_ahbp_ll_set_reg13_value(0);
	sys_ahbp_ll_set_reg14_value(0);

	return BK_OK;
}

static void ap_cpu3_irq_route_mask_all(void)
{
	sys_ahbp_ll_set_reg12_value(0);
	sys_ahbp_ll_set_reg13_value(0);
	sys_ahbp_ll_set_reg14_value(0);
}

static void ap_cpu3_irq_route_restore(void)
{
	if (s_cpu3_irq_route.valid) {
		sys_ahbp_ll_set_reg10_value(s_cpu3_irq_route.primary_route[0]);
		sys_ahbp_ll_set_reg11_value(s_cpu3_irq_route.primary_route[1]);
		sys_ahbp_ll_set_reg12_value(s_cpu3_irq_route.target_route[0]);
		sys_ahbp_ll_set_reg13_value(s_cpu3_irq_route.target_route[1]);
		sys_ahbp_ll_set_reg14_value(s_cpu3_irq_route.target_route[2]);
		s_cpu3_irq_route.valid = 0;
	}
}

static bk_err_t ap_cpu_hotplug_wait_ack(volatile uint32_t *ack, uint32_t timeout_ms)
{
	uint32_t start = rtos_get_time();

	while (*ack == 0) {
		if ((rtos_get_time() - start) >= timeout_ms) {
			return BK_ERR_TIMEOUT;
		}
		rtos_delay_milliseconds(1);
	}

	return BK_OK;
}
#endif

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

#if CONFIG_SOC_SMP
void bk_ap_cpu_hotplug_core_stop_isr(void)
{
	bk_smp_domain_t *domain = &s_ap_domain;

	if (portGET_CORE_ID() != SMP_CORE1_ID) {
		return;
	}

	bk_cpu_hotplug_set_state(domain, CPU3_CORE_ID, BK_CPU_HP_STATE_QUIESCE);
	vSetCoreActive(SMP_CORE1_ID, pdFALSE);
	mbox0_drv_core_int_enable(CPU3_CORE_ID, 0);
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
	s_cpu3_offline_ack = 1;
	ap_cpu_hotplug_barrier();
	ap_cpu_hotplug_disable_local_irq();

	while (1) {
		ap_cpu_hotplug_wfi();
	}
}

void bk_ap_cpu_hotplug_core_online(void)
{
	if ((portGET_CORE_ID() == SMP_CORE1_ID) &&
		(s_ap_domain.cpu_state[CPU3_CORE_ID] == BK_CPU_HP_STATE_SECONDARY_BOOT)) {
		bk_cpu_hotplug_set_state(&s_ap_domain, CPU3_CORE_ID, BK_CPU_HP_STATE_JOIN_SCHEDULER);
		s_cpu3_online_ack = 1;
		ap_cpu_hotplug_barrier();
	}
}

bk_err_t bk_cpu_offline(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	BaseType_t smp_core;
	bk_err_t ret;

	if ((domain == NULL) || ((domain->hotplug_mask & cpu_mask) == 0)) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (cpu_id != CPU3_CORE_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		return BK_ERR_STATE;
	}

	smp_core = (BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id);
	ret = ap_cpu_hotplug_lock_init();
	if (ret != BK_OK) {
		return ret;
	}

	rtos_lock_mutex(&s_ap_cpu_hotplug_lock);

	if ((domain->online_mask & cpu_mask) == 0) {
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return BK_OK;
	}

	if (domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_ONLINE) {
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	if (xTaskHasTasksPinnedToCore(smp_core) == pdTRUE) {
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE_REQUESTED);
	bk_cpu_hotplug_set_dying(domain, cpu_id, 1);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_SCHEDULER_DRAINING);
	bk_cpu_hotplug_set_active(domain, cpu_id, 0);
	s_cpu3_offline_ack = 0;
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_IRQ_MIGRATING);
	ret = ap_cpu3_irq_route_migrate_for_stop_ipi();
	if (ret != BK_OK) {
		ap_cpu3_irq_route_restore();
		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return ret;
	}

	ret = crosscore_int_send_hotplug_stop(CPU3_CORE_ID);
	if (ret == BK_OK) {
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_QUIESCE);
		ret = ap_cpu_hotplug_wait_ack(&s_cpu3_offline_ack, AP_HOTPLUG_TIMEOUT_MS);
	}

	if (ret == BK_OK) {
#if CONFIG_TASK_WDT
		bk_task_wdt_set_feed_bits(smp_core, false);
#endif
		bk_cpu_hotplug_set_online(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_HOLD);
		ap_cpu3_irq_route_mask_all();
		ret = bk_multicore_stop(CPU3_CORE_ID);
		if (ret == BK_OK) {
			bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_OFF);
			bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
			bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
		}
	} else {
		ap_cpu3_irq_route_restore();
		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
	}

	rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
	return ret;
}

bk_err_t bk_cpu_online(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	bk_err_t ret;
	BaseType_t smp_core;

	if ((domain == NULL) || ((domain->hotplug_mask & cpu_mask) == 0)) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (cpu_id != CPU3_CORE_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		return BK_ERR_STATE;
	}

	ret = ap_cpu_hotplug_lock_init();
	if (ret != BK_OK) {
		return ret;
	}

	smp_core = (BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id);
	rtos_lock_mutex(&s_ap_cpu_hotplug_lock);

	if (domain->cpu_state[cpu_id] == BK_CPU_HP_STATE_ONLINE) {
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return BK_OK;
	}

	if ((domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_OFFLINE) &&
		(domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_RESET_HOLD)) {
		rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_ON);
	s_cpu3_online_ack = 0;
	ap_cpu3_irq_route_mask_all();
	vTaskHotplugResetIdleTaskContext((BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id));
	vPortHotplugResetCoreState((BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id));
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_BOOT_PREPARE);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_RELEASE);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_SECONDARY_BOOT);

	ret = bk_multicore_start(CPU3_CORE_ID);
	if (ret == BK_OK) {
		ret = ap_cpu_hotplug_wait_ack(&s_cpu3_online_ack, AP_HOTPLUG_TIMEOUT_MS);
	}

	if (ret == BK_OK) {
#if CONFIG_TASK_WDT
		bk_task_wdt_set_feed_bits(smp_core, true);
#else
		(void)smp_core;
#endif
		bk_cpu_hotplug_set_online(domain, cpu_id, 1);
		ap_cpu3_irq_route_restore();
		mbox0_init_on_current_core(CPU3_CORE_ID);
		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
	} else {
		bk_multicore_stop(CPU3_CORE_ID);
		bk_cpu_hotplug_set_active(domain, cpu_id, 0);
		bk_cpu_hotplug_set_online(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
	}

	rtos_unlock_mutex(&s_ap_cpu_hotplug_lock);
	return ret;
}

uint32_t bk_cpu_hotplug_enter_primary(void)
{
	BaseType_t old_core_id = xTaskHotplugSetCurrentTaskCoreID(SMP_CORE0_ID);

	for (uint32_t i = 0; (i < AP_HOTPLUG_TIMEOUT_MS) &&
		(portGET_CORE_ID() != SMP_CORE0_ID); i++) {
		taskYIELD();
		rtos_delay_milliseconds(1);
	}

	return old_core_id;
}

void bk_cpu_hotplug_exit_primary(uint32_t old_core_id)
{
	(void)xTaskHotplugSetCurrentTaskCoreID(old_core_id);
}

uint32_t bk_cpu_is_online(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	if (domain == NULL) {
		return 0;
	}

	return ((domain->online_mask & BK_CPU_MASK(cpu_id)) != 0) ? 1 : 0;
}

uint32_t bk_cpu_is_active(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	if (domain == NULL) {
		return 0;
	}

	return ((domain->active_mask & BK_CPU_MASK(cpu_id)) != 0) ? 1 : 0;
}

bk_cpu_hotplug_state_t bk_cpu_get_state(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	if (domain == NULL) {
		return BK_CPU_HP_STATE_OFFLINE;
	}

	return domain->cpu_state[cpu_id];
}

const char *bk_cpu_get_state_name(uint32_t cpu_id)
{
	static const char * const state_names[] = {
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
	bk_cpu_hotplug_state_t state = bk_cpu_get_state(cpu_id);

	if (state >= (sizeof(state_names) / sizeof(state_names[0])) || state_names[state] == NULL) {
		return "unknown";
	}

	return state_names[state];
}

uint32_t bk_cpu_get_domain_possible_mask(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->possible_mask;
}

uint32_t bk_cpu_get_domain_online_mask(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->online_mask;
}

uint32_t bk_cpu_get_domain_active_mask(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->active_mask;
}

uint32_t bk_cpu_get_domain_dying_mask(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->dying_mask;
}

uint32_t bk_cpu_get_domain_offline_mask(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);

	return (domain == NULL) ? 0 : domain->offline_mask;
}

bk_err_t bk_ap_cpu_offline(uint32_t smp_core_id)
{
	if (smp_core_id != SMP_CORE1_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	return bk_cpu_offline(CPU3_CORE_ID);
}

bk_err_t bk_ap_cpu_online(uint32_t smp_core_id)
{
	if (smp_core_id != SMP_CORE1_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	return bk_cpu_online(CPU3_CORE_ID);
}

uint32_t bk_ap_cpu_is_online(uint32_t smp_core_id)
{
	if (smp_core_id == SMP_CORE0_ID) {
		return bk_cpu_is_online(CPU2_CORE_ID);
	}

	if (smp_core_id != SMP_CORE1_ID) {
		return 0;
	}

	return bk_cpu_is_online(CPU3_CORE_ID);
}

void multicore_stop_core1(void)
{
	(void)bk_ap_cpu_offline(SMP_CORE1_ID);
}
#endif
