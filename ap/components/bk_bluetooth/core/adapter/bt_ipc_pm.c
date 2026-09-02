#include "bt_ipc_pm.h"

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#include <modules/pm.h>

#define BT_IPC_FAST_QUIESCE_TIMEOUT_MS (100U)

static bool s_bt_ipc_pm_registered;

static bk_err_t bt_ipc_pm_quiesce(void *arg)
{
	(void)arg;
	return bt_ipc_quiesce(BT_IPC_FAST_QUIESCE_TIMEOUT_MS);
}

static bk_err_t bt_ipc_pm_resume(void *arg)
{
	(void)arg;
	bt_ipc_resume();
	return BK_OK;
}

static const pm_ap_fast_pm_ops_t s_bt_ipc_pm_ops = {
	.name = "bt_ble_ipc",
	.quiesce = bt_ipc_pm_quiesce,
	.resume = bt_ipc_pm_resume,
	.priority = PM_AP_FAST_PRIORITY_SERVICE,
};

bk_err_t bt_ipc_pm_init(void)
{
	bk_err_t ret;

	if (s_bt_ipc_pm_registered) {
		return BK_OK;
	}

	ret = bk_pm_ap_fast_ops_register(&s_bt_ipc_pm_ops);
	if (ret == BK_OK) {
		s_bt_ipc_pm_registered = true;
	}
	return ret;
}
#endif
