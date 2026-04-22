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

#include <soc/soc.h>
#include <soc/bk7259/reg_base.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPI_LL_REG_BASE() (SOC_IPI_REG_BASE)

typedef struct {
	volatile uint32_t devid;   /* 0x00 */
	volatile uint32_t verid;   /* 0x04 */
	volatile uint32_t clkrst;  /* 0x08 */
	volatile uint32_t state;   /* 0x0C */
	volatile uint32_t ipig0;   /* 0x10 */
	volatile uint32_t ipig1;   /* 0x14 */
	volatile uint32_t ipig2;   /* 0x18 */
	volatile uint32_t ipig3;   /* 0x1C */
	volatile uint32_t ipig4;   /* 0x20 */
	volatile uint32_t reserved0[3];
	volatile uint32_t ipic0;   /* 0x30 */
	volatile uint32_t ipic1;   /* 0x34 */
	volatile uint32_t ipic2;   /* 0x38 */
	volatile uint32_t ipic3;   /* 0x3C */
	volatile uint32_t ipic4;   /* 0x40 */
	/* 0x44..0x4C reserved, pad to 0x50 */
	volatile uint32_t reserved1[3];
	volatile uint32_t int_reg; /* 0x50 */
} ipi_hw_t;

static inline ipi_hw_t *ipi_ll_get_hw(void)
{
	return (ipi_hw_t *)IPI_LL_REG_BASE();
}

static inline uint32_t ipi_ll_pack_ipig(uint32_t value)
{
	return ((value << 1) & 0xFFFFFFFE) | 0x1;
}

static inline uint32_t ipi_ll_unpack_ipig(uint32_t ipig_reg)
{
	return (ipig_reg & 0xFFFFFFFE) >> 1;
}

static inline volatile uint32_t *ipi_ll_ipig_reg(ipi_hw_t *hw, uint32_t core_id)
{
	return (&hw->ipig0 + core_id);
}

static inline volatile uint32_t *ipi_ll_ipic_reg(ipi_hw_t *hw, uint32_t core_id)
{
	return (&hw->ipic0 + core_id);
}

static inline uint32_t ipi_ll_get_int_reg(ipi_hw_t *hw)
{
	return hw->int_reg;
}

static inline void ipi_ll_set_int_reg(ipi_hw_t *hw, uint32_t v)
{
	hw->int_reg = v;
}

static inline void ipi_ll_write_ipig(ipi_hw_t *hw, uint32_t core_id, uint32_t ipig_val)
{
	*ipi_ll_ipig_reg(hw, core_id) = ipig_val;
}

static inline uint32_t ipi_ll_read_ipig(ipi_hw_t *hw, uint32_t core_id)
{
	return *ipi_ll_ipig_reg(hw, core_id);
}

static inline void ipi_ll_write_ipic(ipi_hw_t *hw, uint32_t core_id, uint32_t ipic_val)
{
	*ipi_ll_ipic_reg(hw, core_id) = ipic_val;
}

static inline uint32_t ipi_ll_read_state(ipi_hw_t *hw)
{
	return hw->state;
}

#ifdef __cplusplus
}
#endif

