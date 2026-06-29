// Copyright 2022-2024 Beken
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
 * Cross-core IPC stress test - AP side clients + "ipc_stress" CLI.
 *
 * Launches N concurrent client threads (pinned to CPU2, the AP master core)
 * that connect to the CP echo server on {cpu0, port 4}, send payloads of
 * varying length with a known pattern, receive the echo and verify it
 * byte-for-byte. After all clients finish it prints a single deterministic
 * sentinel line used as the on-target Oracle gate:
 *
 *     IPC_STRESS_RESULT: PASS total=<n> fail=0
 *     IPC_STRESS_RESULT: FAIL total=<n> fail=<k> reason=<short>
 *
 * Test-only, gated by CONFIG_IPC_STRESS_TEST. The "ipc_stress" command is
 * auto-registered via DRV_CLI_CMD_EXPORT (the .cli_cmdtabl section), so no
 * explicit init call from cli_main.c is needed.
 */

#include <os/os.h>
#include <components/system.h>
#include "cli.h"

#if CONFIG_IPC_STRESS_TEST && (CONFIG_CPU_CNT > 1)

#include <driver/mailbox_channel.h>
#include <driver/mb_ipc.h>
#include <driver/mb_ipc_port_cfg.h>

#define IPC_STRESS_TAG            "ipc_st"

/* CP echo server endpoint. */
#define IPC_STRESS_SVR_CPU        MAILBOX_CPU0
#define IPC_STRESS_SVR_PORT       4

/* free CPU2 client ports (production CPU2 clients use 16/17/18). */
#define IPC_STRESS_CLIENT_PORT_BASE   19

#define IPC_STRESS_MAX_THREADS    3       /* one server port accepts 3 clients. */
#define IPC_STRESS_DEF_THREADS    3
#define IPC_STRESS_DEF_ITER       2000
#define IPC_STRESS_MAX_PAYLOAD    200     /* payload length cycles 1..200 bytes. */
#define IPC_STRESS_BUF_LEN        256

#define IPC_STRESS_CONNECT_TMO    1000
#define IPC_STRESS_SEND_TMO       200
#define IPC_STRESS_RECV_TMO       500
#define IPC_STRESS_RETRY_MAX      500     /* bounded retries on transient busy/timeout. */
#define IPC_STRESS_JOIN_TMO       240000  /* ms to wait for each client to finish. */

typedef struct
{
	int           idx;
	u8            client_port;
	u32           iter;
	int           fail;
	int           total;
	const char  * reason;
} ipc_stress_ctx_t;

static ipc_stress_ctx_t   s_ctx[IPC_STRESS_MAX_THREADS];
static beken_semaphore_t  s_done_sema;
static volatile int       s_busy;

static int ipc_stress_is_transient(int err)
{
	/* negative ipc error codes that warrant a retry rather than a hard fail. */
	if (err == -MB_IPC_TX_BUSY)
		return 1;
	if (err == -MB_IPC_TX_TIMEOUT)
		return 1;
	if (err == -(MB_IPC_ROUTE_BASE_FAILED + IPC_ROUTE_RX_BUSY))
		return 1;
	if (err == -(MB_IPC_ROUTE_BASE_FAILED + IPC_ROUTE_QUEUE_FULL))
		return 1;

	return 0;
}

static void ipc_stress_client_thread(void *param)
{
	ipc_stress_ctx_t * ctx = (ipc_stress_ctx_t *)param;

	u8   tx_buf[IPC_STRESS_BUF_LEN];
	u8   rx_buf[IPC_STRESS_BUF_LEN];

	u32  handle = mb_ipc_socket(ctx->client_port, NULL);

	if (handle == 0)
	{
		ctx->fail   = ctx->iter;
		ctx->reason = "socket";
		goto client_exit;
	}

	int ret = mb_ipc_connect(handle, IPC_STRESS_SVR_CPU, IPC_STRESS_SVR_PORT, IPC_STRESS_CONNECT_TMO);

	if (ret != 0)
	{
		ctx->fail   = ctx->iter;
		ctx->reason = "connect";
		BK_LOGE(IPC_STRESS_TAG, "client-%d connect failed %d.\r\n", ctx->idx, ret);
		goto client_close;
	}

	u8  seed = (u8)(0x11 * (ctx->idx + 1));

	for (u32 i = 0; i < ctx->iter; i++)
	{
		u16  len = (u16)(1 + (i % IPC_STRESS_MAX_PAYLOAD));   /* 1 .. 200 */
		u8   user_cmd = (u8)(i & 0x1f);

		for (u16 k = 0; k < len; k++)
		{
			tx_buf[k] = (u8)((seed + i + k) & 0xff);
		}

		/* send with bounded retry on transient busy/timeout. */
		int send_ret = 0;
		int retry;

		for (retry = 0; retry < IPC_STRESS_RETRY_MAX; retry++)
		{
			send_ret = mb_ipc_send(handle, user_cmd, tx_buf, len, IPC_STRESS_SEND_TMO);

			if (send_ret == 0)
				break;

			if (!ipc_stress_is_transient(send_ret))
				break;

			rtos_delay_milliseconds(1);
		}

		if (send_ret != 0)
		{
			ctx->fail++;
			if (ctx->reason == NULL)
				ctx->reason = "send";
			continue;
		}

		/* receive the echo and verify length + content. */
		u8   rx_cmd = INVALID_USER_CMD_ID;
		int  rx_len = mb_ipc_recv(handle, &rx_cmd, rx_buf, sizeof(rx_buf), IPC_STRESS_RECV_TMO);

		if (rx_len != (int)len)
		{
			ctx->fail++;
			if (ctx->reason == NULL)
				ctx->reason = (rx_len < 0) ? "recv" : "len";
			continue;
		}

		if (memcmp(tx_buf, rx_buf, len) != 0)
		{
			ctx->fail++;
			if (ctx->reason == NULL)
				ctx->reason = "data";
			continue;
		}
	}

client_close:
	if (handle != 0)
	{
		(void)mb_ipc_close(handle, 500);
	}

client_exit:
	rtos_set_semaphore(&s_done_sema);
	rtos_delete_thread(NULL);
}

static void cli_ipc_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (s_busy)
	{
		bk_printf("IPC_STRESS_RESULT: FAIL total=0 fail=1 reason=busy\r\n");
		return;
	}

	u32 iter    = IPC_STRESS_DEF_ITER;
	u32 threads = IPC_STRESS_DEF_THREADS;

	if (argc >= 2)
		iter = os_strtoul(argv[1], NULL, 10);
	if (argc >= 3)
		threads = os_strtoul(argv[2], NULL, 10);

	if (iter == 0)
		iter = IPC_STRESS_DEF_ITER;
	if (threads == 0)
		threads = 1;
	if (threads > IPC_STRESS_MAX_THREADS)
		threads = IPC_STRESS_MAX_THREADS;

	if (rtos_init_semaphore(&s_done_sema, threads) != 0)
	{
		bk_printf("IPC_STRESS_RESULT: FAIL total=0 fail=1 reason=sema\r\n");
		return;
	}

	s_busy = 1;

	int started = 0;

	for (u32 t = 0; t < threads; t++)
	{
		s_ctx[t].idx         = (int)t;
		s_ctx[t].client_port = (u8)(IPC_STRESS_CLIENT_PORT_BASE + t);
		s_ctx[t].iter        = iter;
		s_ctx[t].fail        = 0;
		s_ctx[t].total       = (int)iter;
		s_ctx[t].reason      = NULL;

		/* pin clients to CPU2 (AP master, core 0). */
		int cret = rtos_core0_create_thread(NULL, 4, "ipc_st_cli",
								(beken_thread_function_t)ipc_stress_client_thread,
								2048, (beken_thread_arg_t)&s_ctx[t]);

		if (cret != 0)
		{
			BK_LOGE(IPC_STRESS_TAG, "create client-%u failed %d.\r\n", t, cret);
			break;
		}

		started++;
	}

	int  join_fail = 0;

	for (int j = 0; j < started; j++)
	{
		if (rtos_get_semaphore(&s_done_sema, IPC_STRESS_JOIN_TMO) != 0)
		{
			join_fail = 1;
			break;
		}
	}

	u32 total = 0;
	u32 fail  = 0;
	const char * reason = "send";

	for (int t = 0; t < started; t++)
	{
		total += (u32)s_ctx[t].total;
		fail  += (u32)s_ctx[t].fail;
		if ((s_ctx[t].fail != 0) && (s_ctx[t].reason != NULL))
			reason = s_ctx[t].reason;
	}

	/* threads that never started still count their planned iterations as failed. */
	for (u32 t = (u32)started; t < threads; t++)
	{
		total += iter;
		fail  += iter;
		reason = "spawn";
	}

	rtos_deinit_semaphore(&s_done_sema);
	s_busy = 0;

	if (join_fail)
	{
		bk_printf("IPC_STRESS_RESULT: FAIL total=%u fail=%u reason=join_timeout\r\n", total, total);
		return;
	}

	if (fail == 0)
	{
		bk_printf("IPC_STRESS_RESULT: PASS total=%u fail=0\r\n", total);
	}
	else
	{
		bk_printf("IPC_STRESS_RESULT: FAIL total=%u fail=%u reason=%s\r\n", total, fail, reason);
	}
}

DRV_CLI_CMD_EXPORT static const struct cli_command s_ipc_stress_commands[] = {
	{"ipc_stress", "ipc_stress [iter] [threads] - cross-core IPC stress test", cli_ipc_stress_cmd},
};

#endif /* CONFIG_IPC_STRESS_TEST && (CONFIG_CPU_CNT > 1) */
