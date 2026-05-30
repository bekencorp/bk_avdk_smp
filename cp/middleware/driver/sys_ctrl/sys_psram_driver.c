// Copyright 2020-2021 Beken
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

#include "sys_hal.h"
#include "sys_driver.h"
#include "sys_ll.h"

//sys_ctrl CMD: CMD_QSPI_VDDRAM_VOLTAGE
void sys_drv_set_qspi_vddram_voltage(uint32_t param)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_set_qspi_vddram_voltage(param);

	sys_drv_exit_critical(int_level);
}

//sys_ctrl CMD: CMD_QSPI_IO_VOLTAGE
void sys_drv_set_qspi_io_voltage(uint32_t param)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_set_qspi_io_voltage(param);

	sys_drv_exit_critical(int_level);
}

/**  psram Start **/
uint32_t sys_drv_psram_volstage_sel(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_volstage_sel(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_xtall_osc_enable(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_xtall_osc_enable(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_dco_enable(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_doc_enable(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_dpll_enable(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_dpll_enable(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_ldo_enable(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_ldo_enable(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_psldo_vsel(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_psldo_vsel(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_psldo_vset(uint32_t psldo_swb, uint32_t vpsramsel)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_psldo_vset(psldo_swb, vpsramsel);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_psram0_disckg(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_psram0_disckg(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;

}

uint32_t sys_drv_psram_psram1_disckg(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_psram1_disckg(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;

}

/* Id-routed wrapper around sys_drv_psram_psram[01]_disckg(). Lets
 * callers (PSRAM driver / retention recovery) drop the id branch
 * and keep the code symmetric with sys_drv_psram_clk_sel_with_id()
 * / sys_drv_psram_set_clkdiv_with_id(). */
uint32_t sys_drv_psram_disckg_with_id(uint32_t id, uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_psram_disckg_with_id(id, value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

/* PSRAM I/O pad latch control (ana_reg5.gpio_latch, bit7).
 *
 * Although the bit physically lives in the sys analog register space
 * (not in the PSRAM controller), its only consumers are the PSRAM
 * retention / power-cycle paths: setting it to 1 latches the PSRAM0
 * and PSRAM1 pads at 3V so they keep a valid level while the M55 /
 * AHBP sub-system is powered down; clearing it releases the pads
 * before the controller drives them again.
 *
 * Wrapping the bit-write in sys_drv layer keeps callers (psram
 * driver, psram retention test) at the driver level and adds the
 * standard critical-section protection. */
uint32_t sys_drv_set_psram_pad_latch(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_ll_set_ana_reg5_gpio_latch(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

/**  psram End **/

