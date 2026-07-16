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
#include <driver/spi.h>
#include <components/bk_platform.h>
#include <driver/uart.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/hal/hal_io_matrix_types.h>
#include <common/bk_err.h>
#include "../gpio/v2px/gpio_driver_base.h"

static void cli_spi_help(void)
{
	CLI_LOGD("spi_driver {init|deinit}\r\n");
	CLI_LOGD("spi {id} {init} [mode] [bit_width] [bit_width] [cpol] [cpha] [wire_mode] [baud_rate] [bit_order]\r\n");
	CLI_LOGD("spi {id} {deinit} \r\n");
	CLI_LOGD("spi {id} {write|read|write_async|read_async} [buf_len]\r\n");
	CLI_LOGD("spi_lb {id} [quick|full]                    -- single board loopback (jumper MOSI<->MISO)\r\n");
	CLI_LOGD("spi_peer {master|slave} {id} {baud} {rounds} {data_len} [gap_ms] [dma|fifo]  -- dual board peer\r\n");
	CLI_LOGD("spi_peer stop\r\n");
	CLI_LOGD("spi_api_test {id}                            -- negative / API validation test\r\n");
	CLI_LOGD("spi_flash {id} {readid|erase|read|write} {addr} {len}  -- compat/debug, needs CONFIG_SPI_MST_FLASH\r\n");
}

static void cli_spi_rx_isr(spi_id_t id, void *param)
{
	CLI_LOGD("spi_rx_isr(%d)\n", id);
}

static void cli_spi_tx_isr(spi_id_t id, void *param)
{
	CLI_LOGD("spi_tx_isr(%d)\n", id);
}

static void cli_spi_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_spi_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_spi_driver_init());
		CLI_LOGD("spi driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_spi_driver_deinit());
		CLI_LOGD("spi driver deinit\n");
	} else {
		cli_spi_help();
		return;
	}
}

static void cli_spi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_spi_help();
		return;
	}
	uint32_t spi_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "init") == 0) {
		spi_config_t config = {0};
		if (os_strcmp(argv[3], "master") == 0) {
			config.role = SPI_ROLE_MASTER;
		} else {
			config.role = SPI_ROLE_SLAVE;
		}
		if (os_strtoul(argv[4], NULL, 10) == 8) {
			config.bit_width = SPI_BIT_WIDTH_8BITS;
		} else {
			config.bit_width = SPI_BIT_WIDTH_16BITS;
		}
		config.polarity = os_strtoul(argv[5], NULL, 10);
		config.phase = os_strtoul(argv[6], NULL, 10);
		if (os_strtoul(argv[7], NULL, 10) == 3) {
			config.wire_mode = SPI_3WIRE_MODE;
		} else {
			config.wire_mode = SPI_4WIRE_MODE;
		}
		config.baud_rate = os_strtoul(argv[8], NULL, 10);
		if (os_strcmp(argv[9], "LSB") == 0) {
			config.bit_order = SPI_LSB_FIRST;
		} else {
			config.bit_order = SPI_MSB_FIRST;
		}
#if (CONFIG_SPI_BYTE_INTERVAL)
		config.byte_interval = 1;
#endif
#if CONFIG_SPI_DMA
		config.dma_mode = os_strtoul(argv[10], NULL, 10);
		if (spi_id == 1){
			config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1);
			config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1_RX);
#if (SOC_SPI_UNIT_NUM > 2)
		} else if (spi_id == 2) {
			config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI2);
			config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI2_RX);
#endif
#if (SOC_SPI_UNIT_NUM > 3)
		} else if (spi_id == 3) {
			config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI3);
			config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI3_RX);
#endif
		} else {
			config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
			config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
		}
		if (os_strtoul(argv[4], NULL, 10) == 8) {
			config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
			config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
		} else {
			config.spi_tx_dma_width = DMA_DATA_WIDTH_16BITS;
			config.spi_rx_dma_width = DMA_DATA_WIDTH_16BITS;
		}
#endif
		BK_LOG_ON_ERR(bk_spi_init(spi_id, &config));
		CLI_LOGD("spi init, spi_id=%d\n", spi_id);
	} else if (os_strcmp(argv[2], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_spi_deinit(spi_id));
		CLI_LOGD("spi deinit, spi_id=%d\n", spi_id);
	} else if (os_strcmp(argv[2], "write") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *send_data = (uint8_t *)os_zalloc(buf_len);
		if (send_data == NULL) {
			CLI_LOGE("send buffer malloc failed\r\n");
			return;
		}
		for (int i = 0; i < buf_len; i++) {
			send_data[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_spi_write_bytes(spi_id, send_data, buf_len));
		if (send_data) {
			os_free(send_data);
		}
		send_data = NULL;
		CLI_LOGD("spi write bytes, spi_id=%d, data_len=%d\n", spi_id, buf_len);
	} else if (os_strcmp(argv[2], "read") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
		if (recv_data == NULL) {
			CLI_LOGE("recv buffer malloc failed\r\n");
			return;
		}
		os_memset(recv_data, 0xff, buf_len);
		BK_LOG_ON_ERR(bk_spi_read_bytes(spi_id, recv_data, buf_len));
		CLI_LOGD("spi read, spi_id=%d, size:%d\n", spi_id, buf_len);
		for (int i = 0; i < buf_len; i++) {
			CLI_LOGD("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
		}
		if (recv_data) {
			os_free(recv_data);
		}
		recv_data = NULL;
	} else if (os_strcmp(argv[2], "write_async") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *send_data = (uint8_t *)os_zalloc(buf_len);
		if (send_data == NULL) {
			CLI_LOGE("send buffer malloc failed\r\n");
			return;
		}
		for (int i = 0; i < buf_len; i++) {
			send_data[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_spi_write_bytes_async(spi_id, send_data, buf_len));
		if (send_data) {
			os_free(send_data);
		}
		send_data = NULL;
		CLI_LOGD("spi write bytes async, spi_id=%d, data_len=%d\n", spi_id, buf_len);
	} else if (os_strcmp(argv[2], "read_async") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
		if (recv_data == NULL) {
			CLI_LOGE("recv buffer malloc failed\r\n");
			return;
		}
		os_memset(recv_data, 0xff, buf_len);
		BK_LOG_ON_ERR(bk_spi_read_bytes_async(spi_id, recv_data, buf_len));
		CLI_LOGD("spi read async, spi_id=%d, size:%d\n", spi_id, buf_len);
		for (int i = 0; i < buf_len; i++) {
			CLI_LOGD("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
		}
		if (recv_data) {
			os_free(recv_data);
		}
		recv_data = NULL;
	}
#if CONFIG_SPI_DMA
	else if (os_strcmp(argv[2], "dma_write") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *send_data = (uint8_t *)os_zalloc(buf_len);
		if (send_data == NULL) {
			CLI_LOGE("send buffer malloc failed\r\n");
			return;
		}
		for (int i = 0; i < buf_len; i++) {
			send_data[i] = i & 0xff;
		}
		BK_LOG_ON_ERR(bk_spi_dma_write_bytes(spi_id, send_data, buf_len));
		if (send_data) {
			os_free(send_data);
		}
		send_data = NULL;
		CLI_LOGD("spi dma send, spi_id=%d, data_len=%d\n", spi_id, buf_len);
	} else if (os_strcmp(argv[2], "dma_read") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
		if (recv_data == NULL) {
			CLI_LOGE("recv buffer malloc failed\r\n");
			return;
		}
		os_memset(recv_data, 0xff, buf_len);
		BK_LOG_ON_ERR(bk_spi_dma_read_bytes(spi_id, recv_data, buf_len));
		CLI_LOGD("spi dma recv, spi_id=%d, data_len=%d\n", spi_id, buf_len);
		for (int i = 0; i < buf_len; i++) {
			CLI_LOGD("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
		}
		if (recv_data) {
			os_free(recv_data);
		}
		recv_data = NULL;
	} else if (os_strcmp(argv[2], "dma_duplex") == 0) {
		uint32_t buf_len = os_strtoul(argv[3], NULL, 10);
		uint8_t *recv_data = (uint8_t *)os_malloc(buf_len);
		uint8_t *send_data = (uint8_t *)os_malloc(buf_len);
		os_memset(recv_data, 0xff, buf_len);
		for (int i = 0; i < buf_len; i++) {
			send_data[i] = i & 0xff;
		}
		bk_spi_dma_duplex_init(spi_id);
		BK_LOG_ON_ERR(bk_spi_dma_duplex_xfer(spi_id, send_data, buf_len, recv_data, buf_len));
		for (int i = 0; i < buf_len; i++) {
			CLI_LOGD("recv_buffer[%d]=0x%x\n", i, recv_data[i]);
		}
		bk_spi_dma_duplex_deinit(spi_id);
	}
#endif
	else {
		cli_spi_help();
		return;
	}
}

static void cli_spi_config_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t spi_id;

	if (argc < 4) {
		cli_spi_help();
		return;
	}

	spi_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "baud_rate") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t baud_rate = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_spi_set_baud_rate(spi_id, baud_rate));
		CLI_LOGD("spi(%d) config baud_rate:%d\n", spi_id, baud_rate);
	} else if (os_strcmp(argv[2], "mode") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t mode = os_strtoul(argv[3], NULL, 10);
		BK_LOG_ON_ERR(bk_spi_set_mode(spi_id, mode));
		CLI_LOGD("spi(%d) config mode:%d\n", spi_id, mode);
	} else if (os_strcmp(argv[2], "bit_width") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t bit_width = os_strtoul(argv[3], NULL, 10);
		if (bit_width == 16) {
			BK_LOG_ON_ERR(bk_spi_set_bit_width(spi_id, SPI_BIT_WIDTH_16BITS));
		} else {
			bit_width = 8;
			BK_LOG_ON_ERR(bk_spi_set_bit_width(spi_id, SPI_BIT_WIDTH_8BITS));
		}
		CLI_LOGD("spi(%d) config bit_width:%d\n", spi_id, bit_width);
	} else if (os_strcmp(argv[2], "wire_mode") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		uint32_t wire_mode = os_strtoul(argv[3], NULL, 10);
		if (wire_mode == 3) {
			BK_LOG_ON_ERR(bk_spi_set_wire_mode(spi_id, SPI_3WIRE_MODE));
		} else {
			BK_LOG_ON_ERR(bk_spi_set_wire_mode(spi_id, SPI_4WIRE_MODE));
		}
		CLI_LOGD("spi(%d) config wire_mode:%d\n", spi_id, wire_mode);
	} else if (os_strcmp(argv[2], "bit_order") == 0) {
		CLI_RET_ON_INVALID_ARGC(argc, 4);
		if (os_strcmp(argv[3], "LSB") == 0) {
			BK_LOG_ON_ERR(bk_spi_set_bit_order(spi_id, SPI_LSB_FIRST));
		} else {
			BK_LOG_ON_ERR(bk_spi_set_bit_order(spi_id, SPI_MSB_FIRST));
		}
		CLI_LOGD("spi(%d) config bit_order:%s\n", spi_id, argv[3]);
	} else {
		cli_spi_help();
		return;
	}
}

static void cli_spi_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t spi_id;

	if (argc != 4) {
		cli_spi_help();
		return;
	}

	spi_id = os_strtoul(argv[1], NULL, 10);
	if (os_strcmp(argv[2], "reg") == 0) {
		if (os_strcmp(argv[3], "tx") == 0) {
			BK_LOG_ON_ERR(bk_spi_register_tx_finish_isr(spi_id, cli_spi_tx_isr, NULL));
			CLI_LOGD("spi id:%d register tx finish interrupt isr\n", spi_id);
		} else {
			BK_LOG_ON_ERR(bk_spi_register_rx_isr(spi_id, cli_spi_rx_isr, NULL));
			CLI_LOGD("spi id:%d register rx interrupt isr\n", spi_id);
		}
	} else {
		cli_spi_help();
		return;
	}
}

/*======================================================================
 * Standardized SPI test suite
 *
 * Ported/adapted from common industry practices:
 *   - Espressif ESP-IDF   : spi_test() length/clock/DMA matrix + hex diff
 *                           on mismatch; TEST_CASE_MULTIPLE_DEVICES model.
 *   - Zephyr spi_loopback : clock-mode / word-size / stress sweeps.
 *   - ARM CMSIS-Driver Val: loopback (MOSI<->MISO) + bidirectional peer.
 *
 * Three commands:
 *   spi_lb       - single board loopback self-test (jumper MOSI<->MISO),
 *                  full-duplex via bk_spi_dma_duplex_xfer.
 *   spi_peer     - dual board master/slave peer test, covers BOTH non-DMA
 *                  (bk_spi_write/read_bytes) and DMA (bk_spi_dma_write/read_bytes)
 *                  data paths, synchronized by an extra SYNC gpio (polling).
 *   spi_api_test - negative / API validation.
 *
 * All results use grep-friendly prefixes: SPI_LB / SPI_PEER / SPI_API,
 * with explicit PASS/FAIL and a trailing SUMMARY line for CI parsing.
 *====================================================================*/

#define SPI_TEST_SEED_BASE   0x53504954u  /* "SPIT" */
#define SPI_TEST_MAX_LEN     4096

typedef struct {
	uint32_t pass;
	uint32_t total;
} spi_test_stat_t;

/* Deterministic byte generator (LCG) so master and slave, running on
 * different chips, produce an identical pattern from the same seed. */
static uint8_t spi_test_next_byte(uint32_t *state)
{
	*state = (*state) * 1103515245u + 12345u;
	return (uint8_t)((*state >> 16) & 0xff);
}

static void spi_test_fill_pattern(uint8_t *buf, uint32_t len, uint32_t seed)
{
	uint32_t st = seed;
	for (uint32_t i = 0; i < len; i++) {
		buf[i] = spi_test_next_byte(&st);
	}
}

/* Compare buf against the pattern for seed; on first mismatch print the
 * offset and 16 bytes of expected vs received (Espressif style). */
static bool spi_test_verify(const uint8_t *buf, uint32_t len, uint32_t seed)
{
	uint32_t st = seed;
	for (uint32_t i = 0; i < len; i++) {
		uint8_t exp = spi_test_next_byte(&st);
		if (buf[i] != exp) {
			uint32_t from = (i >= 8) ? (i - 8) : 0;
			CLI_LOGI("mismatch @%u (len=%u)\r\n", i, len);
			st = seed;
			for (uint32_t k = 0; k < from; k++) {
				(void)spi_test_next_byte(&st);
			}
			CLI_LOGI("exp:");
			for (uint32_t k = from; k < from + 16 && k < len; k++) {
				CLI_LOGI(" %02x", spi_test_next_byte(&st));
			}
			CLI_LOGI("\r\ngot:");
			for (uint32_t k = from; k < from + 16 && k < len; k++) {
				CLI_LOGI(" %02x", buf[k]);
			}
			CLI_LOGI("\r\n");
			return false;
		}
	}
	return true;
}

#if CONFIG_SPI_DMA
static void spi_test_dma_dev(spi_id_t id, dma_dev_t *tx_dev, dma_dev_t *rx_dev)
{
#if (SOC_SPI_UNIT_NUM > 1)
	if (id == SPI_ID_1) {
		*tx_dev = DMA_DEV_GSPI1;
		*rx_dev = DMA_DEV_GSPI1_RX;
		return;
	}
#endif
#if (SOC_SPI_UNIT_NUM > 2)
	if (id == SPI_ID_2) {
		*tx_dev = DMA_DEV_GSPI2;
		*rx_dev = DMA_DEV_GSPI2_RX;
		return;
	}
#endif
#if (SOC_SPI_UNIT_NUM > 3)
	if (id == SPI_ID_3) {
		*tx_dev = DMA_DEV_GSPI3;
		*rx_dev = DMA_DEV_GSPI3_RX;
		return;
	}
#endif
	*tx_dev = DMA_DEV_GSPI0;
	*rx_dev = DMA_DEV_GSPI0_RX;
}
#endif

/* Build a spi_config_t. When dma_on, tx/rx channels must be pre-allocated. */
static void spi_test_build_config(spi_config_t *cfg, spi_role_t role, spi_mode_t mode,
				  uint32_t baud, bool dma_on, dma_id_t tx_chan, dma_id_t rx_chan)
{
	os_memset(cfg, 0, sizeof(*cfg));
	cfg->role = role;
	cfg->bit_width = SPI_BIT_WIDTH_8BITS;
	cfg->polarity = (mode & 0x2) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
	cfg->phase = (mode & 0x1) ? SPI_PHASE_2ND_EDGE : SPI_PHASE_1ST_EDGE;
	cfg->wire_mode = SPI_4WIRE_MODE;
	cfg->baud_rate = baud;
	cfg->bit_order = SPI_MSB_FIRST;
#if (CONFIG_SPI_BYTE_INTERVAL)
	cfg->byte_interval = 1;
#endif
#if CONFIG_SPI_DMA
	cfg->dma_mode = dma_on ? SPI_DMA_MODE_ENABLE : SPI_DMA_MODE_DISABLE;
	cfg->spi_tx_dma_chan = tx_chan;
	cfg->spi_rx_dma_chan = rx_chan;
	cfg->spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
	cfg->spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
#else
	(void)dma_on; (void)tx_chan; (void)rx_chan;
#endif
}

/*--------------------- single board loopback (spi_lb) ---------------------*/
#if CONFIG_SPI_DMA
static bool spi_lb_run_one(spi_id_t id, uint32_t len, uint32_t baud, spi_mode_t mode, uint32_t seed)
{
	bool ok = false;
	dma_dev_t tx_dev, rx_dev;
	dma_id_t tx_chan = DMA_ID_MAX, rx_chan = DMA_ID_MAX;
	uint8_t *tx = (uint8_t *)os_malloc(len);
	uint8_t *rx = (uint8_t *)os_malloc(len);

	if (!tx || !rx) {
		CLI_LOGE("SPI_LB: FAIL alloc (len=%u)\r\n", len);
		goto out_free;
	}

	spi_test_dma_dev(id, &tx_dev, &rx_dev);
	tx_chan = bk_dma_alloc(tx_dev);
	rx_chan = bk_dma_alloc(rx_dev);
	if (tx_chan == DMA_ID_MAX || rx_chan == DMA_ID_MAX) {
		CLI_LOGE("SPI_LB: FAIL dma alloc (tx=%u rx=%u)\r\n", tx_chan, rx_chan);
		goto out_dma;
	}

	spi_config_t cfg;
	spi_test_build_config(&cfg, SPI_ROLE_MASTER, mode, baud, true, tx_chan, rx_chan);

	spi_test_fill_pattern(tx, len, seed);
	os_memset(rx, 0x55, len);

	if (bk_spi_init(id, &cfg) != BK_OK) {
		CLI_LOGE("SPI_LB: FAIL init\r\n");
		goto out_dma;
	}
	bk_spi_dma_duplex_init(id);

	if (bk_spi_dma_duplex_xfer(id, tx, len, rx, len) == BK_OK) {
		ok = spi_test_verify(rx, len, seed);
	}

	bk_spi_dma_duplex_deinit(id);
	bk_spi_deinit(id);

out_dma:
	if (tx_chan != DMA_ID_MAX) bk_dma_free(tx_dev, tx_chan);
	if (rx_chan != DMA_ID_MAX) bk_dma_free(rx_dev, rx_chan);
out_free:
	if (tx) os_free(tx);
	if (rx) os_free(rx);

	CLI_LOGI("SPI_LB: %s (len=%u baud=%u mode=%u)\r\n", ok ? "PASS" : "FAIL", len, baud, mode);
	return ok;
}

static void spi_lb_run_matrix(spi_id_t id, bool full)
{
	static const uint32_t lens[] = {1, 3, 4, 16, 21, 32, 36, 63, 64, 128, 129, 255, 256, 1024, 4095, 4096};
	static const uint32_t bauds[] = {100000, 1000000, 4000000, 8000000};
	spi_test_stat_t st = {0, 0};
	uint32_t seed = SPI_TEST_SEED_BASE;

	/* 1. length sweep @ 1MHz mode0 (aligned/unaligned/boundary) */
	for (uint32_t i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
		if (!full && lens[i] > 256 && lens[i] != 4096) continue;
		st.total++;
		if (spi_lb_run_one(id, lens[i], 1000000, SPI_POL_MODE_0, seed++)) st.pass++;
	}

	/* 2. clock mode sweep 0..3 @ len=256 1MHz */
	for (uint32_t m = 0; m < 4; m++) {
		st.total++;
		if (spi_lb_run_one(id, 256, 1000000, (spi_mode_t)m, seed++)) st.pass++;
	}

	/* 3. baud sweep @ len=256 mode0 */
	for (uint32_t b = 0; b < sizeof(bauds) / sizeof(bauds[0]); b++) {
		st.total++;
		if (spi_lb_run_one(id, 256, bauds[b], SPI_POL_MODE_0, seed++)) st.pass++;
	}

	/* 4. stress: repeated random lengths */
	uint32_t rounds = full ? 100 : 20;
	for (uint32_t r = 0; r < rounds; r++) {
		uint32_t len = 1 + (bk_rand() % SPI_TEST_MAX_LEN);
		st.total++;
		if (spi_lb_run_one(id, len, 2000000, SPI_POL_MODE_0, seed++)) st.pass++;
	}

	CLI_LOGI("SPI_LB SUMMARY: %u/%u PASS\r\n", st.pass, st.total);
	if (st.pass == st.total) {
		CLI_LOGI("SPI_LB: ALL PASS\r\n");
	}
}
#endif /* CONFIG_SPI_DMA */

static void cli_spi_lb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_spi_help();
		return;
	}
#if CONFIG_SPI_DMA
	spi_id_t id = os_strtoul(argv[1], NULL, 10);
	bool full = true;
	if (argc >= 3 && os_strcmp(argv[2], "quick") == 0) {
		full = false;
	}
	CLI_LOGI("SPI_LB START: id=%d %s (jumper MOSI<->MISO required)\r\n", id, full ? "full" : "quick");
	spi_lb_run_matrix(id, full);
#else
	CLI_LOGE("SPI_LB: CONFIG_SPI_DMA disabled, loopback needs duplex dma\r\n");
#endif
}

/*--------------------- dual board peer test (spi_peer) ---------------------*/
/*
 * SPI peer-to-peer test between two boards (one master, one slave), ported from
 * the proven thread-based design: each side runs a dedicated thread, generates a
 * deterministic pattern and exchanges it in two phases per round (master drives
 * the clock):
 *   Phase A  master -> slave : master writes data_len bytes,  slave reads data_len bytes
 *   Phase B  slave  -> master: slave  writes data_len payload, master reads data_len+1 bytes
 *
 * Transfer path is selectable at runtime via the trailing [dma|fifo] argument
 * (default dma when CONFIG_SPI_DMA is enabled). In DMA mode a master RX-only
 * transfer would NOT drive the SPI clock and stall forever, so the master
 * clocks the bus with bk_spi_dma_duplex_xfer (TX dummy while capturing RX).
 *
 * BK SPI-slave HW quirk: this SoC's SPI slave automatically inserts one
 * redundant 0x72 byte in front of the first byte it transmits. So the slave
 * sends payload only and the receiving master reads (payload_len + 1) bytes,
 * checks the leading byte is 0x72, then drops it before validating the payload.
 * The slave must NOT add the 0x72 itself (that would double it and shift by one).
 */
#define SPI_PEER_REDUNDANT_BYTE   0x72
#define SPI_PEER_DEFAULT_GAP_MS   5
#define SPI_PEER_MAX_LOG_ERR      8

/* The peer test does blocking SPI transfers. When the link partner is absent or
 * desynced a transfer can stall long enough to trip the task watchdog, so stop
 * it for the lifetime of the test thread and restore it on exit. */
#if CONFIG_TASK_WDT
extern void bk_task_wdt_start(void);
extern void bk_task_wdt_stop(void);
#define SPI_PEER_TASK_WDT_STOP()   bk_task_wdt_stop()
#define SPI_PEER_TASK_WDT_START()  bk_task_wdt_start()
#else
#define SPI_PEER_TASK_WDT_STOP()   do {} while (0)
#define SPI_PEER_TASK_WDT_START()  do {} while (0)
#endif

typedef struct {
	beken_thread_t handle;
	spi_id_t spi_id;
	spi_role_t role;
	uint32_t baud_rate;
	uint32_t rounds;
	uint32_t data_len;
	uint32_t gap_ms;
	bool use_dma;
#if CONFIG_SPI_DMA
	dma_dev_t tx_dma_dev;
	dma_dev_t rx_dma_dev;
	dma_id_t tx_dma_chan;
	dma_id_t rx_dma_chan;
#endif
} spi_peer_test_t;

static spi_peer_test_t s_spi_peer;

/* deterministic pattern for the master -> slave direction */
static inline uint8_t spi_peer_m2s_byte(uint32_t round, uint32_t idx)
{
	return (uint8_t)(round + idx);
}

/* deterministic pattern for the slave -> master direction */
static inline uint8_t spi_peer_s2m_byte(uint32_t round, uint32_t idx)
{
	return (uint8_t)(0xC0 + round + idx);
}

static bk_err_t spi_peer_write(spi_id_t id, const void *data, uint32_t size)
{
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		return bk_spi_dma_write_bytes(id, data, size);
	}
#endif
	return bk_spi_write_bytes(id, data, size);
}

static bk_err_t spi_peer_read(spi_id_t id, void *data, uint32_t size)
{
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		return bk_spi_dma_read_bytes(id, data, size);
	}
#endif
	return bk_spi_read_bytes(id, data, size);
}

static void spi_peer_config(spi_id_t id, spi_role_t role, uint32_t baud_rate)
{
	spi_config_t config = {0};

	config.role = role;
	config.bit_width = SPI_BIT_WIDTH_8BITS;
	config.polarity = SPI_POLARITY_HIGH;
	config.phase = SPI_PHASE_2ND_EDGE;
	config.wire_mode = SPI_4WIRE_MODE;
	config.baud_rate = baud_rate;
	config.bit_order = SPI_MSB_FIRST;
#if (CONFIG_SPI_BYTE_INTERVAL)
	config.byte_interval = 1;
#endif
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		spi_test_dma_dev(id, &s_spi_peer.tx_dma_dev, &s_spi_peer.rx_dma_dev);
		s_spi_peer.tx_dma_chan = bk_dma_alloc(s_spi_peer.tx_dma_dev);
		s_spi_peer.rx_dma_chan = bk_dma_alloc(s_spi_peer.rx_dma_dev);
		config.dma_mode = SPI_DMA_MODE_ENABLE;
		config.spi_tx_dma_chan = s_spi_peer.tx_dma_chan;
		config.spi_rx_dma_chan = s_spi_peer.rx_dma_chan;
		config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
		config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
	}
#endif

	BK_LOG_ON_ERR(bk_spi_init(id, &config));
}

static void spi_peer_deconfig(spi_id_t id)
{
	BK_LOG_ON_ERR(bk_spi_deinit(id));
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		bk_dma_free(s_spi_peer.tx_dma_dev, s_spi_peer.tx_dma_chan);
		bk_dma_free(s_spi_peer.rx_dma_dev, s_spi_peer.rx_dma_chan);
	}
#endif
}

static void spi_peer_master_thread(void *arg)
{
	spi_id_t id = s_spi_peer.spi_id;
	uint32_t len = s_spi_peer.data_len;
	uint32_t rounds = s_spi_peer.rounds;
	uint32_t gap_ms = s_spi_peer.gap_ms;
	uint32_t total_err = 0;

	SPI_PEER_TASK_WDT_STOP();

	uint8_t *tx = (uint8_t *)os_zalloc(len);
	uint8_t *rx = (uint8_t *)os_zalloc(len + 1); /* +1 for the redundant 0x72 */
	/* Dummy buffer for the full-duplex DMA path: a master RX-only DMA transfer
	 * does not drive the SPI clock, so it would stall forever. We instead clock
	 * the bus with a duplex xfer (TX dummy while capturing RX). Sized len+1 to
	 * cover both phases. */
	uint8_t *dummy = (uint8_t *)os_zalloc(len + 1);

	if ((tx == NULL) || (rx == NULL) || (dummy == NULL)) {
		CLI_LOGE("[SPI-PEER][M] buffer malloc failed\r\n");
		goto exit;
	}

	spi_peer_config(id, SPI_ROLE_MASTER, s_spi_peer.baud_rate);
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		bk_spi_dma_duplex_init(id);
	}
#endif
	CLI_LOGI("[SPI-PEER][M] start id=%d baud=%d rounds=%d len=%d gap=%dms mode=%s\r\n",
		 id, s_spi_peer.baud_rate, rounds, len, gap_ms, s_spi_peer.use_dma ? "dma" : "fifo");

	for (uint32_t round = 0; round < rounds; round++) {
		uint32_t round_err = 0;

		/* Phase A: master -> slave */
		for (uint32_t i = 0; i < len; i++) {
			tx[i] = spi_peer_m2s_byte(round, i);
		}
#if CONFIG_SPI_DMA
		if (s_spi_peer.use_dma) {
			/* duplex so the master clocks the bus; RX captured into scratch */
			BK_LOG_ON_ERR(bk_spi_dma_duplex_xfer(id, tx, len, dummy, len));
		} else
#endif
		{
			BK_LOG_ON_ERR(spi_peer_write(id, tx, len));
		}

		/* give the slave time to switch from RX to TX */
		rtos_delay_milliseconds(gap_ms);

		/* Phase B: slave -> master, first byte is the redundant marker */
		os_memset(rx, 0, len + 1);
#if CONFIG_SPI_DMA
		if (s_spi_peer.use_dma) {
			/* duplex: TX dummy drives the clock while we capture the payload */
			BK_LOG_ON_ERR(bk_spi_dma_duplex_xfer(id, dummy, len + 1, rx, len + 1));
		} else
#endif
		{
			BK_LOG_ON_ERR(spi_peer_read(id, rx, len + 1));
		}

		if (rx[0] != SPI_PEER_REDUNDANT_BYTE) {
			CLI_LOGW("[M] round %d redundant byte mismatch: got 0x%02x exp 0x%02x\r\n",
				 round, rx[0], SPI_PEER_REDUNDANT_BYTE);
			round_err++;
		}

		/* strip the redundant byte, then verify the payload */
		for (uint32_t i = 0; i < len; i++) {
			uint8_t exp = spi_peer_s2m_byte(round, i);
			if (rx[i + 1] != exp) {
				if (round_err < SPI_PEER_MAX_LOG_ERR) {
					CLI_LOGW("[M] round %d data[%d] mismatch: got 0x%02x exp 0x%02x\r\n",
						 round, i, rx[i + 1], exp);
				}
				round_err++;
			}
		}

		total_err += round_err;
		CLI_LOGI("[M] round %d %s (errs=%d)\r\n", round, round_err ? "FAIL" : "PASS", round_err);
		rtos_delay_milliseconds(gap_ms);
	}

	CLI_LOGI("[SPI-PEER][M] DONE rounds=%d total_errs=%d result=%s\r\n",
		 rounds, total_err, total_err ? "FAIL" : "PASS");

exit:
#if CONFIG_SPI_DMA
	if (s_spi_peer.use_dma) {
		bk_spi_dma_duplex_deinit(id);
	}
#endif
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
	if (dummy) {
		os_free(dummy);
	}
	spi_peer_deconfig(id);
	SPI_PEER_TASK_WDT_START();
	s_spi_peer.handle = NULL;
	rtos_delete_thread(NULL);
}

static void spi_peer_slave_thread(void *arg)
{
	spi_id_t id = s_spi_peer.spi_id;
	uint32_t len = s_spi_peer.data_len;
	uint32_t rounds = s_spi_peer.rounds;
	uint32_t total_err = 0;

	SPI_PEER_TASK_WDT_STOP();

	uint8_t *rx = (uint8_t *)os_zalloc(len);
	uint8_t *tx = (uint8_t *)os_zalloc(len); /* payload only, HW auto-inserts the 0x72 head byte */

	if ((tx == NULL) || (rx == NULL)) {
		CLI_LOGE("[SPI-PEER][S] buffer malloc failed\r\n");
		goto exit;
	}

	spi_peer_config(id, SPI_ROLE_SLAVE, s_spi_peer.baud_rate);
	CLI_LOGI("[SPI-PEER][S] start id=%d baud=%d rounds=%d len=%d mode=%s\r\n",
		 id, s_spi_peer.baud_rate, rounds, len, s_spi_peer.use_dma ? "dma" : "fifo");

	for (uint32_t round = 0; round < rounds; round++) {
		uint32_t round_err = 0;

		/* Phase A: receive master -> slave */
		os_memset(rx, 0, len);
		BK_LOG_ON_ERR(spi_peer_read(id, rx, len));

		for (uint32_t i = 0; i < len; i++) {
			uint8_t exp = spi_peer_m2s_byte(round, i);
			if (rx[i] != exp) {
				if (round_err < SPI_PEER_MAX_LOG_ERR) {
					CLI_LOGW("[S] round %d data[%d] mismatch: got 0x%02x exp 0x%02x\r\n",
						 round, i, rx[i], exp);
				}
				round_err++;
			}
		}

		/*
		 * Phase B: send slave -> master, payload only.
		 * This SoC's SPI slave HW automatically inserts one 0x72 byte in front
		 * of the first transmitted byte, so we must NOT prepend it in software
		 * (doing so produces a double 0x72 and shifts the payload by one byte).
		 * The master reads len+1 bytes and strips that leading 0x72.
		 */
		for (uint32_t i = 0; i < len; i++) {
			tx[i] = spi_peer_s2m_byte(round, i);
		}
		BK_LOG_ON_ERR(spi_peer_write(id, tx, len));

		total_err += round_err;
		CLI_LOGI("[S] round %d %s (rx errs=%d)\r\n", round, round_err ? "FAIL" : "PASS", round_err);
	}

	CLI_LOGI("[SPI-PEER][S] DONE rounds=%d total_errs=%d result=%s\r\n",
		 rounds, total_err, total_err ? "FAIL" : "PASS");

exit:
	if (tx) {
		os_free(tx);
	}
	if (rx) {
		os_free(rx);
	}
	spi_peer_deconfig(id);
	SPI_PEER_TASK_WDT_START();
	s_spi_peer.handle = NULL;
	rtos_delete_thread(NULL);
}

static void cli_spi_peer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	beken_thread_function_t entry = NULL;

	if ((argc >= 2) && (os_strcmp(argv[1], "stop") == 0)) {
		if (s_spi_peer.handle) {
			rtos_delete_thread(&s_spi_peer.handle);
			s_spi_peer.handle = NULL;
			spi_peer_deconfig(s_spi_peer.spi_id);
			CLI_LOGI("[SPI-PEER] stopped\r\n");
		} else {
			CLI_LOGI("[SPI-PEER] not running\r\n");
		}
		return;
	}

	if (argc < 6) {
		cli_spi_help();
		return;
	}

	if (s_spi_peer.handle) {
		CLI_LOGW("[SPI-PEER] test already running, use 'spi_peer stop' first\r\n");
		return;
	}

	if (os_strcmp(argv[1], "master") == 0) {
		s_spi_peer.role = SPI_ROLE_MASTER;
		entry = (beken_thread_function_t)spi_peer_master_thread;
	} else if (os_strcmp(argv[1], "slave") == 0) {
		s_spi_peer.role = SPI_ROLE_SLAVE;
		entry = (beken_thread_function_t)spi_peer_slave_thread;
	} else {
		cli_spi_help();
		return;
	}

	s_spi_peer.spi_id = os_strtoul(argv[2], NULL, 10);
	s_spi_peer.baud_rate = os_strtoul(argv[3], NULL, 10);
	s_spi_peer.rounds = os_strtoul(argv[4], NULL, 10);
	s_spi_peer.data_len = os_strtoul(argv[5], NULL, 10);
	s_spi_peer.gap_ms = (argc > 6) ? os_strtoul(argv[6], NULL, 10) : SPI_PEER_DEFAULT_GAP_MS;

	/* transfer mode: default DMA when available; pass "fifo" to use the CPU/IRQ
	 * FIFO path (e.g. to coexist with flash writes on SPI0) */
#if CONFIG_SPI_DMA
	s_spi_peer.use_dma = true;
#else
	s_spi_peer.use_dma = false;
#endif
	if ((argc > 7) && (os_strcmp(argv[7], "fifo") == 0)) {
		s_spi_peer.use_dma = false;
	} else if ((argc > 7) && (os_strcmp(argv[7], "dma") == 0)) {
#if CONFIG_SPI_DMA
		s_spi_peer.use_dma = true;
#else
		CLI_LOGW("[SPI-PEER] CONFIG_SPI_DMA off, falling back to FIFO\r\n");
		s_spi_peer.use_dma = false;
#endif
	}

	if ((s_spi_peer.rounds == 0) || (s_spi_peer.data_len == 0)) {
		CLI_LOGE("[SPI-PEER] rounds and data_len must be > 0\r\n");
		return;
	}

	CLI_LOGI("[SPI-PEER] %s start mode=%s (run the peer as the opposite role first)\r\n",
		 (s_spi_peer.role == SPI_ROLE_MASTER) ? "master" : "slave",
		 s_spi_peer.use_dma ? "dma" : "fifo");

	if (rtos_create_thread(&s_spi_peer.handle, 8, "spi_peer_test",
			       entry, 2048, 0)) {
		s_spi_peer.handle = NULL;
		CLI_LOGE("[SPI-PEER] create thread failed\r\n");
	}
}

/*--------------------- negative / API test (spi_api_test) ---------------------*/
static void cli_spi_api_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	spi_id_t id = (argc >= 2) ? os_strtoul(argv[1], NULL, 10) : SPI_ID_0;
	spi_test_stat_t st = {0, 0};
	spi_config_t cfg;

	/* 1. NULL config rejected */
	st.total++;
	if (bk_spi_init(id, NULL) != BK_OK) {
		st.pass++;
		CLI_LOGI("SPI_API: PASS null-config-rejected\r\n");
	} else {
		bk_spi_deinit(id);
		CLI_LOGI("SPI_API: FAIL null-config-rejected\r\n");
	}

	/* 2. invalid id rejected */
	st.total++;
	spi_test_build_config(&cfg, SPI_ROLE_MASTER, SPI_POL_MODE_0, 1000000, false, 0, 0);
	if (bk_spi_init(SPI_ID_MAX, &cfg) != BK_OK) {
		st.pass++;
		CLI_LOGI("SPI_API: PASS invalid-id-rejected\r\n");
	} else {
		CLI_LOGI("SPI_API: FAIL invalid-id-rejected\r\n");
	}

	/* 3. operate on uninit id should be rejected (no GPIO map side effect) */
	st.total++;
	{
		uint8_t dummy = 0;
		if (bk_spi_write_bytes(id, &dummy, 1) == BK_ERR_SPI_ID_NOT_INIT) {
			st.pass++;
			CLI_LOGI("SPI_API: PASS uninit-id-write-rejected\r\n");
		} else {
			CLI_LOGI("SPI_API: FAIL uninit-id-write-rejected\r\n");
		}
	}

	CLI_LOGI("SPI_API SUMMARY: %u/%u PASS\r\n", st.pass, st.total);
	if (st.pass == st.total) {
		CLI_LOGI("SPI_API: ALL PASS\r\n");
	}
}

static void cli_spi_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_spi_help();
		return;
	}
	uint32_t spi_id = os_strtoul(argv[1], NULL, 10);
	CLI_LOGD("spi_id:%08x\r\n",spi_id);

#if CONFIG_SPI_MST_FLASH
	extern uint32_t bk_spi_flash_read_id(spi_id_t id);
	extern int bk_spi_flash_read(spi_id_t id, uint32_t base_addr, uint8_t *dst_data, uint32_t size);
	extern int bk_spi_flash_write(spi_id_t id, uint32_t base_addr, const void *data, uint32_t size);
	extern int bk_spi_flash_erase(spi_id_t id, uint32_t base_addr, uint32_t size);
	if (os_strcmp(argv[2], "readid") == 0) {
		bk_spi_flash_read_id(spi_id);
		return;
	}
	if (argc < 5) {
		cli_spi_help();
		return;
	}
	if (os_strcmp(argv[2], "read") == 0) {
		uint32_t read_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t read_len = os_strtoul(argv[4], NULL, 16);
		uint32_t read_times = 1;
		if (argc >= 6) {
			read_times = os_strtoul(argv[5], NULL, 10);
		}
		CLI_LOGD("read_addr:%08x,read_len:%d, read_times:%d\r\n",read_addr,read_len,read_times);

		uint8_t *buf = (uint8_t *)os_zalloc(read_len);
		for (int i = 0; i < read_times; i++)
			bk_spi_flash_read(spi_id, read_addr, buf, read_len);
		for(int i=0;i < read_len;i++) {
			BK_LOGD(NULL, "%02x ",buf[i]);
			if(0 == (i+1)%16)
				BK_LOGD(NULL, "\r\n");
		}

		if (buf) {
			os_free(buf);
		}
		buf = NULL;
		CLI_LOGD("spi_flash_read finish\r\n");
	} else if (os_strcmp(argv[2], "erase") == 0) {
		uint32_t erase_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t size = os_strtoul(argv[4], NULL, 16);
		CLI_LOGD("erase_addr:%08x,size:%d\r\n",erase_addr);
		bk_spi_flash_erase(spi_id, erase_addr, size);
		CLI_LOGD("spi_flash_erase finish\r\n");
	} else if (os_strcmp(argv[2], "write") == 0) {
		uint32_t write_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t size = os_strtoul(argv[4], NULL, 16);
		CLI_LOGD("write_addr:%08x,size:%d\r\n",write_addr,size);
		uint32_t page_size = 256;
		uint8_t *buf = (uint8_t *)os_zalloc(page_size);
		for (uint32_t i = 0; i < page_size; i++) {
			buf[i] = i;
		}
		for (uint32_t addr = write_addr; addr < (write_addr + size); addr += page_size) {
			bk_spi_flash_write(spi_id, addr, buf, page_size);
		}
		if (buf) {
			os_free(buf);
		}
		buf = NULL;

		CLI_LOGD("spi_flash_write finish\r\n");
	} else {
		cli_spi_help();
	}
#else
	CLI_LOGE("please enable CONFIG_SPI_MST_FLASH\r\n");
#endif
	return;
}

#define SPI_CMD_CNT (sizeof(s_spi_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_spi_commands[] = {
	{"spi_driver", "spi_driver {init|deinit}", cli_spi_driver_cmd},
	{"spi", "spi {init|write|read}", cli_spi_cmd},
	{"spi_config", "spi_config {id} {mode|baud_rate} [...]", cli_spi_config_cmd},
	{"spi_int", "spi_int {id} {reg} {tx|rx}", cli_spi_int_cmd},
	{"spi_lb", "spi_lb {id} [quick|full]", cli_spi_lb_cmd},
	{"spi_peer", "spi_peer {master|slave} {id} {baud} {rounds} {data_len} [gap_ms] [dma|fifo] | spi_peer stop", cli_spi_peer_cmd},
	{"spi_api_test", "spi_api_test {id}", cli_spi_api_test_cmd},
	{"spi_flash", "spi_flash {id} {readid|read|write|erase} {addr} {len}[...]", cli_spi_flash_cmd},
};

int bk_spi_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_spi_driver_init());
	return cli_register_module_test_feature(s_spi_commands, SPI_CMD_CNT);
}
// eof

