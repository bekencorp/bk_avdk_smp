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

#include "ipi_hal.h"

bk_err_t ipi_hal_init(ipi_hal_t *hal)
{
	if (!hal) {
		return BK_FAIL;
	}
	hal->hw = ipi_ll_get_hw();
	return BK_OK;
}

bk_err_t ipi_hal_send(ipi_hal_t *hal, uint32_t core_id, uint32_t value)
{
	if (!hal || !hal->hw) {
		return BK_FAIL;
	}
	ipi_ll_write_ipig(hal->hw, core_id, ipi_ll_pack_ipig(value));
	return BK_OK;
}

bk_err_t ipi_hal_clear(ipi_hal_t *hal, uint32_t core_id)
{
	if (!hal || !hal->hw) {
		return BK_FAIL;
	}
	uint32_t ipig_val = ipi_ll_read_ipig(hal->hw, core_id);
	ipi_ll_write_ipic(hal->hw, core_id, ipig_val);
	return BK_OK;
}

bk_err_t ipi_hal_enable_channel(ipi_hal_t *hal, uint32_t core_id)
{
	if (!hal || !hal->hw) {
		return BK_FAIL;
	}
	uint32_t int_en = ipi_ll_get_int_reg(hal->hw);
	int_en |= (1U << core_id);
	ipi_ll_set_int_reg(hal->hw, int_en);
	return BK_OK;
}

bk_err_t ipi_hal_disable_channel(ipi_hal_t *hal, uint32_t core_id)
{
	if (!hal || !hal->hw) {
		return BK_FAIL;
	}
	uint32_t int_en = ipi_ll_get_int_reg(hal->hw);
	int_en &= ~(1U << core_id);
	ipi_ll_set_int_reg(hal->hw, int_en);
	return BK_OK;
}

uint32_t ipi_hal_get_status(ipi_hal_t *hal, uint32_t core_id)
{
	if (!hal || !hal->hw) {
		return 0;
	}
	uint32_t int_status = ipi_ll_get_int_reg(hal->hw);
	int_status = (int_status >> 8) & 0x1F;
	return (int_status >> core_id) & 0x1;
}

uint32_t ipi_hal_get_all_status(ipi_hal_t *hal)
{
	if (!hal || !hal->hw) {
		return 0;
	}
	uint32_t int_status = ipi_ll_get_int_reg(hal->hw);
	return (int_status >> 8) & 0x1F;
}

uint32_t ipi_hal_get_device_status(ipi_hal_t *hal)
{
	if (!hal || !hal->hw) {
		return 0;
	}
	return ipi_ll_read_state(hal->hw);
}

