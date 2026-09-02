#pragma once

#include <common/bk_include.h>

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
bk_err_t wdrv_ipc_quiesce(uint32_t timeout_ms);
void wdrv_ipc_resume(void);

bk_err_t wdrv_pm_init(void);
bk_err_t wdrv_pm_deinit(void);
#endif
