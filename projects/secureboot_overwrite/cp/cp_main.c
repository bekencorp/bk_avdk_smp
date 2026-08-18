// Copyright 2025 Beken
//
// secureboot_overwrite - CP NS application entry.
//
// Boot chain (secure):
//   BootROM -> CP BL2 (MCUboot) -> CP TF-M Secure (SPE) -> [this NS app]

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{
	/* Start the AP through the common PM vote + multicore path. TF-M performs
	 * only the Secure-privilege AP prepare via the psa_ap_secure_prepare NSC
	 * inside multicore_hal_start(). */
	if (bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,
			PM_POWER_MODULE_STATE_ON) != BK_OK) {
		BK_LOGE(NULL, "AP boot vote failed\r\n");
	}
	BK_LOGI(NULL, "secureboot_overwrite: CP NS world reached (secure boot OK)\r\n");
}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

	return 0;
}
