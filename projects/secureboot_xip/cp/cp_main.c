// Copyright 2025 Beken
//
// secureboot_xip - CP NS application entry.
//
// Boot chain (secure):
//   BootROM -> CP BL2 (MCUboot) -> CP TF-M Secure (SPE) -> [this NS app]

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

#if CONFIG_TFM_AP_BOOT_NSC
#include "tfm_ap_boot_nsc.h"
#endif

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

void user_app_main(void)
{
#if CONFIG_TFM_AP_BOOT_NSC
	if (psa_ap_boot() != 0) {
		BK_LOGE(NULL, "psa_ap_boot failed\r\n");
	}
#endif
	BK_LOGI(NULL, "secureboot_xip: CP NS world reached (secure boot OK)\r\n");
}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();

	return 0;
}
