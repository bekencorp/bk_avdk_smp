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

typedef struct {
	uint32_t clk_freq_input;
	uint32_t carrier_period_cycle; /**< carrier period cycle, unit:1us */
	uint32_t carrier_duty_cycle;   /**< carrier duty cycle, unit:1us */
} irda_tx_init_config_t;

typedef struct {
	uint32_t clk_freq_input;
	uint32_t rx_timeout_us;
	uint32_t rx_start_threshold_us;
} irda_rx_init_config_t;

#ifdef __cplusplus
}
#endif

