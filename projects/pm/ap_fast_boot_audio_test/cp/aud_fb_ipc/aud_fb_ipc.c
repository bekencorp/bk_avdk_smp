// Copyright 2025-2026 Beken
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

#include <os/os.h>
#include <driver/pwr_clk.h>
#include "modules/cif_common.h"

#include "aud_fb_ipc.h"
#include "../../common/aud_fb_ipc.h"

/* Same layout as cifd_cust_msg_hdr_t / lp_ipc_msg_hdr_t. */
typedef struct {
	uint16_t cmd_id;
	uint16_t len;
	uint8_t payload[0];
} aud_fb_cust_msg_hdr_t;

static volatile uint8_t s_ap_off_pending;

static void aud_fb_ap_off_task(void *arg)
{
	(void)arg;

	(void)bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,
		PM_POWER_MODULE_STATE_OFF);
	s_ap_off_pending = 0;
	rtos_delete_thread(NULL);
}

static void aud_fb_schedule_ap_off(void)
{
	bk_err_t ret;

	if (s_ap_off_pending) {
		return;
	}

	s_ap_off_pending = 1;
	ret = rtos_create_thread(NULL,
		BEKEN_DEFAULT_WORKER_PRIORITY,
		"aud_fb_ap_off",
		(beken_thread_function_t)aud_fb_ap_off_task,
		2048,
		0);
	if (ret != BK_OK) {
		s_ap_off_pending = 0;
		(void)bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,
			PM_POWER_MODULE_STATE_OFF);
	}
}

static int aud_fb_ipc_msg_handler(struct bk_msg_hdr *msg)
{
	aud_fb_cust_msg_hdr_t *hdr;

	if (msg == NULL) {
		return -1;
	}

	hdr = (aud_fb_cust_msg_hdr_t *)(msg + 1);

	switch (hdr->cmd_id) {
	case AUD_FB_IPC_CMD_AP_OFF:
		(void)cif_send_customer_cmd_cfm(NULL, 0, msg);
		aud_fb_schedule_ap_off();
		return 0;
	default:
		break;
	}

	(void)cif_send_customer_cmd_cfm(NULL, 0, msg);
	return 0;
}

void aud_fb_ipc_init(void)
{
	cif_register_customer_msg_handler(aud_fb_ipc_msg_handler);
}
