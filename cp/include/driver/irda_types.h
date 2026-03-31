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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_IRDA_NOT_INIT              (BK_ERR_IRDA_BASE - 1) /**< IRDA driver not init */

/* ---- Parameter range limits (derived from hardware register widths) ---- */
#define IRDA_CLK_FREQ_INPUT_MAX            127U    /* 7-bit  [1, 127]   */
#define IRDA_CARRIER_PERIOD_CYCLE_MAX      255U    /* 8-bit  [0, 255]   */
#define IRDA_CARRIER_DUTY_CYCLE_MAX        255U    /* 8-bit  [0, 255]   */
#define IRDA_RX_TIMEOUT_US_MAX             65535U  /* 16-bit [1, 65535] */
#define IRDA_RX_START_THRESHOLD_US_MAX     4095U   /* 12-bit [0, 4095]  */

typedef struct {
	uint32_t clk_freq_input;        /**< Clock Division Factor, range [1, 127]. IRDA default clock is 26MHz */
	uint32_t carrier_period_cycle;  /**< Carrier period cycle, range [0, 255], unit: 1us */
	uint32_t carrier_duty_cycle;    /**< Carrier duty cycle, range [0, 255], unit: 1us */
	uint32_t tx_initial_level;      /**< TX initial level, 0 or 1 */
} irda_tx_init_config_t;

typedef struct {
	uint32_t clk_freq_input;           /**< Clock Division Factor, range [1, 127]. IRDA default clock is 26MHz */
	uint32_t rx_timeout_us;            /**< RX done timeout, range [1, 65535], unit: 1us.
	                                        No edge transition detected for this duration means transfer ended */
	uint32_t rx_start_threshold_us;    /**< RX start threshold, range [0, 4095], unit: 1us.
	                                        First data packet exceeding this threshold begins the transfer */
	uint32_t rx_initial_level;         /**< RX initial level, 0 or 1 */
} irda_rx_init_config_t;

#ifdef __cplusplus
}
#endif
