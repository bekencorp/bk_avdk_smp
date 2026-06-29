// Copyright 2020-2024 Beken
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

/*
 * Cross-core IPC stress test - CP side echo server.
 *
 * Runs on CPU0. Listens on a FREE CPU0 server port (production CPU0 servers
 * use ports 1/2/3 for FLASH/SARADC/PHY; SERVER_PORT_MAX is 4, so this test
 * uses server port 4). It echoes every received payload back to the sending
 * client unchanged so the AP-side client can verify cross-core IPC integrity
 * under concurrency. Test-only, gated by CONFIG_IPC_STRESS_TEST.
 */

#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include <os/rtos_ext.h>
#include <driver/mailbox_channel.h>
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#if CONFIG_IPC_STRESS_TEST && (CONFIG_CPU_CNT > 1)

#define IPC_STRESS_TAG            "ipc_st"

/* echo server lives on a free CPU0 server port (port 4). */
#define IPC_STRESS_SVR_ID         (IPC_SVR_ID_START(0) + 4)
#define IPC_STRESS_SVR_PORT       IPC_GET_ID_PORT(IPC_STRESS_SVR_ID)

/* one server port supports up to MAX_CONNET_PER_SVR (3) concurrent clients. */
#define IPC_STRESS_CONN_MAX       3
#define IPC_STRESS_CONN_FLAGS     ((0x01 << IPC_STRESS_CONN_MAX) - 1)

#define IPC_STRESS_BUF_LEN        256

static rtos_event_ext_t   s_svr_event;

static u32 ipc_stress_svr_rx_cb(u32 handle, u32 connect_id)
{
	if (connect_id >= IPC_STRESS_CONN_MAX)
		return 0;

	rtos_set_event_ex(&s_svr_event, (0x01 << connect_id));

	return 0;
}

static void ipc_stress_svr_handle_conn(u32 handle)
{
	u32  cmd_id = 0;

	if (mb_ipc_get_recv_event(handle, &cmd_id) != 0)
		return;

	/* connect/disconnect are answered internally by the ipc layer. */
	if (cmd_id != MB_IPC_SEND_CMD)
		return;

	int  rem_len = mb_ipc_get_recv_data_len(handle);

	if (rem_len <= 0)
	{
		/* drain a zero-length send so the ipc layer responds to the client. */
		u8  user_cmd;
		(void)mb_ipc_recv(handle, &user_cmd, NULL, 0, 0);
		return;
	}

	static u8  echo_buf[IPC_STRESS_BUF_LEN];
	u8         user_cmd = INVALID_USER_CMD_ID;
	int        total = 0;

	while ((rem_len > 0) && (total < (int)sizeof(echo_buf)))
	{
		int read_len = mb_ipc_recv(handle, &user_cmd, echo_buf + total,
								   sizeof(echo_buf) - total, 0);
		if (read_len <= 0)
			break;

		total   += read_len;
		rem_len -= read_len;
	}

	if (total > 0)
	{
		/* echo the payload back to this client unchanged. */
		(void)mb_ipc_send(handle, user_cmd, echo_buf, total, 500);
	}
}

static void ipc_stress_svr_thread(void *param)
{
	u32  base_handle;

	/* mb_ipc_init is idempotent; make sure the ipc layer is up. */
	mb_ipc_init();

	rtos_init_event_ex(&s_svr_event);

	base_handle = mb_ipc_socket(IPC_STRESS_SVR_PORT, ipc_stress_svr_rx_cb);

	if (base_handle == 0)
	{
		BK_LOGE(IPC_STRESS_TAG, "echo server socket failed.\r\n");
		rtos_deinit_event_ex(&s_svr_event);
		rtos_delete_thread(NULL);
		return;
	}

	BK_LOGI(IPC_STRESS_TAG, "echo server up on cpu0 port %d.\r\n", IPC_STRESS_SVR_PORT);

	while (1)
	{
		u32 event = rtos_wait_event_ex(&s_svr_event, IPC_STRESS_CONN_FLAGS, 1, BEKEN_WAIT_FOREVER);

		if (event == 0)
			continue;

		for (int i = 0; i < IPC_STRESS_CONN_MAX; i++)
		{
			if (event & (0x01 << i))
			{
				u32 conn_handle = mb_ipc_server_get_connect_handle(base_handle, i);
				ipc_stress_svr_handle_conn(conn_handle);
			}
		}
	}
}

void mb_ipc_stress_srv_init(void)
{
	rtos_create_thread(NULL, 3, "ipc_st_svr", ipc_stress_svr_thread, 2048, NULL);
}

#endif /* CONFIG_IPC_STRESS_TEST && (CONFIG_CPU_CNT > 1) */
