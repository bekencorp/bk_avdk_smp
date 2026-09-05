// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <driver/mailbox_channel.h>
#include <sys_sw_regs.h>
#include <os/mem.h>
#include <os/os.h>
#include "cache.h"
#include "pm_debug.h"
#include "bk_pm_internal_api.h"
#if CONFIG_SOC_SMP
#include "spinlock.h"
#endif

#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms

#if CONFIG_MAILBOX
pm_mailbox_communication_state_e bk_pm_ap_ctrl_state_get(void);
bk_err_t bk_pm_ap_ctrl_state_set(pm_mailbox_communication_state_e state);
#endif

bk_err_t bk_pm_module_vote_boot_ap_ctrl(pm_boot_ap_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_MAILBOX
    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    bk_err_t ret            = 0;
    bk_pm_ap_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_CTRL_AP_STATE_CMD, module,power_state,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_ctrl_state_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_ctrl_state_get())
    {
        LOGE("ap vote ctrl ap time out\r\n");
    }
#endif//CONFIG_MAILBOX

    return BK_OK;
}

bool bk_pm_ap_first_boot_get(void)
{
	pm_shared_info_t shared_info = {0};

	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	return (shared_info.pm_ap_work_state & PM_AP_WORK_STATE_FIRST_BOOT) != 0;
}

bk_err_t __attribute__((weak)) bk_pm_ap_boot_success_set(bool boot_success)
{
	pm_shared_info_t shared_info = {0};

	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	if (boot_success) {
		shared_info.pm_ap_work_state |= PM_AP_WORK_STATE_BOOT_SUCCESS;
	} else {
		shared_info.pm_ap_work_state &= (uint8_t)~PM_AP_WORK_STATE_BOOT_SUCCESS;
	}
	bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_WORK_STATE, BK_SYS_SW_REGS_LOCK_ENABLE);
	__DSB();
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
	return BK_OK;
}

typedef enum {
	PM_AP_POWER_STATE_RUNNING = 0,
	PM_AP_FAST_STATE_QUIESCING,
	PM_AP_FAST_STATE_PREPARED,
	PM_AP_FAST_STATE_RESTORING,
	PM_AP_FAST_STATE_APP_RESUMING,
	PM_AP_FAST_STATE_FAILED,
} pm_ap_power_state_t;

typedef enum {
	PM_AP_PREPARE_STATE_IDLE = 0,
	PM_AP_PREPARE_STATE_PREPARING,
	PM_AP_PREPARE_STATE_READY,
	PM_AP_PREPARE_STATE_ABORTING,
} pm_ap_prepare_state_t;

typedef struct pm_ap_power_node {
	const pm_ap_power_ops_t *owner;
	pm_ap_power_ops_t ops;
	bk_err_t prepare_result;
	bool prepare_result_valid;
	bool power_prepare_called;
	bool quiesced;
	bool backed_up;
	struct pm_ap_power_node *prev;
	struct pm_ap_power_node *next;
} pm_ap_power_node_t;

static pm_ap_power_node_t *s_power_ops_head;
static pm_ap_power_node_t *s_power_ops_tail;
static volatile pm_ap_power_state_t s_power_state = PM_AP_POWER_STATE_RUNNING;
static volatile pm_ap_prepare_state_t s_prepare_state =
	PM_AP_PREPARE_STATE_IDLE;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static volatile bool s_fast_ipc_rx_blocked;
static volatile bool s_fast_app_resume_pending;
#endif
#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_power_ops_lock = SPIN_LOCK_INIT;
#endif

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
void bk_pm_ap_fast_ipc_rx_block_set(bool blocked)
{
	s_fast_ipc_rx_blocked = blocked;
	__DMB();
}

/*
 * Strong override of the mailbox driver's weak receive gate. CPU3/internal
 * traffic is left untouched; only CP->AP business channels are rejected.
 * PWC must stay open so suspend retries and ABORT can always reach the PM task.
 */
bool mb_chnl_read_is_allowed(u8 log_chnl)
{
	if (GET_SRC_CPU_ID(log_chnl) != MAILBOX_CPU0) {
		return true;
	}

	if (log_chnl == MB_CHNL_PWC) {
		return true;
	}

	__DMB();
	return !s_fast_ipc_rx_blocked;
}
#endif

static uint32_t pm_ap_power_lock(void)
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&s_power_ops_lock);
#endif
	return flags;
}

static void pm_ap_power_unlock(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&s_power_ops_lock);
#endif
	rtos_enable_int(flags);
}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
bk_err_t bk_pm_ap_full_ready_set(bool ready)
{
	pm_shared_info_t shared_info = {0};

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	__DSB();
	arch_dcache_invd_range((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
#endif
	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	if (ready) {
		shared_info.pm_ap_work_state |= PM_AP_WORK_STATE_FULL_READY;
	} else {
		shared_info.pm_ap_work_state &=
			(uint8_t)~PM_AP_WORK_STATE_FULL_READY;
	}
	bk_sys_sw_regs_update_pm_shared_info(&shared_info,
		BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_WORK_STATE,
		BK_SYS_SW_REGS_LOCK_ENABLE);
	__DSB();
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	arch_dcache_flush_range((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
#else
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
#endif
	__DSB();
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_SLAVE_HEART_BEAT_USE_IPI
	if (ready) {
		extern bk_err_t mb_ipc_heartbeat_full_ready_notify(void);
		(void)mb_ipc_heartbeat_full_ready_notify();
	}
#endif
	return BK_OK;
}

bool bk_pm_ap_full_ready_get(void)
{
	pm_shared_info_t shared_info = {0};

	__DSB();
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	arch_dcache_invd_range((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
#else
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
#endif
	__DSB();
	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	return (shared_info.pm_ap_work_state &
		PM_AP_WORK_STATE_FULL_READY) != 0U;
}
#endif

bk_err_t bk_pm_ap_power_ops_register(const pm_ap_power_ops_t *ops)
{
	pm_ap_power_node_t *node;
	pm_ap_power_node_t *curr;
	uint32_t flags;

	if ((ops == NULL) || (ops->name == NULL) ||
		((ops->prepare_power_off == NULL) &&
		 (ops->quiesce == NULL) && (ops->backup == NULL) &&
		 (ops->restore == NULL) && (ops->resume == NULL) &&
		 (ops->app_resume == NULL)) ||
		((ops->backup == NULL) != (ops->restore == NULL))) {
		return BK_ERR_PARAM;
	}

	node = os_malloc(sizeof(*node));
	if (node == NULL) {
		return BK_ERR_NO_MEM;
	}
	os_memset(node, 0, sizeof(*node));
	node->owner = ops;
	node->ops = *ops;

	flags = pm_ap_power_lock();
	if ((s_power_state != PM_AP_POWER_STATE_RUNNING) ||
		(s_prepare_state != PM_AP_PREPARE_STATE_IDLE)) {
		pm_ap_power_unlock(flags);
		os_free(node);
		return BK_ERR_BUSY;
	}

	for (curr = s_power_ops_head; curr != NULL; curr = curr->next) {
		if (curr->owner == ops) {
			pm_ap_power_unlock(flags);
			os_free(node);
			return BK_ERR_BUSY;
		}
		if (curr->ops.priority > ops->priority) {
			break;
		}
	}

	if (curr == NULL) {
		node->prev = s_power_ops_tail;
		if (s_power_ops_tail != NULL) {
			s_power_ops_tail->next = node;
		} else {
			s_power_ops_head = node;
		}
		s_power_ops_tail = node;
	} else {
		node->next = curr;
		node->prev = curr->prev;
		if (curr->prev != NULL) {
			curr->prev->next = node;
		} else {
			s_power_ops_head = node;
		}
		curr->prev = node;
	}
	pm_ap_power_unlock(flags);
	return BK_OK;
}

bk_err_t bk_pm_ap_power_ops_unregister(const pm_ap_power_ops_t *ops)
{
	pm_ap_power_node_t *curr;
	uint32_t flags;

	if (ops == NULL) {
		return BK_ERR_PARAM;
	}

	flags = pm_ap_power_lock();
	if ((s_power_state != PM_AP_POWER_STATE_RUNNING) ||
		(s_prepare_state != PM_AP_PREPARE_STATE_IDLE)) {
		pm_ap_power_unlock(flags);
		return BK_ERR_BUSY;
	}

	for (curr = s_power_ops_head; curr != NULL; curr = curr->next) {
		if (curr->owner == ops) {
			if (curr->prev != NULL) {
				curr->prev->next = curr->next;
			} else {
				s_power_ops_head = curr->next;
			}
			if (curr->next != NULL) {
				curr->next->prev = curr->prev;
			} else {
				s_power_ops_tail = curr->prev;
			}
			pm_ap_power_unlock(flags);
			os_free(curr);
			return BK_OK;
		}
	}
	pm_ap_power_unlock(flags);
	return BK_FAIL;
}

bk_err_t bk_pm_ap_power_prepare(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	bk_err_t cb_ret;
	uint32_t flags = pm_ap_power_lock();

	if (s_prepare_state == PM_AP_PREPARE_STATE_READY) {
		pm_ap_power_unlock(flags);
		return BK_OK;
	}
	if (s_prepare_state == PM_AP_PREPARE_STATE_IDLE) {
		s_prepare_state = PM_AP_PREPARE_STATE_PREPARING;
	} else if (s_prepare_state != PM_AP_PREPARE_STATE_PREPARING) {
		pm_ap_power_unlock(flags);
		return BK_ERR_STATE;
	}
	pm_ap_power_unlock(flags);

	for (node = s_power_ops_tail; node != NULL; node = node->prev) {
		if (node->ops.prepare_power_off != NULL) {
			node->power_prepare_called = true;
			cb_ret = node->ops.prepare_power_off(node->ops.arg);
			node->prepare_result = cb_ret;
			node->prepare_result_valid = true;
			if (cb_ret != BK_OK) {
				ret = BK_ERR_BUSY;
			}
		}
	}

	if (ret == BK_OK) {
		flags = pm_ap_power_lock();
		if (s_prepare_state == PM_AP_PREPARE_STATE_PREPARING) {
			s_prepare_state = PM_AP_PREPARE_STATE_READY;
		}
		pm_ap_power_unlock(flags);
	}
	return ret;
}

bool bk_pm_ap_power_prepare_is_ready(void)
{
	__DMB();
	return s_prepare_state == PM_AP_PREPARE_STATE_READY;
}

void bk_pm_ap_power_prepare_dump(void)
{
	pm_ap_power_node_t *node;
	pm_ap_prepare_state_t state;
	uint32_t flags;
	uint32_t index = 0;
	uint32_t busy_count = 0;
	uint32_t sampled_count = 0;

	flags = pm_ap_power_lock();
	state = s_prepare_state;
	pm_ap_power_unlock(flags);
	LOGI("AP power prepare state=%u\r\n", state);

	for (;;) {
		char name[32];
		uint8_t priority;
		bk_err_t result;
		bool has_callback;
		bool result_valid;
		uint32_t current = 0;
		uint32_t name_index = 0;

		flags = pm_ap_power_lock();
		node = s_power_ops_head;
		while ((node != NULL) && (current < index)) {
			node = node->next;
			current++;
		}
		if (node == NULL) {
			pm_ap_power_unlock(flags);
			break;
		}
		has_callback = node->ops.prepare_power_off != NULL;
		result = node->prepare_result;
		result_valid = node->prepare_result_valid;
		priority = node->ops.priority;
		while ((name_index < (sizeof(name) - 1)) &&
			(node->ops.name[name_index] != '\0')) {
			name[name_index] = node->ops.name[name_index];
			name_index++;
		}
		name[name_index] = '\0';
		pm_ap_power_unlock(flags);

		if (has_callback && result_valid) {
			sampled_count++;
			if (result != BK_OK) {
				LOGI("AP power prepare busy: name=%s priority=%u ret=%d\r\n",
					name, priority, result);
				busy_count++;
			}
		}
		index++;
	}
	LOGI("AP power prepare sampled=%u busy=%u\r\n",
		sampled_count, busy_count);
}

bk_err_t bk_pm_ap_power_prepare_abort(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	bk_err_t cb_ret;
	uint32_t flags = pm_ap_power_lock();

	/*
	 * Fast-boot rollback is completed by bk_pm_ap_fast_resume_modules() in
	 * PM task context. Never run resume callbacks early from the idle path.
	 */
	if (s_power_state != PM_AP_POWER_STATE_RUNNING) {
		pm_ap_power_unlock(flags);
		return BK_ERR_BUSY;
	}
	if ((s_prepare_state != PM_AP_PREPARE_STATE_PREPARING) &&
		(s_prepare_state != PM_AP_PREPARE_STATE_READY)) {
		pm_ap_power_unlock(flags);
		return BK_OK;
	}
	s_prepare_state = PM_AP_PREPARE_STATE_ABORTING;
	pm_ap_power_unlock(flags);

	for (node = s_power_ops_head; node != NULL; node = node->next) {
		if (node->power_prepare_called && (node->ops.resume != NULL)) {
			cb_ret = node->ops.resume(node->ops.arg);
			if ((ret == BK_OK) && (cb_ret != BK_OK)) {
				ret = cb_ret;
			}
		}
		node->power_prepare_called = false;
	}

	flags = pm_ap_power_lock();
	s_prepare_state = PM_AP_PREPARE_STATE_IDLE;
	pm_ap_power_unlock(flags);
	return ret;
}

bk_err_t bk_pm_ap_fast_ops_register(const pm_ap_fast_pm_ops_t *ops)
{
	return bk_pm_ap_power_ops_register(ops);
}

bk_err_t bk_pm_ap_fast_ops_unregister(const pm_ap_fast_pm_ops_t *ops)
{
	return bk_pm_ap_power_ops_unregister(ops);
}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
bk_err_t bk_pm_ap_fast_suspend_prepare(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t callback_count = 0;
	uint32_t flags = pm_ap_power_lock();

	if (s_power_state != PM_AP_POWER_STATE_RUNNING) {
		pm_ap_power_unlock(flags);
		return BK_ERR_STATE;
	}
	s_power_state = PM_AP_FAST_STATE_QUIESCING;
	s_fast_app_resume_pending = false;
	pm_ap_power_unlock(flags);
	bk_pm_ap_full_ready_set(false);

	for (node = s_power_ops_tail; node != NULL; node = node->prev) {
		callback_count++;
		LOGI("AP_FAST_CB quiesce begin: name=%s priority=%u cb=%p\r\n",
			node->ops.name, node->ops.priority, node->ops.quiesce);
		node->quiesced = true;
		if (node->ops.quiesce != NULL) {
			ret = node->ops.quiesce(node->ops.arg);
		}
		LOGI("AP_FAST_CB quiesce end: name=%s ret=%d\r\n",
			node->ops.name, ret);
		if (ret != BK_OK) {
			break;
		}
	}
	LOGI("AP_FAST_CB prepare summary: count=%u ret=%d\r\n",
		callback_count, ret);

	if (ret != BK_OK) {
		(void)bk_pm_ap_fast_resume_modules();
	}
	return ret;
}

bk_err_t bk_pm_ap_fast_suspend_backup(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t flags;

	if (s_power_state != PM_AP_FAST_STATE_QUIESCING) {
		return BK_ERR_STATE;
	}

	/*
	 * CPU3 must already be offline. Keep every hardware snapshot atomic
	 * against CPU2 ISRs; callbacks in this phase must never block.
	 */
	flags = rtos_disable_int();
	for (node = s_power_ops_tail; node != NULL; node = node->prev) {
		if (node->ops.backup != NULL) {
			/*
			 * Mark before entry so a callback that fails after partially
			 * touching hardware is included in the rollback restore pass.
			 */
			node->backed_up = true;
			ret = node->ops.backup(node->ops.arg);
			if (ret != BK_OK) {
				break;
			}
		}
	}

	if (ret != BK_OK) {
		for (node = s_power_ops_head; node != NULL; node = node->next) {
			if (node->backed_up && (node->ops.restore != NULL)) {
				(void)node->ops.restore(node->ops.arg);
			}
			node->backed_up = false;
		}
	} else {
		s_power_state = PM_AP_FAST_STATE_PREPARED;
		__DMB();
	}
	rtos_enable_int(flags);
	return ret;
}

bk_err_t bk_pm_ap_fast_restore_hardware(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t flags;

	if (s_power_state != PM_AP_FAST_STATE_PREPARED) {
		return BK_ERR_STATE;
	}

	s_power_state = PM_AP_FAST_STATE_RESTORING;
	__DMB();
	flags = rtos_disable_int();
	for (node = s_power_ops_head; node != NULL; node = node->next) {
		if (node->backed_up && (node->ops.restore != NULL)) {
			ret = node->ops.restore(node->ops.arg);
			if (ret != BK_OK) {
				break;
			}
		}
		node->backed_up = false;
	}
	rtos_enable_int(flags);

	if (ret != BK_OK) {
		s_power_state = PM_AP_FAST_STATE_FAILED;
		__DMB();
	}
	return ret;
}

bk_err_t bk_pm_ap_fast_resume_modules(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	bk_err_t cb_ret;

	if ((s_power_state != PM_AP_FAST_STATE_QUIESCING) &&
		(s_power_state != PM_AP_FAST_STATE_RESTORING)) {
		return BK_ERR_STATE;
	}

	for (node = s_power_ops_head; node != NULL; node = node->next) {
		if (node->quiesced && (node->ops.resume != NULL)) {
			cb_ret = node->ops.resume(node->ops.arg);
			if ((ret == BK_OK) && (cb_ret != BK_OK)) {
				ret = cb_ret;
			}
		}
		node->quiesced = false;
		node->power_prepare_called = false;
	}
	s_prepare_state = PM_AP_PREPARE_STATE_IDLE;
	__DMB();

	if (ret == BK_OK) {
		s_power_state = PM_AP_POWER_STATE_RUNNING;
		s_fast_app_resume_pending = true;
		__DMB();
		bk_pm_ap_fast_ipc_rx_block_set(false);
		bk_pm_ap_full_ready_set(true);
	} else {
		s_power_state = PM_AP_FAST_STATE_FAILED;
		__DMB();
	}
	return ret;
}

bk_err_t bk_pm_ap_fast_app_resume(void)
{
	pm_ap_power_node_t *node;
	bk_err_t ret = BK_OK;
	bk_err_t cb_ret;
	uint32_t flags = pm_ap_power_lock();

	if ((s_power_state != PM_AP_POWER_STATE_RUNNING) ||
		!s_fast_app_resume_pending) {
		pm_ap_power_unlock(flags);
		return BK_ERR_STATE;
	}
	s_power_state = PM_AP_FAST_STATE_APP_RESUMING;
	s_fast_app_resume_pending = false;
	pm_ap_power_unlock(flags);

	for (node = s_power_ops_head; node != NULL; node = node->next) {
		if (node->ops.app_resume != NULL) {
			cb_ret = node->ops.app_resume(node->ops.arg);
			if ((ret == BK_OK) && (cb_ret != BK_OK)) {
				ret = cb_ret;
			}
		}
	}

	flags = pm_ap_power_lock();
	s_power_state = PM_AP_POWER_STATE_RUNNING;
	pm_ap_power_unlock(flags);
	return ret;
}

bool bk_pm_ap_fast_suspend_is_prepared(void)
{
	__DMB();
	return s_power_state == PM_AP_FAST_STATE_PREPARED;
}
#endif