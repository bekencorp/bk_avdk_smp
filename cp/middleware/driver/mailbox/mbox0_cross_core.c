// Copyright 2020-2021 Beken
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

// Mailbox for CrossCore
#include "sdkconfig.h"
#include "mbox0_drv.h"
#include "driver/mailbox_types.h"

extern void crosscore_mb_rx_isr(mailbox_data_t *data);

void crosscore_smp_cmd_handler(uint8_t src_core, uint32_t cmd)
{
	mailbox_data_t data;

	data.param0 = src_core;
	data.param1 = portGET_CORE_ID();
	data.param2 = cmd;
	data.param3 = 0;

	crosscore_mb_rx_isr(&data);
}

bk_err_t bk_mailbox_master_send(mailbox_data_t *data, uint8_t src, uint8_t dst)
{
	mbox0_message_t msg;
	uint32_t fifo_status = 0;

	if(dst >= CONFIG_CPU_CNT)
		return BK_ERR_PARAM;

	/* Keep cross-core IPIs from being silently dropped when the
	 * destination RX FIFO is full. Writing the HW mailbox registers
	 * when the FIFO is full would otherwise lose the message and let
	 * the caller believe the send succeeded, leaving the destination
	 * core's crosscore_mb_busy[] flag stuck at 1 and corrupting the
	 * subsequent IPI accounting (e.g. hotplug stop ack would never
	 * arrive). */
	if(mbox0_drv_get_send_stat((uint32_t)dst, &fifo_status) != MBOX0_HAL_OK)
		return BK_ERR_MAILBOX_SRC_DST;

	if(fifo_status & RX_FIFO_STAT_FULL)
		return BK_ERR_MAILBOX_TIMEOUT;

	msg.dest_cpu = dst;
	msg.data[0] = data->param2;  /* cmd id. */
	msg.data[1] = 0;

	return mbox0_drv_send_message(&msg);
}

bk_err_t bk_mailbox_cc_init_on_current_core(int id)
{
	extern int mbox0_init_on_current_core(int id);

	mbox0_init_on_current_core(id);
	
	return 0;
}

bk_err_t bk_mailbox_cc_init(void)
{
	return mbox0_drv_init();
}

bk_err_t bk_mailbox_cc_deinit(void)
{
	return mbox0_drv_deinit();
}

// eof

