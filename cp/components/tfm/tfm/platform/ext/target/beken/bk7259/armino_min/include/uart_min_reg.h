// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: self-contained UART0 register layout.
//
// Copied from SDK soc/bk7259/soc/uart_struct.h with the CONFIG_ENABLE_FILTER_GLITCH
// dependency removed (the glitch-cancel register is always laid out so the
// struct size is stable regardless of build config). This header intentionally
// does NOT include SDK uart_ll.h, which pulls in gpio_map.h pin-mux coupling.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile struct {
	uint32_t dev_id;        /* REG_0x00 */
	uint32_t dev_version;   /* REG_0x01 */
	union {                 /* REG_0x02 */
		struct {
			uint32_t soft_reset:      1;
			uint32_t clk_gate_bypass: 1;
			uint32_t reserved:       30;
		};
		uint32_t v;
	} global_ctrl;
	uint32_t dev_status;    /* REG_0x03 */
	union {                 /* REG_0x04 */
		struct {
			uint32_t tx_enable:  1;
			uint32_t rx_enable:  1;
			uint32_t reserved1:  1;
			uint32_t data_bits:  2;
			uint32_t parity_en:  1;
			uint32_t parity:     1;
			uint32_t stop_bits:  1;
			uint32_t clk_div:   16;
			uint32_t reserved:   8;
		};
		uint32_t v;
	} config;
	union {                 /* REG_0x05 */
		struct {
			uint32_t tx_fifo_threshold:   8;
			uint32_t rx_fifo_threshold:   8;
			uint32_t rx_stop_detect_time: 2;
			uint32_t reserved:           14;
		};
		uint32_t v;
	} fifo_config;
	union {                 /* REG_0x06 */
		struct {
			uint32_t tx_fifo_count: 8;
			uint32_t rx_fifo_count: 8;
			uint32_t tx_fifo_full:  1;
			uint32_t tx_fifo_empty: 1;
			uint32_t rx_fifo_full:  1;
			uint32_t rx_fifo_empty: 1;
			uint32_t fifo_wr_ready: 1;
			uint32_t fifo_rd_ready: 1;
			uint32_t reserved:     10;
		};
		uint32_t v;
	} fifo_status;
	union {                 /* REG_0x07 */
		struct {
			uint32_t tx_fifo_data_in:  8;
			uint32_t rx_fifo_data_out: 8;
			uint32_t reserved:        16;
		};
		uint32_t v;
	} fifo_port;
	union {                 /* REG_0x08 */
		struct {
			uint32_t tx_fifo_need_write: 1;
			uint32_t rx_fifo_need_read:  1;
			uint32_t rx_fifo_overflow:   1;
			uint32_t rx_parity_err:      1;
			uint32_t rx_stop_bits_err:   1;
			uint32_t tx_finish:          1;
			uint32_t rx_finish:          1;
			uint32_t rxd_wakeup:         1;
			uint32_t reserved:          24;
		};
		uint32_t v;
	} int_enable;
	union {                 /* REG_0x09, W1C for error/finish bits */
		struct {
			uint32_t tx_fifo_need_write: 1;
			uint32_t rx_fifo_need_read:  1;
			uint32_t rx_fifo_overflow:   1;
			uint32_t rx_parity_err:      1;
			uint32_t rx_stop_bits_err:   1;
			uint32_t tx_finish:          1;
			uint32_t rx_finish:          1;
			uint32_t rxd_wakeup:         1;
			uint32_t reserved:          24;
		};
		uint32_t v;
	} int_status;
	union {                 /* REG_0x0A */
		struct {
			uint32_t flow_ctrl_low_cnt:  8;
			uint32_t flow_ctrl_high_cnt: 8;
			uint32_t flow_ctrl_en:       1;
			uint32_t rts_polarity_sel:   1;
			uint32_t cts_polarity_sel:   1;
			uint32_t reserved:          13;
		};
		uint32_t v;
	} flow_ctrl_config;
	union {                 /* REG_0x0B */
		struct {
			uint32_t wake_cnt:             10;
			uint32_t txd_wait_cnt:         10;
			uint32_t rxd_wake_en:           1;
			uint32_t txd_wake_en:           1;
			uint32_t rxd_neg_edge_wake_en:  1;
			uint32_t reserved:              9;
		};
		uint32_t v;
	} wake_config;
	union {                 /* REG_0x0C */
		struct {
			uint32_t glitch_width: 15;
			uint32_t cancel_en:     1;
			uint32_t reserved:     16;
		};
		uint32_t v;
	} glitch_cancel_config;
} uart_min_hw_t;

#ifdef __cplusplus
}
#endif
