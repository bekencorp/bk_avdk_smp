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
#include "sys_driver.h"
#include "sys_reg.h"
#include <driver/int.h>

#if CONFIG_SOC_SMP
#define CP_HOTPLUG_TIMEOUT_MS         (100)
#define CP_HOTPLUG_IRQ_WORDS          (3)
#define CP_HOTPLUG_NVIC_WORDS         (4)
#define CP_HOTPLUG_SYSTICK_CTRL       (*(volatile uint32_t *)0xe000e010)
#define CP_HOTPLUG_SYSTICK_VAL        (*(volatile uint32_t *)0xe000e018)
#define CP_HOTPLUG_SCB_ICSR           (*(volatile uint32_t *)0xe000ed04)
#define CP_HOTPLUG_FPCCR              (*(volatile uint32_t *)0xe000ef34)
#define CP_HOTPLUG_NVIC_ICER_BASE     ((volatile uint32_t *)0xe000e180)
#define CP_HOTPLUG_NVIC_ICPR_BASE     ((volatile uint32_t *)0xe000e280)
#define CP_HOTPLUG_PENDSVCLR          (1UL << 27)
#define CP_HOTPLUG_PENDSTCLR          (1UL << 25)
#define CP_HOTPLUG_FPCCR_ASPEN        (1UL << 31)
#define CP_HOTPLUG_FPCCR_LSPEN        (1UL << 30)
#define BK_CPU_MASK(cpu)              BIT(cpu)

typedef enum {
	BK_SMP_DOMAIN_CP = 0,
} bk_smp_domain_id_t;

typedef struct {
	bk_smp_domain_id_t id;
	uint32_t possible_mask;			/* CPUs physically/logically possible in this domain. */
	uint32_t primary_mask;			/* Primary CPU mask, normally the CPU controlling hotplug. */
	uint32_t hotplug_mask;			/* CPUs allowed to be online/offline hotplugged. */
	uint32_t online_mask;			/* CPUs currently online from the scheduler/domain view. */
	uint32_t active_mask;			/* CPUs currently active and allowed to run tasks. */
	uint32_t dying_mask;			/* CPUs in the middle of offline teardown. */
	uint32_t offline_mask;			/* CPUs currently offline. */
	uint32_t primary_cpu;					/* CPU that owns domain-level hotplug control. */
	uint32_t tick_owner_cpu;				/* CPU responsible for system tick ownership. */
	bk_cpu_hotplug_state_t cpu_state[4];	/* Per-CPU hotplug state. */
} bk_smp_domain_t;

typedef struct {
	uint32_t valid;
	uint32_t primary_en[CP_HOTPLUG_IRQ_WORDS];
	uint32_t target_en[CP_HOTPLUG_IRQ_WORDS];
} cp_cpu1_irq_route_snapshot_t;

static beken_mutex_t s_cp_cpu_hotplug_lock;
static volatile uint32_t s_cpu1_offline_ack;
static volatile uint32_t s_cpu1_online_ack;
static cp_cpu1_irq_route_snapshot_t s_cpu1_irq_route;
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

extern void vPortHotplugResetCoreState(BaseType_t xCoreID);

static void cp_cpu_hotplug_barrier(void)
{
	__asm volatile("dsb sy" ::: "memory");
	__asm volatile("isb sy" ::: "memory");
}

static void cp_cpu_hotplug_disable_local_irq(void)
{
	__asm volatile("cpsid i" ::: "memory");
}

static void cp_cpu_hotplug_wfi(void)
{
	__asm volatile("wfi" ::: "memory");
}

static bk_err_t cp_cpu_hotplug_lock_init(void)
{
	if (s_cp_cpu_hotplug_lock == NULL) {
		return rtos_init_mutex(&s_cp_cpu_hotplug_lock);
	}

	return BK_OK;
}

static bk_smp_domain_t *bk_cpu_hotplug_domain(uint32_t cpu_id)
{
	if ((cpu_id == CPU0_CORE_ID) || (cpu_id == CPU1_CORE_ID)) {
		return &s_cp_domain;
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

static uint32_t cp_cpu_irq_en_addr(uint32_t cpu_id, uint32_t word)
{
	uint32_t base = (cpu_id == CPU0_CORE_ID) ? SYS_CPU0_INT_0_31_EN_ADDR :
		SYS_CPU1_INT_0_31_EN_ADDR;

	return base + (word << 2);
}

static uint32_t cp_cpu_irq_get_en(uint32_t cpu_id, uint32_t word)
{
	return REG_READ(cp_cpu_irq_en_addr(cpu_id, word));
}

static void cp_cpu_irq_set_en_word(uint32_t cpu_id, uint32_t word, uint32_t value)
{
	REG_WRITE(cp_cpu_irq_en_addr(cpu_id, word), value);
}

static uint32_t cp_irq_source_enabled(const uint32_t en[CP_HOTPLUG_IRQ_WORDS],
	uint32_t src)
{
	uint32_t word = src / 32;
	uint32_t bit = src % 32;

	if (word >= CP_HOTPLUG_IRQ_WORDS) {
		return 0;
	}

	return ((en[word] & BIT(bit)) != 0) ? 1 : 0;
}

static bk_err_t cp_cpu1_irq_route_backup(void)
{
	if (s_cpu1_irq_route.valid == 0) {
		for (uint32_t word = 0; word < CP_HOTPLUG_IRQ_WORDS; word++) {
			s_cpu1_irq_route.primary_en[word] = cp_cpu_irq_get_en(CPU0_CORE_ID, word);
			s_cpu1_irq_route.target_en[word] = cp_cpu_irq_get_en(CPU1_CORE_ID, word);
		}
		s_cpu1_irq_route.valid = 1;
	}

	return BK_OK;
}

static bk_err_t cp_cpu1_irq_route_migrate_for_stop_ipi(void)
{
	bk_err_t ret = cp_cpu1_irq_route_backup();

	if (ret != BK_OK) {
		return ret;
	}

	for (uint32_t src = 0; src < INT_SRC_NONE; src++) {
		if (src == INT_SRC_MAILBOX || src == INT_SRC_IPI) {
			continue;
		}
		if (cp_irq_source_enabled(s_cpu1_irq_route.target_en, src)) {
			sys_drv_set_int_en(CPU0_CORE_ID, src, 1);
			sys_drv_set_int_en(CPU1_CORE_ID, src, 0);
		}
	}

	return BK_OK;
}

static void cp_cpu1_irq_route_mask_all(void)
{
	for (uint32_t word = 0; word < CP_HOTPLUG_IRQ_WORDS; word++) {
		cp_cpu_irq_set_en_word(CPU1_CORE_ID, word, 0);
	}
}

static void cp_cpu1_irq_route_restore(void)
{
	if (s_cpu1_irq_route.valid) {
		for (uint32_t src = 0; src < INT_SRC_NONE; src++) {
			if (cp_irq_source_enabled(s_cpu1_irq_route.target_en, src) &&
				!cp_irq_source_enabled(s_cpu1_irq_route.primary_en, src)) {
				sys_drv_set_int_en(CPU0_CORE_ID, src, 0);
			}
		}

		for (uint32_t word = 0; word < CP_HOTPLUG_IRQ_WORDS; word++) {
			cp_cpu_irq_set_en_word(CPU1_CORE_ID, word, s_cpu1_irq_route.target_en[word]);
		}
		s_cpu1_irq_route.valid = 0;
	}
}

static bk_err_t cp_cpu_hotplug_wait_ack(volatile uint32_t *ack, uint32_t timeout_ms)
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
void bk_cp_cpu_hotplug_core_stop_isr(void)
{
	bk_smp_domain_t *domain = &s_cp_domain;

	if (portGET_CORE_ID() != SMP_CORE1_ID) {
		return;
	}

	bk_cpu_hotplug_set_state(domain, CPU1_CORE_ID, BK_CPU_HP_STATE_QUIESCE);
	mbox0_drv_core_int_enable(CPU1_CORE_ID, 0);
	CP_HOTPLUG_SYSTICK_CTRL = 0;
	CP_HOTPLUG_SYSTICK_VAL = 0;
	CP_HOTPLUG_SCB_ICSR = CP_HOTPLUG_PENDSVCLR | CP_HOTPLUG_PENDSTCLR;
	CP_HOTPLUG_FPCCR &= ~(CP_HOTPLUG_FPCCR_ASPEN | CP_HOTPLUG_FPCCR_LSPEN);

	for (uint32_t i = 0; i < CP_HOTPLUG_NVIC_WORDS; i++) {
		CP_HOTPLUG_NVIC_ICER_BASE[i] = 0xffffffff;
		CP_HOTPLUG_NVIC_ICPR_BASE[i] = 0xffffffff;
	}

#if CONFIG_DCACHE
	flush_all_dcache();
#endif
	vTaskHotplugClearCurrentTCB(SMP_CORE1_ID);
	s_cpu1_offline_ack = 1;
	cp_cpu_hotplug_barrier();
	cp_cpu_hotplug_disable_local_irq();

	while (1) {
		cp_cpu_hotplug_wfi();
	}
}

void bk_cp_cpu_hotplug_core_online(void)
{
	if ((portGET_CORE_ID() == SMP_CORE1_ID) &&
		(s_cp_domain.cpu_state[CPU1_CORE_ID] == BK_CPU_HP_STATE_SECONDARY_BOOT)) {
		bk_cpu_hotplug_set_state(&s_cp_domain, CPU1_CORE_ID, BK_CPU_HP_STATE_JOIN_SCHEDULER);
		s_cpu1_online_ack = 1;
		cp_cpu_hotplug_barrier();
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

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		return BK_ERR_STATE;
	}

	smp_core = (BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id);
	ret = cp_cpu_hotplug_lock_init();
	if (ret != BK_OK) {
		return ret;
	}

	rtos_lock_mutex(&s_cp_cpu_hotplug_lock);

	if ((domain->online_mask & cpu_mask) == 0) {
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return BK_OK;
	}

	if (domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_ONLINE) {
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	if (xTaskHasTasksPinnedToCore(smp_core) == pdTRUE) {
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE_REQUESTED);
	bk_cpu_hotplug_set_dying(domain, cpu_id, 1);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_SCHEDULER_DRAINING);
	bk_cpu_hotplug_set_active(domain, cpu_id, 0);
	s_cpu1_offline_ack = 0;
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_IRQ_MIGRATING);
	ret = cp_cpu1_irq_route_migrate_for_stop_ipi();
	if (ret != BK_OK) {
		cp_cpu1_irq_route_restore();
		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return ret;
	}

	ret = crosscore_int_send_hotplug_stop(CPU1_CORE_ID);
	if (ret == BK_OK) {
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_QUIESCE);
		ret = cp_cpu_hotplug_wait_ack(&s_cpu1_offline_ack, CP_HOTPLUG_TIMEOUT_MS);
	}

	if (ret == BK_OK) {
		bk_cpu_hotplug_set_online(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_HOLD);
		cp_cpu1_irq_route_mask_all();
		ret = bk_multicore_stop(CPU1_CORE_ID);
		if (ret == BK_OK) {
			bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_OFF);
			bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
			bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
		} else {
			/* CPU1 has already acked quiesce and is parked in WFI with mailbox disabled */
			BK_ASSERT(ret == BK_OK);
		}
	} else {
		cp_cpu1_irq_route_restore();
		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_dying(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
	}

	rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
	return ret;
}

bk_err_t bk_cpu_online(uint32_t cpu_id)
{
	bk_smp_domain_t *domain = bk_cpu_hotplug_domain(cpu_id);
	uint32_t cpu_mask = BK_CPU_MASK(cpu_id);
	bk_err_t ret;

	if ((domain == NULL) || ((domain->hotplug_mask & cpu_mask) == 0)) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (cpu_id != CPU1_CORE_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	if (bk_multicore_get_cpu_id() != domain->primary_cpu) {
		return BK_ERR_STATE;
	}

	ret = cp_cpu_hotplug_lock_init();
	if (ret != BK_OK) {
		return ret;
	}

	rtos_lock_mutex(&s_cp_cpu_hotplug_lock);

	if (domain->cpu_state[cpu_id] == BK_CPU_HP_STATE_ONLINE) {
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return BK_OK;
	}

	if ((domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_OFFLINE) &&
		(domain->cpu_state[cpu_id] != BK_CPU_HP_STATE_RESET_HOLD)) {
		rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
		return BK_ERR_BUSY;
	}

	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_POWER_ON);
	s_cpu1_online_ack = 0;
	cp_cpu1_irq_route_mask_all();
	vTaskHotplugResetIdleTaskContext((BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id));
	vPortHotplugResetCoreState((BaseType_t)bk_cpu_hotplug_smp_core(domain, cpu_id));
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_BOOT_PREPARE);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_RESET_RELEASE);
	bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_SECONDARY_BOOT);

	ret = bk_multicore_start(CPU1_CORE_ID);
	if (ret == BK_OK) {
		ret = cp_cpu_hotplug_wait_ack(&s_cpu1_online_ack, CP_HOTPLUG_TIMEOUT_MS);
	}

	if (ret == BK_OK) {
		bk_cpu_hotplug_set_online(domain, cpu_id, 1);

		mbox0_init_on_current_core(CPU1_CORE_ID);
		cp_cpu1_irq_route_restore();

		bk_cpu_hotplug_set_active(domain, cpu_id, 1);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_ONLINE);
	} else {
		bk_multicore_stop(CPU1_CORE_ID);
		bk_cpu_hotplug_set_active(domain, cpu_id, 0);
		bk_cpu_hotplug_set_online(domain, cpu_id, 0);
		bk_cpu_hotplug_set_state(domain, cpu_id, BK_CPU_HP_STATE_OFFLINE);
	}

	rtos_unlock_mutex(&s_cp_cpu_hotplug_lock);
	return ret;
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

bk_err_t bk_cp_cpu_offline(uint32_t smp_core_id)
{
	if (smp_core_id != SMP_CORE1_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	return bk_cpu_offline(CPU1_CORE_ID);
}

bk_err_t bk_cp_cpu_online(uint32_t smp_core_id)
{
	if (smp_core_id != SMP_CORE1_ID) {
		return BK_ERR_NOT_SUPPORT;
	}

	return bk_cpu_online(CPU1_CORE_ID);
}

uint32_t bk_cp_cpu_is_online(uint32_t smp_core_id)
{
	if (smp_core_id == SMP_CORE0_ID) {
		return bk_cpu_is_online(CPU0_CORE_ID);
	}

	if (smp_core_id != SMP_CORE1_ID) {
		return 0;
	}

	return bk_cpu_is_online(CPU1_CORE_ID);
}

void multicore_stop_core1(void)
{
	(void)bk_cp_cpu_offline(SMP_CORE1_ID);
}
#endif