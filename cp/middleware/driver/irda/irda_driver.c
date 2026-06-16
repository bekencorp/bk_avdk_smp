// Copyright 2023-2024 Beken
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

#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "driver/int.h"
#include "driver/irda.h"
#include "sys_driver.h"
#include "gpio_driver.h"
#include "bk_fifo.h"
#include "irda_driver.h"
#include "irda_hal.h"
#include "driver/gpio.h"

#define IRDA_FIFO_ENTRY_BYTES             (2U)
#define IRDA_HW_FIFO_DEPTH_ENTRIES        (256U)
#define IRDA_RX_SW_FIFO_BYTES             (1024U)


#define IRDA_RETURN_ON_NOT_INIT() do {\
	if (!s_irda_driver_is_init) {\
		return BK_ERR_IRDA_NOT_INIT;\
	}\
} while(0)

typedef struct {
	const uint16_t *tx_buf;
	uint32_t tx_entries;
	volatile uint32_t tx_offset;
	volatile bool tx_busy;

	kfifo_ptr_t rx_fifo;
	volatile bool rx_overflow;
	volatile bool rx_waiting;

	beken_semaphore_t irda_rx_sema;
	beken_semaphore_t irda_tx_sema;
} irda_driver_t;

static bool s_irda_driver_is_init = false;
static irda_driver_t s_irda = {0};

static void irda_isr(void);
static uint32_t irda_push_tx_entries(uint32_t max_entries);
static void irda_drain_rx_fifo(void);
// static void irda_reset_sema(beken_semaphore_t *sema);

static void irda_init_gpio(void)
{
#if CONFIG_USR_GPIO_CFG_EN
	gpio_dev_map_by_func(GPIO_DEV_IRDA);
#endif
}

static void irda_deinit_gpio(void)
{
#if CONFIG_USR_GPIO_CFG_EN
	gpio_dev_unmap_by_func(GPIO_DEV_IRDA);
#endif
}

/* 1) power up irda
 * 2) enable system irda interrupt
 * 3) init irda gpio
 */
static void irda_init_common(void)
{
	// TODO: need to add clock enable
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_IRDA, 1);
	irda_init_gpio();
}

static void irda_deinit_common(void)
{
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_IRDA, 0);
	// TODO: need to add clock disable
	irda_deinit_gpio();
}

static void irda_sw_init(void)
{
	int ret = 0;
	if (s_irda.irda_tx_sema == NULL) {
		ret = rtos_init_semaphore(&s_irda.irda_tx_sema, 1);
		BK_ASSERT(kNoErr == ret);
	}
	if (s_irda.irda_rx_sema == NULL) {
		ret = rtos_init_semaphore(&s_irda.irda_rx_sema, 1);
		BK_ASSERT(kNoErr == ret);
	}
	if (s_irda.rx_fifo == NULL) {
		s_irda.rx_fifo = kfifo_alloc(IRDA_RX_SW_FIFO_BYTES);
		BK_ASSERT(s_irda.rx_fifo != NULL);
	}

	s_irda.tx_busy = false;
	s_irda.rx_waiting = false;
	s_irda.rx_overflow = false;
}

static void irda_sw_deinit(void)
{
	if (s_irda.irda_tx_sema) {
		rtos_deinit_semaphore(&s_irda.irda_tx_sema);
		s_irda.irda_tx_sema = NULL;
	}
	if (s_irda.irda_rx_sema) {
		rtos_deinit_semaphore(&s_irda.irda_rx_sema);
		s_irda.irda_rx_sema = NULL;
	}
	if (s_irda.rx_fifo) {
		kfifo_free(s_irda.rx_fifo);
		s_irda.rx_fifo = NULL;
	}
}

bk_err_t bk_irda_driver_init(void)
{
	if (s_irda_driver_is_init) {
		return BK_OK;
	}

	irda_hal_init();
	bk_int_isr_register(INT_SRC_IRDA, irda_isr, NULL);
	irda_sw_init();
	irda_init_common();

	s_irda_driver_is_init = true;

#if CONFIG_CLI && CONFIG_EFUSE_TEST
	int bk_irda_register_cli_test_feature(void);
	bk_irda_register_cli_test_feature();
#endif
	return BK_OK;
}

bk_err_t bk_irda_driver_deinit(void)
{
	if (!s_irda_driver_is_init) {
		IRDA_LOGI("%s:isn't init \r\n", __func__);
		return BK_OK;
	}
	
	irda_deinit_common();
	bk_int_isr_register(INT_SRC_IRDA, NULL, NULL);
	irda_sw_deinit();

	s_irda_driver_is_init = false;
	return BK_OK;
}

bk_err_t bk_irda_init_tx(const irda_tx_init_config_t *tx_config)
{
	IRDA_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(tx_config);

	/* Parameter range validation */
	if (tx_config->clk_freq_input == 0 || tx_config->clk_freq_input > IRDA_CLK_FREQ_INPUT_MAX) {
		IRDA_LOGE("invalid clk_freq_input:%u, range [1, %u]\r\n",
			tx_config->clk_freq_input, IRDA_CLK_FREQ_INPUT_MAX);
		return BK_ERR_PARAM;
	}
	if (tx_config->carrier_period_cycle > IRDA_CARRIER_PERIOD_CYCLE_MAX) {
		IRDA_LOGE("invalid carrier_period_cycle:%u, range [0, %u]\r\n",
			tx_config->carrier_period_cycle, IRDA_CARRIER_PERIOD_CYCLE_MAX);
		return BK_ERR_PARAM;
	}
	if (tx_config->carrier_duty_cycle > IRDA_CARRIER_DUTY_CYCLE_MAX) {
		IRDA_LOGE("invalid carrier_duty_cycle:%u, range [0, %u]\r\n",
			tx_config->carrier_duty_cycle, IRDA_CARRIER_DUTY_CYCLE_MAX);
		return BK_ERR_PARAM;
	}
	if (tx_config->tx_initial_level > 1) {
		IRDA_LOGE("invalid tx_initial_level:%u, must be 0 or 1\r\n",
			tx_config->tx_initial_level);
		return BK_ERR_PARAM;
	}

	irda_hal_init_tx(tx_config);
	irda_hal_set_tx_done_int(true);
	irda_hal_set_tx_enable(true);
	irda_hal_set_rx_timeout_int(true);
	irda_hal_set_rx_enable(false);
	return BK_OK;
}

bk_err_t bk_irda_init_rx(const irda_rx_init_config_t *rx_config)
{
	IRDA_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(rx_config);

	/* Parameter range validation */
	if (rx_config->clk_freq_input == 0 || rx_config->clk_freq_input > IRDA_CLK_FREQ_INPUT_MAX) {
		IRDA_LOGE("invalid clk_freq_input:%u, range [1, %u]\r\n",
			rx_config->clk_freq_input, IRDA_CLK_FREQ_INPUT_MAX);
		return BK_ERR_PARAM;
	}
	if (rx_config->rx_timeout_us == 0 || rx_config->rx_timeout_us > IRDA_RX_TIMEOUT_US_MAX) {
		IRDA_LOGE("invalid rx_timeout_us:%u, range [1, %u]\r\n",
			rx_config->rx_timeout_us, IRDA_RX_TIMEOUT_US_MAX);
		return BK_ERR_PARAM;
	}
	if (rx_config->rx_start_threshold_us > IRDA_RX_START_THRESHOLD_US_MAX) {
		IRDA_LOGE("invalid rx_start_threshold_us:%u, range [0, %u]\r\n",
			rx_config->rx_start_threshold_us, IRDA_RX_START_THRESHOLD_US_MAX);
		return BK_ERR_PARAM;
	}
	if (rx_config->rx_initial_level > 1) {
		IRDA_LOGE("invalid rx_initial_level:%u, must be 0 or 1\r\n",
			rx_config->rx_initial_level);
		return BK_ERR_PARAM;
	}

	irda_hal_init_rx(rx_config);
	irda_hal_set_rx_int(true);
	irda_hal_set_rx_enable(true);
	irda_hal_set_tx_enable(false);
	return BK_OK;
}

bk_err_t bk_irda_write_words(const uint16_t *data, uint32_t entry_count)
{
	BK_RETURN_ON_NULL(data);
	if (entry_count == 0) {
		return BK_OK;
	}

	uint32_t int_level = rtos_disable_int();
	if (s_irda.tx_busy) {
		rtos_enable_int(int_level);
		return BK_ERR_BUSY;
	}

	s_irda.tx_busy = true;
	rtos_enable_int(int_level);

	uint32_t sent_entries = 0;
	while (sent_entries < entry_count) {
		uint32_t left_entries = entry_count - sent_entries;
		uint32_t chunk_entries = min(left_entries, min(IRDA_REG0X4_TXDATA_NUM_MASK, IRDA_HW_FIFO_DEPTH_ENTRIES));

		s_irda.tx_buf = data + sent_entries;
		s_irda.tx_entries = chunk_entries;
		s_irda.tx_offset = 0;
		// irda_reset_sema(&s_irda.irda_tx_sema);

		irda_hal_set_tx_data_num(chunk_entries);
		IRDA_LOGI("irda set tx data num, chunk_entries:%u\r\n", chunk_entries);
		uint32_t pushed_entries = irda_push_tx_entries(chunk_entries);
		IRDA_LOGI("irda push tx entries, pushed_entries:%u\r\n", pushed_entries);
		if (pushed_entries != chunk_entries) {
			uint32_t err_level = rtos_disable_int();
			s_irda.tx_busy = false;
			rtos_enable_int(err_level);
			IRDA_LOGE("irda tx preload failed, push:%u expect:%u\r\n", pushed_entries, chunk_entries);
			return BK_FAIL;
		}

		IRDA_LOGV("[%s], chunk_entries:%u, sent_entries:%u\r\n", __func__, chunk_entries, sent_entries);
		irda_hal_start_tx();

		bk_err_t ret = rtos_get_semaphore(&s_irda.irda_tx_sema, BEKEN_WAIT_FOREVER);
		if (ret != BK_OK) {
			uint32_t err_level = rtos_disable_int();
			s_irda.tx_busy = false;
			rtos_enable_int(err_level);
			return BK_FAIL;
		}

		sent_entries += chunk_entries;
	}

	uint32_t done_level = rtos_disable_int();
	s_irda.tx_busy = false;
	rtos_enable_int(done_level);

	return BK_OK;
}

int bk_irda_read_words(uint16_t *data, uint32_t entry_count, uint32_t timeout_ms)
{
	BK_RETURN_ON_NULL(data);
	if (entry_count == 0) {
		return 0;
	}
	BK_RETURN_ON_NULL(s_irda.rx_fifo);

	while (1) {
		uint32_t int_level = rtos_disable_int();
		uint32_t fifo_bytes = kfifo_data_size(s_irda.rx_fifo);
		bool overflow = s_irda.rx_overflow;

		if (overflow) {
			s_irda.rx_overflow = false;
			rtos_enable_int(int_level);
			IRDA_LOGW("irda rx software fifo overflow detected\r\n");
			return BK_ERR_NO_MEM;
		}

		if (fifo_bytes >= IRDA_FIFO_ENTRY_BYTES) {
			uint32_t read_entries = min(entry_count, fifo_bytes / IRDA_FIFO_ENTRY_BYTES);
			uint32_t read_bytes = read_entries * IRDA_FIFO_ENTRY_BYTES;
			kfifo_get(s_irda.rx_fifo, (uint8_t *)data, read_bytes);
			rtos_enable_int(int_level);
			return (int)read_entries;
		}

		s_irda.rx_waiting = true;
		// irda_reset_sema(&s_irda.irda_rx_sema);
		rtos_enable_int(int_level);

		bk_err_t ret = rtos_get_semaphore(&s_irda.irda_rx_sema, timeout_ms);
		if (ret == kTimeoutErr) {
			uint32_t timeout_int_level = rtos_disable_int();
			s_irda.rx_waiting = false;
			rtos_enable_int(timeout_int_level);
			return BK_ERR_TIMEOUT;
		} else if (ret != BK_OK) {
			uint32_t err_int_level = rtos_disable_int();
			s_irda.rx_waiting = false;
			rtos_enable_int(err_int_level);
			return BK_FAIL;
		}
	}
}

// static void irda_reset_sema(beken_semaphore_t *sema)
// {
// 	while (rtos_get_semaphore(sema, BEKEN_NO_WAIT) == BK_OK) {
// 	}
// }

static uint32_t irda_push_tx_entries(uint32_t max_entries)
{
	uint32_t tx_fifo_count = irda_hal_get_tx_fifo_count();
	if (tx_fifo_count >= IRDA_HW_FIFO_DEPTH_ENTRIES) {
		return 0;
	}

	uint32_t fifo_free_entries = IRDA_HW_FIFO_DEPTH_ENTRIES - tx_fifo_count;
	uint32_t left_entries = s_irda.tx_entries - s_irda.tx_offset;
	uint32_t push_entries = min(max_entries, min(fifo_free_entries, left_entries));

	for (uint32_t i = 0; i < push_entries; i++) {
		irda_hal_write_byte(s_irda.tx_buf[s_irda.tx_offset]);
		IRDA_LOGI("irda push tx entries, data_word:%u\r\n", s_irda.tx_buf[s_irda.tx_offset]);
		s_irda.tx_offset++;
	}

	return push_entries;
}

static void irda_drain_rx_fifo(void)
{
	if (s_irda.rx_fifo == NULL) {
		return;
	}

	uint32_t drain_cnt = 0;
	while (irda_hal_is_rx_fifo_read_ready()) {
		uint16_t data_word = (uint16_t)irda_hal_read_byte();
		if (kfifo_put(s_irda.rx_fifo, (uint8_t *)&data_word, IRDA_FIFO_ENTRY_BYTES) != IRDA_FIFO_ENTRY_BYTES) {
			s_irda.rx_overflow = true;
		}

		/* Safety guard: prevent ISR from being stuck if HW status is abnormal. */
		if (++drain_cnt > 2048) {
			IRDA_LOGE("irda drain stuck, force break\r\n");
			break;
		}
	}
	IRDA_LOGV("irda drain rx fifo, drained:%u\r\n", drain_cnt);

	if (s_irda.rx_waiting && ((kfifo_data_size(s_irda.rx_fifo) >= IRDA_FIFO_ENTRY_BYTES) || s_irda.rx_overflow)) {
		s_irda.rx_waiting = false;
		rtos_set_semaphore(&s_irda.irda_rx_sema);
	}
}

static void __BK_IRQ irda_isr(void)
{
	uint32_t int_status = irda_hal_get_int_status();

	IRDA_LOGV("irda_isr, int_status:%x\r\n", int_status);
	irda_hal_clear_int_status(int_status);

	if (irda_hal_is_tx_done_int_triggered(int_status)) {
		IRDA_LOGV("irda tx done\r\n");
		rtos_set_semaphore(&s_irda.irda_tx_sema);
	}

	if (irda_hal_is_rx_need_rd_int_triggered(int_status)) {
		IRDA_LOGV("irda rx need read\r\n");
		irda_drain_rx_fifo();
	}

	if (irda_hal_is_rx_done_int_triggered(int_status)) {
		IRDA_LOGV("irda rx done\r\n");
		irda_drain_rx_fifo();
	}
}