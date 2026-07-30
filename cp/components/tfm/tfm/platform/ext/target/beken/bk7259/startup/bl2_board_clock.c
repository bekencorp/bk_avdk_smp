/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Self-contained BL2 clock bring-up for BK7259 secure world.
 * Ported from NS bootloader board_clock.c; uses direct MMIO only.
 */
#include "bl2_board_clock.h"
#include <stdint.h>

#ifndef REG_WRITE
#define REG_WRITE(addr, val)  (*(volatile uint32_t *)(addr) = (uint32_t)(val))
#endif
#ifndef REG_READ
#define REG_READ(addr)        (*(volatile uint32_t *)(addr))
#endif

#define BL2_SOC_SYS_REG_BASE           0x44010000U
#define BL2_SYS_REG0XD_ADDR            (BL2_SOC_SYS_REG_BASE + (0xdU << 2))
#define BL2_SYS_ANA_REG5_ADDR          (BL2_SOC_SYS_REG_BASE + (0x45U << 2))
#define BL2_SYS_ANA_REG0_ADDR          (BL2_SOC_SYS_REG_BASE + (0x40U << 2))
#define BL2_SYS_ANA_REG6_ADDR          (BL2_SOC_SYS_REG_BASE + (0x46U << 2))
#define BL2_SYS_ANA_REG10_ADDR         (BL2_SOC_SYS_REG_BASE + (0x4aU << 2))
#define BL2_SYS_CPU_CLK_DIV_MODE1_ADDR (BL2_SOC_SYS_REG_BASE + (0x8U << 2))
#define BL2_SYS_CPU_DEVICE_CLK_EN_ADDR (BL2_SOC_SYS_REG_BASE + (0xcU << 2))
#define BL2_SYS_CPU_ANASPI_FREQ_ADDR   (BL2_SOC_SYS_REG_BASE + (0xbU << 2))

#define BL2_CLK_CORE_480M              3U
#define BL2_CLK_FLASH_240M             2U
#define BL2_CLK_CORE_DIV               1U   /* 480M / (1+1) = 240MHz CPU  */
#define BL2_CLK_FLASH_DIV              2U   /* 240M / (2+1) = 80MHz flash */
#define BL2_CLK_VDDDIG_0V95            0xEU

typedef enum {
    BL2_ANALOG_REG0  = 0,
    BL2_ANALOG_REG1  = 1,
    BL2_ANALOG_REG2  = 2,
    BL2_ANALOG_REG3  = 3,
    BL2_ANALOG_REG4  = 4,
    BL2_ANALOG_REG5  = 5,
    BL2_ANALOG_REG9  = 9,
    BL2_ANALOG_REG10 = 10,
    BL2_ANALOG_REG11 = 11,
    BL2_ANALOG_REG12 = 12,
    BL2_ANALOG_REG13 = 13,
    BL2_ANALOG_REG14 = 14,
    BL2_ANALOG_REG15 = 15,
    BL2_ANALOG_REG16 = 16,
    BL2_ANALOG_REG19 = 19,
} bl2_analog_reg_t;

typedef volatile union {
    struct {
        uint32_t cksel_core   : 2;
        uint32_t ckdiv_core   : 4;
        uint32_t cksel_flash  : 2;
        uint32_t ckdiv_flash  : 3;
        uint32_t reserved     : 21;
    };
    uint32_t v;
} bl2_cpu_clk_div_mode1_t;

static void bl2_clock_delay(volatile uint32_t times)
{
    while (times--) {
    }
}

static uint32_t bl2_analog_reg_addr(bl2_analog_reg_t reg)
{
    return BL2_SOC_SYS_REG_BASE + ((0x40U + (uint32_t)reg) << 2);
}

static uint32_t bl2_get_analog_reg_idx(uint32_t addr)
{
    return (addr - BL2_SYS_ANA_REG0_ADDR) >> 2;
}

static void bl2_set_analog_reg_value(uint32_t addr, uint32_t value)
{
    uint32_t idx = bl2_get_analog_reg_idx(addr);

    REG_WRITE(addr, value);
    bl2_clock_delay(10);
    while (REG_READ(BL2_SYS_CPU_ANASPI_FREQ_ADDR) &
           (1U << (idx + 8U))) {
    }
}

static void bl2_set_ana_reg_bit(uint32_t reg_addr, uint32_t pos,
                                uint32_t mask, uint32_t value)
{
    uint32_t reg_value = REG_READ(reg_addr);

    reg_value &= ~(mask << pos);
    reg_value |= ((value & mask) << pos);
    bl2_set_analog_reg_value(reg_addr, reg_value);
}

static void bl2_analog_set(bl2_analog_reg_t reg, uint32_t value)
{
    bl2_set_analog_reg_value(bl2_analog_reg_addr(reg), value);
}

static uint32_t bl2_analog_get(bl2_analog_reg_t reg)
{
    return REG_READ(bl2_analog_reg_addr(reg));
}

static void bl2_set_spi_latch1v(uint32_t v)
{
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG10_ADDR, 9, 0x1, v);
}

static uint32_t bl2_get_vcorehsel(void)
{
    return (REG_READ(BL2_SYS_ANA_REG10_ADDR) >> 16) & 0xfU;
}

static void bl2_set_vcorehsel(uint32_t v)
{
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG10_ADDR, 16, 0xf, v);
}

static void bl2_set_ckdiv_core(uint32_t v)
{
    bl2_cpu_clk_div_mode1_t *r =
        (bl2_cpu_clk_div_mode1_t *)BL2_SYS_CPU_CLK_DIV_MODE1_ADDR;

    r->ckdiv_core = v;
}

static void bl2_set_cksel_core(uint32_t v)
{
    bl2_cpu_clk_div_mode1_t *r =
        (bl2_cpu_clk_div_mode1_t *)BL2_SYS_CPU_CLK_DIV_MODE1_ADDR;

    r->cksel_core = v;
}

static void bl2_set_ckdiv_flash(uint32_t v)
{
    bl2_cpu_clk_div_mode1_t *r =
        (bl2_cpu_clk_div_mode1_t *)BL2_SYS_CPU_CLK_DIV_MODE1_ADDR;

    r->ckdiv_flash = v;
}

static void bl2_set_cksel_flash(uint32_t v)
{
    bl2_cpu_clk_div_mode1_t *r =
        (bl2_cpu_clk_div_mode1_t *)BL2_SYS_CPU_CLK_DIV_MODE1_ADDR;

    r->cksel_flash = v;
}

static void bl2_enable_sources(void)
{
    uint32_t reg_value;

    reg_value = REG_READ(BL2_SYS_ANA_REG5_ADDR);
    reg_value |= (1U << 5);
    REG_WRITE(BL2_SYS_ANA_REG5_ADDR, reg_value);

    reg_value = REG_READ(BL2_SYS_REG0XD_ADDR);
    reg_value |= (1U << 12) | (1U << 13) | (1U << 14) | (1U << 15) | (1U << 16);
    REG_WRITE(BL2_SYS_REG0XD_ADDR, reg_value);
    bl2_clock_delay(120);
}

static void bl2_set_vdddig(uint32_t vol_value)
{
    uint32_t cur_vol = bl2_get_vcorehsel();
    uint32_t next_vol;

    if (cur_vol >= vol_value) {
        return;
    }

    for (next_vol = cur_vol + 1; next_vol <= vol_value; next_vol++) {
        bl2_set_spi_latch1v(1);
        bl2_set_vcorehsel(next_vol);
        bl2_set_spi_latch1v(0);
        bl2_clock_delay(2600);
    }
}

static void bl2_apply_core(uint32_t cksel_core, uint32_t ckdiv_core)
{
    bl2_set_ckdiv_core(ckdiv_core);
    bl2_set_cksel_core(cksel_core);
}

static void bl2_apply_flash(uint32_t cksel_flash, uint32_t ckdiv_flash)
{
    bl2_set_ckdiv_flash(7U);
    bl2_set_cksel_flash(cksel_flash);
    bl2_clock_delay(120);
    bl2_set_ckdiv_flash(ckdiv_flash);
}

static void bl2_cali_dpll(void)
{
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG6_ADDR, 20, 0x1, 0);
    bl2_clock_delay(120);
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG6_ADDR, 20, 0x1, 1);
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG0_ADDR, 4, 0x1, 0);
    bl2_clock_delay(3400);
    bl2_set_ana_reg_bit(BL2_SYS_ANA_REG0_ADDR, 4, 0x1, 1);
    bl2_clock_delay(3400);
}

static void bl2_dpll_cpu_early_init(void)
{
    bl2_cali_dpll();

    REG_WRITE(BL2_SYS_CPU_DEVICE_CLK_EN_ADDR, 0xFFFFFFFF);
    REG_WRITE(BL2_SYS_REG0XD_ADDR, 0xFFFFFFFF);

    bl2_set_ckdiv_core(0x1);
    bl2_set_cksel_core(0x3);
}

void bl2_clock_analog_early_init(void)
{
    uint32_t val;

    bl2_set_spi_latch1v(1);

    bl2_analog_set(BL2_ANALOG_REG0, 0xC1385B56);
    bl2_analog_set(BL2_ANALOG_REG5, 0x640F836C);

    val = bl2_analog_get(BL2_ANALOG_REG0);
    val |= (1U << 26);
    bl2_analog_set(BL2_ANALOG_REG0, val);

    val = bl2_analog_get(BL2_ANALOG_REG0);
    val &= ~(1U << 26);
    bl2_analog_set(BL2_ANALOG_REG0, val);

    bl2_analog_set(BL2_ANALOG_REG2, 0x04248050);
    bl2_analog_set(BL2_ANALOG_REG3, 0xC5F00B88);
    bl2_analog_set(BL2_ANALOG_REG4, 0x9FC9A7F0);
    bl2_analog_set(BL2_ANALOG_REG9, 0x57E627E6);

    bl2_analog_set(BL2_ANALOG_REG10, 0x786BC867 | (1U << 9));
    bl2_analog_set(BL2_ANALOG_REG11, 0xC3DD4587);
    bl2_analog_set(BL2_ANALOG_REG12, 0x346E9878);
    bl2_analog_set(BL2_ANALOG_REG13, 0x346E9858);
    bl2_analog_set(BL2_ANALOG_REG14, 0xF4E670EE);
    bl2_analog_set(BL2_ANALOG_REG15, 0);
    bl2_analog_set(BL2_ANALOG_REG16, 0x9E436000);
    bl2_analog_set(BL2_ANALOG_REG19, 0xEE1D8033);

    bl2_set_spi_latch1v(0);

    bl2_dpll_cpu_early_init();
}

void bl2_clock_enable_pll(void)
{
    bl2_enable_sources();
}

void bl2_clock_enable_high_freq(void)
{
    bl2_enable_sources();
    bl2_set_vdddig(BL2_CLK_VDDDIG_0V95);
    bl2_apply_flash(BL2_CLK_FLASH_240M, BL2_CLK_FLASH_DIV);
    bl2_apply_core(BL2_CLK_CORE_480M, BL2_CLK_CORE_DIV);
}
