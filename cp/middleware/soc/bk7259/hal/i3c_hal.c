/**
 * @file i3c_hal.c
 * @brief I3C HAL: Beken strap register implementation.
 */
#include "i3c_hal.h"
#include "i3c_reg.h"

void i3c_hal_beken_set_sw_reset(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_SW_RESET, I3C_BEKEN_SW_RESET, val ? I3C_BEKEN_SW_RESET : 0u);
}

void i3c_hal_beken_set_clkg_bps_cdn(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_SW_RESET, I3C_BEKEN_CLKG_BPS_CDN, val ? I3C_BEKEN_CLKG_BPS_CDN : 0u);
}

void i3c_hal_beken_set_clkg_bps_beken(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_SW_RESET, I3C_BEKEN_CLKG_BPS_BEKEN, val ? I3C_BEKEN_CLKG_BPS_BEKEN : 0u);
}

void i3c_hal_beken_set_ps_device_role(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_DEVICE_ROLE, 0x3u, val & 0x3u);
}

void i3c_hal_beken_set_ps_dcr(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_DCR, 0xffu, val & 0xffu);
}

void i3c_hal_beken_set_ps_mrl(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_MRL, 0xffffffu, val & 0xffffffu);
}

void i3c_hal_beken_set_ps_mwl(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_MWL, 0xffffu, val & 0xffffu);
}

void i3c_hal_beken_set_ps_mxds_limited(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_MXDS, 1u, val ? 1u : 0u);
}

void i3c_hal_beken_set_ps_mxds_maxwr(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_MXDS, 0xffu << 16, (val & 0xffu) << 16);
}

void i3c_hal_beken_set_ps_mxds_maxrd(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_MXDS, 0xffu << 24, (val & 0xffu) << 24);
}

void i3c_hal_beken_set_ps_pid_mfr_id(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_PID_MFR, 0x7fffu, val & 0x7fffu);
}

void i3c_hal_beken_set_ps_pid_instance_id(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_PID_INST, 0xfu, val & 0xfu);
}

void i3c_hal_beken_set_ps_bus_avail_timer(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_BUS_AVAIL, 0xffu, val & 0xffu);
}

void i3c_hal_beken_set_ps_bus_idle_timer(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_BUS_IDLE, 0x3ffffu, val & 0x3ffffu);
}

void i3c_hal_beken_set_ps_stat_addr(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_STAT_ADDR, 0x7fu, val & 0x7fu);
}

void i3c_hal_beken_set_ps_flow_ctrl_pr_dis(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 0, val ? (1u << 0) : 0u);
}

void i3c_hal_beken_set_ps_flow_ctrl_pw_dis(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 1, val ? (1u << 1) : 0u);
}

void i3c_hal_beken_set_ps_fpf_pw_sel(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 2, val ? (1u << 2) : 0u);
}

void i3c_hal_beken_set_ps_alt_mode_en(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 3, val ? (1u << 3) : 0u);
}

void i3c_hal_beken_set_ps_hj_in_use(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 4, val ? (1u << 4) : 0u);
}

void i3c_hal_beken_set_ps_ibi_mdb_prn(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 5, val ? (1u << 5) : 0u);
}

void i3c_hal_beken_set_ps_rx_data_fifo_mode(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_FLOW_CTRL, 1u << 6, val ? (1u << 6) : 0u);
}

void i3c_hal_beken_set_ps_periph_rst_ret_time(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_RST_TIME, 0xffu, val & 0xffu);
}

void i3c_hal_beken_set_ps_chip_rst_ret_time(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_RST_TIME, 0xffu << 8, (val & 0xffu) << 8);
}

void i3c_hal_beken_set_ps_xtime_freq_byte(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_XTIME, 0xffu, val & 0xffu);
}

void i3c_hal_beken_set_ps_xtime_inacc_byte(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_XTIME, 0xffu << 8, (val & 0xffu) << 8);
}

void i3c_hal_beken_set_tgt_tcam0_t_c1_xdel(i3c_ll_id_t id, uint32_t val)
{
	i3c_ll_beken_rmw(id, I3C_BEKEN_R_TCAM_XDEL, 0xffffu, val & 0xffffu);
}

uint32_t i3c_hal_beken_get_reg(i3c_ll_id_t id, uint32_t off)
{
	return i3c_ll_beken_read(id, off);
}
