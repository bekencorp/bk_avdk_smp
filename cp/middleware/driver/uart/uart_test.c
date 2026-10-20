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

#include <os/os.h>
#include "cli.h"
#include <driver/uart.h>
#include <stdbool.h>
#include <components/bk_platform.h>
#if CONFIG_UART_STRESS_TEST && CONFIG_SHELL_ASYNCLOG
#include <components/shell_task.h>
#endif
#include "uart_statis.h"
#include "bk_misc.h"
#include "sys_driver.h"

#if CONFIG_FACIAL_RECOGN
extern  void cli_fr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#if CONFIG_UART_TX_DMA
bk_err_t uart_tx_dma_init(uart_id_t id);
bk_err_t bk_uart_dma_write_string(uart_id_t id, const char *string);
bk_err_t uart_tx_dma_deinit(uart_id_t id);
#endif

#if CONFIG_UART_RX_DMA
bk_err_t uart_rx_dma_init(uart_id_t id);
bk_err_t uart_rx_dma_deinit(uart_id_t id);
#endif

static void cli_uart_help(void)
{
	CLI_LOGD("uart_driver init\n");
	CLI_LOGD("uart_driver deinit\n");
	CLI_LOGD("uart {id} {init|deinit|write|read|write_string|dump_statis} [...]\n");
	CLI_LOGD("uart {id} {init} [baud_rate][data_bits:0~3 means 5~8bits][parity:0 none,1 odd, 2 even][stopbits:0 means 1bit, 1 means 2bits][flow ctrl]\n");
	CLI_LOGD("uart_int {id} {enable|disable|reg} {tx|rx}\n");
	CLI_LOGD("uart_test {idle_start|idle_stop} {uart1|uart2|uart3}\n");
	CLI_LOGD("uart_dma {tx|rx} {uart0|uart1|uart2|uart3}\n");
	CLI_LOGD("uart_lb {id} [quick|full]     -- TX<->RX jumper loopback matrix (SPI_LB style)\n");
	CLI_LOGD("uart_lb_api {id}              -- timeout/empty/oversized/deinit negative tests\n");
	CLI_LOGD("uart_lb_dma {id} [quick|full] -- DMA TX+RX loopback (needs CONFIG_UART_TX/RX_DMA)\n");
	CLI_LOGD("uart_dma_test {start|stop} {id} [baud] [len] -- dual-task TX+RX DMA loopback\n");
	CLI_LOGD("uart_dma_stress {start|stop} [id] [baud] -- ping-pong integrity (byteerrs/shorts/resyncs)\n");
	CLI_LOGD("uart_dma_cycle {id} [baud] [iterations] -- init/io/deinit cycle (init_fail/io_fail)\n");
	CLI_LOGD("uart_lb_flow {id}             -- HW CTS/RTS loopback (wire TX-RX and RTS-CTS)\n");
#if CONFIG_UART_STRESS_TEST
#if CONFIG_SHELL_ASYNCLOG
	CLI_LOGD("uart_log_test [count] [interval_ms] -- print deterministic 256-byte log frames\n");
#endif
	CLI_LOGD("uart_lb_256 {id} [9600|38400|115200|all] -- 256-byte 8N1 loopback\n");
#endif
}

static void cli_uart_dma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        uart_id_t uart_id = UART_ID_0;

	if (argc < 2) {
		cli_uart_help();
		return;
	}

	if (os_strcmp(argv[1], "tx") == 0) {
		CLI_LOGD("uart dma tx\n");

                if(0 == os_strncmp(argv[2], "uart", os_strlen("uart"))){
                        uart_id = argv[2][os_strlen("uart")] - '0';
                }

                bk_err_t ret;
                const uart_config_t config =
                {
                        .baud_rate = UART_BAUD_RATE,
                        .data_bits = UART_DATA_8_BITS,
                        .parity = UART_PARITY_NONE,
                        .stop_bits = UART_STOP_BITS_1,
                        .flow_ctrl = UART_FLOWCTRL_DISABLE,
                        .src_clk = UART_SCLK_XTAL_26M
                };

                ret = bk_uart_init(uart_id, &config);
                if (BK_OK != ret)
                {
                        CLI_LOGD("bk_uart_init failed\n");
                        return;
                }
                bk_uart_disable_sw_fifo(uart_id);

#if CONFIG_UART_TX_DMA
                char *tx_string = "abcdefghijklmnopqrstuvwxyz";

                uart_tx_dma_init(uart_id);
                bk_uart_dma_write_string(uart_id, tx_string);
                //uart_tx_dma_deinit(uart_id);
#else
                CLI_LOGD("tx dma of uart doesnot be supported, and configure the macreo:CONFIG_UART_TX_DMA\n");
#endif
	} else if (os_strcmp(argv[1], "rx") == 0) {
                CLI_LOGD("uart dma rx\n");

                if(0 == os_strncmp(argv[2], "uart", os_strlen("uart"))){
                        uart_id = argv[2][os_strlen("uart")] - '0';
                }

                bk_err_t ret;
                const uart_config_t config =
                {
                        .baud_rate = UART_BAUD_RATE,
                        .data_bits = UART_DATA_8_BITS,
                        .parity = UART_PARITY_NONE,
                        .stop_bits = UART_STOP_BITS_1,
                        .flow_ctrl = UART_FLOWCTRL_DISABLE,
                        .src_clk = UART_SCLK_XTAL_26M
                };

                ret = bk_uart_init(uart_id, &config);
                if (BK_OK != ret)
                {
                        CLI_LOGD("bk_uart_init failed\n");
                        return;
                }
                bk_uart_enable_rx_interrupt(uart_id);

#if CONFIG_UART_RX_DMA
                uint8_t rx_buf[16] = {0};
                char *tx_string = "please input string at the specified uart!\r\n";
                uart_rx_dma_init(uart_id);

                CLI_LOGD("bk_uart_reading_[%d]bytes\n", sizeof(rx_buf));
                ret = uart_write_string(uart_id, tx_string);
                ret = bk_uart_read_bytes(uart_id, rx_buf, sizeof(rx_buf), BEKEN_WAIT_FOREVER);
                CLI_LOGD("bk_uart_readed_[%d]bytes\n", ret);

                if(ret > 0){
                        print_hex_dump("rx_buf:", rx_buf, ret);
                }
                uart_rx_dma_deinit(uart_id);
#endif
	} else {
		cli_uart_help();
		return;
	}
}

static void cli_uart_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_uart_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_uart_driver_init());
		CLI_LOGD("uart driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_uart_driver_deinit());
		CLI_LOGD("uart driver deinit\n");
	} else {
		cli_uart_help();
		return;
	}
}

static void cli_uart_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t uart_id;

	if (argc < 2) {
		cli_uart_help();
		return;
	}

	uart_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 5);
		uart_config_t config = {0};
		os_memset(&config, 0, sizeof(uart_config_t));
		config.baud_rate = os_strtoul(argv[3], NULL, 10);
		config.data_bits = os_strtoul(argv[4], NULL, 10);
		config.parity = os_strtoul(argv[5], NULL, 10);
		config.stop_bits = os_strtoul(argv[6], NULL, 10);
		if (argc > 7) {
			config.flow_ctrl = os_strtoul(argv[7], NULL, 10);
		}
		if (argc > 8) {
			config.src_clk = os_strtoul(argv[8], NULL, 10);
		}

		#if CONFIG_UART_RX_DMA
		config.rx_dma_en = 1;
		#endif

		BK_LOG_ON_ERR(bk_uart_init(uart_id, &config));
		CLI_LOGD("uart init, uart_id=%d\n", uart_id);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_uart_deinit(uart_id));
		CLI_LOGD("uart deinit, uart_id=%d\n", uart_id);
	} else if (os_strcmp(argv[2], "write") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
		if (send_data == NULL) {
			CLI_LOGE("send buffer malloc failed\r\n");
			return;
		}

		/* If only a single byte with a value 0 is sent, and the logic analyzer uses baudrate auto-detection for
		 * uart, it may cause misinterpretation.An all-zero byte might be understood by the logic analyzer as a
		 * uart stat bit(logic 0)
		 */
		os_memset(send_data, 0xaa, buf_len);
		for (int i = 0; i < buf_len; i++) {
			send_data[i] = (i + 0xaa) & 0xff;
		}
		BK_LOG_ON_ERR(bk_uart_write_bytes(uart_id, send_data, buf_len));
		if (send_data) {
			os_free(send_data);
		}
		send_data = NULL;
		CLI_LOGD("uart write, uart_id=%d, data_len:%d\n", uart_id, buf_len);
	} else if (os_strcmp(argv[2], "read") == 0) {
		if(argc < 5){
				CLI_LOGE("uart read param exceptional\r\n");
				return;
		}

		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
		if (recv_data == NULL) {
			CLI_LOGE("recv buffer malloc failed\r\n");
			return;
		}
		int time_out = os_strtoul(argv[4], NULL, 10);
		if (time_out < 0) {
			time_out = BEKEN_WAIT_FOREVER;
		}
		int data_len = bk_uart_read_bytes(uart_id, recv_data, buf_len, time_out);
		if (data_len < 0) {
			CLI_LOGE("uart read failed, ret:-0x%x\r\n", -data_len);
			goto exit;
		}
		CLI_LOGD("uart read, uart_id=%d, time_out:%x data_len:%d\n", uart_id, time_out, data_len);
		for (int i = 0; i < data_len; i++) {
			CLI_LOGD("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
		}
exit:
		if (recv_data) {
			os_free(recv_data);
		}
		recv_data = NULL;
	} else if (os_strcmp(argv[2], "write_string") == 0) {
		char send_data[] = "beken uart write string test\r\n";
		BK_LOG_ON_ERR(bk_uart_write_bytes(uart_id, send_data, os_strlen(send_data)));
		CLI_LOGD("uart write string, uart_id=%d, data_len:%d\n", uart_id, os_strlen(send_data));
	}
#if CONFIG_UART_STATIS
	else if (os_strcmp(argv[2], "dump_statis") == 0) {
		uart_statis_dump(uart_id);
		CLI_LOGD("uart dump statis ok\r\n");
	} else if (os_strcmp(argv[2], "reset_statis") == 0) {
		uart_statis_id_init(uart_id);
		CLI_LOGD("uart reset statis ok\r\n");
	}
#endif
	else {
		cli_uart_help();
		return;
	}
}

static void cli_uart_config_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t uart_id;

	if (argc < 4) {
		cli_uart_help();
		return;
	}

	uart_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "baud_rate") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t baud_rate = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_baud_rate(uart_id, baud_rate));
		CLI_LOGD("uart(%d) config baud_rate:%d\n", uart_id, baud_rate);
	} else if (os_strcmp(argv[2], "data_bits") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t data_bits = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_data_bits(uart_id, data_bits));
		CLI_LOGD("uart(%d) config data_bits:%d\n", uart_id, data_bits);
	} else if (os_strcmp(argv[2], "stop_bits") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t stop_bits = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_stop_bits(uart_id, stop_bits));
		CLI_LOGD("uart(%d) config stop_bits:%d\n", uart_id, stop_bits);
	} else if (os_strcmp(argv[2], "parity") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t parity = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_parity(uart_id, parity));
		CLI_LOGD("uart(%d) config parity:%d\n", uart_id, parity);
	} else if (os_strcmp(argv[2], "flow_ctrl") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t rx_threshold = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_hw_flow_ctrl(uart_id, rx_threshold));
		CLI_LOGD("uart(%d) config flow_ctrl:%d\n", uart_id, rx_threshold);
	} else if (os_strcmp(argv[2], "rx_thresh") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t rx_thresh = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_rx_full_threshold(uart_id, rx_thresh));
		CLI_LOGD("uart(%d) config rx_thresh:%d\n", uart_id, rx_thresh);
	} else if (os_strcmp(argv[2], "tx_thresh") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t tx_thresh = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_tx_empty_threshold(uart_id, tx_thresh));
		CLI_LOGD("uart(%d) config tx_thresh:%d\n", uart_id, tx_thresh);
	} else if (os_strcmp(argv[2], "rx_timeout") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t timeout_thresh = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_uart_set_rx_timeout(uart_id, timeout_thresh));
		CLI_LOGD("uart(%d) config rx_timeout:%d\n", uart_id, timeout_thresh);
	}  else if (os_strcmp(argv[2], "clk_select") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		if (os_strcmp(argv[3], "xtal") == 0){
			sys_drv_uart_select_clock(uart_id, UART_SCLK_XTAL_26M);
			CLI_LOGI("uart(%d) set clk_source xtal\r\n", uart_id);
		} else if (os_strcmp(argv[3], "apll") == 0){
			sys_drv_uart_select_clock(uart_id, UART_SCLK_APLL);
			CLI_LOGI("uart(%d) set clk_source apll\r\n", uart_id);
		}  else if (os_strcmp(argv[3], "80m") == 0){
		sys_drv_uart_select_clock(uart_id, UART_SCLK_80M);
		CLI_LOGI("uart(%d) set clk_source 80m\r\n", uart_id);
		} else {
			CLI_LOGI("uart config set failed,clock_source only support xtal/apll\r\n");
			return;
		}
		CLI_LOGI("uart(%d) config clk_select succeed\r\n", uart_id);
	} else {
		cli_uart_help();
		return;
	}
}

static void cli_uart_rx_isr(uart_id_t id, void *param)
{
	CLI_LOGD("uart_rx_isr(%d)\n", id);
}

static void cli_uart_tx_isr(uart_id_t id, void *param)
{
	CLI_LOGD("uart_tx_isr(%d)\n", id);
}

static void cli_uart_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t uart_id;

	if (argc != 4) {
		cli_uart_help();
		return;
	}

	uart_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "enable") == 0) {
		if (os_strcmp(argv[3], "tx") == 0) {
			BK_LOG_ON_ERR(bk_uart_enable_tx_interrupt(uart_id));
			CLI_LOGD("uart id:%d enable tx interrupt\n", uart_id);
		} else {
			BK_LOG_ON_ERR(bk_uart_enable_rx_interrupt(uart_id));
			CLI_LOGD("uart id:%d enable rx interrupt\n", uart_id);
		}
	} else if (os_strcmp(argv[2], "disable") == 0) {
		if (os_strcmp(argv[3], "tx") == 0) {
			BK_LOG_ON_ERR(bk_uart_disable_tx_interrupt(uart_id));
			CLI_LOGD("uart id:%d disable tx interrupt\n", uart_id);
		} else {
			BK_LOG_ON_ERR(bk_uart_disable_rx_interrupt(uart_id));
			CLI_LOGD("uart id:%d disable rx interrupt\n", uart_id);
		}
	} else if (os_strcmp(argv[2], "reg") == 0) {
		if (os_strcmp(argv[3], "tx") == 0) {
			BK_LOG_ON_ERR(bk_uart_register_tx_isr(uart_id, cli_uart_tx_isr, NULL));
			CLI_LOGD("uart id:%d register tx interrupt isr\n", uart_id);
		} else {
			BK_LOG_ON_ERR(bk_uart_register_rx_isr(uart_id, cli_uart_rx_isr, NULL));
			CLI_LOGD("uart id:%d register rx interrupt isr\n", uart_id);
		}
	} else {
		cli_uart_help();
		return;
	}
}

#if CONFIG_UART_STRESS_TEST
static bool uart_test_parse_u32(const char *text, uint32_t *value)
{
	uint32_t parsed = 0;

	if (!text || !value || text[0] == '\0') {
		return false;
	}
	for (uint32_t i = 0; text[i] != '\0'; i++) {
		uint32_t digit;

		if (text[i] < '0' || text[i] > '9') {
			return false;
		}
		digit = (uint32_t)(text[i] - '0');
		if (parsed > (0xffffffffu - digit) / 10u) {
			return false;
		}
		parsed = parsed * 10u + digit;
	}

	*value = parsed;
	return true;
}

#if CONFIG_SHELL_ASYNCLOG
#define UART_LOG_TEST_FRAME_SIZE       (256u)
#define UART_LOG_TEST_CRC_OFFSET       (246u)
#define UART_LOG_TEST_DEFAULT_COUNT    (1u)
#define UART_LOG_TEST_MAX_COUNT        (100000u)
#define UART_LOG_TEST_MAX_INTERVAL_MS  (60000u)

static uint32_t uart_log_test_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xffffffffu;

	for (uint32_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint32_t bit = 0; bit < 8; bit++) {
			if (crc & 1u) {
				crc = (crc >> 1) ^ 0xedb88320u;
			} else {
				crc >>= 1;
			}
		}
	}

	return ~crc;
}

static void uart_log_test_put_hex32(uint8_t *output, uint32_t value)
{
	static const uint8_t hex[] = "0123456789ABCDEF";

	for (uint32_t i = 0; i < 8; i++) {
		uint32_t shift = 28u - (i * 4u);
		output[i] = hex[(value >> shift) & 0xfu];
	}
}

static void uart_log_test_build_frame(uint8_t *frame, uint32_t sequence)
{
	static const uint8_t pattern[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	uint32_t crc;

	os_memcpy(frame, "ULOG", 4);
	uart_log_test_put_hex32(&frame[4], sequence);
	frame[12] = ':';
	for (uint32_t i = 13; i < 245; i++) {
		frame[i] = pattern[(i - 13u) % (sizeof(pattern) - 1u)];
	}
	frame[245] = ':';

	crc = uart_log_test_crc32(frame, UART_LOG_TEST_CRC_OFFSET);
	uart_log_test_put_hex32(&frame[UART_LOG_TEST_CRC_OFFSET], crc);
	frame[254] = '\r';
	frame[255] = '\n';
}

static void cli_uart_log_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t frame[UART_LOG_TEST_FRAME_SIZE];
	uint32_t count = UART_LOG_TEST_DEFAULT_COUNT;
	uint32_t interval_ms = 0;
	uint32_t queued = 0;

	if (argc > 3) {
		CLI_LOGE("UART_LOG_TEST: usage: uart_log_test [count] [interval_ms]\r\n");
		return;
	}
	if (argc >= 2 && !uart_test_parse_u32(argv[1], &count)) {
		CLI_LOGE("UART_LOG_TEST: invalid count=%s\r\n", argv[1]);
		return;
	}
	if (argc >= 3 && !uart_test_parse_u32(argv[2], &interval_ms)) {
		CLI_LOGE("UART_LOG_TEST: invalid interval_ms=%s\r\n", argv[2]);
		return;
	}
	if (count == 0 || count > UART_LOG_TEST_MAX_COUNT) {
		CLI_LOGE("UART_LOG_TEST: count must be 1..%u\r\n", UART_LOG_TEST_MAX_COUNT);
		return;
	}
	if (interval_ms > UART_LOG_TEST_MAX_INTERVAL_MS) {
		CLI_LOGE("UART_LOG_TEST: interval_ms must be 0..%u\r\n",
			 UART_LOG_TEST_MAX_INTERVAL_MS);
		return;
	}

	for (uint32_t sequence = 0; sequence < count; sequence++) {
		uart_log_test_build_frame(frame, sequence);
		if (!shell_log_raw_data(frame, sizeof(frame))) {
			CLI_LOGE("UART_LOG_TEST: queue failed at sequence=%u\r\n", sequence);
			break;
		}
		queued++;
		if (interval_ms > 0 && sequence + 1u < count) {
			rtos_delay_milliseconds(interval_ms);
		}
	}

	CLI_LOGI("UART_LOG_TEST: queued=%u requested=%u interval_ms=%u frame_size=%u\r\n",
		 queued, count, interval_ms, UART_LOG_TEST_FRAME_SIZE);
}
#endif
#endif

#if CONFIG_IDLE_UART_OUT_TEST
static beken_thread_t idle_uart_out_test_handle = NULL;
static uint16_t idle_uart_out_test_id = 0;
static void cli_idle_uart_out_test_isr(uart_id_t id, void *param)
{
	return;
}

static void cli_idle_uart_out_test(void *arg)
{
	while (1) {
		unsigned long random;
		char tx_buffer[16];

		random = bk_rand();
		itoa(random, tx_buffer, 14);
		tx_buffer[15] = '\0';
		uart_write_string(idle_uart_out_test_id, tx_buffer);

	}
	rtos_delete_thread(&idle_uart_out_test_handle);
}

static void cli_uart_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_uart_help();
		return;
	}

	if (os_strcmp(argv[1], "idle_start") == 0) {
		if (!idle_uart_out_test_handle) {
			if (os_strcmp(argv[2], "uart0") == 0) {
#if (CONFIG_UART_PRINT_PORT != 0)
				idle_uart_out_test_id = UART_ID_0;
				CLI_LOGD("idle_uart_out task start: uart_id = UART1\n" );

#else
				CLI_LOGD("cli_uart_test_cmd UART1 for log output!!!\n");
				return;
#endif
			} else if (os_strcmp(argv[2], "uart1")== 0) {
#if (CONFIG_UART_PRINT_PORT != 1)
				idle_uart_out_test_id = UART_ID_1;
				CLI_LOGD("idle_uart_out task start: uart_id = UART2\n" );

#else
				CLI_LOGD("cli_uart_test_cmd UART2 for log output!!!\n");
				return;
#endif
			} else if (os_strcmp(argv[2], "uart2")== 0) {
#if (CONFIG_UART_PRINT_PORT != 2)
				idle_uart_out_test_id = UART_ID_2;
				CLI_LOGD("idle_uart_out task start: uart_id = UART3\n" );

#else
				CLI_LOGD("cli_uart_test_cmd UART3 for log output!!!\n");
				return;
#endif

			} else {
				cli_uart_help();
				return;
			}

			uart_config_t config = {0};
			os_memset(&config, 0, sizeof(uart_config_t));

			config.baud_rate = UART_BAUD_RATE;
			config.data_bits = UART_DATA_8_BITS;
			config.parity = UART_PARITY_NONE;
			config.stop_bits = UART_STOP_BITS_1;
			config.flow_ctrl = UART_FLOWCTRL_DISABLE;
			config.src_clk = UART_SCLK_XTAL_26M;

			BK_LOG_ON_ERR(bk_uart_init(idle_uart_out_test_id, &config));
			BK_LOG_ON_ERR(bk_uart_deinit(idle_uart_out_test_id));

			BK_LOG_ON_ERR(bk_uart_register_tx_isr(idle_uart_out_test_id, cli_idle_uart_out_test_isr, NULL));
			BK_LOG_ON_ERR(bk_uart_enable_tx_interrupt(idle_uart_out_test_id));
			BK_LOG_ON_ERR(bk_uart_init(idle_uart_out_test_id, &config));
			if(rtos_create_thread(&idle_uart_out_test_handle, 8, "idle_uart_out",
					(beken_thread_function_t) cli_idle_uart_out_test, 2048, 0)) {
				CLI_LOGD("cli_uart_test_cmd rtos_create_thread FAILED!\n");
				return;
			}
		}else {
			CLI_LOGD("PLEASE stop the task\n");
		}
		return;
	} else if (os_strcmp(argv[1], "idle_stop") == 0) {

		if (idle_uart_out_test_handle) {
			if (os_strcmp(argv[2], "uart1") == 0) {
				if(idle_uart_out_test_id != UART_ID_0) {
					CLI_LOGD("PLEASE enter a correct ID\n");
					return;
				} else
					idle_uart_out_test_id = UART_ID_0;
			} else if (os_strcmp(argv[2], "uart2")== 0) {
				if(idle_uart_out_test_id != UART_ID_1) {
					CLI_LOGD("PLEASE enter a correct ID\n");
					return;
				} else
					idle_uart_out_test_id = UART_ID_1;
			} else if (os_strcmp(argv[2], "uart3")== 0) {
				if(idle_uart_out_test_id != UART_ID_2) {
					CLI_LOGD("PLEASE enter a correct ID\n");
					return;
				} else
					idle_uart_out_test_id = UART_ID_2;
			} 
#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)			
			else if (os_strcmp(argv[2], "uart4")== 0) {
				if(idle_uart_out_test_id != UART_ID_3) {
					CLI_LOGD("PLEASE enter a correct ID\n");
					return;
				} else
					idle_uart_out_test_id = UART_ID_3;
			} 
#endif
			else {
				cli_uart_help();
				return;
			}

			rtos_delete_thread(&idle_uart_out_test_handle);
			idle_uart_out_test_handle = NULL;
			BK_LOG_ON_ERR(bk_uart_disable_tx_interrupt(idle_uart_out_test_id));
			BK_LOG_ON_ERR(bk_uart_register_tx_isr(idle_uart_out_test_id, NULL, NULL));
			BK_LOG_ON_ERR(bk_uart_deinit(idle_uart_out_test_id));
			CLI_LOGD("idle_uart_out task stop\n");
		} else {
			CLI_LOGD("PLEASE start task FIRST!!!\n");
		}
		return;
	}
}
#endif //CONFIG_IDLE_UART_OUT_TEST

/*======================================================================
 * Standardized UART loopback test suite (SPI_LB / I2C style)
 *
 * Adapted from ESP-IDF uart loopback matrix + Zephyr/CMSIS practices,
 * aligned with local spi_lb / i2c loopback PASS/FAIL reporting.
 *
 * Hardware note:
 *   - BK7259 UART has NO internal TX->RX loopback bit (unlike ESP
 *     uart_set_loop_back). External jumper TX<->RX is required.
 *   - uart_lb_flow additionally needs RTS<->CTS shorted.
 *
 * Commands:
 *   uart_lb      - baud/len/parity/stop/data_bits matrix + stress
 *   uart_lb_api  - timeout / empty read / oversized write / post-deinit I/O
 *   uart_lb_dma  - TX+RX DMA path loopback (if Kconfig enabled)
 *   uart_lb_flow - HW flowctrl loopback (TX-RX + RTS-CTS wired)
 *
 * Grep-friendly prefixes: UART_LB / UART_LB_API / UART_LB_DMA / UART_LB_FLOW
 *====================================================================*/

#define UART_LB_SEED_BASE     0x55415254u  /* "UART" */
#define UART_LB_HW_FIFO       128
#if defined(CONFIG_KFIFO_SIZE)
#define UART_LB_SAFE_LEN      CONFIG_KFIFO_SIZE  /* single-shot write then read */
#else
#define UART_LB_SAFE_LEN      128
#endif

typedef struct {
	uint32_t pass;
	uint32_t total;
	uint32_t skip;
} uart_lb_stat_t;

static uint8_t uart_lb_next_byte(uint32_t *state)
{
	*state = (*state) * 1103515245u + 12345u;
	return (uint8_t)((*state >> 16) & 0xff);
}

static void uart_lb_fill_pattern(uint8_t *buf, uint32_t len, uint32_t seed, uint8_t mask)
{
	uint32_t st = seed;
	for (uint32_t i = 0; i < len; i++) {
		buf[i] = uart_lb_next_byte(&st) & mask;
	}
}

static bool uart_lb_verify(const uint8_t *buf, uint32_t len, uint32_t seed, uint8_t mask)
{
	uint32_t st = seed;
	for (uint32_t i = 0; i < len; i++) {
		uint8_t exp = uart_lb_next_byte(&st) & mask;
		if ((buf[i] & mask) != exp) {
			uint32_t from = (i >= 8) ? (i - 8) : 0;
			CLI_LOGI("mismatch @%u (len=%u)\r\n", i, len);
			st = seed;
			for (uint32_t k = 0; k < from; k++) {
				(void)uart_lb_next_byte(&st);
			}
			CLI_LOGI("exp:");
			for (uint32_t k = from; k < from + 16 && k < len; k++) {
				CLI_LOGI(" %02x", uart_lb_next_byte(&st) & mask);
			}
			CLI_LOGI("\r\ngot:");
			for (uint32_t k = from; k < from + 16 && k < len; k++) {
				CLI_LOGI(" %02x", buf[k] & mask);
			}
			CLI_LOGI("\r\n");
			return false;
		}
	}
	return true;
}

static uint8_t uart_lb_data_mask(uart_data_bits_t bits)
{
	switch (bits) {
	case UART_DATA_5_BITS: return 0x1f;
	case UART_DATA_6_BITS: return 0x3f;
	case UART_DATA_7_BITS: return 0x7f;
	default:               return 0xff;
	}
}

static uint32_t uart_lb_xfer_timeout_ms(uint32_t baud, uint32_t len)
{
	uint32_t ms;

	if (baud == 0) {
		return 1000;
	}
	/* 12 bit-times/byte worst-case + margin */
	ms = (len * 12u * 1000u) / baud + 200;
	if (ms < 100) {
		ms = 100;
	}
	return ms;
}

static void uart_lb_build_config(uart_config_t *cfg, uint32_t baud,
				 uart_data_bits_t db, uart_parity_t par,
				 uart_stop_bits_t sb, uart_flow_control_t flow,
				 bool use_dma)
{
	os_memset(cfg, 0, sizeof(*cfg));
	cfg->baud_rate = baud;
	cfg->data_bits = db;
	cfg->parity = par;
	cfg->stop_bits = sb;
	cfg->flow_ctrl = flow;
	cfg->src_clk = UART_SCLK_XTAL_26M;
#if CONFIG_UART_RX_DMA
	cfg->rx_dma_en = use_dma ? UART_DMA_ENABLE : UART_DMA_DISABLE;
#else
	(void)use_dma;
#endif
#if CONFIG_UART_TX_DMA
	cfg->tx_dma_en = use_dma ? UART_DMA_ENABLE : UART_DMA_DISABLE;
#endif
}

static void uart_lb_flush_rx(uart_id_t id)
{
	uint8_t dump[64];
	int ret;

	do {
		ret = bk_uart_read_bytes(id, dump, sizeof(dump), 20);
	} while (ret > 0);
}

/*
 * bk_uart_read_bytes() returns whatever is already in the SW kfifo after at
 * most one wait for the first chunk (POSIX-style partial read). Loopback needs
 * a full-length transfer: keep reading until len bytes arrive or timeout.
 * write() also returns once TX FIFO accepts the last byte, while those bytes
 * may still be shifting out — so the caller must keep waiting.
 */
static int uart_lb_read_full(uart_id_t id, uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
	uint32_t got = 0;
	uint32_t start = rtos_get_time();

	while (got < len) {
		uint32_t elapsed = rtos_get_time() - start;
		uint32_t remain;
		int n;

		if (elapsed >= timeout_ms) {
			break;
		}
		remain = timeout_ms - elapsed;
		n = bk_uart_read_bytes(id, buf + got, len - got, remain);
		if (n < 0) {
			if (got == 0) {
				return n;
			}
			break;
		}
		if (n == 0) {
			break;
		}
		got += (uint32_t)n;
	}

	return (int)got;
}

/* Single-shot write-then-read. Safe only when len <= software RX kfifo. */
static bool uart_lb_run_one(uart_id_t id, uint32_t len, uint32_t baud,
			    uart_data_bits_t db, uart_parity_t par,
			    uart_stop_bits_t sb, bool use_dma, uint32_t seed)
{
	bool ok = false;
	uint8_t mask = uart_lb_data_mask(db);
	uint8_t *tx = (uint8_t *)os_malloc(len);
	uint8_t *rx = (uint8_t *)os_malloc(len);
	uart_config_t cfg;
	uint32_t to_ms;
	bk_err_t wret;
	int rlen;

	if (!tx || !rx) {
		CLI_LOGE("UART_LB: FAIL alloc (len=%u)\r\n", len);
		goto out_free;
	}
	if (len == 0 || len > UART_LB_SAFE_LEN) {
		CLI_LOGE("UART_LB: FAIL len=%u exceeds safe kfifo=%u\r\n",
			 len, (unsigned)UART_LB_SAFE_LEN);
		goto out_free;
	}

	uart_lb_build_config(&cfg, baud, db, par, sb, UART_FLOWCTRL_DISABLE, use_dma);
	uart_lb_fill_pattern(tx, len, seed, mask);
	os_memset(rx, 0x55, len);

	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGE("UART_LB: FAIL init\r\n");
		goto out_free;
	}
	bk_uart_enable_rx_interrupt(id);
	uart_lb_flush_rx(id);

	to_ms = uart_lb_xfer_timeout_ms(baud, len);
	wret = bk_uart_write_bytes(id, tx, len);
	if (wret != BK_OK) {
		CLI_LOGE("UART_LB: FAIL write ret=%d\r\n", wret);
		goto out_deinit;
	}

	rlen = uart_lb_read_full(id, rx, len, to_ms);
	if (rlen < 0) {
		CLI_LOGE("UART_LB: FAIL read ret=%d\r\n", rlen);
	} else if ((uint32_t)rlen != len) {
		CLI_LOGE("UART_LB: FAIL len mismatch sent=%u recv=%d\r\n", len, rlen);
	} else {
		ok = uart_lb_verify(rx, len, seed, mask);
	}

out_deinit:
	bk_uart_deinit(id);
out_free:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}

	CLI_LOGI("UART_LB: %s (len=%u baud=%u db=%u par=%u sb=%u dma=%u)\r\n",
		 ok ? "PASS" : "FAIL", len, baud, db, par, sb, use_dma ? 1 : 0);
	return ok;
}

/* Chunked loopback for totals larger than kfifo (write+read per chunk). */
static bool uart_lb_run_chunked(uart_id_t id, uint32_t total, uint32_t chunk,
				uint32_t baud, uint32_t seed)
{
	bool ok = true;
	uart_config_t cfg;
	uint8_t *tx;
	uint8_t *rx;
	uint32_t done = 0;
	uint32_t local_seed = seed;

	if (chunk == 0 || chunk > UART_LB_SAFE_LEN) {
		chunk = (UART_LB_SAFE_LEN >= 64) ? 64 : UART_LB_SAFE_LEN;
	}
	tx = (uint8_t *)os_malloc(chunk);
	rx = (uint8_t *)os_malloc(chunk);
	if (!tx || !rx) {
		CLI_LOGE("UART_LB: FAIL chunk alloc\r\n");
		ok = false;
		goto out_free;
	}

	uart_lb_build_config(&cfg, baud, UART_DATA_8_BITS, UART_PARITY_NONE,
			     UART_STOP_BITS_1, UART_FLOWCTRL_DISABLE, false);
	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGE("UART_LB: FAIL chunk init\r\n");
		ok = false;
		goto out_free;
	}
	bk_uart_enable_rx_interrupt(id);
	uart_lb_flush_rx(id);

	while (done < total) {
		uint32_t n = total - done;
		uint32_t to_ms;
		bk_err_t wret;
		int rlen;

		if (n > chunk) {
			n = chunk;
		}
		uart_lb_fill_pattern(tx, n, local_seed, 0xff);
		os_memset(rx, 0x55, n);
		to_ms = uart_lb_xfer_timeout_ms(baud, n);

		wret = bk_uart_write_bytes(id, tx, n);
		if (wret != BK_OK) {
			ok = false;
			break;
		}
		rlen = uart_lb_read_full(id, rx, n, to_ms);
		if (rlen < 0 || (uint32_t)rlen != n ||
		    !uart_lb_verify(rx, n, local_seed, 0xff)) {
			ok = false;
			break;
		}
		done += n;
		local_seed++;
	}

	bk_uart_deinit(id);
out_free:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
	CLI_LOGI("UART_LB: %s (chunked total=%u chunk=%u baud=%u)\r\n",
		 ok ? "PASS" : "FAIL", total, chunk, baud);
	return ok;
}

#if CONFIG_UART_STRESS_TEST
#define UART_LB_256_LEN             (256u)
#define UART_LB_256_RX_TASK_PRIO    (3u)  /* Higher priority than the CLI transmitter task */
#define UART_LB_256_RX_TASK_STACK   (2048u)

typedef struct {
	uart_id_t id;
	uint8_t *data;
	uint32_t len;
	uint32_t timeout_ms;
	beken_semaphore_t armed;
	beken_semaphore_t done;
	int read_len;
	bool finished;
} uart_lb_256_rx_ctx_t;

static void uart_lb_256_rx_task(beken_thread_arg_t arg)
{
	uart_lb_256_rx_ctx_t *ctx = (uart_lb_256_rx_ctx_t *)arg;

	rtos_set_semaphore(&ctx->armed);
	ctx->read_len = uart_lb_read_full(ctx->id, ctx->data, ctx->len, ctx->timeout_ms);
	rtos_set_semaphore(&ctx->done);
	__atomic_store_n(&ctx->finished, true, __ATOMIC_RELEASE);
	rtos_delete_thread(NULL);
}

static bool uart_lb_run_256(uart_id_t id, uint32_t baud, uint32_t seed)
{
	bool ok = false;
	bool armed_init = false;
	bool done_init = false;
	bool uart_inited = false;
	bool rx_created = false;
	uint8_t *tx = (uint8_t *)os_malloc(UART_LB_256_LEN);
	uint8_t *rx = (uint8_t *)os_malloc(UART_LB_256_LEN);
	beken_thread_t rx_thread = NULL;
	uart_lb_256_rx_ctx_t ctx;
	uart_config_t cfg;
	uint32_t timeout_ms = uart_lb_xfer_timeout_ms(baud, UART_LB_256_LEN);
	bk_err_t write_ret = BK_FAIL;

	if (!tx || !rx) {
		CLI_LOGE("UART_LB_256: FAIL alloc\r\n");
		goto out;
	}

	os_memset(&ctx, 0, sizeof(ctx));
	ctx.id = id;
	ctx.data = rx;
	ctx.len = UART_LB_256_LEN;
	ctx.timeout_ms = timeout_ms;
	ctx.read_len = BK_FAIL;
	uart_lb_fill_pattern(tx, UART_LB_256_LEN, seed, 0xff);
	os_memset(rx, 0x55, UART_LB_256_LEN);

	if (rtos_init_semaphore(&ctx.armed, 1) != kNoErr) {
		CLI_LOGE("UART_LB_256: FAIL armed semaphore\r\n");
		goto out;
	}
	armed_init = true;
	if (rtos_init_semaphore(&ctx.done, 1) != kNoErr) {
		CLI_LOGE("UART_LB_256: FAIL done semaphore\r\n");
		goto out;
	}
	done_init = true;

	uart_lb_build_config(&cfg, baud, UART_DATA_8_BITS, UART_PARITY_NONE,
			     UART_STOP_BITS_1, UART_FLOWCTRL_DISABLE, false);
	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGE("UART_LB_256: FAIL init\r\n");
		goto out;
	}
	uart_inited = true;
	bk_uart_enable_rx_interrupt(id);
	uart_lb_flush_rx(id);

	if (rtos_create_thread(&rx_thread, UART_LB_256_RX_TASK_PRIO, "uart_lb_256_rx",
			       uart_lb_256_rx_task, UART_LB_256_RX_TASK_STACK,
			       (beken_thread_arg_t)&ctx) != kNoErr) {
		CLI_LOGE("UART_LB_256: FAIL rx thread\r\n");
		goto out;
	}
	rx_created = true;

	if (rtos_get_semaphore(&ctx.armed, BEKEN_WAIT_FOREVER) != kNoErr) {
		CLI_LOGE("UART_LB_256: FAIL rx arm wait\r\n");
		goto out;
	}

	write_ret = bk_uart_write_bytes(id, tx, UART_LB_256_LEN);
	if (rtos_get_semaphore(&ctx.done, BEKEN_WAIT_FOREVER) != kNoErr) {
		CLI_LOGE("UART_LB_256: FAIL rx completion wait\r\n");
		goto out;
	}
	while (!__atomic_load_n(&ctx.finished, __ATOMIC_ACQUIRE)) {
		rtos_delay_milliseconds(1);
	}
	rx_created = false;

	if (write_ret != BK_OK) {
		CLI_LOGE("UART_LB_256: FAIL write ret=%d\r\n", write_ret);
	} else if (ctx.read_len < 0) {
		CLI_LOGE("UART_LB_256: FAIL read ret=%d\r\n", ctx.read_len);
	} else if ((uint32_t)ctx.read_len != UART_LB_256_LEN) {
		CLI_LOGE("UART_LB_256: FAIL len sent=%u recv=%d\r\n",
			 UART_LB_256_LEN, ctx.read_len);
	} else {
		ok = uart_lb_verify(rx, UART_LB_256_LEN, seed, 0xff);
	}

out:
	if (rx_created && rx_thread) {
		rtos_delete_thread(&rx_thread);
	}
	if (uart_inited) {
		bk_uart_deinit(id);
	}
	if (done_init) {
		rtos_deinit_semaphore(&ctx.done);
	}
	if (armed_init) {
		rtos_deinit_semaphore(&ctx.armed);
	}
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}

	CLI_LOGI("UART_LB_256: %s (len=%u baud=%u 8N1)\r\n",
		 ok ? "PASS" : "FAIL", UART_LB_256_LEN, baud);
	return ok;
}
#endif

static void uart_lb_run_matrix(uart_id_t id, bool full, bool use_dma)
{
	static const uint32_t lens_quick[] = {1, 16, 64, 128};
	static const uint32_t lens_full[]  = {1, 3, 16, 32, 63, 64, 127, 128};
	static const uint32_t bauds_quick[] = {115200, 921600};
	static const uint32_t bauds_full[]  = {
		UART_BAUDRATE_9600, UART_BAUDRATE_115200, UART_BAUDRATE_460800,
		UART_BAUDRATE_921600, UART_BAUDRATE_2000000
	};
	static const uart_parity_t pars[] = {
		UART_PARITY_NONE, UART_PARITY_ODD, UART_PARITY_EVEN
	};
	static const uart_stop_bits_t stops[] = {
		UART_STOP_BITS_1, UART_STOP_BITS_2
	};
	static const uart_data_bits_t dbits[] = {
		UART_DATA_5_BITS, UART_DATA_6_BITS, UART_DATA_7_BITS, UART_DATA_8_BITS
	};

	const uint32_t *lens = full ? lens_full : lens_quick;
	const uint32_t *bauds = full ? bauds_full : bauds_quick;
	uint32_t n_lens = full ? (sizeof(lens_full) / sizeof(lens_full[0]))
			       : (sizeof(lens_quick) / sizeof(lens_quick[0]));
	uint32_t n_bauds = full ? (sizeof(bauds_full) / sizeof(bauds_full[0]))
				: (sizeof(bauds_quick) / sizeof(bauds_quick[0]));
	uart_lb_stat_t st = {0, 0, 0};
	uint32_t seed = UART_LB_SEED_BASE;
	uint32_t i;

	/* Cap single-shot lens to available kfifo */
	for (i = 0; i < n_lens; i++) {
		if (lens[i] > UART_LB_SAFE_LEN) {
			continue;
		}
		st.total++;
		if (uart_lb_run_one(id, lens[i], UART_BAUDRATE_115200,
				    UART_DATA_8_BITS, UART_PARITY_NONE,
				    UART_STOP_BITS_1, use_dma, seed++)) {
			st.pass++;
		}
	}

	/* Baud sweep @ len=64 8N1 */
	for (i = 0; i < n_bauds; i++) {
		st.total++;
		if (uart_lb_run_one(id, 64, bauds[i],
				    UART_DATA_8_BITS, UART_PARITY_NONE,
				    UART_STOP_BITS_1, use_dma, seed++)) {
			st.pass++;
		}
	}

	/* Parity sweep */
	for (i = 0; i < sizeof(pars) / sizeof(pars[0]); i++) {
		st.total++;
		if (uart_lb_run_one(id, 64, UART_BAUDRATE_115200,
				    UART_DATA_8_BITS, pars[i],
				    UART_STOP_BITS_1, use_dma, seed++)) {
			st.pass++;
		}
	}

	/* Stop-bit sweep */
	for (i = 0; i < sizeof(stops) / sizeof(stops[0]); i++) {
		st.total++;
		if (uart_lb_run_one(id, 64, UART_BAUDRATE_115200,
				    UART_DATA_8_BITS, UART_PARITY_NONE,
				    stops[i], use_dma, seed++)) {
			st.pass++;
		}
	}

	/* Data-bits sweep (masked pattern) */
	for (i = 0; i < sizeof(dbits) / sizeof(dbits[0]); i++) {
		st.total++;
		if (uart_lb_run_one(id, 32, UART_BAUDRATE_115200,
				    dbits[i], UART_PARITY_NONE,
				    UART_STOP_BITS_1, use_dma, seed++)) {
			st.pass++;
		}
	}

	/* Large packet via chunked transfers (1B / FIFO / big) */
	{
		static const uint32_t big_totals_q[] = {256};
		static const uint32_t big_totals_f[] = {256, 512, 1024};
		const uint32_t *bigs = full ? big_totals_f : big_totals_q;
		uint32_t n_big = full ? (sizeof(big_totals_f) / sizeof(big_totals_f[0]))
				      : (sizeof(big_totals_q) / sizeof(big_totals_q[0]));

		for (i = 0; i < n_big; i++) {
			st.total++;
			if (uart_lb_run_chunked(id, bigs[i], UART_LB_HW_FIFO / 2,
						UART_BAUDRATE_115200, seed++)) {
				st.pass++;
			}
		}
	}

	/* Stress: random lengths */
	{
		uint32_t rounds = full ? 40 : 10;
		uint32_t r;

		for (r = 0; r < rounds; r++) {
			uint32_t len = 1 + (bk_rand() % UART_LB_SAFE_LEN);

			st.total++;
			if (uart_lb_run_one(id, len, UART_BAUDRATE_460800,
					    UART_DATA_8_BITS, UART_PARITY_NONE,
					    UART_STOP_BITS_1, use_dma, seed++)) {
				st.pass++;
			}
		}
	}

	/* Internal loopback: not available on this SoC */
	st.skip++;
	CLI_LOGI("UART_LB: SKIP internal-loopback (no HW TX->RX loopback bit)\r\n");

	CLI_LOGI("UART_LB SUMMARY: %u/%u PASS, skip=%u\r\n", st.pass, st.total, st.skip);
	if (st.pass == st.total) {
		CLI_LOGI("UART_LB: ALL PASS\r\n");
	}
}

static void cli_uart_lb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uart_id_t id;
	bool full = true;

	if (argc < 2) {
		cli_uart_help();
		return;
	}
	id = (uart_id_t)os_strtoul(argv[1], NULL, 10);
	if (argc >= 3 && os_strcmp(argv[2], "quick") == 0) {
		full = false;
	}
	CLI_LOGI("UART_LB START: id=%d %s (jumper TX<->RX required)\r\n",
		 id, full ? "full" : "quick");
	uart_lb_run_matrix(id, full, false);
}

#if CONFIG_UART_STRESS_TEST
static void cli_uart_lb_256_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	static const uint32_t required_bauds[] = {9600u, 38400u, 115200u};
	uart_id_t id;
	uint32_t selected_baud = 0;
	uint32_t id_value;
	uint32_t pass = 0;
	uint32_t total = 0;
	bool run_all = true;

	if (argc < 2 || argc > 3) {
		CLI_LOGE("UART_LB_256: usage: uart_lb_256 {id} [9600|38400|115200|all]\r\n");
		return;
	}

	if (!uart_test_parse_u32(argv[1], &id_value) ||
	    id_value >= (uint32_t)UART_ID_MAX) {
		CLI_LOGE("UART_LB_256: invalid uart id=%s\r\n", argv[1]);
		return;
	}
	id = (uart_id_t)id_value;
	if (id == UART_ID_0 || (uint32_t)id == (uint32_t)CONFIG_UART_PRINT_PORT) {
		CLI_LOGE("UART_LB_256: uart id=%u is reserved for debug output\r\n",
			 (uint32_t)id);
		return;
	}
	if (bk_uart_is_in_used(id)) {
		CLI_LOGE("UART_LB_256: uart id=%u is already in use\r\n", (uint32_t)id);
		return;
	}

	if (argc == 3 && os_strcmp(argv[2], "all") != 0) {
		if (!uart_test_parse_u32(argv[2], &selected_baud)) {
			CLI_LOGE("UART_LB_256: invalid baud=%s\r\n", argv[2]);
			return;
		}
		run_all = false;
		if (selected_baud != 9600u && selected_baud != 38400u &&
		    selected_baud != 115200u) {
			CLI_LOGE("UART_LB_256: unsupported baud=%u\r\n", selected_baud);
			return;
		}
	}

	CLI_LOGI("UART_LB_256 START: id=%u baud=%s (jumper TX<->RX required)\r\n",
		 (uint32_t)id, run_all ? "all" : argv[2]);
	for (uint32_t i = 0; i < sizeof(required_bauds) / sizeof(required_bauds[0]); i++) {
		uint32_t baud = required_bauds[i];

		if (!run_all && baud != selected_baud) {
			continue;
		}
		total++;
		if (uart_lb_run_256(id, baud, UART_LB_SEED_BASE + 0x2560u + i)) {
			pass++;
		}
	}

	CLI_LOGI("UART_LB_256 SUMMARY: %u/%u PASS\r\n", pass, total);
	if (total > 0 && pass == total) {
		CLI_LOGI("UART_LB_256: ALL PASS\r\n");
	}
}
#endif

/*--------------------- negative / API exception tests ---------------------*/
static void cli_uart_lb_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uart_id_t id = (argc >= 2) ? (uart_id_t)os_strtoul(argv[1], NULL, 10) : UART_ID_1;
	uart_lb_stat_t st = {0, 0, 0};
	uart_config_t cfg;
	uint8_t buf[16];
	uint8_t *big = NULL;
	int rlen;
	bk_err_t ret;

	CLI_LOGI("UART_LB_API START: id=%d\r\n", id);
	uart_lb_build_config(&cfg, UART_BAUDRATE_115200, UART_DATA_8_BITS,
			     UART_PARITY_NONE, UART_STOP_BITS_1,
			     UART_FLOWCTRL_DISABLE, false);

	/* 1. NULL config rejected */
	st.total++;
	if (bk_uart_init(id, NULL) != BK_OK) {
		st.pass++;
		CLI_LOGI("UART_LB_API: PASS null-config-rejected\r\n");
	} else {
		bk_uart_deinit(id);
		CLI_LOGI("UART_LB_API: FAIL null-config-rejected\r\n");
	}

	/* 2. invalid id rejected */
	st.total++;
	if (bk_uart_init(UART_ID_MAX, &cfg) != BK_OK) {
		st.pass++;
		CLI_LOGI("UART_LB_API: PASS invalid-id-rejected\r\n");
	} else {
		CLI_LOGI("UART_LB_API: FAIL invalid-id-rejected\r\n");
	}

	/* 3. RX timeout / empty read (no peer data) */
	st.total++;
	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGI("UART_LB_API: FAIL timeout-setup\r\n");
	} else {
		bk_uart_enable_rx_interrupt(id);
		uart_lb_flush_rx(id);
		rlen = bk_uart_read_bytes(id, buf, sizeof(buf), 50);
		if (rlen == BK_ERR_UART_RX_TIMEOUT) {
			st.pass++;
			CLI_LOGI("UART_LB_API: PASS rx-timeout-empty\r\n");
		} else {
			CLI_LOGI("UART_LB_API: FAIL rx-timeout-empty ret=%d\r\n", rlen);
		}
		bk_uart_deinit(id);
	}

	/* 4. Oversized write (>> HW FIFO) must complete without crash */
	st.total++;
	big = (uint8_t *)os_malloc(2048);
	if (!big) {
		CLI_LOGI("UART_LB_API: FAIL oversized-write alloc\r\n");
	} else if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGI("UART_LB_API: FAIL oversized-write init\r\n");
	} else {
		os_memset(big, 0xa5, 2048);
		ret = bk_uart_write_bytes(id, big, 2048);
		if (ret == BK_OK) {
			st.pass++;
			CLI_LOGI("UART_LB_API: PASS oversized-write-2048\r\n");
		} else {
			CLI_LOGI("UART_LB_API: FAIL oversized-write-2048 ret=%d\r\n", ret);
		}
		bk_uart_deinit(id);
	}
	if (big) {
		os_free(big);
	}

	/* 5. I/O after deinit (deinit mid-session) */
	st.total++;
	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGI("UART_LB_API: FAIL deinit-io setup\r\n");
	} else {
		bk_uart_deinit(id);
		ret = bk_uart_write_bytes(id, buf, 1);
		rlen = bk_uart_read_bytes(id, buf, 1, 20);
		if (ret == BK_ERR_UART_ID_NOT_INIT && rlen == BK_ERR_UART_ID_NOT_INIT) {
			st.pass++;
			CLI_LOGI("UART_LB_API: PASS io-after-deinit\r\n");
		} else {
			CLI_LOGI("UART_LB_API: FAIL io-after-deinit write=%d read=%d\r\n",
				 ret, rlen);
		}
	}

	CLI_LOGI("UART_LB_API SUMMARY: %u/%u PASS\r\n", st.pass, st.total);
	if (st.pass == st.total) {
		CLI_LOGI("UART_LB_API: ALL PASS\r\n");
	}
}

/*--------------------- DMA full-duplex loopback ---------------------*/
static void cli_uart_lb_dma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uart_id_t id;
	bool full = true;

	if (argc < 2) {
		cli_uart_help();
		return;
	}
	id = (uart_id_t)os_strtoul(argv[1], NULL, 10);
	if (argc >= 3 && os_strcmp(argv[2], "quick") == 0) {
		full = false;
	}

#if (CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA)
	CLI_LOGI("UART_LB_DMA START: id=%d %s (jumper TX<->RX required)\r\n",
		 id, full ? "full" : "quick");
	uart_lb_run_matrix(id, full, true);
#else
	(void)id;
	(void)full;
	CLI_LOGI("UART_LB_DMA: SKIP (need CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA)\r\n");
#endif
}

/*--------------------- HW flowctrl loopback ---------------------*/
static void cli_uart_lb_flow_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uart_id_t id = (argc >= 2) ? (uart_id_t)os_strtoul(argv[1], NULL, 10) : UART_ID_1;
	uart_lb_stat_t st = {0, 0, 0};
	uart_config_t cfg;
	uint8_t *tx;
	uint8_t *rx;
	const uint32_t len = 64;
	uint32_t seed = UART_LB_SEED_BASE + 0xF100u;
	bk_err_t wret;
	int rlen;

	CLI_LOGI("UART_LB_FLOW START: id=%d (wire TX<->RX AND RTS<->CTS)\r\n", id);

	tx = (uint8_t *)os_malloc(len);
	rx = (uint8_t *)os_malloc(len);
	if (!tx || !rx) {
		CLI_LOGI("UART_LB_FLOW: FAIL alloc\r\n");
		goto out_free;
	}

	uart_lb_build_config(&cfg, UART_BAUDRATE_115200, UART_DATA_8_BITS,
			     UART_PARITY_NONE, UART_STOP_BITS_1,
			     UART_FLOWCTRL_CTS_RTS, false);
	uart_lb_fill_pattern(tx, len, seed, 0xff);
	os_memset(rx, 0x55, len);

	st.total++;
	if (bk_uart_init(id, &cfg) != BK_OK) {
		CLI_LOGI("UART_LB_FLOW: FAIL init\r\n");
		goto out_free;
	}
	/* Ensure HW flow threshold is programmed */
	BK_LOG_ON_ERR(bk_uart_set_hw_flow_ctrl(id, 64));
	bk_uart_enable_rx_interrupt(id);
	uart_lb_flush_rx(id);

	wret = bk_uart_write_bytes(id, tx, len);
	rlen = uart_lb_read_full(id, rx, len, uart_lb_xfer_timeout_ms(UART_BAUDRATE_115200, len));
	if (wret == BK_OK && rlen == (int)len && uart_lb_verify(rx, len, seed, 0xff)) {
		st.pass++;
		CLI_LOGI("UART_LB_FLOW: PASS hw-flow-loopback\r\n");
	} else {
		CLI_LOGI("UART_LB_FLOW: FAIL hw-flow-loopback write=%d read=%d (check RTS<->CTS)\r\n",
			 wret, rlen);
	}

	bk_uart_disable_hw_flow_ctrl(id);
	bk_uart_deinit(id);

out_free:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
	CLI_LOGI("UART_LB_FLOW SUMMARY: %u/%u PASS\r\n", st.pass, st.total);
	if (st.total && st.pass == st.total) {
		CLI_LOGI("UART_LB_FLOW: ALL PASS\r\n");
	}
}

/*--------------------- UART TX+RX DMA loopback test suite --------------------
 *
 * uart_dma_test   {start|stop} {id} [baud] [len]   -- dual-task RX loopback
 * uart_dma_stress {start|stop} [id] [baud]         -- ping-pong integrity
 * uart_dma_cycle  {id} [baud] [iterations]         -- init/io/deinit cycle
 *
 * High baud (>2M) must run off APLL: the 7259 UART source clock is
 * DCO/XTAL_26M/APLL. XTAL cannot derive 4M (26/6 = 4.333M, 8.33% error)
 * while APLL at 120MHz divides exactly (120/30 = 4.000M). Requires
 * CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA and an external TX<->RX jumper.
 *====================================================================*/

#if (CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA)

/* Pick the clock the requested baud rate can actually be derived from.
 *
 * uart_hal_set_baud_rate() computes clk_div = clk / baud - 1, so the error
 * depends on how evenly the source clock divides:
 *   XTAL 26MHz @ 2Mbps -> 26/13   = 2.0000Mbps, 0.00% error
 *   XTAL 26MHz @ 4Mbps -> 26/6    = 4.3333Mbps, 8.33% error (unusable)
 *   APLL 120MHz @ 4Mbps -> 120/30 = 4.0000Mbps, 0.00% error
 *
 * So anything above 2Mbps has to run off the APLL. Stay on the XTAL below
 * that: the APLL is a chip-global resource shared with SPI and audio, so
 * bringing it up when the XTAL is exact would be a needless risk. */
#define UART_DMA_TEST_XTAL_MAX_BAUD  (2000000)

static uart_src_clk_t uart_dma_test_pick_clk(uint32_t baud)
{
	return (baud > UART_DMA_TEST_XTAL_MAX_BAUD) ? UART_SCLK_APLL : UART_SCLK_XTAL_26M;
}

static const char *uart_dma_test_clk_name(uart_src_clk_t clk)
{
	return (clk == UART_SCLK_APLL) ? "APLL" : "XTAL";
}

/* ============================================================
 * UART DMA Dual-Task Loopback Test
 *
 *   TX task --write--> UART TX DMA --loopback--> UART RX DMA
 *                |                              |
 *          TX ISR fires (FIFO empty = sent)     |
 *                |                              v
 *          set semaphore -----------------> RX task wakes
 *                                            read + verify
 *
 * Requires TX pin <-> RX pin externally loopback connected.
 *
 * Usage:
 *   uart_dma_test start {uart_id} [baud_rate] [test_len]
 *   uart_dma_test stop
 * ============================================================ */

#define DMA_TEST_DEFAULT_LEN      256
#define DMA_TEST_SEMA_TIMEOUT_MS  2000
#define DMA_TEST_READ_TIMEOUT_MS  2000
#define DMA_TEST_PASS_LOG_EVERY   512

typedef struct {
	uart_id_t uart_id;
	uint32_t  test_len;
	beken_semaphore_t round_start_sema;
	beken_thread_t    tx_thread;
	beken_thread_t    rx_thread;
	volatile bool     running;
	volatile uint32_t tx_round;
	volatile uint32_t rx_round;
	uint32_t pass_count;
	uint32_t fail_count;
} uart_dma_test_t;

static uart_dma_test_t s_dma_test = {0};

static void dma_test_fill_pattern(uint8_t *buf, uint32_t len, uint32_t pattern_id)
{
	for (uint32_t i = 0; i < len; i++) {
		if (pattern_id == 0) {
			buf[i] = (uint8_t)(0x55U ^ (i & 0xFF));
		} else {
			buf[i] = (uint8_t)(0xAAU ^ (i & 0xFF));
		}
	}
}

static bool dma_test_match_buffer(const uint8_t *buf_a, const uint8_t *buf_b,
				  uint32_t len, int *first_err)
{
	for (uint32_t i = 0; i < len; i++) {
		if (buf_a[i] != buf_b[i]) {
			if (first_err) {
				*first_err = (int)i;
			}
			return false;
		}
	}
	if (first_err) {
		*first_err = -1;
	}
	return true;
}

/* ---- TX Task: alternate two buffers and send via DMA ---- */
static void dma_test_tx_task(void *arg)
{
	uart_dma_test_t *t = (uart_dma_test_t *)arg;
	uint8_t *tx_buf[2] = {0};

	tx_buf[0] = (uint8_t *)os_malloc(t->test_len);
	tx_buf[1] = (uint8_t *)os_malloc(t->test_len);
	if (!tx_buf[0] || !tx_buf[1]) {
		CLI_LOGE("TX task: malloc %u bytes failed\r\n", t->test_len);
		goto tx_exit;
	}

	dma_test_fill_pattern(tx_buf[0], t->test_len, 0);
	dma_test_fill_pattern(tx_buf[1], t->test_len, 1);

	CLI_LOGW("TX task started (uart_id=%d, len=%u)\r\n", t->uart_id, t->test_len);

	while (t->running) {
		if (rtos_get_semaphore(&t->round_start_sema, DMA_TEST_SEMA_TIMEOUT_MS) != kNoErr) {
			if (t->running) {
				CLI_LOGW("[Round %u] TX: wait round start timeout\r\n",
					 (uint32_t)t->tx_round);
			}
			continue;
		}

		if (!t->running) {
			break;
		}

		uint32_t round = t->tx_round;
		uint32_t pattern_id = round & 0x1;

		/* Send one of two fixed buffers so stale reads are easy to identify. */
		bk_uart_write_bytes(t->uart_id, tx_buf[pattern_id], t->test_len);

		t->tx_round++;
	}

	os_free(tx_buf[0]);
	os_free(tx_buf[1]);
tx_exit:
	CLI_LOGW("TX task exiting\r\n");
	t->tx_thread = NULL;
	rtos_delete_thread(NULL);
}

/* ---- RX Task: read data, verify it, then release next TX round ---- */
static void dma_test_rx_task(void *arg)
{
	uart_dma_test_t *t = (uart_dma_test_t *)arg;
	uint8_t *rx_buf = (uint8_t *)os_malloc(t->test_len);
	uint8_t *expect_buf[2] = {0};

	expect_buf[0] = (uint8_t *)os_malloc(t->test_len);
	expect_buf[1] = (uint8_t *)os_malloc(t->test_len);
	if (!rx_buf || !expect_buf[0] || !expect_buf[1]) {
		CLI_LOGE("RX task: malloc %u bytes failed\r\n", t->test_len);
		goto rx_exit;
	}

	dma_test_fill_pattern(expect_buf[0], t->test_len, 0);
	dma_test_fill_pattern(expect_buf[1], t->test_len, 1);

	CLI_LOGW("RX task started (uart_id=%d, len=%u)\r\n", t->uart_id, t->test_len);

	while (t->running) {
		os_memset(rx_buf, 0, t->test_len);
		uint32_t total_read = 0;
		uint32_t retry = 0;
		while (t->running && total_read < t->test_len && retry < 3) {
			int ret = bk_uart_read_bytes(t->uart_id,
						     rx_buf + total_read,
						     t->test_len - total_read,
						     DMA_TEST_READ_TIMEOUT_MS);
			if (ret < 0) {
				if (!t->running) {
					break;
				}
				retry++;
				CLI_LOGW("[Round %u] RX: read timeout, got %u/%u\r\n",
					 (uint32_t)t->rx_round, total_read, t->test_len);
				continue;
			}
			total_read += (uint32_t)ret;
		}

		if (!t->running) {
			break;
		}

		uint32_t round = t->rx_round;
		uint32_t expect_pattern_id = round & 0x1;
		uint32_t other_pattern_id = expect_pattern_id ^ 0x1;
		bool pass = true;
		bool matched_other = false;
		int first_err = -1;
		int other_first_err = -1;

		if (total_read != t->test_len) {
			pass = false;
		} else {
			pass = dma_test_match_buffer(rx_buf, expect_buf[expect_pattern_id],
						     t->test_len, &first_err);
			if (!pass) {
				matched_other = dma_test_match_buffer(rx_buf, expect_buf[other_pattern_id],
								       t->test_len, &other_first_err);
			}
		}

		if (pass) {
			t->pass_count++;
			if ((round % DMA_TEST_PASS_LOG_EVERY) == 0) {
				CLI_LOGW("[Round %u] PASS (%u bytes, buffer%u)\r\n",
					 round, total_read, expect_pattern_id + 1);
			}
		} else {
			t->fail_count++;
			if (matched_other) {
				CLI_LOGE("[Round %u] FAIL: expect buffer%u but matched buffer%u\r\n",
					 round, expect_pattern_id + 1, other_pattern_id + 1);
			} else if (first_err >= 0) {
				uint8_t exp = expect_buf[expect_pattern_id][first_err];
				CLI_LOGE("[Round %u] FAIL @%d: expect buffer%u=0x%02X got=0x%02X\r\n",
					 round, first_err, expect_pattern_id + 1, exp, rx_buf[first_err]);
			} else {
				CLI_LOGE("[Round %u] FAIL: received %u/%u bytes\r\n",
					 round, total_read, t->test_len);
			}
		}

		t->rx_round++;

		if (t->round_start_sema) {
			rtos_set_semaphore(&t->round_start_sema);
		}
	}

	os_free(expect_buf[0]);
	os_free(expect_buf[1]);
	os_free(rx_buf);
rx_exit:
	CLI_LOGW("RX task exiting (pass=%u, fail=%u)\r\n", t->pass_count, t->fail_count);
	t->rx_thread = NULL;
	rtos_delete_thread(NULL);
}

static void cli_uart_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_uart_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		uint32_t baud_rate;
		uart_config_t config;

		if (s_dma_test.running) {
			CLI_LOGW("Test already running, stop first\r\n");
			return;
		}
		if (argc < 3) {
			CLI_LOGW("Usage: uart_dma_test start {uart_id} [baud_rate] [test_len]\r\n");
			return;
		}

		os_memset(&s_dma_test, 0, sizeof(s_dma_test));
		s_dma_test.uart_id = (uart_id_t)os_strtoul(argv[2], NULL, 10);
		baud_rate = (argc > 3) ? os_strtoul(argv[3], NULL, 10) : UART_BAUD_RATE;
		s_dma_test.test_len = (argc > 4) ? os_strtoul(argv[4], NULL, 10) : DMA_TEST_DEFAULT_LEN;

		if (s_dma_test.uart_id == CONFIG_UART_PRINT_PORT) {
			CLI_LOGW("UART%d is the print port, use a different one\r\n", s_dma_test.uart_id);
			return;
		}
		if (bk_uart_is_in_used(s_dma_test.uart_id)) {
			CLI_LOGW("UART%d already in use\r\n", s_dma_test.uart_id);
			return;
		}

		os_memset(&config, 0, sizeof(config));
		config.baud_rate = baud_rate;
		config.data_bits = UART_DATA_8_BITS;
		config.parity = UART_PARITY_NONE;
		config.stop_bits = UART_STOP_BITS_1;
		config.flow_ctrl = UART_FLOWCTRL_DISABLE;
		config.src_clk = uart_dma_test_pick_clk(baud_rate);
		config.rx_dma_en = UART_DMA_ENABLE;
		config.tx_dma_en = UART_DMA_ENABLE;

		if (bk_uart_init(s_dma_test.uart_id, &config) != BK_OK) {
			CLI_LOGE("UART%d init failed\r\n", s_dma_test.uart_id);
			return;
		}
		bk_uart_enable_rx_interrupt(s_dma_test.uart_id);

		if (rtos_init_semaphore(&s_dma_test.round_start_sema, 1) != kNoErr) {
			CLI_LOGE("Create round_start semaphore failed\r\n");
			bk_uart_deinit(s_dma_test.uart_id);
			return;
		}
		rtos_set_semaphore(&s_dma_test.round_start_sema);

		s_dma_test.running = true;

		CLI_LOGW("=== UART%d DMA Dual-Task Test ===\r\n", s_dma_test.uart_id);
		CLI_LOGW("  Baud: %u, Clk: %s, Len: %u, TX<->RX loopback\r\n",
			 baud_rate, uart_dma_test_clk_name(config.src_clk), s_dma_test.test_len);

		if (rtos_create_thread(&s_dma_test.rx_thread, 4, "dma_rx",
				       dma_test_rx_task, 4096, &s_dma_test) != kNoErr) {
			CLI_LOGE("Create RX task failed\r\n");
			s_dma_test.running = false;
			goto start_fail;
		}
		if (rtos_create_thread(&s_dma_test.tx_thread, 4, "dma_tx",
				       dma_test_tx_task, 4096, &s_dma_test) != kNoErr) {
			CLI_LOGE("Create TX task failed\r\n");
			s_dma_test.running = false;
			goto start_fail;
		}

		CLI_LOGW("Test running... use 'uart_dma_test stop' to end\r\n");
		return;

start_fail:
		if (s_dma_test.rx_thread) {
			rtos_delete_thread(&s_dma_test.rx_thread);
			s_dma_test.rx_thread = NULL;
		}
		rtos_deinit_semaphore(&s_dma_test.round_start_sema);
		s_dma_test.round_start_sema = NULL;
		bk_uart_deinit(s_dma_test.uart_id);
		return;

	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (!s_dma_test.running) {
			CLI_LOGW("No test running\r\n");
			return;
		}

		s_dma_test.running = false;

		if (s_dma_test.round_start_sema) {
			rtos_set_semaphore(&s_dma_test.round_start_sema);
		}

		rtos_delay_milliseconds(DMA_TEST_READ_TIMEOUT_MS + 200);

		if (s_dma_test.tx_thread) {
			rtos_delete_thread(&s_dma_test.tx_thread);
			s_dma_test.tx_thread = NULL;
		}
		if (s_dma_test.rx_thread) {
			rtos_delete_thread(&s_dma_test.rx_thread);
			s_dma_test.rx_thread = NULL;
		}

		bk_uart_deinit(s_dma_test.uart_id);

		if (s_dma_test.round_start_sema) {
			rtos_deinit_semaphore(&s_dma_test.round_start_sema);
			s_dma_test.round_start_sema = NULL;
		}

		CLI_LOGW("=== Test Stopped ===\r\n");
		CLI_LOGW("  Rounds: %u, PASS: %u, FAIL: %u\r\n",
			 (uint32_t)s_dma_test.rx_round, s_dma_test.pass_count, s_dma_test.fail_count);

	} else {
		cli_uart_help();
	}
}

/* ============================================================
 * UART DMA ping-pong integrity stress (half-duplex)
 *
 * Send one block via TX DMA, read it back via RX DMA, verify
 * mod-256 ramp. Requires TX pin <-> RX pin self-loopback.
 *
 * Usage:
 *   uart_dma_stress start [uart_id] [baud_rate]
 *   uart_dma_stress stop
 * ============================================================ */
#define STRESS_DEFAULT_UART_ID   UART_ID_1
#define STRESS_DEFAULT_BAUD      4000000
/* Must stay below CONFIG_UART_RX_DMA_KFIFO_SIZE, otherwise one ping-pong block
 * fills the RX kfifo, the driver disables RX, and the test reports short reads
 * that are a config artefact rather than a driver defect. The Kconfig default
 * is only 512, so a build that does not override it cannot run this test. */
#if (CONFIG_UART_RX_DMA_KFIFO_SIZE <= 1024)
#error "uart_dma_stress needs CONFIG_UART_RX_DMA_KFIFO_SIZE > 1024 (e.g. 4096)"
#endif
#define STRESS_BLOCK             1024
#define STRESS_READ_TIMEOUT_MS   200
#define STRESS_MAX_DISCONT       8
#define STRESS_REPORT_BLOCKS     512
#define STRESS_TASK_STACK        4096
#define STRESS_TASK_PRIO         2
#define STRESS_MAX_ERR_LOG       30

typedef struct {
	uart_id_t uart_id;
	uint32_t baud;
	beken_thread_t thread;
	volatile bool running;
	uint32_t tx_total;
	uint32_t rx_total;
	uint32_t err_total;
	uint32_t byteerr_total;
	uint32_t short_total;
	uint32_t resync_total;
} uart_dma_stress_t;

static uart_dma_stress_t s_dma_stress = {0};

static void uart_dma_stress_task(void *arg)
{
	uart_dma_stress_t *t = (uart_dma_stress_t *)arg;
	uint8_t *tx = NULL;
	uint8_t *rx = NULL;
	uint32_t counter = 0;
	uint32_t block = 0;
	uint32_t burst_run = 0;
	uart_config_t cfg = {0};

	tx = (uint8_t *)os_malloc(STRESS_BLOCK);
	rx = (uint8_t *)os_malloc(STRESS_BLOCK);
	if (!tx || !rx) {
		CLI_LOGE("uart_dma_stress: malloc failed\r\n");
		goto exit;
	}

	cfg.baud_rate = t->baud;
	cfg.data_bits = UART_DATA_8_BITS;
	cfg.parity = UART_PARITY_NONE;
	cfg.stop_bits = UART_STOP_BITS_1;
	cfg.flow_ctrl = UART_FLOWCTRL_DISABLE;
	cfg.src_clk = uart_dma_test_pick_clk(t->baud);
	cfg.rx_dma_en = UART_DMA_ENABLE;
	cfg.tx_dma_en = UART_DMA_ENABLE;

	if (bk_uart_init(t->uart_id, &cfg) != BK_OK) {
		CLI_LOGE("uart_dma_stress: uart(%d) init failed\r\n", t->uart_id);
		goto exit;
	}
	bk_uart_enable_rx_interrupt(t->uart_id);

	CLI_LOGW("uart_dma_stress START id=%d baud=%u clk=%s block=%d\r\n",
		 t->uart_id, t->baud,
		 uart_dma_test_clk_name(cfg.src_clk),
		 STRESS_BLOCK);

	/* Drain stale RX (bootloader echo on loopback) before first write. */
	{
		int drained = 0;
		while (1) {
			int n = bk_uart_read_bytes(t->uart_id, rx, STRESS_BLOCK, 100);
			if (n <= 0) {
				break;
			}
			drained += n;
			if (drained > 64 * 1024) {
				break;
			}
		}
		CLI_LOGW("uart_dma_stress DRAIN stale rx=%d\r\n", drained);
	}

	while (t->running) {
		int off = 0;
		int nbad = 0;
		int discont = 0;
		int offs[6];

		for (int i = 0; i < STRESS_BLOCK; i++) {
			tx[i] = (uint8_t)(counter + i);
		}
		counter += STRESS_BLOCK;

		if (bk_uart_write_bytes(t->uart_id, tx, STRESS_BLOCK) != BK_OK) {
			t->err_total++;
			if (t->err_total <= STRESS_MAX_ERR_LOG) {
				CLI_LOGE("uart_dma_stress write failed blk=%u\r\n", block);
			}
			continue;
		}
		t->tx_total += STRESS_BLOCK;

		while (off < STRESS_BLOCK) {
			int n = bk_uart_read_bytes(t->uart_id, rx + off,
						   STRESS_BLOCK - off, STRESS_READ_TIMEOUT_MS);
			if (n <= 0) {
				break;
			}
			off += n;
		}
		t->rx_total += (off > 0) ? (uint32_t)off : 0;

		if (off != STRESS_BLOCK) {
			t->short_total++;
			if (burst_run++ == 0) {
				CLI_LOGW("uart_dma_stress ANOMALY start blk=%u kind=short got=%d/%d\r\n",
					 block, off, STRESS_BLOCK);
			}
			while (bk_uart_read_bytes(t->uart_id, rx, STRESS_BLOCK, 20) > 0) {
				;
			}
		} else {
			for (int i = 0; i < STRESS_BLOCK; i++) {
				if (rx[i] != tx[i]) {
					if (nbad < 6) {
						offs[nbad] = i;
					}
					nbad++;
				}
			}
			if (nbad) {
				for (int i = 1; i < STRESS_BLOCK; i++) {
					if (rx[i] != (uint8_t)(rx[i - 1] + 1)) {
						discont++;
					}
				}
				if (discont <= STRESS_MAX_DISCONT) {
					int flushed = 0;
					int quiet = 0;
					while (quiet < 2) {
						if (bk_uart_read_bytes(t->uart_id, rx, STRESS_BLOCK, 20) > 0) {
							flushed++;
							quiet = 0;
							if (flushed > 128) {
								break;
							}
						} else {
							quiet++;
						}
					}
					t->resync_total++;
					if (burst_run++ == 0) {
						CLI_LOGW("uart_dma_stress ANOMALY start blk=%u kind=frame shift=0x%02x discont=%d\r\n",
							 block, rx[0], discont);
					}
				} else {
					t->byteerr_total += (uint32_t)nbad;
					t->err_total++;
					if (t->err_total <= STRESS_MAX_ERR_LOG) {
						CLI_LOGE("uart_dma_stress MISMATCH blk=%u nbad=%d discont=%d offs=%d,%d,%d\r\n",
							 block, nbad, discont,
							 offs[0], (nbad > 1) ? offs[1] : -1, (nbad > 2) ? offs[2] : -1);
					}
				}
			} else if (burst_run) {
				CLI_LOGW("uart_dma_stress ANOMALY end blk=%u run=%u shorts=%u resyncs=%u\r\n",
					 block, burst_run, t->short_total, t->resync_total);
				burst_run = 0;
			}
		}

		if ((++block % STRESS_REPORT_BLOCKS) == 0) {
			CLI_LOGW("uart_dma_stress %s blocks=%u tx=%u rx=%u byteerrs=%u shorts=%u resyncs=%u\r\n",
				 t->byteerr_total ? "FAIL" : "PASS",
				 block, t->tx_total, t->rx_total,
				 t->byteerr_total, t->short_total, t->resync_total);
		}
	}

	bk_uart_deinit(t->uart_id);
exit:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
	CLI_LOGW("uart_dma_stress STOPPED blocks_approx err=%u byteerrs=%u shorts=%u resyncs=%u\r\n",
		 t->err_total, t->byteerr_total, t->short_total, t->resync_total);
	t->thread = NULL;
	t->running = false;
	rtos_delete_thread(NULL);
}

static void cli_uart_dma_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_uart_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (s_dma_stress.running || s_dma_stress.thread) {
			CLI_LOGW("uart_dma_stress already running, stop first\r\n");
			return;
		}
		if (s_dma_test.running) {
			CLI_LOGW("uart_dma_test is running, stop it first\r\n");
			return;
		}

		os_memset(&s_dma_stress, 0, sizeof(s_dma_stress));
		s_dma_stress.uart_id = (argc > 2) ? (uart_id_t)os_strtoul(argv[2], NULL, 10)
						  : STRESS_DEFAULT_UART_ID;
		s_dma_stress.baud = (argc > 3) ? os_strtoul(argv[3], NULL, 10)
					       : STRESS_DEFAULT_BAUD;

		if (s_dma_stress.uart_id == CONFIG_UART_PRINT_PORT) {
			CLI_LOGW("UART%d is the print port, use a different one\r\n", s_dma_stress.uart_id);
			return;
		}
		if (bk_uart_is_in_used(s_dma_stress.uart_id)) {
			CLI_LOGW("UART%d already in use\r\n", s_dma_stress.uart_id);
			return;
		}

		s_dma_stress.running = true;
		if (rtos_create_thread(&s_dma_stress.thread, STRESS_TASK_PRIO, "uart_dma_stress",
				       uart_dma_stress_task,
				       STRESS_TASK_STACK, &s_dma_stress) != kNoErr) {
			CLI_LOGE("create uart_dma_stress task failed\r\n");
			s_dma_stress.running = false;
			return;
		}
		CLI_LOGW("uart_dma_stress running (loopback TX<->RX), use 'uart_dma_stress stop'\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (!s_dma_stress.running && !s_dma_stress.thread) {
			CLI_LOGW("uart_dma_stress not running\r\n");
			return;
		}
		s_dma_stress.running = false;
		for (int i = 0; i < 50 && s_dma_stress.thread; i++) {
			rtos_delay_milliseconds(100);
		}
		if (s_dma_stress.thread) {
			CLI_LOGW("uart_dma_stress task still alive, force delete\r\n");
			rtos_delete_thread(&s_dma_stress.thread);
			s_dma_stress.thread = NULL;
			bk_uart_deinit(s_dma_stress.uart_id);
		}
		CLI_LOGW("uart_dma_stress stopped: tx=%u rx=%u byteerrs=%u shorts=%u resyncs=%u\r\n",
			 s_dma_stress.tx_total, s_dma_stress.rx_total,
			 s_dma_stress.byteerr_total, s_dma_stress.short_total, s_dma_stress.resync_total);
	} else {
		cli_uart_help();
	}
}

/* ============================================================
 * UART DMA init/deinit cycle test
 *
 * Repeatedly init the UART with DMA, move one block over the
 * loopback, then deinit. Catches resources that are not released
 * on teardown.
 *
 * Usage:
 *   uart_dma_cycle {uart_id} [baud_rate] [iterations]
 * ============================================================ */
#define CYCLE_DEFAULT_ITERATIONS  20
#define CYCLE_BLOCK               256
#define CYCLE_READ_TIMEOUT_MS     200

static void cli_uart_dma_cycle_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uart_id_t uart_id;
	uint32_t baud;
	uint32_t iterations;
	uint32_t init_fail = 0;
	uint32_t io_fail = 0;
	uint8_t *tx = NULL;
	uint8_t *rx = NULL;
	uart_config_t cfg = {0};

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_uart_help();
		return;
	}

	uart_id = (uart_id_t)os_strtoul(argv[1], NULL, 10);
	baud = (argc > 2) ? os_strtoul(argv[2], NULL, 10) : STRESS_DEFAULT_BAUD;
	iterations = (argc > 3) ? os_strtoul(argv[3], NULL, 10) : CYCLE_DEFAULT_ITERATIONS;

	if (uart_id == CONFIG_UART_PRINT_PORT) {
		CLI_LOGW("UART%d is the print port, use a different one\r\n", uart_id);
		return;
	}
	if (s_dma_stress.running || s_dma_test.running) {
		CLI_LOGW("another uart dma test is running, stop it first\r\n");
		return;
	}

	tx = (uint8_t *)os_malloc(CYCLE_BLOCK);
	rx = (uint8_t *)os_malloc(CYCLE_BLOCK);
	if (!tx || !rx) {
		CLI_LOGE("uart_dma_cycle: malloc failed\r\n");
		goto exit;
	}
	for (uint32_t i = 0; i < CYCLE_BLOCK; i++) {
		tx[i] = (uint8_t)i;
	}

	cfg.baud_rate = baud;
	cfg.data_bits = UART_DATA_8_BITS;
	cfg.parity = UART_PARITY_NONE;
	cfg.stop_bits = UART_STOP_BITS_1;
	cfg.flow_ctrl = UART_FLOWCTRL_DISABLE;
	cfg.src_clk = uart_dma_test_pick_clk(baud);
	cfg.rx_dma_en = UART_DMA_ENABLE;
	cfg.tx_dma_en = UART_DMA_ENABLE;

	CLI_LOGW("uart_dma_cycle START id=%d baud=%u clk=%s iterations=%u\r\n",
		 uart_id, baud, uart_dma_test_clk_name(cfg.src_clk), iterations);

	for (uint32_t n = 0; n < iterations; n++) {
		int off = 0;

		if (bk_uart_init(uart_id, &cfg) != BK_OK) {
			init_fail++;
			CLI_LOGE("uart_dma_cycle iter=%u init failed\r\n", n);
			continue;
		}
		bk_uart_enable_rx_interrupt(uart_id);
		uart_lb_flush_rx(uart_id);

		if (bk_uart_write_bytes(uart_id, tx, CYCLE_BLOCK) != BK_OK) {
			io_fail++;
			CLI_LOGE("uart_dma_cycle iter=%u write failed\r\n", n);
		} else {
			while (off < CYCLE_BLOCK) {
				int r = bk_uart_read_bytes(uart_id, rx + off,
							   CYCLE_BLOCK - off, CYCLE_READ_TIMEOUT_MS);
				if (r <= 0) {
					break;
				}
				off += r;
			}
			if (off != CYCLE_BLOCK ||
			    !dma_test_match_buffer(rx, tx, CYCLE_BLOCK, NULL)) {
				io_fail++;
				CLI_LOGE("uart_dma_cycle iter=%u io mismatch got=%d/%d\r\n",
					 n, off, CYCLE_BLOCK);
			}
		}

		while (!bk_uart_is_tx_over(uart_id)) {
			rtos_delay_milliseconds(1);
		}

		bk_uart_deinit(uart_id);
	}

	CLI_LOGW("uart_dma_cycle %s iterations=%u init_fail=%u io_fail=%u\r\n",
		 (init_fail || io_fail) ? "FAIL" : "PASS",
		 iterations, init_fail, io_fail);

exit:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
}

#endif /* CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA */

#define UART_CMD_CNT (sizeof(s_uart_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_uart_commands[] = {
        {"uart_driver", "{init|deinit}", cli_uart_driver_cmd},
        {"uart_dma", "{tx|rx} {uart0|uart1|uart2|uart3}", cli_uart_dma_cmd},
        {"uart", "uart {id} {init|deinit|write|read|write_string|dump_statis} [...]", cli_uart_cmd},
        {"uart_config", "uart_config {id} {baud_rate|data_bits|clk_select} [...]", cli_uart_config_cmd},
        {"uart_int", "uart_int {id} {enable|disable|reg} {tx|rx}", cli_uart_int_cmd},
        {"uart_lb", "uart_lb {id} [quick|full]", cli_uart_lb_cmd},
        {"uart_lb_api", "uart_lb_api {id}", cli_uart_lb_api_cmd},
        {"uart_lb_dma", "uart_lb_dma {id} [quick|full]", cli_uart_lb_dma_cmd},
#if (CONFIG_UART_TX_DMA && CONFIG_UART_RX_DMA)
        {"uart_dma_test", "uart_dma_test {start|stop} {id} [baud] [len]", cli_uart_dma_test_cmd},
        {"uart_dma_stress", "uart_dma_stress {start|stop} [id] [baud]", cli_uart_dma_stress_cmd},
        {"uart_dma_cycle", "uart_dma_cycle {id} [baud] [iterations]", cli_uart_dma_cycle_cmd},
#endif
        {"uart_lb_flow", "uart_lb_flow {id}", cli_uart_lb_flow_cmd},
#if CONFIG_UART_STRESS_TEST
#if CONFIG_SHELL_ASYNCLOG
        {"uart_log_test", "uart_log_test [count] [interval_ms]", cli_uart_log_test_cmd},
#endif
        {"uart_lb_256", "uart_lb_256 {id} [9600|38400|115200|all]", cli_uart_lb_256_cmd},
#endif
#if CONFIG_IDLE_UART_OUT_TEST
        {"uart_test", "{idle_start|idle_stop} {uart0|uart1|uart2}", cli_uart_test_cmd},
#endif //CONFIG_IDLE_UART_OUT_TEST

#if CONFIG_FACIAL_RECOGN
        {"fr_test", "start|enroll", cli_fr_cmd},
#endif
};

int bk_uart_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_uart_driver_init());
	return cli_register_module_test_feature(s_uart_commands, UART_CMD_CNT);
}

