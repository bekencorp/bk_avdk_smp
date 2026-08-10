#include "bk_private/bk_init.h"
#include "cli.h"
#include <components/system.h>
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include <os/os.h>
#include <string.h>

#define TAG "ap_fast_boot"

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

static void user_app_main(void)
{
	(void)ap_fast_boot_vote(PM_POWER_MODULE_STATE_ON);
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
