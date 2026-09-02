#ifndef __BT_IPC_PM_H__
#define __BT_IPC_PM_H__

#include <common/bk_include.h>

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
bk_err_t bt_ipc_quiesce(uint32_t timeout_ms);
void bt_ipc_resume(void);

bk_err_t bt_ipc_pm_init(void);
#endif

#endif
