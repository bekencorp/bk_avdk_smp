// Copyright 2026 Beken
//
// BK7259 BL2 download UART0 transport.

#include "../download_internal.h"

#include <stdbool.h>
#include "cmsis.h"
#include "soc/bk7259/reg_base.h"
#include "uart_min_reg.h"

#define DOWNLOAD_UART0_IRQ_NUM          (4)
#define DOWNLOAD_UART_CLOCK_FREQ        (26000000u)
#define DOWNLOAD_UART0_CLOCK_BIT        (1u << 4)
#define DOWNLOAD_UART0_INT_BIT          (1u << 4)
#define DOWNLOAD_UART0_RX_GPIO          (10)
#define DOWNLOAD_UART0_TX_GPIO          (11)
#define DOWNLOAD_UART0_RX_FUNC          (95)
#define DOWNLOAD_UART0_TX_FUNC          (96)
#define DOWNLOAD_GPIO_PULL_UP_CFG       ((1u << 5) | (1u << 4))
#define DOWNLOAD_SYSTEM_REG(id)         (*(volatile uint32_t *)(SOC_SYSTEM_REG_BASE + ((id) << 2)))
#define DOWNLOAD_AON_GPIO_REG(id)       (*(volatile uint32_t *)(SOC_AON_GPIO_REG_BASE + ((id) << 2)))

#define UART_INT_RX_NEED_READ           (1u << 1)
#define UART_INT_RX_OVER_FLOW           (1u << 2)
#define UART_INT_RX_PARITY_ERROR        (1u << 3)
#define UART_INT_RX_STOP_ERROR          (1u << 4)
#define UART_INT_RX_STOP_END            (1u << 6)
#define UART_INT_RX_CLEAR_MASK          (UART_INT_RX_OVER_FLOW | UART_INT_RX_PARITY_ERROR | \
					 UART_INT_RX_STOP_ERROR | UART_INT_RX_STOP_END)

extern rx_link_buf_t rx_link_buf;

static void download_uart_enable_rx_irq(void);

static uart_min_hw_t *download_uart0_hw(void)
{
	return (uart_min_hw_t *)SOC_UART0_REG_BASE;
}

static void download_uart_peripheral_irq_enable(bool enable)
{
	if (enable) {
		DOWNLOAD_SYSTEM_REG(0x14) |= DOWNLOAD_UART0_INT_BIT;
	} else {
		DOWNLOAD_SYSTEM_REG(0x14) &= ~DOWNLOAD_UART0_INT_BIT;
	}
}

static void download_uart_set_gpio_func(uint32_t gpio_id, uint32_t func_code)
{
	DOWNLOAD_AON_GPIO_REG(gpio_id) = (func_code << 24) | DOWNLOAD_GPIO_PULL_UP_CFG;
}

static uint32_t download_uart_divider(uint32_t baud_rate)
{
	if (baud_rate == 0) {
		baud_rate = 115200;
	}

	return (DOWNLOAD_UART_CLOCK_FREQ / baud_rate) - 1;
}

static void download_uart_clock_enable(void)
{
	DOWNLOAD_SYSTEM_REG(0x0c) |= DOWNLOAD_UART0_CLOCK_BIT;
}

static void download_uart_clock_disable(void)
{
	DOWNLOAD_SYSTEM_REG(0x0c) &= ~DOWNLOAD_UART0_CLOCK_BIT;
}

static void download_uart_gpio_init(void)
{
	download_uart_set_gpio_func(DOWNLOAD_UART0_RX_GPIO, DOWNLOAD_UART0_RX_FUNC);
	download_uart_set_gpio_func(DOWNLOAD_UART0_TX_GPIO, DOWNLOAD_UART0_TX_FUNC);
}

void download_uart_init(const download_uart_config_t *cfg)
{
	uart_min_hw_t *hw = download_uart0_hw();
	uint32_t baud_rate = cfg ? cfg->baud_rate : 115200;
	uint8_t rx_threshold = cfg ? cfg->rx_fifo_threshold : 32;
	uint8_t tx_threshold = cfg ? cfg->tx_fifo_threshold : 64;

	NVIC_DisableIRQ(DOWNLOAD_UART0_IRQ_NUM);
	download_uart_peripheral_irq_enable(false);

	download_uart_clock_enable();
	hw->global_ctrl.soft_reset = 1;

	download_uart_gpio_init();

	hw->int_enable.v = 0;
	hw->int_status.v = UART_INT_RX_CLEAR_MASK;

	hw->config.v = 0;
	hw->config.tx_enable = 1;
	hw->config.rx_enable = 1;
	hw->config.data_bits = 3; /* 8-bit */
	hw->config.parity_en = 0;
	hw->config.parity = 0;
	hw->config.stop_bits = 0; /* 1 stop bit */
	hw->config.clk_div = download_uart_divider(baud_rate) & 0xffffu;

	hw->fifo_config.v = 0;
	hw->fifo_config.tx_fifo_threshold = tx_threshold;
	hw->fifo_config.rx_fifo_threshold = rx_threshold;
	hw->fifo_config.rx_stop_detect_time = 0;

	hw->flow_ctrl_config.v = 0;
	hw->wake_config.v = 0;
	hw->glitch_cancel_config.v = 0;

	download_uart_enable_rx_irq();
}

void download_uart_set_baudrate(uint32_t baud_rate)
{
	uart_min_hw_t *hw = download_uart0_hw();

	hw->config.clk_div = download_uart_divider(baud_rate) & 0xffffu;
}

static void download_uart_enable_rx_irq(void)
{
	uart_min_hw_t *hw = download_uart0_hw();

	hw->int_status.v = UART_INT_RX_CLEAR_MASK;
	hw->int_enable.v = UART_INT_RX_NEED_READ | UART_INT_RX_STOP_END;
	download_uart_peripheral_irq_enable(true);
	NVIC_EnableIRQ(DOWNLOAD_UART0_IRQ_NUM);
}

void download_uart_disable(void)
{
	uart_min_hw_t *hw = download_uart0_hw();

	hw->int_enable.v = 0;
	download_uart_peripheral_irq_enable(false);
	NVIC_DisableIRQ(DOWNLOAD_UART0_IRQ_NUM);

	while (!hw->fifo_status.tx_fifo_empty) {
	}

	hw->config.tx_enable = 0;
	hw->config.rx_enable = 0;
	download_uart_clock_disable();
}

void download_uart_write(const uint8_t *buf, uint32_t len)
{
	uart_min_hw_t *hw = download_uart0_hw();

	while (len--) {
		while (!hw->fifo_status.fifo_wr_ready) {
		}
		hw->fifo_port.tx_fifo_data_in = *buf++;
	}
}

static void download_uart_irq_handler(void)
{
	uart_min_hw_t *hw = download_uart0_hw();
	uint32_t status = hw->int_status.v;
	uint8_t *rx_buf = (uint8_t *)&rx_link_buf.rx_buf[0];

	if (status & (UART_INT_RX_NEED_READ | UART_INT_RX_STOP_END)) {
		while (hw->fifo_status.fifo_rd_ready) {
			uint8_t value = (uint8_t)hw->fifo_port.rx_fifo_data_out;
			uint16_t next_wr_idx = rx_link_buf.write_idx + 1;

			if (next_wr_idx >= sizeof(rx_link_buf.rx_buf)) {
				next_wr_idx = 0;
			}

			if (next_wr_idx != rx_link_buf.read_idx) {
				rx_buf[rx_link_buf.write_idx] = value;
				rx_link_buf.write_idx = next_wr_idx;
			}
		}
	}

	hw->int_status.v = status & UART_INT_RX_CLEAR_MASK;
}

void UART_InterruptHandler(void)
{
	download_uart_irq_handler();
}
