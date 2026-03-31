/**
 * @file i3c_ll.h
 * @brief I3C Low-Level: register access wrappers. Multi-instance ready (id parameter).
 */
#ifndef I3C_LL_H
#define I3C_LL_H

#include <stdint.h>
#include "i3c_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** I3C unit ID. 0 = first instance; for future multi-I3C SoCs. */
typedef uint8_t i3c_ll_id_t;

#define I3C_LL_ID_0   0u
#define I3C_LL_MAX_ID 1u  /* Current SoC has 1 I3C; increase for multi-instance */

/** Get I3C controller register base for unit id. */
static inline uint32_t i3c_ll_get_ip_base(i3c_ll_id_t id)
{
	(void)id;
	return SOC_I3C_IP_REG_BASE;
}

/** Get Beken strap block base for unit id. */
static inline uint32_t i3c_ll_get_beken_base(i3c_ll_id_t id)
{
	(void)id;
	return SOC_I3C_REG_BASE;
}

/** Read I3C controller register by word index. */
static inline uint32_t i3c_ll_ip_read(i3c_ll_id_t id, uint32_t word_idx)
{
	uint32_t base = i3c_ll_get_ip_base(id);
	return *(volatile uint32_t *)(base + word_idx * I3C_IP_WORD_SIZE);
}

/** Write I3C controller register by word index. */
static inline void i3c_ll_ip_write(i3c_ll_id_t id, uint32_t word_idx, uint32_t val)
{
	uint32_t base = i3c_ll_get_ip_base(id);
	*(volatile uint32_t *)(base + word_idx * I3C_IP_WORD_SIZE) = val;
}

/** Read Beken strap register by word index. */
static inline uint32_t i3c_ll_beken_read(i3c_ll_id_t id, uint32_t word_idx)
{
	uint32_t base = i3c_ll_get_beken_base(id);
	return *(volatile uint32_t *)(base + word_idx * 4u);
}

/** Write Beken strap register by word index. */
static inline void i3c_ll_beken_write(i3c_ll_id_t id, uint32_t word_idx, uint32_t val)
{
	uint32_t base = i3c_ll_get_beken_base(id);
	*(volatile uint32_t *)(base + word_idx * 4u) = val;
}

/** Beken RMW. */
static inline void i3c_ll_beken_rmw(i3c_ll_id_t id, uint32_t off, uint32_t mask, uint32_t val)
{
	uint32_t r = i3c_ll_beken_read(id, off);
	r = (r & ~mask) | (val & mask);
	i3c_ll_beken_write(id, off, r);
}

/* -----  IP accessors (convenience) ----- */
#define i3c_ll_ctrl_read(id)         i3c_ll_ip_read(id, I3C_R_CTRL)
#define i3c_ll_ctrl_write(id, v)     i3c_ll_ip_write(id, I3C_R_CTRL, v)
#define i3c_ll_prescl0_read(id)      i3c_ll_ip_read(id, I3C_R_PRESCL_CTRL0)
#define i3c_ll_prescl0_write(id, v)  i3c_ll_ip_write(id, I3C_R_PRESCL_CTRL0, v)
#define i3c_ll_prescl1_read(id)      i3c_ll_ip_read(id, I3C_R_PRESCL_CTRL1)
#define i3c_ll_prescl1_write(id, v)  i3c_ll_ip_write(id, I3C_R_PRESCL_CTRL1, v)
#define i3c_ll_devs_ctrl_read(id)    i3c_ll_ip_read(id, I3C_R_DEVS_CTRL)
#define i3c_ll_mst_status0_read(id)  i3c_ll_ip_read(id, I3C_R_MST_STATUS0)
#define i3c_ll_mst_isr_read(id)     i3c_ll_ip_read(id, I3C_R_MST_ISR)
#define i3c_ll_mst_ier_read(id)      i3c_ll_ip_read(id, I3C_R_MST_IER)
#define i3c_ll_mst_ier_write(id, v)  i3c_ll_ip_write(id, I3C_R_MST_IER, v)
#define i3c_ll_mst_icr_write(id, v)  i3c_ll_ip_write(id, I3C_R_MST_ICR, v)
#define i3c_ll_cmdr_read(id)         i3c_ll_ip_read(id, I3C_R_CMDR)
#define i3c_ll_cmd0_write(id, v)     i3c_ll_ip_write(id, I3C_R_CMD0_FIFO, v)
#define i3c_ll_cmd1_write(id, v)     i3c_ll_ip_write(id, I3C_R_CMD1_FIFO, v)
#define i3c_ll_tx_fifo_write(id, v)  i3c_ll_ip_write(id, I3C_R_TX_FIFO, v)
#define i3c_ll_rx_fifo_read(id)      i3c_ll_ip_read(id, I3C_R_RX_FIFO)
#define i3c_ll_rx_fifo_status_read(id) i3c_ll_ip_read(id, I3C_R_RX_FIFO_STATUS)
#define i3c_ll_ibir_read(id)         i3c_ll_ip_read(id, I3C_R_IBIR)
#define i3c_ll_ibi_data_read(id)     i3c_ll_ip_read(id, I3C_R_IBI_DATA_FIFO)
#define i3c_ll_tx_rx_thr_write(id,v) i3c_ll_ip_write(id, I3C_R_TX_RX_THR_CTRL, v)

#define i3c_ll_slv_ctrl_write(id, v)    i3c_ll_ip_write(id, I3C_R_SLV_CTRL, v)
#define i3c_ll_slv_status1_read(id)     i3c_ll_ip_read(id, I3C_R_SLV_STATUS1)
#define i3c_ll_slv_status1_write(id, v) i3c_ll_ip_write(id, I3C_R_SLV_STATUS1, v)
#define i3c_ll_slv_ier_write(id, v)     i3c_ll_ip_write(id, I3C_R_SLV_IER, v)
#define i3c_ll_slv_isr_read(id)         i3c_ll_ip_read(id, I3C_R_SLV_ISR)
#define i3c_ll_slv_icr_write(id, v)     i3c_ll_ip_write(id, I3C_R_SLV_ICR, v)
#define i3c_ll_slv_ibi_ctrl_read(id)    i3c_ll_ip_read(id, I3C_R_SLV_IBI_CTRL)
#define i3c_ll_slv_ibi_ctrl_write(id,v) i3c_ll_ip_write(id, I3C_R_SLV_IBI_CTRL, v)
#define i3c_ll_slv_ddr_tx_write(id, v)   i3c_ll_ip_write(id, I3C_R_SLV_DDR_TX_FIFO, v)
#define i3c_ll_slv_ddr_rx_read(id)      i3c_ll_ip_read(id, I3C_R_SLV_DDR_RX_FIFO)

/* SIR_MAP: index 0..5 -> word 0x60..0x65 */
static inline void i3c_ll_sir_map_write(i3c_ll_id_t id, uint32_t map_index, uint32_t value)
{
	if (map_index < 6u)
		i3c_ll_ip_write(id, I3C_R_SIR_MAP_BASE + map_index, value);
}

/* DEV table: per UG, dev i has RR0/RR1/RR2 at word 0x30+i*4, 0x31+i*4, 0x32+i*4 */
static inline uint32_t i3c_ll_dev_rr0(i3c_ll_id_t id, uint32_t dev_idx)
{
	return i3c_ll_ip_read(id, I3C_R_DEV_TABLE_BASE + dev_idx * 4u + 0u);
}

static inline void i3c_ll_dev_rr1_write(i3c_ll_id_t id, uint32_t dev_idx, uint32_t val)
{
	i3c_ll_ip_write(id, I3C_R_DEV_TABLE_BASE + dev_idx * 4u + 1u, val);
}

static inline uint32_t i3c_ll_dev_rr1(i3c_ll_id_t id, uint32_t dev_idx)
{
	return i3c_ll_ip_read(id, I3C_R_DEV_TABLE_BASE + dev_idx * 4u + 1u);
}

static inline uint32_t i3c_ll_dev_rr2(i3c_ll_id_t id, uint32_t dev_idx)
{
	return i3c_ll_ip_read(id, I3C_R_DEV_TABLE_BASE + dev_idx * 4u + 2u);
}

static inline void i3c_ll_dev_rr2_write(i3c_ll_id_t id, uint32_t dev_idx, uint32_t val)
{
	i3c_ll_ip_write(id, I3C_R_DEV_TABLE_BASE + dev_idx * 4u + 2u, val);
}

#ifdef __cplusplus
}
#endif

#endif /* I3C_LL_H */
