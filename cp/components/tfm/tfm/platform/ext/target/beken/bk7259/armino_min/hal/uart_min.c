// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: self-contained UART blocking-TX driver.
//
// The secure world (BL2 + SPE) logs through UART1 (TFM_DRIVER_STDIO =
// Driver_USART1, matching the bk7259 bringup), while the non-secure world logs
// through UART0 (NS_DRIVER_STDIO = Driver_USART0). This driver therefore maps
// uart_min_hw_t onto the requested UART instance (UART0 0x44820000 / UART1
// 0x45830000 / UART2 0x45840000), configures 8N1 + baud divider, and writes
// bytes by polling tx_fifo_full. No RX / DMA / ISR / PM / flow-control is used;
// GPIO pin-mux for the debug UART is established by the bootrom (UART1 = GPIO0
// TX / GPIO1 RX), so this file does not include the SDK uart_ll.h (gpio_map.h
// coupling).
//
// Exports the SDK-compatible symbol contract so TF-M call sites are unchanged.

#include <stdint.h>
#include <stdbool.h>
#include <common/bk_include.h>
#include <driver/uart.h>
#include "bk_uart.h"
#include "uart_min_reg.h"
#include "sys_ll.h"
#include "gpio_ll.h"

/* IO-matrix function codes for the debug UART pads (soc/bk7259
 * hal_io_matrix_types_impl.h). Used with gpio_ll_set_cfg_gpio_fun_sel to route
 * the pad without dragging in the full SDK gpio driver. */
#define FUNC_CODE_UART0_RXD_VAL   95
#define FUNC_CODE_UART0_TXD_VAL   96
#define FUNC_CODE_UART1_RXD_VAL   97
#define FUNC_CODE_UART1_TXD_VAL   98

#define UART0_REG_BASE        (0x44820000)
#define UART1_REG_BASE        (0x45830000)
#define UART2_REG_BASE        (0x45840000)
#ifndef UART_CLOCK_FREQ_120M
#define UART_CLOCK_FREQ_120M  (120000000)
#endif
#ifndef UART_CLOCK_FREQ_26M
#define UART_CLOCK_FREQ_26M   (26000000)
#endif

/* Debug UART pin map (mirrors soc/bk7259/Kconfig defaults / idk sdkconfig.h):
 *   UART0 TX=GPIO11 RX=GPIO10 (non-secure log, bootrom-muxed)
 *   UART1 TX=GPIO0  RX=GPIO1  (secure log, this driver muxes it). */
#define UART0_TX_GPIO   11
#define UART0_RX_GPIO   10
#define UART1_TX_GPIO   0
#define UART1_RX_GPIO   1

/* Resolve the register block for a UART id. Returns NULL for unsupported ids
 * (the secure-boot path only uses UART0/UART1, UART2 kept for completeness). */
static uart_min_hw_t *uart_min_hw(uart_id_t id)
{
	switch (id) {
	case UART_ID_0:
		return (uart_min_hw_t *)UART0_REG_BASE;
	case UART_ID_1:
		return (uart_min_hw_t *)UART1_REG_BASE;
	case UART_ID_2:
		return (uart_min_hw_t *)UART2_REG_BASE;
	default:
		return (void *)0;
	}
}

/* Per-UART low-level bring-up: power the UART clock, select the XTAL-26M source
 * (matches the bk7259 bringup secure log), and route the TX/RX GPIO pads via
 * the IO-matrix. UART0 is already brought up by the bootrom (non-secure log);
 * UART1 (secure log on GPIO0/GPIO1) is fully set up here. Pad routing uses the
 * pure inline gpio_ll (gpio_ll_set_cfg_gpio_fun_sel) so no SDK gpio driver is
 * pulled into the secure world. */
static void uart_min_bringup(uart_id_t id)
{
	switch (id) {
	case UART_ID_0:
		sys_ll_set_cpu_device_clk_enable_uart0_cken(1);
		sys_ll_set_cpu_clk_div_mode2_cksel_uart0(0); /* XTAL 26M */
		gpio_ll_set_cfg_gpio_fun_sel(UART0_TX_GPIO, FUNC_CODE_UART0_TXD_VAL);
		gpio_ll_set_cfg_gpio_fun_sel(UART0_RX_GPIO, FUNC_CODE_UART0_RXD_VAL);
		break;
	case UART_ID_1:
		sys_ll_set_cpu_device_clk_enable_uart1_cken(1);
		sys_ll_set_cpu_clk_div_mode2_cksel_uart1(0); /* XTAL 26M */
		gpio_ll_set_cfg_gpio_fun_sel(UART1_TX_GPIO, FUNC_CODE_UART1_TXD_VAL);
		gpio_ll_set_cfg_gpio_fun_sel(UART1_RX_GPIO, FUNC_CODE_UART1_RXD_VAL);
		break;
	default:
		break;
	}
}

static uint32_t uart_min_clk_freq(const uart_config_t *config)
{
	/* armino_min selects the XTAL-26M UART source (uart_min_bringup); the baud
	 * divider is therefore computed against 26M regardless of src_clk. */
	(void)config;
	return UART_CLOCK_FREQ_26M;
}

/* Matches the shared SDK <driver/uart.h> prototype bk_uart_driver_init(void).
 * (The psa_level3-lineage secure call sites that passed a uart_id_t arg have
 * been aligned to the no-arg SDK contract.) */
bk_err_t bk_uart_driver_init(void)
{
	return BK_OK;
}

/* The debug UART0 pin-mux is established by the bootrom before BL2 runs (and
 * PPC keeps the pads attributed correctly); the secure-boot path does not
 * reconfigure GPIO. Provide the contract symbol Driver_USART.c expects. */
bk_err_t bk_gpio_driver_init(void)
{
	return BK_OK;
}

bk_err_t bk_uart_driver_deinit(void)
{
	return BK_OK;
}

bk_err_t bk_uart_init(uart_id_t id, const uart_config_t *config)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (!hw) {
		return BK_OK;
	}
	if (!config) {
		return BK_ERR_NULL_PARAM;
	}

	/* Bring up the UART clock + GPIO mux before touching the controller. */
	uart_min_bringup(id);

	uint32_t clk = uart_min_clk_freq(config);
	uint32_t clk_div = (config->baud_rate ? (clk / config->baud_rate - 1) : 0);

	hw->global_ctrl.soft_reset = 1;

	uart_min_hw_t cfg;
	cfg.config.v = 0;
	cfg.config.tx_enable = 1;
	cfg.config.rx_enable = 0;
	cfg.config.data_bits = config->data_bits;
	cfg.config.parity_en = (config->parity != UART_PARITY_NONE) ? 1 : 0;
	cfg.config.parity = (config->parity == UART_PARITY_ODD) ? 1 : 0;
	cfg.config.stop_bits = config->stop_bits;
	cfg.config.clk_div = clk_div & 0xffff;
	hw->config.v = cfg.config.v;

	/* No interrupts, no flow control. */
	hw->int_enable.v = 0;
	hw->flow_ctrl_config.v = 0;
	hw->wake_config.v = 0;

	return BK_OK;
}

bk_err_t bk_uart_deinit(uart_id_t id)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (hw) {
		hw->config.tx_enable = 0;
	}
	return BK_OK;
}

bk_err_t bk_uart_set_enable_tx(uart_id_t id, bool enable)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (hw) {
		hw->config.tx_enable = enable ? 1 : 0;
	}
	return BK_OK;
}

bk_err_t bk_uart_set_baud_rate(uart_id_t id, uint32_t baud_rate)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (!hw || baud_rate == 0) {
		return BK_ERR_NULL_PARAM;
	}
	/* UART source is XTAL-26M (see uart_min_bringup). */
	uint32_t clk_div = UART_CLOCK_FREQ_26M / baud_rate - 1;
	hw->config.clk_div = clk_div & 0xffff;
	return BK_OK;
}

/* BL2/SPE only transmit; RX is never used in the secure-boot path. Provide the
 * contract symbol so CMSIS Driver_USART links, returning 0 bytes. */
int bk_uart_read_bytes(uart_id_t id, void *data, uint32_t size, uint32_t timeout_ms)
{
	(void)id;
	(void)data;
	(void)size;
	(void)timeout_ms;
	return 0;
}

bk_err_t bk_uart_set_enable_rx(uart_id_t id, bool enable)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (hw) {
		hw->config.rx_enable = enable ? 1 : 0;
	}
	return BK_OK;
}

bk_err_t bk_uart_write_bytes(uart_id_t id, const void *data, uint32_t size)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (!hw || !data) {
		return BK_ERR_NULL_PARAM;
	}

	const uint8_t *p = (const uint8_t *)data;
	for (uint32_t i = 0; i < size; i++) {
		while (hw->fifo_status.tx_fifo_full) {
			;
		}
		hw->fifo_port.tx_fifo_data_in = p[i];
	}
	return BK_OK;
}

bk_err_t uart_write_string(uart_id_t id, const char *string)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (!hw || !string) {
		return BK_ERR_NULL_PARAM;
	}
	const char *p = string;
	while (*p) {
		while (hw->fifo_status.tx_fifo_full) {
			;
		}
		hw->fifo_port.tx_fifo_data_in = (uint8_t)(*p++);
	}
	return BK_OK;
}

void bk_uart_wait_tx_over(uart_id_t id)
{
	uart_min_hw_t *hw = uart_min_hw(id);
	if (!hw) {
		return;
	}
	while (!hw->fifo_status.tx_fifo_empty) {
		;
	}
}

bk_err_t bk_uart_isr_set_priority(uart_id_t id, uint8_t prio)
{
	(void)id;
	(void)prio;
	return BK_OK;
}

/* psa_level3-lineage NS driver-state marker (Driver_USART.c). No software fifo
 * / driver bookkeeping in the minimal driver, so this is a no-op. */
bk_err_t bk_uart_mark_initialized(uart_id_t id)
{
	(void)id;
	return BK_OK;
}
