#include "wdrv_pm.h"

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#include <modules/pm.h>

#define WDRV_FAST_QUIESCE_TIMEOUT_MS (100U)

static bool s_wdrv_pm_registered;

static bk_err_t wdrv_pm_quiesce(void *arg)
{
	(void)arg;
	return wdrv_ipc_quiesce(WDRV_FAST_QUIESCE_TIMEOUT_MS);
}

static bk_err_t wdrv_pm_resume(void *arg)
{
	(void)arg;
	wdrv_ipc_resume();
	return BK_OK;
}

static const pm_ap_fast_pm_ops_t s_wdrv_pm_ops = {
	.name = "wifi_ipc",
	.quiesce = wdrv_pm_quiesce,
	.resume = wdrv_pm_resume,
	.priority = PM_AP_FAST_PRIORITY_SERVICE,
};

bk_err_t wdrv_pm_init(void)
{
	bk_err_t ret;

	if (s_wdrv_pm_registered) {
		return BK_OK;
	}

	ret = bk_pm_ap_fast_ops_register(&s_wdrv_pm_ops);
	if (ret == BK_OK) {
		s_wdrv_pm_registered = true;
	}
	return ret;
}

bk_err_t wdrv_pm_deinit(void)
{
	bk_err_t ret;

	if (!s_wdrv_pm_registered) {
		return BK_OK;
	}

	ret = bk_pm_ap_fast_ops_unregister(&s_wdrv_pm_ops);
	if (ret == BK_OK) {
		s_wdrv_pm_registered = false;
	}
	return ret;
}
#endif
