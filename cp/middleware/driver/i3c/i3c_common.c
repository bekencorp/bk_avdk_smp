/**
 * @file i3c_common.c
 * @brief I3C common: Beken HAL wrappers, HDR CRC5/parity, platform init.
 */
#include "i3c_common.h"
#include "gpio_driver.h"
#include <modules/pm.h>
#include <common/bk_err.h>

void i3c_delay_ms(uint32_t ms)
{
	volatile uint32_t i, n = ms * 10000u;
	for (i = 0; i < n; i++)
		(void)i;
}

#if defined(CONFIG_I3C)
static uint32_t s_i3c_core_hz = I3C_CORE_HZ_APLL;

void i3c_core_clk_src_apply(i3c_core_clk_src_t src, uint32_t core_hz_override)
{
	uint32_t hz;

	if (core_hz_override != 0u)
		hz = core_hz_override;
	else if (src == I3C_CORE_CLK_SRC_XTAL_26M)
		hz = I3C_CORE_HZ_XTAL_26M;
	else
		hz = I3C_CORE_HZ_APLL;

	s_i3c_core_hz = hz;

	/* Do not touch SYSTEM cksel_i3c / i3c_cken here: PM / boot already gates the block; forcing mux
	 * can fight that default and stall the controller. Only IP-side Beken straps below (see I3C_beken). */

	/* I3C_beken.txt Reg2: clkg_bps_cdn = CDN hclk gate bypass; clkg_bps_beken = Beken hclk gate bypass.
	 * Reference (test_i3c): APLL path uses CDN=1; 26 MHz XTAL path uses Beken=1, CDN=0. */
	if (src == I3C_CORE_CLK_SRC_XTAL_26M) {
		i3c_beken_set_clkg_bps_cdn(0);
		i3c_beken_set_clkg_bps_beken(1);
	} else {
		i3c_beken_set_clkg_bps_cdn(1);
		i3c_beken_set_clkg_bps_beken(0);
	}
}

uint32_t i3c_core_hz_get(void)
{
	return s_i3c_core_hz;
}

/* Beken strap wrappers (driver calls these; they use HAL) */
void i3c_beken_set_sw_reset(uint32_t val)
{
	i3c_hal_beken_set_sw_reset(I3C_UNIT_ID, val);
}

void i3c_beken_set_clkg_bps_cdn(uint32_t val)
{
	i3c_hal_beken_set_clkg_bps_cdn(I3C_UNIT_ID, val);
}

void i3c_beken_set_clkg_bps_beken(uint32_t val)
{
	i3c_hal_beken_set_clkg_bps_beken(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_device_role(uint32_t val)
{
	i3c_hal_beken_set_ps_device_role(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_dcr(uint32_t val)
{
	i3c_hal_beken_set_ps_dcr(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_mrl(uint32_t val)
{
	i3c_hal_beken_set_ps_mrl(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_mwl(uint32_t val)
{
	i3c_hal_beken_set_ps_mwl(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_mxds_limited(uint32_t val)
{
	i3c_hal_beken_set_ps_mxds_limited(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_mxds_maxwr(uint32_t val)
{
	i3c_hal_beken_set_ps_mxds_maxwr(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_mxds_maxrd(uint32_t val)
{
	i3c_hal_beken_set_ps_mxds_maxrd(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_pid_mfr_id(uint32_t val)
{
	i3c_hal_beken_set_ps_pid_mfr_id(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_pid_instance_id(uint32_t val)
{
	i3c_hal_beken_set_ps_pid_instance_id(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_bus_avail_timer(uint32_t val)
{
	i3c_hal_beken_set_ps_bus_avail_timer(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_bus_idle_timer(uint32_t val)
{
	i3c_hal_beken_set_ps_bus_idle_timer(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_stat_addr(uint32_t val)
{
	i3c_hal_beken_set_ps_stat_addr(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_flow_ctrl_pr_dis(uint32_t val)
{
	i3c_hal_beken_set_ps_flow_ctrl_pr_dis(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_flow_ctrl_pw_dis(uint32_t val)
{
	i3c_hal_beken_set_ps_flow_ctrl_pw_dis(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_fpf_pw_sel(uint32_t val)
{
	i3c_hal_beken_set_ps_fpf_pw_sel(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_alt_mode_en(uint32_t val)
{
	i3c_hal_beken_set_ps_alt_mode_en(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_hj_in_use(uint32_t val)
{
	i3c_hal_beken_set_ps_hj_in_use(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_ibi_mdb_prn(uint32_t val)
{
	i3c_hal_beken_set_ps_ibi_mdb_prn(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_rx_data_fifo_mode(uint32_t val)
{
	i3c_hal_beken_set_ps_rx_data_fifo_mode(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_periph_rst_ret_time(uint32_t val)
{
	i3c_hal_beken_set_ps_periph_rst_ret_time(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_chip_rst_ret_time(uint32_t val)
{
	i3c_hal_beken_set_ps_chip_rst_ret_time(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_xtime_freq_byte(uint32_t val)
{
	i3c_hal_beken_set_ps_xtime_freq_byte(I3C_UNIT_ID, val);
}

void i3c_beken_set_ps_xtime_inacc_byte(uint32_t val)
{
	i3c_hal_beken_set_ps_xtime_inacc_byte(I3C_UNIT_ID, val);
}

void i3c_beken_set_tgt_tcam0_t_c1_xdel(uint32_t val)
{
	i3c_hal_beken_set_tgt_tcam0_t_c1_xdel(I3C_UNIT_ID, val);
}

uint32_t i3c_beken_get_reg(uint32_t off)
{
	return i3c_hal_beken_get_reg(I3C_UNIT_ID, off);
}
#endif

#ifndef I3C_GPIO_SCL_DEFAULT
#define I3C_GPIO_SCL_DEFAULT  32
#define I3C_GPIO_SDA_DEFAULT  33
#define I3C_GPIO_PURN_DEFAULT 34
#endif

void i3c_platform_init(const i3c_platform_config_t *cfg)
{
	uint32_t scl = cfg ? cfg->gpio_scl : I3C_GPIO_SCL_DEFAULT;
	uint32_t sda = cfg ? cfg->gpio_sda : I3C_GPIO_SDA_DEFAULT;
	uint32_t purn = cfg ? cfg->gpio_purn : I3C_GPIO_PURN_DEFAULT;

	bk_pm_clock_ctrl(PM_CLK_ID_I3C, PM_CLK_CTRL_PWR_UP);

	(void)scl;
	(void)sda;
	(void)purn;
}

void i3c_platform_deinit(void)
{
#if defined(CONFIG_I3C)
	extern void i3c_master_int_unregister(void);
	extern void i3c_slave_int_unregister(void);
	i3c_master_int_unregister();
	i3c_slave_int_unregister();
	uint32_t ctrl = i3c_hal_ctrl_read(I3C_UNIT_ID);
	I3C_TEST_LOGI("i3c: deinit: begin CTRL=%08lx\r\n", (unsigned long)ctrl);
	i3c_hal_ctrl_write(I3C_UNIT_ID, ctrl & ~(1u << 31));
	I3C_TEST_LOGI("i3c: deinit: after disable CTRL=%08lx\r\n", (unsigned long)i3c_hal_ctrl_read(I3C_UNIT_ID));
	i3c_beken_set_sw_reset(1);
	I3C_DELAY_MS(1);
#if defined(I3C_TEST_POWER_DOWN_CLOCK)
	bk_pm_clock_ctrl(PM_CLK_ID_I3C, PM_CLK_CTRL_PWR_DOWN);
	I3C_TEST_LOGI("i3c: deinit: clock power down done\r\n");
#endif
#endif
	I3C_TEST_LOGI("i3c: deinit: end\r\n");
}

uint16_t i3c_hdr_parity_odd_even(uint16_t data, uint16_t is_even)
{
	uint16_t result = 0;
	int i;
	if (is_even == 0) {
		for (i = 15; i >= 1; i -= 2)
			result ^= (uint16_t)((data >> i) & 1u);
	} else {
		for (i = 14; i >= 0; i -= 2)
			result ^= (uint16_t)((data >> i) & 1u);
		result ^= 1u;
	}
	return result & 1u;
}

uint32_t i3c_hdr_tx_word(uint8_t preamble, uint8_t data_hi, uint8_t data_lo)
{
	uint16_t w = (uint16_t)((data_hi << 8) | data_lo);
	uint16_t p0 = i3c_hdr_parity_odd_even(w, 0);
	uint16_t p1 = i3c_hdr_parity_odd_even(w, 1);
	return (uint32_t)((preamble & 3u) << 18) | ((uint32_t)w << 2) | ((uint32_t)p0 << 1) | (uint32_t)p1;
}

uint8_t i3c_hdr_crc5(uint8_t crc_in, uint16_t data_word)
{
	uint8_t icrc = crc_in;
	uint8_t crc0;
	uint8_t i;

	for (i = 0; i < 16u; i++) {
		crc0 = (uint8_t)((((data_word >> (15u - i)) ^ (uint16_t)(icrc >> 4)) & 1u));
		icrc = (uint8_t)(((icrc << 1) & (0x18u | 0x2u)) | (((icrc >> 1) ^ crc0) << 2) | crc0);
	}
	return icrc & 0x1fu;
}
