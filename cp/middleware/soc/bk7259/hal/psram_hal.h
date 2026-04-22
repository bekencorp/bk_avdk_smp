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

#pragma once

#include "hal_config.h"
#include <soc/soc.h>
#include "psram_ll_macro_def.h"
#include <driver/psram_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	PSRAM_MODE1 = 0xE8054043,
	PSRAM_MODE2 = 0xD8054043,
	PSRAM_MODE3 = 0xA8054043,
	PSRAM_MODE4 = 0xB80B4045,
	PSRAM_MODE5 = 0x100b4045,
	PSRAM_MODE6 = 0xD8054041,
	PSRAM_MODE7 = 0xd8054049,
	PSRAM_MODE8 = 0xB0054045,
	PSRAM_MODE9 = 0xF80b4049,    //for PSRAM_SCB18X128XX_OAF_ID
	PSRAM_MODE_INVAL,
} psram_mode_t;

#define PSRAM_APS6408L_ID         (0x8d09)
#define PSRAM_W955D8MKY_5J_ID     (0xd0aa)
#define PSRAM_APS128XXO_OB9_ID    (0x8d08)
#define PSRAM_SCB18X128XX_OAF_ID  (0x9a08)

/* REG_0x00 */
#define psram_hal_get_reg0_value(psram_id)                 psram_ll_get_reg0_value(psram_id)
#define psram_hal_set_reg0_value(psram_id, value)          psram_ll_set_reg0_value(psram_id, value)
#define psram_hal_get_reg0_value_legacy()                  psram_ll_get_reg0_value_legacy()
#define psram_hal_set_reg0_value_legacy(value)             psram_ll_set_reg0_value_legacy(value)

/* REG_0x01 */
#define psram_hal_get_reg1_value(psram_id)                 psram_ll_get_reg1_value(psram_id)
#define psram_hal_set_reg1_value(psram_id, value)           psram_ll_set_reg1_value(psram_id, value)
#define psram_hal_get_reg1_value_legacy()                   psram_ll_get_reg1_value_legacy()
#define psram_hal_set_reg1_value_legacy(value)             psram_ll_set_reg1_value_legacy(value)

/* REG_0x02 */
#define psram_hal_get_reg2_value(psram_id)                 psram_ll_get_reg2_value(psram_id)
#define psram_hal_set_reg2_value(psram_id, value)           psram_ll_set_reg2_value(psram_id, value)
#define psram_hal_get_reg2_value_legacy()                   psram_ll_get_reg2_value_legacy()
#define psram_hal_set_reg2_value_legacy(value)             psram_ll_set_reg2_value_legacy(value)

/* REG_0x03 */
#define psram_hal_get_reg3_value(psram_id)                 psram_ll_get_reg3_value(psram_id)
#define psram_hal_set_reg3_value(psram_id, value)          psram_ll_set_reg3_value(psram_id, value)
#define psram_hal_get_reg3_value_legacy()                   psram_ll_get_reg3_value_legacy()
#define psram_hal_set_reg3_value_legacy(value)             psram_ll_set_reg3_value_legacy(value)

/* REG_0x04 */
#define psram_hal_get_mode_value(psram_id)                 psram_ll_get_mode_value(psram_id)
#define psram_hal_set_mode_value(psram_id, value)           psram_ll_set_mode_value(psram_id, value)
#define psram_hal_get_mode_value_legacy()                   psram_ll_get_mode_value_legacy()
#define psram_hal_set_mode_value_legacy(value)             psram_ll_set_mode_value_legacy(value)

/* REG_0x05 */
#define psram_hal_get_reg5_value(psram_id)                  psram_ll_get_reg5_value(psram_id)
#define psram_hal_set_reg5_value(psram_id, value)           psram_ll_set_reg5_value(psram_id, value)
#define psram_hal_get_reg5_value_legacy()                   psram_ll_get_reg5_value_legacy()
#define psram_hal_set_reg5_value_legacy(value)              psram_ll_set_reg5_value_legacy(value)

/* REG_0x06 */
#define psram_hal_get_reg6_value()                         psram_ll_get_reg6_value()
#define psram_hal_set_reg6_value(value)                    psram_ll_set_reg6_value(value)

/* REG_0x07 */
#define psram_hal_get_reg7_value()                         psram_ll_get_reg7_value()
#define psram_hal_set_reg7_value(value)                    psram_ll_set_reg7_value(value)

/* REG_0x08 */
#define psram_hal_get_reg8_value(psram_id)                  psram_ll_get_reg8_value(psram_id)
#define psram_hal_set_reg8_value(psram_id, value)           psram_ll_set_reg8_value(psram_id, value)
#define psram_hal_get_reg8_value_legacy()                   psram_ll_get_reg8_value_legacy()
#define psram_hal_set_reg8_value_legacy(value)              psram_ll_set_reg8_value_legacy(value)

/* REG_0x09 */
#define psram_hal_get_reg9_value(psram_id)                  psram_ll_get_write_address(psram_id)
#define psram_hal_set_reg9_value(psram_id, value)           psram_ll_set_write_address(psram_id, value)
#define psram_hal_get_reg9_value_legacy()                   psram_ll_get_write_address_legacy()
#define psram_hal_set_reg9_value_legacy(value)              psram_ll_set_write_address_legacy(value)

/* REG_0x0a */
#define psram_hal_get_write_data(psram_id)                  psram_ll_get_write_data(psram_id)
#define psram_hal_set_write_data(psram_id, value)           psram_ll_set_write_data(psram_id, value)
#define psram_hal_get_write_data_legacy()                   psram_ll_get_write_data_legacy()
#define psram_hal_set_write_data_legacy(value)              psram_ll_set_write_data_legacy(value)

/* REG_0x0b */
#define psram_hal_get_regb_value(psram_id)                  psram_ll_get_regb_value(psram_id)
#define psram_hal_set_regb_value(psram_id, value)           psram_ll_set_regb_value(psram_id, value)
#define psram_hal_get_regb_value_legacy()                   psram_ll_get_regb_value_legacy()
#define psram_hal_set_regb_value_legacy(value)              psram_ll_set_regb_value_legacy(value)

// New functions with psram_id parameter
int psram_hal_set_write_through_with_id(psram_id_t psram_id, psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end);
void psram_hal_set_sf_reset_with_id(psram_id_t psram_id, uint32_t value);
void psram_hal_set_cmd_reset_with_id(psram_id_t psram_id);
void psram_hal_cmd_write_with_id(psram_id_t psram_id, uint32_t addr, uint32_t value);
uint32_t psram_hal_cmd_read_with_id(psram_id_t psram_id, uint32_t addr);
void psram_hal_set_transfer_mode_with_id(psram_id_t psram_id, uint32_t value);
uint32_t psram_hal_config_init_with_id(psram_id_t psram_id, uint32_t id);

// Legacy functions for backward compatibility (use PSRAM_ID_0)
int psram_hal_set_write_through(psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end);
void psram_hal_set_sf_reset(uint32_t value);
void psram_hal_set_cmd_reset(void);
void psram_hal_cmd_write(uint32_t addr, uint32_t value);
uint32_t psram_hal_cmd_read(uint32_t addr);
void psram_hal_set_transfer_mode(uint32_t value);
void psram_hal_power_clk_enable(uint8_t enable);
void psram_hal_reset(void);
void psram_hal_config(void);
uint32_t psram_hal_config_init(uint32_t id);
void psram_hal_set_clk(psram_clk_t clk);
/** Set PSRAM clock by id. Note: PSRAM_640M means 640M source; actual bus clock is half (divided by 2). */
void psram_hal_set_clk_with_id(psram_id_t psram_id, psram_clk_t clk);
void psram_hal_set_voltage(psram_voltage_t voltage);
void psram_hal_set_default_clk(void);
void psram_hal_set_default_clk_with_id(psram_id_t psram_id);

/** Set PSRAM interleave step (0=256B 1=128B 2=64B 3=32B). Used when CONFIG_PSRAM_INTERLEAVE. */
void psram_hal_set_interleave_config(uint32_t step);

#if CFG_HAL_DEBUG_PSRAM
void psram_struct_dump(void);
#else
#define psram_struct_dump(void)
#endif

#ifdef __cplusplus
}
#endif


