// Copyright 2020-2025 Beken
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

#include "hal_config.h"
#include "hal_port.h"
#include "ipi_ll.h"

typedef struct {
	ipi_hw_t *hw;
} ipi_hal_t;

bk_err_t ipi_hal_init(ipi_hal_t *hal);
bk_err_t ipi_hal_send(ipi_hal_t *hal, uint32_t core_id, uint32_t value);
bk_err_t ipi_hal_clear(ipi_hal_t *hal, uint32_t core_id);

bk_err_t ipi_hal_enable_channel(ipi_hal_t *hal, uint32_t core_id);
bk_err_t ipi_hal_disable_channel(ipi_hal_t *hal, uint32_t core_id);

uint32_t ipi_hal_get_status(ipi_hal_t *hal, uint32_t core_id);
uint32_t ipi_hal_get_all_status(ipi_hal_t *hal);
uint32_t ipi_hal_get_device_status(ipi_hal_t *hal);

#ifdef __cplusplus
}
#endif

