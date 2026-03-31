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
#include <driver/trng.h>
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
			BK_LOG_ON_ERR(bk_trng_driver_init());
			BK_LOG_ON_ERR(bk_trng_start());
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
			BK_LOG_ON_ERR(bk_trng_stop());
			CLI_LOGD("idle_uart_out task stop\n");
		} else {
			CLI_LOGD("PLEASE start task FIRST!!!\n");
		}
		return;
	}
}
#endif //CONFIG_IDLE_UART_OUT_TEST

typedef struct {
    uart_id_t id;
    uint32_t baud_rate;
} uart_loopback_config_t;

#define UART_LOOPBACK_DELAY_MS  10
#define UART_LOOPBACK_TIMEOUT_MS 1000
#define UART_LOOPBACK_BUF_SIZE  128

static volatile bool s_loopback_running = false;
static beken_thread_t s_loopback_thread = NULL;
static void uart_loopback_task(beken_thread_arg_t arg)
{
    uart_loopback_config_t *cfg = (uart_loopback_config_t *)arg;
    uart_id_t uart_id = cfg->id;
    uint32_t baud_rate = cfg->baud_rate;
    os_free(cfg);

    const uart_config_t config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_FLOWCTRL_DISABLE,
        .src_clk = UART_SCLK_XTAL_26M
    };

    bk_err_t ret = bk_uart_init(uart_id, &config);
    if (BK_OK != ret) {
        BK_DUMP_OUT("uart init failed\r\n");
        rtos_delete_thread(NULL);
        return;
    }
    bk_uart_enable_rx_interrupt(uart_id);

    uint8_t send_data[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    uint8_t recv_data[UART_LOOPBACK_BUF_SIZE];
    uint32_t loop_cnt = 0;
    uint32_t fail_cnt = 0;
    uint32_t send_len = strlen((char *)send_data);

    CLI_LOGI("uart loopback task started\r\n");

    while (s_loopback_running) {
        os_memset(recv_data, 0, sizeof(recv_data));

        ret = bk_uart_write_bytes(uart_id, send_data, send_len);
        if (BK_OK != ret) {
            CLI_LOGI("[loopback] write failed, loop=%u\r\n", loop_cnt);
            fail_cnt++;
            rtos_delay_milliseconds(UART_LOOPBACK_DELAY_MS);
            loop_cnt++;
            continue;
        }

        int32_t recv_len = bk_uart_read_bytes(uart_id, recv_data,
                                            sizeof(recv_data), UART_LOOPBACK_TIMEOUT_MS);
        if (recv_len < 0) {
            CLI_LOGI("[loopback] read failed, loop=%u\r\n", loop_cnt);
            fail_cnt++;
        } else if ((uint32_t)recv_len != send_len) {
            CLI_LOGI("[loopback] length mismatch: sent=%u recv=%d, loop=%u\r\n",
                        send_len, recv_len, loop_cnt);
            fail_cnt++;
        } else if (os_memcmp(send_data, recv_data, send_len) != 0) {
            CLI_LOGI("[loopback] data mismatch, loop=%u\r\n", loop_cnt);
            fail_cnt++;
        } else {
            CLI_LOGI("[loopback] OK, loop=%u\r\n", loop_cnt);
        }

        loop_cnt++;
        rtos_delay_milliseconds(UART_LOOPBACK_DELAY_MS);
    }

    CLI_LOGI("uart loopback task stopped, total=%u fail=%u\r\n", loop_cnt, fail_cnt);
    s_loopback_thread = NULL;
    rtos_delete_thread(NULL);
}

static void cli_uart_loopback_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        CLI_LOGI("usage: uart_loopback {start|stop} [uart_id] [baud_rate]\r\n");
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (s_loopback_running) {
            CLI_LOGI("uart loopback already running\r\n");
            return;
        }

        uart_loopback_config_t *cfg = os_zalloc(sizeof(uart_loopback_config_t));
        if (!cfg) {
            CLI_LOGI("alloc loopback config failed\r\n");
            return;
        }
        cfg->id = (argc >= 3) ? (uart_id_t)os_strtoul(argv[2], NULL, 10) : UART_ID_1;
        cfg->baud_rate = (argc >= 4) ? os_strtoul(argv[3], NULL, 10) : UART_BAUD_RATE;

        s_loopback_running = true;
        bk_err_t ret = rtos_create_thread(&s_loopback_thread, 5, "uart_loopback",
                                          uart_loopback_task, 2048, (beken_thread_arg_t)cfg);
        if (BK_OK != ret) {
            CLI_LOGI("create loopback thread failed\r\n");
            s_loopback_running = false;
            os_free(cfg);
        } else {
            CLI_LOGI("uart loopback started, id=%d baud=%u\r\n",
                        cfg->id, cfg->baud_rate);
        }
    } else if (os_strcmp(argv[1], "stop") == 0) {
        if (!s_loopback_running) {
            CLI_LOGI("uart loopback not running\r\n");
            return;
        }
        s_loopback_running = false;
        CLI_LOGI("uart loopback stopping...\r\n");
    } else {
        CLI_LOGI("usage: uart_loopback {start|stop} [uart_id] [baud_rate]\r\n");
    }
}

#define UART_CMD_CNT (sizeof(s_uart_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_uart_commands[] = {
        {"uart_driver", "{init|deinit}", cli_uart_driver_cmd},
        {"uart_dma", "{tx|rx} {uart0|uart1|uart2|uart3}", cli_uart_dma_cmd},
        {"uart", "uart {id} {init|deinit|write|read|write_string|dump_statis} [...]", cli_uart_cmd},
        {"uart_config", "uart_config {id} {baud_rate|data_bits|clk_select} [...]", cli_uart_config_cmd},
        {"uart_int", "uart_int {id} {enable|disable|reg} {tx|rx}", cli_uart_int_cmd},
        {"uart_loopback", "uart_loopback {start|stop} [uart_id] [baud_rate]", cli_uart_loopback_cmd},
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

