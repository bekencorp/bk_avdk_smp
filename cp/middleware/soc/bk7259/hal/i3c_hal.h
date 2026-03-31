/**
 * @file i3c_hal.h
 * @brief I3C HAL: hardware abstraction. Uses LL for register access. Multi-instance ready.
 */
#ifndef I3C_HAL_H
#define I3C_HAL_H

#include <stdint.h>
#include "i3c_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default I3C unit ID (single instance on current SoC). */
#define I3C_HAL_DEFAULT_ID  I3C_LL_ID_0

/* ---------- Beken strap (replaces i3c_beken_*) ---------- */
void i3c_hal_beken_set_sw_reset(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_clkg_bps_cdn(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_clkg_bps_beken(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_device_role(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_dcr(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_mrl(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_mwl(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_mxds_limited(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_mxds_maxwr(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_mxds_maxrd(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_pid_mfr_id(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_pid_instance_id(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_bus_avail_timer(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_bus_idle_timer(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_stat_addr(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_flow_ctrl_pr_dis(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_flow_ctrl_pw_dis(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_fpf_pw_sel(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_alt_mode_en(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_hj_in_use(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_ibi_mdb_prn(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_rx_data_fifo_mode(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_periph_rst_ret_time(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_chip_rst_ret_time(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_xtime_freq_byte(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_ps_xtime_inacc_byte(i3c_ll_id_t id, uint32_t val);
void i3c_hal_beken_set_tgt_tcam0_t_c1_xdel(i3c_ll_id_t id, uint32_t val);
uint32_t i3c_hal_beken_get_reg(i3c_ll_id_t id, uint32_t off);

/* ---------- I3C controller register access (for driver use) ---------- */
static inline uint32_t i3c_hal_ctrl_read(i3c_ll_id_t id) { return i3c_ll_ctrl_read(id); }
static inline void i3c_hal_ctrl_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_ctrl_write(id, v); }
static inline uint32_t i3c_hal_prescl0_read(i3c_ll_id_t id) { return i3c_ll_prescl0_read(id); }
static inline void i3c_hal_prescl0_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_prescl0_write(id, v); }
static inline void i3c_hal_prescl1_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_prescl1_write(id, v); }
static inline uint32_t i3c_hal_devs_ctrl_read(i3c_ll_id_t id) { return i3c_ll_devs_ctrl_read(id); }
static inline void i3c_hal_devs_ctrl_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_ip_write(id, I3C_R_DEVS_CTRL, v); }
static inline uint32_t i3c_hal_mst_status0_read(i3c_ll_id_t id) { return i3c_ll_mst_status0_read(id); }
static inline void i3c_hal_mst_status0_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_ip_write(id, I3C_R_MST_STATUS0, v); }
static inline uint32_t i3c_hal_mst_isr_read(i3c_ll_id_t id) { return i3c_ll_mst_isr_read(id); }
static inline void i3c_hal_mst_ier_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_mst_ier_write(id, v); }
static inline void i3c_hal_mst_icr_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_mst_icr_write(id, v); }
static inline uint32_t i3c_hal_cmdr_read(i3c_ll_id_t id) { return i3c_ll_cmdr_read(id); }
static inline void i3c_hal_cmd0_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_cmd0_write(id, v); }
static inline void i3c_hal_cmd1_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_cmd1_write(id, v); }
static inline void i3c_hal_tx_fifo_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_tx_fifo_write(id, v); }
static inline uint32_t i3c_hal_rx_fifo_read(i3c_ll_id_t id) { return i3c_ll_rx_fifo_read(id); }
static inline uint32_t i3c_hal_rx_fifo_status_read(i3c_ll_id_t id) { return i3c_ll_rx_fifo_status_read(id); }
static inline void i3c_hal_tx_rx_thr_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_tx_rx_thr_write(id, v); }
static inline uint32_t i3c_hal_ibir_read(i3c_ll_id_t id) { return i3c_ll_ibir_read(id); }
static inline uint32_t i3c_hal_ibi_data_read(i3c_ll_id_t id) { return i3c_ll_ibi_data_read(id); }
static inline void i3c_hal_sir_map_write(i3c_ll_id_t id, uint32_t idx, uint32_t v) { i3c_ll_sir_map_write(id, idx, v); }
static inline uint32_t i3c_hal_sir_map_read(i3c_ll_id_t id, uint32_t idx) { return i3c_ll_ip_read(id, I3C_R_SIR_MAP_BASE + idx); }
static inline uint32_t i3c_hal_dev_rr0(i3c_ll_id_t id, uint32_t di) { return i3c_ll_dev_rr0(id, di); }
static inline void i3c_hal_dev_rr1_write(i3c_ll_id_t id, uint32_t di, uint32_t v) { i3c_ll_dev_rr1_write(id, di, v); }
static inline uint32_t i3c_hal_dev_rr1(i3c_ll_id_t id, uint32_t di) { return i3c_ll_dev_rr1(id, di); }
static inline void i3c_hal_dev_rr2_write(i3c_ll_id_t id, uint32_t di, uint32_t v) { i3c_ll_dev_rr2_write(id, di, v); }
static inline uint32_t i3c_hal_dev_rr2(i3c_ll_id_t id, uint32_t di) { return i3c_ll_dev_rr2(id, di); }

static inline void i3c_hal_slv_ctrl_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_ctrl_write(id, v); }
static inline uint32_t i3c_hal_slv_status1_read(i3c_ll_id_t id) { return i3c_ll_slv_status1_read(id); }
static inline void i3c_hal_slv_status1_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_status1_write(id, v); }
static inline void i3c_hal_slv_ier_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_ier_write(id, v); }
static inline uint32_t i3c_hal_slv_isr_read(i3c_ll_id_t id) { return i3c_ll_slv_isr_read(id); }
static inline void i3c_hal_slv_icr_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_icr_write(id, v); }
static inline uint32_t i3c_hal_slv_ibi_ctrl_read(i3c_ll_id_t id) { return i3c_ll_slv_ibi_ctrl_read(id); }
static inline void i3c_hal_slv_ibi_ctrl_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_ibi_ctrl_write(id, v); }
static inline void i3c_hal_slv_ddr_tx_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_slv_ddr_tx_write(id, v); }
static inline uint32_t i3c_hal_slv_ddr_rx_read(i3c_ll_id_t id) { return i3c_ll_slv_ddr_rx_read(id); }
static inline void i3c_hal_slv_ddr_thr_write(i3c_ll_id_t id, uint32_t v) { i3c_ll_ip_write(id, I3C_R_SLV_DDR_TX_RX_THR, v); }
static inline uint32_t i3c_hal_slv_status0_read(i3c_ll_id_t id) { return i3c_ll_ip_read(id, I3C_R_SLV_STATUS0); }

#ifdef __cplusplus
}
#endif

#endif /* I3C_HAL_H */
