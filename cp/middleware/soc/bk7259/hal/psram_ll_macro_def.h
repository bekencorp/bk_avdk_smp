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

#ifdef __cplusplus
extern "C" {
#endif

#include <soc/soc.h>
#include <driver/psram_types.h>

/**
 * LL layer supports multi-PSRAM instance access by psram_id.
 * soc/common HAL can use this macro to select correct call signatures.
 */
#define PSRAM_LL_HAS_PSRAM_ID 1

// Get PSRAM register base address by psram_id
static inline uint32_t psram_ll_get_reg_base(psram_id_t psram_id)
{
	switch (psram_id) {
		case PSRAM_ID_0:
			return SOC_PSRAM0_REG_BASE;
		case PSRAM_ID_1:
			return SOC_PSRAM1_REG_BASE;
		default:
			return SOC_PSRAM0_REG_BASE;  // Default to PSRAM0
	}
}

// Get PSRAM data base address by psram_id
static inline uint32_t psram_ll_get_data_base(psram_id_t psram_id)
{
	switch (psram_id) {
		case PSRAM_ID_0:
			return SOC_PSRAM0_DATA_BASE;
		case PSRAM_ID_1:
			return SOC_PSRAM1_DATA_BASE;
		default:
			return SOC_PSRAM0_DATA_BASE;  // Default to PSRAM0
	}
}

#define PSRAM_LL_REG_BASE    (SOC_PSRAM_REG_BASE)  // For backward compatibility, use PSRAM0

/* REG_0x00 */
#define PSRAM_REG0(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x0 * 4)
#define PSRAM_REG0_LEGACY     (PSRAM_LL_REG_BASE + 0x0 * 4)  // For backward compatibility

static inline uint32_t psram_ll_get_reg0_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG0(psram_id));
}

// Legacy function for backward compatibility (uses PSRAM0)
static inline uint32_t psram_ll_get_reg0_value_legacy(void)
{
	return REG_READ(PSRAM_REG0_LEGACY);
}

/* REG_0x01 */
#define PSRAM_REG1(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x1 * 4)
#define PSRAM_REG1_LEGACY     (PSRAM_LL_REG_BASE + 0x1 * 4)

static inline uint32_t psram_ll_get_reg1_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG1(psram_id));
}

// Legacy function for backward compatibility
static inline uint32_t psram_ll_get_reg1_value_legacy(void)
{
	return REG_READ(PSRAM_REG1_LEGACY);
}

/* REG_0x02 */
#define PSRAM_REG2(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x2 * 4)
#define PSRAM_REG2_LEGACY     (PSRAM_LL_REG_BASE + 0x2 * 4)
#define PSRAM_SF_RESET_POS (0)
#define PSRAM_SF_RESET_MASK (0x1)

static inline uint32_t psram_ll_get_reg2_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG2(psram_id));
}

static inline void psram_ll_set_reg2_value(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG2(psram_id), value);
}

static inline uint32_t psram_ll_get_sf_reset_value(psram_id_t psram_id)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG2(psram_id));
	reg_value = ((reg_value >> PSRAM_SF_RESET_POS) & PSRAM_SF_RESET_MASK);
	return reg_value;
}

static inline void psram_ll_set_sf_reset_value(psram_id_t psram_id, uint32_t value)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG2(psram_id));
	reg_value &= ~(PSRAM_SF_RESET_MASK << PSRAM_SF_RESET_POS);
	reg_value |= ((value & PSRAM_SF_RESET_MASK) << PSRAM_SF_RESET_POS);
	REG_WRITE(PSRAM_REG2(psram_id), reg_value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_reg2_value_legacy(void)
{
	return REG_READ(PSRAM_REG2_LEGACY);
}

static inline void psram_ll_set_reg2_value_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REG2_LEGACY, value);
}

static inline uint32_t psram_ll_get_sf_reset_value_legacy(void)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG2_LEGACY);
	reg_value = ((reg_value >> PSRAM_SF_RESET_POS) & PSRAM_SF_RESET_MASK);
	return reg_value;
}

static inline void psram_ll_set_sf_reset_value_legacy(uint32_t value)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG2_LEGACY);
	reg_value &= ~(PSRAM_SF_RESET_MASK << PSRAM_SF_RESET_POS);
	reg_value |= ((value & PSRAM_SF_RESET_MASK) << PSRAM_SF_RESET_POS);
	REG_WRITE(PSRAM_REG2_LEGACY, reg_value);
}

/* REG_0x03 */
#define PSRAM_REG3(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x3 * 4)
#define PSRAM_REG3_LEGACY     (PSRAM_LL_REG_BASE + 0x3 * 4)

static inline uint32_t psram_ll_get_reg3_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG3(psram_id));
}

// Legacy function for backward compatibility
static inline uint32_t psram_ll_get_reg3_value_legacy(void)
{
	return REG_READ(PSRAM_REG3_LEGACY);
}

/* REG_0x04 */
#define PSRAM_REG4(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x4 * 4)
#define PSRAM_REG4_LEGACY     (PSRAM_LL_REG_BASE + 0x4 * 4)

static inline uint32_t psram_ll_get_mode_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG4(psram_id));
}

static inline void psram_ll_set_mode_value(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG4(psram_id), value);
}

static inline void psram_ll_set_reg4_wrap_config(psram_id_t psram_id, uint32_t value)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG4(psram_id));
	reg_value &= ~(0x7 << 28);
	reg_value |= ((value & 0x7) << 28);
	REG_WRITE(PSRAM_REG4(psram_id), reg_value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_mode_value_legacy(void)
{
	return REG_READ(PSRAM_REG4_LEGACY);
}

static inline void psram_ll_set_mode_value_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REG4_LEGACY, value);
}

static inline void psram_ll_set_reg4_wrap_config_legacy(uint32_t value)
{
	uint32_t reg_value;
	reg_value = REG_READ(PSRAM_REG4_LEGACY);
	reg_value &= ~(0x7 << 28);
	reg_value |= ((value & 0x7) << 28);
	REG_WRITE(PSRAM_REG4_LEGACY, reg_value);
}

/* REG_0x05 */
#define PSRAM_REG5(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x5 * 4)
#define PSRAM_REG5_LEGACY     (PSRAM_LL_REG_BASE + 0x5 * 4)

static inline uint32_t psram_ll_get_reg5_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG5(psram_id));
}

static inline void psram_ll_set_reg5_value(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG5(psram_id), value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_reg5_value_legacy(void)
{
	return REG_READ(PSRAM_REG5_LEGACY);
}

static inline void psram_ll_set_reg5_value_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REG5_LEGACY, value);
}

/* REG_0x08 */
#define PSRAM_REG8(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x8 * 4)
#define PSRAM_REG8_LEGACY     (PSRAM_LL_REG_BASE + 0x8 * 4)

static inline uint32_t psram_ll_get_reg8_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG8(psram_id));
}

static inline void psram_ll_set_reg8_value(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG8(psram_id), value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_reg8_value_legacy(void)
{
	return REG_READ(PSRAM_REG8_LEGACY);
}

static inline void psram_ll_set_reg8_value_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REG8_LEGACY, value);
}

/* REG_0x09 */
#define PSRAM_REG9(psram_id)  (psram_ll_get_reg_base(psram_id) + 0x9 * 4)
#define PSRAM_REG9_LEGACY     (PSRAM_LL_REG_BASE + 0x9 * 4)

static inline uint32_t psram_ll_get_write_address(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REG9(psram_id));
}

static inline void psram_ll_set_write_address(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG9(psram_id), value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_write_address_legacy(void)
{
	return REG_READ(PSRAM_REG9_LEGACY);
}

static inline void psram_ll_set_write_address_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REG9_LEGACY, value);
}

/* REG_0x0a */
#define PSRAM_REGa(psram_id)  (psram_ll_get_reg_base(psram_id) + 0xa * 4)
#define PSRAM_REGa_LEGACY     (PSRAM_LL_REG_BASE + 0xa * 4)

static inline uint32_t psram_ll_get_write_data(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REGa(psram_id));
}

static inline void psram_ll_set_write_data(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REGa(psram_id), value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_write_data_legacy(void)
{
	return REG_READ(PSRAM_REGa_LEGACY);
}

static inline void psram_ll_set_write_data_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REGa_LEGACY, value);
}

/* REG_0x0b */
#define PSRAM_REGb(psram_id)  (psram_ll_get_reg_base(psram_id) + 0xb * 4)
#define PSRAM_REGb_LEGACY     (PSRAM_LL_REG_BASE + 0xb * 4)

static inline uint32_t psram_ll_get_regb_value(psram_id_t psram_id)
{
	return REG_READ(PSRAM_REGb(psram_id));
}

static inline void psram_ll_set_regb_value(psram_id_t psram_id, uint32_t value)
{
	REG_WRITE(PSRAM_REGb(psram_id), value);
}

// Legacy functions for backward compatibility
static inline uint32_t psram_ll_get_regb_value_legacy(void)
{
	return REG_READ(PSRAM_REGb_LEGACY);
}

static inline void psram_ll_set_regb_value_legacy(uint32_t value)
{
	REG_WRITE(PSRAM_REGb_LEGACY, value);
}

/* REG 0x10~0x17 */
#define PSRAM_REG_COVER_START(psram_id, area_id)    (psram_ll_get_reg_base(psram_id) + ((0x10 + ((area_id) << 1)) << 2))
#define PSRAM_REG_COVER_STOP_ENA(psram_id, area_id) (psram_ll_get_reg_base(psram_id) + ((0x11 + ((area_id) << 1)) << 2))
#define PSRAM_REG_COVER_START_LEGACY(area_id)      (PSRAM_LL_REG_BASE + ((0x10 + ((area_id) << 1)) << 2))
#define PSRAM_REG_COVER_STOP_ENA_LEGACY(area_id)   (PSRAM_LL_REG_BASE + ((0x11 + ((area_id) << 1)) << 2))

static inline void psram_ll_set_cover_start(psram_id_t psram_id, uint32_t area_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG_COVER_START(psram_id, area_id), value);
}

static inline void psram_ll_set_cover_stop_enable(psram_id_t psram_id, uint32_t area_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG_COVER_STOP_ENA(psram_id, area_id), value);
}

int psram_ll_set_write_through(psram_id_t psram_id, psram_write_through_area_t area, uint32_t enable, uint32_t start, uint32_t end);

static inline uint32_t psram_ll_get_cover_start_address(psram_id_t psram_id, uint32_t area_id)
{
	if (area_id > 3)
		return 0;
	return REG_READ(PSRAM_REG_COVER_START(psram_id, area_id));
}

static inline uint32_t psram_ll_get_cover_stop_address(psram_id_t psram_id, uint32_t area_id)
{
	if (area_id > 3)
		return 0;
	return REG_READ(PSRAM_REG_COVER_STOP_ENA(psram_id, area_id));
}

// Legacy functions for backward compatibility
static inline void psram_ll_set_cover_start_legacy(uint32_t area_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG_COVER_START_LEGACY(area_id), value);
}

static inline void psram_ll_set_cover_stop_enable_legacy(uint32_t area_id, uint32_t value)
{
	REG_WRITE(PSRAM_REG_COVER_STOP_ENA_LEGACY(area_id), value);
}

static inline uint32_t psram_ll_get_cover_start_address_legacy(uint32_t area_id)
{
	if (area_id > 3)
		return 0;
	return REG_READ(PSRAM_REG_COVER_START_LEGACY(area_id));
}

static inline uint32_t psram_ll_get_cover_stop_address_legacy(uint32_t area_id)
{
	if (area_id > 3)
		return 0;
	return REG_READ(PSRAM_REG_COVER_STOP_ENA_LEGACY(area_id));
}

#ifdef __cplusplus
}
#endif
