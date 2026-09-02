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
#if CONFIG_PM_AP_FAST_BOOT_ENABLE && CONFIG_SOC_SMP
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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
typedef enum {
	PM_AP_FAST_STATE_RUNNING = 0,
	PM_AP_FAST_STATE_QUIESCING,
	PM_AP_FAST_STATE_PREPARED,
	PM_AP_FAST_STATE_RESTORING,
	PM_AP_FAST_STATE_FAILED,
} pm_ap_fast_state_t;

typedef struct pm_ap_fast_node {
	const pm_ap_fast_pm_ops_t *owner;
	pm_ap_fast_pm_ops_t ops;
	bool quiesced;
	bool backed_up;
	struct pm_ap_fast_node *prev;
	struct pm_ap_fast_node *next;
} pm_ap_fast_node_t;

static pm_ap_fast_node_t *s_fast_ops_head;
static pm_ap_fast_node_t *s_fast_ops_tail;
static volatile pm_ap_fast_state_t s_fast_state = PM_AP_FAST_STATE_RUNNING;
static volatile bool s_fast_ipc_rx_blocked;
#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_fast_ops_lock = SPIN_LOCK_INIT;
#endif

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

static uint32_t pm_ap_fast_lock(void)
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&s_fast_ops_lock);
#endif
	return flags;
}

static void pm_ap_fast_unlock(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&s_fast_ops_lock);
#endif
	rtos_enable_int(flags);
}

bk_err_t bk_pm_ap_full_ready_set(bool ready)
{
	pm_shared_info_t shared_info = {0};

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
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
	return BK_OK;
}

bool bk_pm_ap_full_ready_get(void)
{
	pm_shared_info_t shared_info = {0};

	__DSB();
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info,
		sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	return (shared_info.pm_ap_work_state &
		PM_AP_WORK_STATE_FULL_READY) != 0U;
}

bk_err_t bk_pm_ap_fast_ops_register(const pm_ap_fast_pm_ops_t *ops)
{
	pm_ap_fast_node_t *node;
	pm_ap_fast_node_t *curr;
	uint32_t flags;

	if ((ops == NULL) || (ops->name == NULL) ||
		((ops->quiesce == NULL) && (ops->backup == NULL) &&
		 (ops->restore == NULL) && (ops->resume == NULL)) ||
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

	flags = pm_ap_fast_lock();
	if (s_fast_state != PM_AP_FAST_STATE_RUNNING) {
		pm_ap_fast_unlock(flags);
		os_free(node);
		return BK_ERR_BUSY;
	}

	for (curr = s_fast_ops_head; curr != NULL; curr = curr->next) {
		if (curr->owner == ops) {
			pm_ap_fast_unlock(flags);
			os_free(node);
			return BK_ERR_BUSY;
		}
		if (curr->ops.priority > ops->priority) {
			break;
		}
	}

	if (curr == NULL) {
		node->prev = s_fast_ops_tail;
		if (s_fast_ops_tail != NULL) {
			s_fast_ops_tail->next = node;
		} else {
			s_fast_ops_head = node;
		}
		s_fast_ops_tail = node;
	} else {
		node->next = curr;
		node->prev = curr->prev;
		if (curr->prev != NULL) {
			curr->prev->next = node;
		} else {
			s_fast_ops_head = node;
		}
		curr->prev = node;
	}
	pm_ap_fast_unlock(flags);
	return BK_OK;
}

bk_err_t bk_pm_ap_fast_ops_unregister(const pm_ap_fast_pm_ops_t *ops)
{
	pm_ap_fast_node_t *curr;
	uint32_t flags;

	if (ops == NULL) {
		return BK_ERR_PARAM;
	}

	flags = pm_ap_fast_lock();
	if (s_fast_state != PM_AP_FAST_STATE_RUNNING) {
		pm_ap_fast_unlock(flags);
		return BK_ERR_BUSY;
	}

	for (curr = s_fast_ops_head; curr != NULL; curr = curr->next) {
		if (curr->owner == ops) {
			if (curr->prev != NULL) {
				curr->prev->next = curr->next;
			} else {
				s_fast_ops_head = curr->next;
			}
			if (curr->next != NULL) {
				curr->next->prev = curr->prev;
			} else {
				s_fast_ops_tail = curr->prev;
			}
			pm_ap_fast_unlock(flags);
			os_free(curr);
			return BK_OK;
		}
	}
	pm_ap_fast_unlock(flags);
	return BK_FAIL;
}

bk_err_t bk_pm_ap_fast_suspend_prepare(void)
{
	pm_ap_fast_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t flags = pm_ap_fast_lock();

	if (s_fast_state != PM_AP_FAST_STATE_RUNNING) {
		pm_ap_fast_unlock(flags);
		return BK_ERR_STATE;
	}
	s_fast_state = PM_AP_FAST_STATE_QUIESCING;
	pm_ap_fast_unlock(flags);
	bk_pm_ap_full_ready_set(false);

	for (node = s_fast_ops_tail; node != NULL; node = node->prev) {
		node->quiesced = true;
		if (node->ops.quiesce != NULL) {
			ret = node->ops.quiesce(node->ops.arg);
			if (ret != BK_OK) {
				break;
			}
		}
	}

	if (ret != BK_OK) {
		(void)bk_pm_ap_fast_resume_modules();
	}
	return ret;
}

bk_err_t bk_pm_ap_fast_suspend_backup(void)
{
	pm_ap_fast_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t flags;

	if (s_fast_state != PM_AP_FAST_STATE_QUIESCING) {
		return BK_ERR_STATE;
	}

	/*
	 * CPU3 must already be offline. Keep every hardware snapshot atomic
	 * against CPU2 ISRs; callbacks in this phase must never block.
	 */
	flags = rtos_disable_int();
	for (node = s_fast_ops_tail; node != NULL; node = node->prev) {
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
		for (node = s_fast_ops_head; node != NULL; node = node->next) {
			if (node->backed_up && (node->ops.restore != NULL)) {
				(void)node->ops.restore(node->ops.arg);
			}
			node->backed_up = false;
		}
	} else {
		s_fast_state = PM_AP_FAST_STATE_PREPARED;
		__DMB();
	}
	rtos_enable_int(flags);
	return ret;
}

bk_err_t bk_pm_ap_fast_restore_hardware(void)
{
	pm_ap_fast_node_t *node;
	bk_err_t ret = BK_OK;
	uint32_t flags;

	if (s_fast_state != PM_AP_FAST_STATE_PREPARED) {
		return BK_ERR_STATE;
	}

	s_fast_state = PM_AP_FAST_STATE_RESTORING;
	__DMB();
	flags = rtos_disable_int();
	for (node = s_fast_ops_head; node != NULL; node = node->next) {
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
		s_fast_state = PM_AP_FAST_STATE_FAILED;
		__DMB();
	}
	return ret;
}

bk_err_t bk_pm_ap_fast_resume_modules(void)
{
	pm_ap_fast_node_t *node;
	bk_err_t ret = BK_OK;
	bk_err_t cb_ret;

	if ((s_fast_state != PM_AP_FAST_STATE_QUIESCING) &&
		(s_fast_state != PM_AP_FAST_STATE_RESTORING)) {
		return BK_ERR_STATE;
	}

	for (node = s_fast_ops_head; node != NULL; node = node->next) {
		if (node->quiesced && (node->ops.resume != NULL)) {
			cb_ret = node->ops.resume(node->ops.arg);
			if ((ret == BK_OK) && (cb_ret != BK_OK)) {
				ret = cb_ret;
			}
		}
		node->quiesced = false;
	}

	if (ret == BK_OK) {
		s_fast_state = PM_AP_FAST_STATE_RUNNING;
		__DMB();
		bk_pm_ap_fast_ipc_rx_block_set(false);
		bk_pm_ap_full_ready_set(true);
	} else {
		s_fast_state = PM_AP_FAST_STATE_FAILED;
		__DMB();
	}
	return ret;
}

bool bk_pm_ap_fast_suspend_is_prepared(void)
{
	__DMB();
	return s_fast_state == PM_AP_FAST_STATE_PREPARED;
}
#endif