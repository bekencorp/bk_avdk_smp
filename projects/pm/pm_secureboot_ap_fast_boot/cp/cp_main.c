// Copyright 2025 Beken
//
// pm_secureboot_ap_fast_boot - CP NS application entry.
//
// Boot chain (secure):
//   BootROM -> CP BL2 (MCUboot) -> CP TF-M Secure (SPE) -> [this NS app]
//
// After the non-secure world is reached, the CP votes the AP on and registers
// the ap_fast_boot CLI used to cycle AP power and verify fast resume.

#include "bk_private/bk_init.h"
#include "cli.h"
#include <components/system.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include <os/os.h>
#include <string.h>

#define TAG "pm_secureboot_ap_fast_boot"

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

static bk_err_t ap_fast_boot_vote(pm_power_module_state_e state)
{
	bk_err_t ret = bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP, state);

	BK_LOGI(TAG, "AP vote %s ret=%d\r\n",
		(state == PM_POWER_MODULE_STATE_ON) ? "ON" : "OFF", ret);
	return ret;
}

static void cli_ap_fast_boot_cmd(char *pcWriteBuffer, int xWriteBufferLen,
	int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc != 2) {
		BK_LOGI(TAG, "usage: ap_fast_boot on|off|cycle\r\n");
		return;
	}

	if (strcmp(argv[1], "on") == 0) {
		(void)ap_fast_boot_vote(PM_POWER_MODULE_STATE_ON);
	} else if (strcmp(argv[1], "off") == 0) {
		(void)ap_fast_boot_vote(PM_POWER_MODULE_STATE_OFF);
	} else if (strcmp(argv[1], "cycle") == 0) {
		if (ap_fast_boot_vote(PM_POWER_MODULE_STATE_OFF) == BK_OK) {
			rtos_delay_milliseconds(1000);
			(void)ap_fast_boot_vote(PM_POWER_MODULE_STATE_ON);
		}
	} else {
		BK_LOGI(TAG, "usage: ap_fast_boot on|off|cycle\r\n");
	}
}

static const struct cli_command s_ap_fast_boot_commands[] = {
	{
		"ap_fast_boot",
		"ap_fast_boot on|off|cycle",
		cli_ap_fast_boot_cmd,
	},
};

void user_app_main(void)
{
	/* Start the AP through the common PM vote + multicore path. TF-M performs
	 * only the Secure-privilege AP prepare via the psa_ap_secure_prepare NSC
	 * inside multicore_hal_start(). */
	if (ap_fast_boot_vote(PM_POWER_MODULE_STATE_ON) != BK_OK) {
		BK_LOGE(TAG, "AP boot vote failed\r\n");
	}
	BK_LOGI(TAG, "CP NS world reached (secure boot OK)\r\n");
}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();
	cli_register_commands(s_ap_fast_boot_commands,
		sizeof(s_ap_fast_boot_commands) / sizeof(s_ap_fast_boot_commands[0]));
	BK_LOGI(TAG, "CLI ready: ap_fast_boot on|off|cycle\r\n");

	return 0;
}
