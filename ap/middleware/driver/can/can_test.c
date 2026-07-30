#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include <driver/can.h>
#include <driver/can_types.h>
#include "can_statis.h"

#define CLI_CAN_STB 			0
#define CLI_CAN_PTB 			1
#define CLI_CAN_RCV_BUF_SIZE	69 // default 256
#define CLI_CAN_SEND_BUF_SIZE   64 // default 256

static uint32_t cli_can_id = 0x10000001;

static uint8_t demo_rcv_buf[CLI_CAN_RCV_BUF_SIZE];
static uint8_t demo_send_buf[CLI_CAN_SEND_BUF_SIZE];

static beken_thread_t s_can_rx_thread = NULL;
static volatile bool s_can_rx_running = false;

static void cli_can_help(void)
{
	CLI_LOGI("can_transmit tx {stb|ptb} {size} [fd|20] [brs]\r\n");
	CLI_LOGI("can_transmit rx {size} [count]\r\n");
	CLI_LOGI("can_transmit rx stop\r\n");
	CLI_LOGI("can_init\r\n");
	CLI_LOGI("can_exit\r\n");
	CLI_LOGI("can_filter_cfg {aid} {acode} {amask}\r\n");
	CLI_LOGI("can_loop {i|e} [fd|20] [size] [brs]\r\n");
	CLI_LOGI("can_speed {s_speed} {f_speed}\r\n");
	CLI_LOGI("can_statis {dump|reset}\r\n");
}

static void cli_can_driver_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (bk_can_driver_init() == BK_OK) {
		CLI_LOGI("init success\r\n");
	} else {
		CLI_LOGE("init failed\r\n");
	}
}

static void cli_can_driver_exit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	s_can_rx_running = false;
	bk_can_driver_deinit();
	CLI_LOGI("exit success\r\n");
}

/* Validate frame parameters the same way the driver does, but report *why* it
 * would be rejected so a test operator gets feedback instead of silence. */
static bk_err_t cli_can_check_frame(uint8_t tb, uint8_t fdf, uint8_t brs, uint32_t size)
{
	if (fdf == CAN_PROTO_20) {
		if (brs != CAN_BIT_RATE_SLOW) {
			CLI_LOGE("CAN2.0 frame cannot set brs (drop 'brs')\r\n");
			return BK_ERR_PARAM;
		}
		if (size > CAN_20_MAX_PAYLOAD) {
			CLI_LOGE("CAN2.0 payload must be <= %d, got %u\r\n", CAN_20_MAX_PAYLOAD, size);
			return BK_ERR_PARAM;
		}
	} else {
		if (size > CAN_FD_MAX_PAYLOAD) {
			CLI_LOGE("CAN FD payload must be <= %d, got %u\r\n", CAN_FD_MAX_PAYLOAD, size);
			return BK_ERR_PARAM;
		}
		/* PTB writes the buffer directly, so FD lengths >8 must align to 4
		 * (matches bk_can_send_ptb()); STB streams through the FIFO instead. */
		if (tb == CLI_CAN_PTB && size > CAN_20_MAX_PAYLOAD && (size % 4)) {
			CLI_LOGE("FD PTB payload >8 must be a multiple of 4, got %u\r\n", size);
			return BK_ERR_PARAM;
		}
	}
	return BK_OK;
}

static bk_err_t cli_can_do_send(uint8_t tb, uint8_t fdf, uint8_t brs, uint32_t size)
{
	can_frame_s frame;
	uint32_t i;
	bk_err_t ret;

	if (size > CLI_CAN_SEND_BUF_SIZE) {
		size = CLI_CAN_SEND_BUF_SIZE;
	}

	ret = cli_can_check_frame(tb, fdf, brs, size);
	if (ret != BK_OK) {
		return ret;
	}

	for (i = 0; i < size; i++) {
		demo_send_buf[i] = (uint8_t)(i & 0xff);
	}

	frame.tag.fdf = fdf;
	frame.tag.id = cli_can_id;
	frame.tag.rtr = 0;
	frame.tag.ide = 1;
	frame.tag.brs = (fdf == CAN_PROTO_FD) ? brs : 0;
	frame.tag.esi = 0;
	frame.tag.ttsen = 0;
	frame.size = size;
	frame.data = demo_send_buf;

	bk_can_dump_bit_rate();

	if (tb == CLI_CAN_STB) {
		ret = bk_can_send(&frame, 1000);
	} else {
		ret = bk_can_send_ptb(&frame);
	}

	if (ret == BK_OK) {
		CLI_LOGI("%s send ok: proto=%s id=0x%x size=%u brs=%u\r\n",
			(tb == CLI_CAN_STB) ? "stb" : "ptb",
			(fdf == CAN_PROTO_FD) ? "FD" : "2.0",
			cli_can_id, size, frame.tag.brs);
	} else {
		CLI_LOGE("%s send failed ret=%d\r\n",
			(tb == CLI_CAN_STB) ? "stb" : "ptb", ret);
	}
	return ret;
}

static void cli_can_receive_thread(void *param)
{
	uint32_t id;
	uint32_t rec_size = 0, i = 0;
	uint32_t ex_size = (uint32_t)param & 0xffff;
	uint32_t max_cnt = ((uint32_t)param >> 16) & 0xffff;   /* 0 = unlimited */
	uint32_t got_cnt = 0;

	if (ex_size == 0 || ex_size > CLI_CAN_RCV_BUF_SIZE) {
		ex_size = CLI_CAN_RCV_BUF_SIZE;
	}

	while (s_can_rx_running) {
		rec_size = 0;
		bk_can_receive(demo_rcv_buf, ex_size, &rec_size, 1000);
		if (rec_size) {
			CLI_LOGI("recv data %u\r\n", rec_size);
			CLI_LOGI("dlc is 0x%02x\r\n", demo_rcv_buf[0]);
			id = (demo_rcv_buf[4] << 24) |
				(demo_rcv_buf[3] << 16) |
				(demo_rcv_buf[2] << 8) |
				(demo_rcv_buf[1]);
			CLI_LOGI("id is 0x%08x\r\n", id);
			for (i = 5; i < rec_size; i++) {
				CLI_LOGI("data[%02u]:0x%02x\r\n", i, demo_rcv_buf[i]);
			}
			got_cnt++;
			if (max_cnt && got_cnt >= max_cnt) {
				break;
			}
		}
		os_memset(demo_rcv_buf, 0, sizeof(demo_rcv_buf));
		rtos_delay_milliseconds(100);
	}

	CLI_LOGI("recv end (got %u)\r\n", got_cnt);
	s_can_rx_running = false;
	s_can_rx_thread = NULL;
	rtos_delete_thread(NULL);
}

static void cli_can_transmit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		CLI_LOGE("param too few\r\n");
		cli_can_help();
		return;
	}

	if (os_strcmp(argv[1], "rx") == 0) {
		if (os_strcmp(argv[2], "stop") == 0) {
			s_can_rx_running = false;
			CLI_LOGI("rx stopping\r\n");
			return;
		}
		if (s_can_rx_running) {
			CLI_LOGW("rx already running, use 'can_transmit rx stop' first\r\n");
			return;
		}
		uint32_t ex_size = os_strtoul(argv[2], NULL, 10);
		uint32_t cnt = (argc >= 4) ? os_strtoul(argv[3], NULL, 10) : 0;
		uint32_t arg = (ex_size & 0xffff) | ((cnt & 0xffff) << 16);

		s_can_rx_running = true;
		if (rtos_create_thread(&s_can_rx_thread, 5, "can_rcv", cli_can_receive_thread,
				configMINIMAL_STACK_SIZE * 4, (void *)arg) != kNoErr) {
			s_can_rx_running = false;
			s_can_rx_thread = NULL;
			CLI_LOGE("rx thread create failed\r\n");
		} else {
			CLI_LOGI("rx started size=%u cnt=%u\r\n", ex_size, cnt);
		}
	} else if (os_strcmp(argv[1], "tx") == 0) {
		uint8_t tb;
		uint8_t fdf = CAN_PROTO_20;
		uint8_t brs = CAN_BIT_RATE_SLOW;

		if (os_strcmp(argv[2], "stb") == 0) {
			tb = CLI_CAN_STB;
		} else if (os_strcmp(argv[2], "ptb") == 0) {
			tb = CLI_CAN_PTB;
		} else {
			CLI_LOGE("2nd param must be stb or ptb\r\n");
			return;
		}

		if (argc < 4) {
			CLI_LOGE("missing tx size\r\n");
			cli_can_help();
			return;
		}
		uint32_t ex_size = os_strtoul(argv[3], NULL, 10);

		if (argc >= 5) {
			if (os_strcmp(argv[4], "fd") == 0) {
				fdf = CAN_PROTO_FD;
			} else if (os_strcmp(argv[4], "20") == 0) {
				fdf = CAN_PROTO_20;
			} else {
				CLI_LOGE("4th param must be fd or 20\r\n");
				return;
			}
		}
		if (argc >= 6) {
			if (os_strcmp(argv[5], "brs") == 0) {
				brs = CAN_BIT_RATE_FAST;
			} else {
				brs = (uint8_t)(os_strtoul(argv[5], NULL, 10) ? CAN_BIT_RATE_FAST : CAN_BIT_RATE_SLOW);
			}
		}

		cli_can_do_send(tb, fdf, brs, ex_size);
	} else {
		CLI_LOGE("param err\r\n");
		cli_can_help();
	}
}

static void can_filter_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_acc_filter_cmd_s cmd;
	bk_err_t ret;
	if (argc < 4) {
		CLI_LOGE("param too few\r\n");
		cli_can_help();
		return;
	}
	cmd.aide = os_strtoul(argv[1], NULL, 10);
	cmd.code = os_strtoul(argv[2], NULL, 16);
	cmd.mask = os_strtoul(argv[3], NULL, 16);
	cmd.seq = 0;
	cmd.onoff = DRIVER_ENABLE;
	ret = bk_can_acc_filter_set(&cmd);
	if (ret == BK_OK) {
		CLI_LOGI("filter set ok: aide=%u code=0x%x mask=0x%x\r\n", cmd.aide, cmd.code, cmd.mask);
	} else {
		CLI_LOGE("filter set failed ret=%d\r\n", ret);
	}
}

static void can_loop(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	beken_thread_t rcv_thread = NULL;
	int ret;
	bool internal;
	uint8_t fdf = CAN_PROTO_20;
	uint8_t brs = CAN_BIT_RATE_SLOW;
	uint32_t ex_size = 2;

	if (argc < 2) {
		CLI_LOGE("param too few\r\n");
		cli_can_help();
		return;
	}

	if (os_strcmp(argv[1], "i") == 0) {
		internal = true;
	} else if (os_strcmp(argv[1], "e") == 0) {
		internal = false;
	} else {
		CLI_LOGE("1st param must be i or e\r\n");
		return;
	}

	if (argc >= 3) {
		if (os_strcmp(argv[2], "fd") == 0) {
			fdf = CAN_PROTO_FD;
		} else if (os_strcmp(argv[2], "20") == 0) {
			fdf = CAN_PROTO_20;
		} else {
			CLI_LOGE("2nd param must be fd or 20\r\n");
			return;
		}
	}
	if (argc >= 4) {
		ex_size = os_strtoul(argv[3], NULL, 10);
	}
	if (argc >= 5) {
		if (os_strcmp(argv[4], "brs") == 0) {
			brs = CAN_BIT_RATE_FAST;
		} else {
			brs = (uint8_t)(os_strtoul(argv[4], NULL, 10) ? CAN_BIT_RATE_FAST : CAN_BIT_RATE_SLOW);
		}
	}

	if (internal) {
		bk_can_set_loopback_internal(true);
	} else {
		bk_can_set_loopback_external(true);
	}

	s_can_rx_running = true;
	ret = rtos_create_thread(&rcv_thread, 2, "can_rcv", cli_can_receive_thread,
			configMINIMAL_STACK_SIZE * 20, (void *)((ex_size + 5) | (1 << 16)));
	if (ret != kNoErr) {
		s_can_rx_running = false;
		CLI_LOGE("rtos_create_thread failed!!!\r\n");
		goto loop_off;
	}

	cli_can_do_send(CLI_CAN_STB, fdf, brs, ex_size);
	rtos_delay_milliseconds(1000);

	s_can_rx_running = false;
	rtos_delay_milliseconds(50);

loop_off:
	if (internal) {
		bk_can_set_loopback_internal(false);
	} else {
		bk_can_set_loopback_external(false);
	}
}

static void cli_can_speed_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_bit_rate_e s_speed;
	can_bit_rate_e f_speed;
	bk_err_t ret;
	if (argc < 3) {
		cli_can_help();
		return;
	}

	s_speed = os_strtoul(argv[1], NULL, 10);
	f_speed = os_strtoul(argv[2], NULL, 10);
	ret = can_driver_bit_rate_config(s_speed, f_speed);
	if (ret == BK_OK) {
		CLI_LOGI("speed set ok: s=%d f=%d\r\n", s_speed, f_speed);
	} else {
		CLI_LOGE("speed set failed ret=%d (valid range %d~%d)\r\n", ret, CAN_BR_250K, CAN_BR_5M);
	}
}

static void cli_can_ssp_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t sspoff;
	if (argc < 2) {
		cli_can_help();
		return;
	}

	sspoff = os_strtoul(argv[1], NULL, 10);
	bk_can_set_ssp(sspoff);
	CLI_LOGI("ssp set ok: sspoff=%u\r\n", sspoff);
}

static void cli_can_iso_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t iso;
	if (argc < 2) {
		cli_can_help();
		return;
	}

	iso = os_strtoul(argv[1], NULL, 10);
	bk_can_set_iso(iso);
	CLI_LOGI("iso set ok: iso=%u\r\n", iso ? 1 : 0);
}

static void cli_can_statis(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_can_help();
		return;
	}

#if CONFIG_CAN_STATIS
	if (os_strcmp(argv[1], "dump") == 0) {
		can_statis_dump();
		CLI_LOGI("can dump statis ok\r\n");
	} else if (os_strcmp(argv[1], "reset") == 0) {
		can_statis_init();
		CLI_LOGI("can reset statis ok\r\n");
	}
#else
	CLI_LOGW("CONFIG_CAN_STATIS not enabled\r\n");
#endif

	return;
}

static const struct cli_command s_can_commands[] = {
	{"can_transmit", "can_transmit {rx|tx} {rx size|stb|ptb} {tx size} [fd|20] [brs]", cli_can_transmit},
	{"can_init", "can_init", cli_can_driver_init},
	{"can_exit", "can_exit", cli_can_driver_exit},
	{"can_filter_cfg", "can_filter_cfg {aid} {acode} {amask}", can_filter_cfg},
	{"can_loop", "can_loop {i|e} [fd|20] [size] [brs]", can_loop},
	{"can_speed", "can_speed {s_speed} {f_speed}", cli_can_speed_cfg},
	{"can_ssp", "can_ssp {sspoff_tq}", cli_can_ssp_cfg},
	{"can_iso", "can_iso {0|1}", cli_can_iso_cfg},
	{"can_statis", "can_statis {dump|reset}", cli_can_statis},
};
#define CAN_CMD_CNT (sizeof(s_can_commands) / sizeof(struct cli_command))

int bk_can_register_cli_test_feature(void)
{
	return cli_register_commands(s_can_commands, CAN_CMD_CNT);
}
