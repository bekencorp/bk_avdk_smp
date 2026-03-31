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

#include <common/bk_include.h>
#include <driver/irda_types.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_irda_driver_init(void);

bk_err_t bk_irda_driver_deinit(void);

bk_err_t bk_irda_init_tx(const irda_tx_init_config_t *tx_config);

bk_err_t bk_irda_init_rx(const irda_rx_init_config_t *rx_config);

bk_err_t bk_irda_write_bytes(const void *data, uint32_t size);

int bk_irda_read_bytes(void *data, uint32_t size);

#ifdef __cplusplus
}
#endif