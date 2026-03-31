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
#include "irda_driver.h"
#include "irda_hal.h"

typedef struct {
	uint16_t *rx_buf;
	uint32_t rx_size;
	volatile uint32_t rx_offset;
	beken_semaphore_t irda_rx_sema;
	beken_semaphore_t irda_tx_sema;
} irda_driver_t;

static bool s_irda_driver_is_init = false;
static irda_driver_t s_irda = {0};

static void irda_isr(void);

static void irda_init_gpio(void)
{
	gpio_dev_unmap(IRDA_PIN);
	gpio_dev_map(IRDA_PIN, GPIO_DEV_IRDA);
}

static void irda_deinit_gpio(void)
{
	gpio_dev_unmap(IRDA_PIN);
}

/* 1) power up irda
 * 2) enable system irda interrupt
 * 3) init irda gpio
 */
static void irda_init_common(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_IRDA, CLK_PWR_CTRL_PWR_UP);
	sys_drv_int_enable(IRDA_INTERRUPT_CTRL_BIT);
	irda_init_gpio();
}

static void irda_deinit_common(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_IRDA, CLK_PWR_CTRL_PWR_DOWN);
	sys_drv_int_disable(IRDA_INTERRUPT_CTRL_BIT);
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
}

bk_err_t bk_irda_driver_init(void)
{
	if (s_irda_driver_is_init) {
		return BK_OK;
	}

	irda_hal_init();
	bk_int_isr_register(INT_SRC_IRDA, irda_isr, NULL);
	irda_sw_init();

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
		return BK_OK;
	}
	bk_int_isr_register(INT_SRC_IRDA, NULL, NULL);
	irda_sw_deinit();
	s_irda_driver_is_init = false;
	return BK_OK;
}

bk_err_t bk_irda_init_tx(const irda_tx_init_config_t *tx_config)
{
	irda_init_common();
	irda_hal_init_tx(tx_config);
	irda_hal_set_tx_enable(true);
	irda_hal_set_tx_done_int(true);
	irda_hal_set_rx_timeout_int(true);

	return BK_OK;
}

bk_err_t bk_irda_init_rx(const irda_rx_init_config_t *rx_config)
{
	irda_init_common();
	irda_hal_init_rx(rx_config);
	irda_hal_set_rx_enable(true);
	irda_hal_set_rx_int(true);

	return BK_OK;
}

bk_err_t bk_irda_write_bytes(const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);

	uint16_t *tx_data = (uint16_t *)data;
	uint32_t tx_data_num = size / 2;

	for (uint32_t i = 0; i < tx_data_num; i++) {
		irda_hal_write_byte(tx_data[i]);
	}
	irda_hal_set_tx_data_num(tx_data_num);
	IRDA_LOGV("[%s], tx_data_num:%d\r\n", __func__, tx_data_num);
	irda_hal_set_tx_fifo_threshold(1);
	irda_hal_start_tx();
	rtos_get_semaphore(&s_irda.irda_tx_sema, BEKEN_WAIT_FOREVER);
	return BK_OK;
}

int bk_irda_read_bytes(void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);

	uint32_t int_level = rtos_disable_int();
	s_irda.rx_buf = (uint16_t *)data;
	s_irda.rx_size = size;
	s_irda.rx_offset = 0;
	rtos_enable_int(int_level);

	rtos_get_semaphore(&s_irda.irda_rx_sema, BEKEN_WAIT_FOREVER);

	return s_irda.rx_offset;
}

static void __BK_IRQ irda_isr(void)
{
	uint32_t int_status = irda_hal_get_int_status();
	uint32_t rx_num = 0;
	uint32_t rx_offset = s_irda.rx_offset;

	IRDA_LOGV("irda_isr, int_status:%x\r\n", int_status);
	irda_hal_clear_int_status(int_status);

	if (irda_hal_is_tx_done_int_triggered(int_status)) {
		IRDA_LOGV("irda tx done\r\n");
		rtos_set_semaphore(&s_irda.irda_tx_sema);
	}

	if (irda_hal_is_rx_done_int_triggered(int_status)) {
		IRDA_LOGV("irda rx done\r\n");
		rx_num = irda_hal_get_rx_data_num();
		IRDA_LOGV("isr rx_data_num:%d\r\n", rx_num);
		for (int i = 0; i < rx_num; i++) {
			s_irda.rx_buf[rx_offset++] = irda_hal_read_byte();
		}
		s_irda.rx_offset = rx_offset;
		rtos_set_semaphore(&s_irda.irda_rx_sema);
	}

	if (irda_hal_is_rx_need_rd_int_triggered(int_status)) {
		IRDA_LOGV("irda rx need read\r\n");
	}
}