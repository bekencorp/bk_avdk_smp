/**
 * @file i3c_master.c
 * @brief I3C Master-only: strap, SDR/HDR ctrl, ENTDAA, CCC, SDR/HDR tx/rx, I2C compat.
 *        Standard API (bk_i3c_master_*) encapsulates transfer + ISR.
 */
#include <stddef.h>
#include <string.h>
#include "i3c_common.h"
#include "i3c_master.h"

typedef struct { uint16_t mwl_bytes; uint16_t mrl_bytes; } i3c_bus_init_config_t;
#include "gpio_driver.h"
#include <os/os.h>
#include <driver/int.h>
#include "sys_driver.h"

#if defined(CONFIG_I3C_TEST)
static void i3c_mst_isr_dump(uint32_t st)
{
	/* UG: Table 65 Master Mode Interrupt Registers Layout */
	struct bit_name { uint32_t bit; const char *name; };
	static const struct bit_name bits[] = {
		{ 1u << 0,  "CMDR_OVF" },
		{ 1u << 1,  "CMDR_UNF" },
		{ 1u << 2,  "CMDR_THR" },
		{ 1u << 3,  "CMDD_OVF" },
		{ 1u << 4,  "CMDD_THR" },
		{ 1u << 5,  "CMDD_EMP" },
		{ 1u << 6,  "RX_UNF" },
		{ 1u << 7,  "RX_THR" },
		{ 1u << 8,  "IBIR_OVF" },
		{ 1u << 9,  "IBIR_UNF" },
		{ 1u << 10, "IBIR_THR" },
		{ 1u << 11, "IBID_UNF" },
		{ 1u << 12, "IBID_THR" },
		{ 1u << 14, "TX_OVF" },
		{ 1u << 15, "TX_THR" },
		{ 1u << 16, "IMM_COMP" },
		{ 1u << 17, "MR_DONE" },
		{ 1u << 18, "HALTED" },
	};

	I3C_MST_LOGD("i3c mst_isr: 0x%08x", (unsigned)st);
	for (unsigned i = 0; i < (unsigned)(sizeof(bits) / sizeof(bits[0])); i++) {
		if (st & bits[i].bit)
			I3C_MST_LOGD(" %s", bits[i].name);
	}
	I3C_MST_LOGD("\r\n");
}
#endif

static void i3c_master_ps_set(void)
{
	i3c_beken_set_ps_device_role(I3C_BEKEN_PS_DEVICE_ROLE_MAIN_MASTER);
	i3c_beken_set_ps_dcr(0x5a);
	i3c_beken_set_ps_mrl(0xffffff);
	i3c_beken_set_ps_mwl(0xffff);
	i3c_beken_set_ps_mxds_limited(0);
	i3c_beken_set_ps_mxds_maxwr(0);
	i3c_beken_set_ps_mxds_maxrd(0);
	i3c_beken_set_sw_reset(1);
	I3C_DELAY_MS(6);
	i3c_beken_set_sw_reset(1);

}

/* PRESCL_CTRL1: OD/PP timing; historical platform default (see I3C.txt reset / board bring-up). */
#define I3C_MASTER_PRESCL1_DEFAULT  ((0x0u << 8) | (0x8u << 0))
/* DEVS_CTRL: enable master device-table slots used for ENTDAA / dynamic addressing. */
#define I3C_MASTER_DEVS_CTRL_SLOTS  (0x1ffu << 17)

/**
 * Program master SDR clock/control after soft reset: CTRL mode, PRESCL0/1, DEV_EN, DEVS_CTRL.
 * @param pp_high  PP_HIGH field (12 bits used, same as i3c_master_sdr_ctrl_set_freq).
 * @param pp_low   PP_LOW field (low 8 bits).
 */
static void i3c_master_apply_sdr_hw_regs(uint32_t pp_high, uint32_t pp_low)
{
	uint32_t ctrl;
	uint32_t prescl0;

	i3c_beken_set_sw_reset(1);

	ctrl = i3c_hal_ctrl_read(I3C_UNIT_ID);
	ctrl = (ctrl & ~I3C_CTRL_MODE_MASK) | I3C_CTRL_MODE_MASTER;
	i3c_hal_ctrl_write(I3C_UNIT_ID, ctrl);

	prescl0 = ((pp_high & 0xfffu) << 16) | (pp_low & 0xffu);
	i3c_hal_prescl0_write(I3C_UNIT_ID, prescl0);

	i3c_hal_prescl1_write(I3C_UNIT_ID, I3C_MASTER_PRESCL1_DEFAULT);

	ctrl = i3c_hal_ctrl_read(I3C_UNIT_ID);
	i3c_hal_ctrl_write(I3C_UNIT_ID, ctrl | I3C_CTRL_DEV_EN);

	i3c_hal_devs_ctrl_write(I3C_UNIT_ID, I3C_MASTER_DEVS_CTRL_SLOTS);
}

/* Legacy SDR timing: fixed PP_HIGH=0x19, PP_LOW=i3c_clk_div. SCL ≈ f_core / (2*(PP_HIGH+1)*(PP_LOW+1)). */
static void i3c_master_sdr_ctrl_set(uint8_t i3c_clk_div)
{
	i3c_master_apply_sdr_hw_regs(I3C_MST_PP_HIGH_LEGACY, (uint32_t)i3c_clk_div);
}

static void i3c_master_hdr_ctrl_set(uint8_t i3c_clk_div)
{
	i3c_master_sdr_ctrl_set(i3c_clk_div);
}

/**
 * Set SCL frequency by target Hz. Uses f_SCL = f_core / (2*(PP_HIGH+1)*(PP_LOW+1)).
 * f_core comes from i3c_core_clk_src_apply() (APLL default 98 MHz or XTAL 26 MHz path).
 * product = floor(f_core/(2*target)) truncates: e.g. f_core=98.304 MHz and target=10 MHz also
 * yield product=4 → actual f_SCL = 12.288 MHz (f_core/8). For an exact 12.288 MHz intent use
 * I3C_MST_SDR_TARGET_HZ_12M288 (same prescaler). Higher targets need board/slave margin.
 *
 * Prescaler: (PP_HIGH+1)*(PP_LOW+1) = product. Default legacy PP_HIGH=product-1, PP_LOW=0 (test_i3c).
 * product=24 (2M): always (23,0). Never use generic 12×2 → (11,1); that breaks 2M.
 * product=4 (~12.3M): always (1,1) vs (3,0).
 * product=12 (~4M): legacy (11,0) like test_i3c. (5,1) lab tests still failed 4M; 2M is product=24
 * (23,0) so it does not pick up any 12×2 split — do not confuse with product=12.
 * product=49 (~1M @ 98.304 MHz): (6,6) not (48,0); huge PP_HIGH + PP_LOW=0 glitches slowest SCL.
 */
static void i3c_master_sdr_ctrl_set_freq(uint32_t target_hz)
{
	uint32_t f_core = i3c_core_hz_get();
	uint32_t product;

	if (target_hz == 0u)
		target_hz = 400000u; /* fallback ~400k */

	product = f_core / (2u * target_hz);
	if (product < 2u)
		product = 2u;

	uint32_t pp_high, pp_low;
	if (product <= 256u) {
		if (product == 4u) {
			/* 4 = 2×2 — (1,1) widens low phase vs (3,0) at ~12.3 MHz SCL */
			pp_high = 1u;
			pp_low = 1u;
		} else if (product == 12u) {
			pp_high = 11u;
			pp_low = 0u;
		} else if (product == 49u) {
			pp_high = 6u;
			pp_low = 6u;
		} else {
			pp_high = product - 1u;
			pp_low = 0u;
		}
	} else {
		pp_high = I3C_MST_PP_HIGH_LEGACY; /* legacy fixed PP_HIGH */
		pp_low = (product / (pp_high + 1u)) - 1u;
		if (pp_low > 255u)
			pp_low = 255u;
	}

	i3c_master_apply_sdr_hw_regs(pp_high, pp_low);
}

static void i3c_master_hdr_ctrl_set_freq(uint32_t target_hz)
{
	i3c_master_sdr_ctrl_set_freq(target_hz);
}

static int i3c_master_entdaa(void)
{
	uint32_t isr = i3c_hal_mst_isr_read(I3C_UNIT_ID);
	/* Clear sticky status bits before issuing ENTDAA so debug isn't polluted by stale/print-induced flags.
	 * UG Table 65: most MST_ISR bits are W1C via MST_ICR. */
	i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ICR_CLEAR_MASK); /* clear sticky MST_ISR bits */
	isr = i3c_hal_mst_isr_read(I3C_UNIT_ID);

	/* Drain stale CMDR responses and RX data so the next CMDR read belongs to ENTDAA. */
	{
		unsigned n = 0;
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0u) { /* CMDR_EMP==0 => not empty */
			(void)i3c_hal_cmdr_read(I3C_UNIT_ID);
			if (++n > 64u)
				break;
		}
		(void)n;
	}
	{
		unsigned n = 0;
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & (1u << 2)) == 0u) { /* RX_EMP==0 => not empty */
			(void)i3c_hal_rx_fifo_read(I3C_UNIT_ID);
			if (++n > 64u)
				break;
		}
		(void)n;
	}

	/* NOTE: do not read CMDR here; if response queue is empty it can set CMDR_UNF. */
	I3C_MST_LOGD("i3c entdaa: before, MST_STATUS0=%08x MST_ISR=%08x DEVS_CTRL=%08x\r\n",
	            (unsigned)i3c_hal_mst_status0_read(I3C_UNIT_ID),
	            (unsigned)isr,
	            (unsigned)i3c_hal_devs_ctrl_read(I3C_UNIT_ID));
#if defined(CONFIG_I3C_TEST)
	i3c_mst_isr_dump(isr);
#endif
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ENTDAA);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
	/* Wait for ENTDAA to complete; in no-slave cases, avoid hanging forever. */
	{
		uint32_t timeout = 200; /* ~1s at 5ms/loop, adjust as needed */
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u && timeout-- > 0u)
			I3C_DELAY_MS(5);
		if ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		{
			uint32_t isr_to = i3c_hal_mst_isr_read(I3C_UNIT_ID);
#if defined(CONFIG_I3C_TEST)
			i3c_mst_isr_dump(isr_to);
#endif
			return -1;
		}
	}
	{
		uint32_t cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
		uint32_t err = I3C_MST_CMDR_ERR_GET(cmdr);
		uint32_t isr_done = i3c_hal_mst_isr_read(I3C_UNIT_ID);
#if defined(CONFIG_I3C_TEST)
		i3c_mst_isr_dump(isr_done);
#endif
		return (err != 0u) ? -1 : 0;
	}
}

static int i3c_master_rstdaa_broadcast(void)
{
	/* RSTDAA CCC (broadcast): clear all dynamic addresses so repeated ENTDAA works across runs. */
	i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ICR_CLEAR_MASK);

	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_RSTDAA));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);

	/* Wait for completion (busy bit clears). */
	{
		uint32_t timeout = 200;
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u && timeout-- > 0u)
			I3C_DELAY_MS(5);
		if ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
			return -1;
	}
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

static void i3c_master_ccc_getpid(uint8_t da)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC_DUP24(I3C_CCC_GETPID));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_R_PL6(da));
}

static void i3c_master_ccc_getbcr(uint8_t da)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC_DUP24(I3C_CCC_GETBCR));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_R_PL1(da));
}

static void i3c_master_ccc_getdcr(uint8_t da)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC_DUP24(I3C_CCC_GETDCR));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_R_PL1(da));
}

/* Address parity for SETDASA/SETNEWDA payload: XNOR of 7-bit addr bits (UG DEVID*_RR0 ADDR_PAR). */
static uint8_t i3c_addr_parity(uint8_t addr_7bit)
{
	uint8_t p = (addr_7bit >> 0) & 1u;
	p ^= (addr_7bit >> 1) & 1u;
	p ^= (addr_7bit >> 2) & 1u;
	p ^= (addr_7bit >> 3) & 1u;
	p ^= (addr_7bit >> 4) & 1u;
	p ^= (addr_7bit >> 5) & 1u;
	p ^= (addr_7bit >> 6) & 1u;
	return p & 1u; /* even parity: payload bit0 = parity so {addr,parity} has even number of 1s */
}

static int i3c_master_ccc_setdasa(uint8_t static_addr, uint8_t dynamic_addr)
{
	uint32_t payload = (uint32_t)((dynamic_addr & 0x7Fu) << 1) | (uint32_t)i3c_addr_parity(dynamic_addr & 0x7Fu);
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, payload);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_SETDASA));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_W_PL1(static_addr & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

static int i3c_master_ccc_setnewda(uint8_t current_da, uint8_t new_dynamic_addr)
{
	uint32_t payload = (uint32_t)((new_dynamic_addr & 0x7Fu) << 1) | (uint32_t)i3c_addr_parity(new_dynamic_addr & 0x7Fu);
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, payload);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_SETNEWDA));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_W_PL1(current_da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

static int i3c_master_ccc_setmwl(uint8_t da, uint16_t mwl_bytes)
{
	/* UG Table 70: 2-byte payload, LSB octet first on the bus. On this IP the TX shifter outputs [15:8]
	 * before [7:0], so for value 256 (bytes 0x00, 0x01) the register low 16 bits must be 0x0001:
	 * (lsb<<8)|msb — NOT (uint32_t)256 (0x0100), which would swap the two octets on the wire.
	 * RX FIFO assembles the logical uint16 differently; GET uses (uint16_t)(w & 0xFFFF). */
	uint8_t lsb = (uint8_t)(mwl_bytes & 0xFFu);
	uint8_t msb = (uint8_t)((mwl_bytes >> 8) & 0xFFu);
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, ((uint32_t)lsb << 8) | (uint32_t)msb);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_SETMWL));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_W_PL2(da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

static int i3c_master_ccc_setmrl(uint8_t da, uint16_t mrl_bytes)
{
	/* Same TX halfword packing as SETMWL (see comment there). */
	uint8_t lsb = (uint8_t)(mrl_bytes & 0xFFu);
	uint8_t msb = (uint8_t)((mrl_bytes >> 8) & 0xFFu);
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, ((uint32_t)lsb << 8) | (uint32_t)msb);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_SETMRL));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_W_PL2(da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

/* CCC 2-byte payload in RX FIFO low 16 bits: same lane order as SET TX ((lsb<<8)|msb). Logical uint16 = byte-swap. */
static uint16_t i3c_master_ccc_rx_u16_from_fifo(uint32_t w)
{
	return (uint16_t)(((w & 0xFFu) << 8) | ((w >> 8) & 0xFFu));
}

static int i3c_master_ccc_getmwl(uint8_t da, uint16_t *out_mwl_bytes)
{
	/* GETMWL (directed) returns 2 bytes payload: LSB, MSB (UG Table 70 references layout for SET*;
	 * for GET* we read RX FIFO and decode similarly). */
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC_DUP24(I3C_CCC_GETMWL));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_R_PL2(da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	uint32_t cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
	if (I3C_MST_CMDR_ERR_GET(cmdr) != 0u)
		return -1;
	uint32_t w = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
	uint16_t v = i3c_master_ccc_rx_u16_from_fifo(w);
	if (out_mwl_bytes)
		*out_mwl_bytes = v;
	return 0;
}

static int i3c_master_ccc_getmrl(uint8_t da, uint16_t *out_mrl_bytes)
{
	/* GETMRL (directed) returns 2 bytes payload: LSB, MSB. */
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC_DUP24(I3C_CCC_GETMRL));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_R_PL2(da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	uint32_t cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
	if (I3C_MST_CMDR_ERR_GET(cmdr) != 0u)
		return -1;
	uint32_t w = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
	uint16_t v = i3c_master_ccc_rx_u16_from_fifo(w);
	if (out_mrl_bytes)
		*out_mrl_bytes = v;
	return 0;
}

/* ENEC (Enable Events): directed 0xC1 or broadcast 0xC0. event_byte bit0=IBI (0x01). */
static int i3c_master_ccc_enec(uint8_t da, uint8_t event_byte)
{
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, (uint32_t)event_byte);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_ENEC_DIR));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_DIRECT_CCC_W_PL1(da & 0x7Fu));
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

/* ENEC broadcast 0xC0: enable IBI for all devices (fallback when directed ENEC NACKed). */
static int i3c_master_ccc_enec_broadcast(uint8_t event_byte)
{
	i3c_hal_tx_fifo_write(I3C_UNIT_ID, (uint32_t)event_byte);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_ENEC_BC));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL1);
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
		I3C_DELAY_MS(5);
	return (I3C_MST_CMDR_ERR_GET(i3c_hal_cmdr_read(I3C_UNIT_ID)) != 0u) ? -1 : 0;
}

/* Build one dev slot for SIR_MAP: UG Table 160 - da in [7:1]/[23:17], resp in [0]/[16], pl in [12:8]/[28:24]. */
static uint16_t i3c_sir_map_dev_slot(uint8_t da, uint8_t resp, uint8_t pl)
{
	return (uint16_t)(((uint32_t)(da & 0x7Fu) << 1) | (resp & 1u) | ((pl & 0x1Fu) << 8));
}

/* Enable IBI for all active devices in DEVS_CTRL (UG Table 160: per-device DA + resp). */
static void i3c_master_ibi_sir_map_enable_all(void)
{
	uint32_t ctrl = i3c_hal_devs_ctrl_read(I3C_UNIT_ID);
	for (uint32_t i = 1u; i < 12u; i++) {
		if (!((ctrl >> i) & 1u))
			continue;
		uint8_t da = (uint8_t)((i3c_hal_dev_rr0(I3C_UNIT_ID, i) >> 1) & 0x7Fu);
		uint16_t slot = i3c_sir_map_dev_slot(da, 1u, 0u);
		uint32_t idx = i / 2u;
		uint32_t old = i3c_hal_sir_map_read(I3C_UNIT_ID, idx);
		uint32_t v = ((i & 1u) == 0u) ? ((old & 0xFFFF0000u) | (uint32_t)slot) : ((old & 0x0000FFFFu) | ((uint32_t)slot << 16));
		i3c_hal_sir_map_write(I3C_UNIT_ID, idx, v);
	}
}

static void i3c_master_sir_map_set(uint32_t map_index, uint32_t value);

/* Enable IBI for devices in DEVS_CTRL. SIR_MAP per UG Table 160: program DA for each active dev. */
static void i3c_master_ibi_sir_map_enable_from_devs_ctrl(void)
{
	/* UG Table 160: default da=0x7F means "no device" - use it for unprogrammed slots to avoid
	 * spurious matches (slv_id=10, MR, HJ from da=0 matching bus glitches). */
	uint16_t slot_invalid = i3c_sir_map_dev_slot(0x7Fu, 0u, 0u);
	for (uint32_t i = 0u; i < 6u; i++)
		i3c_master_sir_map_set(i, (uint32_t)slot_invalid | ((uint32_t)slot_invalid << 16));

	uint32_t ctrl = i3c_hal_devs_ctrl_read(I3C_UNIT_ID);
	for (uint32_t i = 1u; i < 12u; i++) {
		if (!((ctrl >> i) & 1u))
			continue;
		uint8_t da = (uint8_t)((i3c_hal_dev_rr0(I3C_UNIT_ID, i) >> 1) & 0x7Fu);
		uint16_t slot = i3c_sir_map_dev_slot(da, 1u, 0u); /* ACK, pl=0 (MDB only) */
		uint32_t idx = i / 2u;
		uint32_t old = i3c_hal_sir_map_read(I3C_UNIT_ID, idx);
		uint32_t v = ((i & 1u) == 0u) ? ((old & 0xFFFF0000u) | (uint32_t)slot) : ((old & 0x0000FFFFu) | ((uint32_t)slot << 16));
		i3c_hal_sir_map_write(I3C_UNIT_ID, idx, v);
	}
}

/* Forward declare for use before definition */
static void i3c_master_hdr_set_daa(void);

static int i3c_master_hdr_bus_init(const i3c_bus_init_config_t *cfg)
{
	uint16_t mwl = (cfg && cfg->mwl_bytes != 0u) ? cfg->mwl_bytes : 256u;
	uint16_t mrl = (cfg && cfg->mrl_bytes != 0u) ? cfg->mrl_bytes : 256u;

	if (i3c_master_rstdaa_broadcast() != 0)
		return -1;
	i3c_master_hdr_set_daa();
	{
		uint32_t to = 2000u;
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u && to-- > 0u)
			I3C_DELAY_MS(1);
	}
	for (uint32_t di = 1u; di < 12u; di++) {
		if (!((i3c_hal_devs_ctrl_read(I3C_UNIT_ID) >> di) & 1u))
			continue;
		uint8_t da = (uint8_t)((i3c_hal_dev_rr0(I3C_UNIT_ID, di) >> 1) & 0x3fu);
		(void)i3c_master_ccc_setmwl(da, mwl);
		(void)i3c_master_ccc_setmrl(da, mrl);
	}
	return 0;
}

static int i3c_master_sdr_bus_init(const i3c_bus_init_config_t *cfg)
{
	uint16_t mwl_cfg = (cfg && cfg->mwl_bytes != 0u) ? cfg->mwl_bytes : 256u;
	uint16_t mrl_cfg = (cfg && cfg->mrl_bytes != 0u) ? cfg->mrl_bytes : 256u;
	uint8_t cur_da;
	uint32_t pid_h;
	uint16_t pid_l;
	uint8_t bcr, dcr;
	uint32_t cmdr, rx_fifo;
	uint32_t i;

	/* Make repeated CLI tests idempotent: clear DA then run ENTDAA. */
	(void)i3c_master_rstdaa_broadcast();

	if (i3c_master_entdaa() != 0)
		return -1;

	for (i = 1u; i < 12u; i++) {
		if (!((i3c_hal_devs_ctrl_read(I3C_UNIT_ID) >> i) & 0x1u))
			continue;
		cur_da = (uint8_t)((i3c_hal_dev_rr0(I3C_UNIT_ID, i) >> 1) & 0x3fu);

		/* Read default MWL/MRL as seen on the bus. */
		{
			uint16_t def_mwl = 0, def_mrl = 0;
			int r1 = i3c_master_ccc_getmwl(cur_da, &def_mwl);
			int r2 = i3c_master_ccc_getmrl(cur_da, &def_mrl);
			(void)r1;
			(void)r2;
#if defined(CONFIG_I3C_TEST)
			I3C_MST_LOGD("i3c: dev DA=0x%02x default MWL=%u (ret=%d) MRL=%u (ret=%d)\r\n",
			           (unsigned)cur_da,
			           (unsigned)def_mwl, r1,
			           (unsigned)def_mrl, r2);
#endif
		}

		/* Ensure private transfer lengths are not limited by stale/default MRL/MWL. */
		(void)i3c_master_ccc_setmwl(cur_da, mwl_cfg);
		(void)i3c_master_ccc_setmrl(cur_da, mrl_cfg);

		/* Read back after setting (sanity). */
		{
			uint16_t mwl = 0, mrl = 0;
			int r1 = i3c_master_ccc_getmwl(cur_da, &mwl);
			int r2 = i3c_master_ccc_getmrl(cur_da, &mrl);
			(void)r1;
			(void)r2;
#if defined(CONFIG_I3C_TEST)
			I3C_MST_LOGD("i3c: dev DA=0x%02x after  SETMWL/MRL MWL=%u (ret=%d) MRL=%u (ret=%d)\r\n",
			           (unsigned)cur_da,
			           (unsigned)mwl, r1,
			           (unsigned)mrl, r2);
#endif
		}

		i3c_master_ccc_getpid(cur_da);
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
			I3C_DELAY_MS(5);
		cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
		if (I3C_MST_CMDR_ERR_GET(cmdr) != 0u)
			continue;
		rx_fifo = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		pid_h = ((rx_fifo & 0xffu) << 24) | (((rx_fifo >> 8) & 0xffu) << 16) |
			(((rx_fifo >> 16) & 0xffu) << 8) | ((rx_fifo >> 24) & 0xffu);
		rx_fifo = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		pid_l = (uint16_t)(((rx_fifo & 0xffu) << 8) | ((rx_fifo >> 8) & 0xffu));

		i3c_master_ccc_getbcr(cur_da);
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
			I3C_DELAY_MS(5);
		cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
		if (I3C_MST_CMDR_ERR_GET(cmdr) != 0u)
			continue;
		rx_fifo = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		bcr = (uint8_t)(rx_fifo & 0xffu);

		i3c_master_ccc_getdcr(cur_da);
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u)
			I3C_DELAY_MS(5);
		cmdr = i3c_hal_cmdr_read(I3C_UNIT_ID);
		if (I3C_MST_CMDR_ERR_GET(cmdr) != 0u)
			continue;
		rx_fifo = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		dcr = (uint8_t)(rx_fifo & 0xffu);

		i3c_hal_dev_rr1_write(I3C_UNIT_ID, i, pid_h);
		i3c_hal_dev_rr2_write(I3C_UNIT_ID, i, (uint32_t)(pid_l << 16) | ((uint32_t)bcr << 8) | dcr);
	}
	return 0;
}

/* Drain CMDR/RX after controller reprogram (same polarity as bk_i3c_master_sdr_read preamble). */
static void i3c_master_mst_drain_cmdr_rx_bounded(void)
{
	unsigned n;

	for (n = 0u; n < 64u && (i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0u; n++)
		(void)i3c_hal_cmdr_read(I3C_UNIT_ID);
	for (n = 0u; n < 64u && (i3c_hal_mst_status0_read(I3C_UNIT_ID) & (1u << 2)) == 0u; n++)
		(void)i3c_hal_rx_fifo_read(I3C_UNIT_ID);
}

/* After SCL is raised past I3C_MST_SDR_ENUM_MAX_HZ, apply_sdr_hw_regs() soft-resets the master.
 * Re-issue SETMWL/MRL at the final SCL so targets see limits on the wire rate used for private transfers. */
static void i3c_master_sdr_reapply_mwl_mrl(uint16_t mwl_bytes, uint16_t mrl_bytes)
{
	uint16_t mwl_cfg = (mwl_bytes != 0u) ? mwl_bytes : 256u;
	uint16_t mrl_cfg = (mrl_bytes != 0u) ? mrl_bytes : 256u;

	for (uint32_t di = 1u; di < 12u; di++) {
		if (!((i3c_hal_devs_ctrl_read(I3C_UNIT_ID) >> di) & 1u))
			continue;
		uint8_t da = (uint8_t)((i3c_hal_dev_rr0(I3C_UNIT_ID, di) >> 1) & 0x3fu);
		(void)i3c_master_ccc_setmwl(da, mwl_cfg);
		(void)i3c_master_ccc_setmrl(da, mrl_cfg);
	}
}

/* SDR private transfer CMD0: I3C_MST_SDR_CMD0_PRIVATE_BASE | PL_LEN[23:12] | DEV_ADDR[7:1] | RnW[0]. */
static inline void i3c_master_sdr_cmd0(uint8_t dev_addr, uint32_t pl_len_bytes, uint8_t rnw)
{
	uint32_t cmd0 = I3C_MST_SDR_CMD0_PRIVATE_BASE | ((pl_len_bytes & 0xFFFu) << 12) |
			((uint32_t)(dev_addr & 0x7Fu) << 1) | (rnw & 1u);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, cmd0);
}

static void i3c_master_sdr_tx(void)
{
	i3c_master_sdr_cmd0(0x09u, 256u, 0u);
}

static void i3c_master_sdr_rx(void)
{
	i3c_master_sdr_cmd0(0x09u, 128u, 1u);
}

static void i3c_master_hdr_tx(void)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_HDR_DDR_PREAMBLE);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_HDR_DDR_TX_FIRST);
}

static void i3c_master_hdr_rx(void)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_HDR_DDR_PREAMBLE);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_HDR_DDR_RX_FIRST);
}

static void i3c_master_hdr_rx_n(uint32_t pl_len)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_HDR_DDR_PREAMBLE);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_HDR_DDR_CHUNK(pl_len & 0xFFFu));
}

static void i3c_master_hdr_set_daa(void)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_CCC(I3C_CCC_ENTDAA));
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
}

static void i3c_master_i2c_tx(void)
{
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_I2C_TX);
}

static void i3c_master_i2c_rx(void)
{
	i3c_hal_tx_rx_thr_write(I3C_UNIT_ID, I3C_MST_TX_RX_THR_DEFAULT);
	i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
	i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_I2C_RX);
}

static void i3c_master_sdr_int_en(void)
{
	i3c_hal_mst_ier_write(I3C_UNIT_ID, I3C_MST_IER_RX_THR);
}

/* IBI: Master receive. ISR bit defs in i3c_master.h (I3C_MST_ISR_IBI_*). */
static void i3c_master_ibi_int_clear_bits(uint32_t mask)
{
	i3c_hal_mst_icr_write(I3C_UNIT_ID, mask & I3C_MST_ISR_IBI_ALL);
}

static void i3c_master_sir_map_set(uint32_t map_index, uint32_t value)
{
	if (map_index < 6u)
		i3c_hal_sir_map_write(I3C_UNIT_ID, map_index, value);
}

static void i3c_master_ibi_int_clear(void)
{
	i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ISR_IBIR_THR);
}

/*
 * Drain IBI response queue (UG section 3.12, Tables 65/67/92):
 * - Loop while IBIR_EMP=0 (IBIR queue not empty)
 * - Per entry: read IBIR (pops queue) -> get xfer_bytes -> read that many from IBI_DATA_FIFO
 * - IBI always has MDB (min 1 byte); when xfer_bytes=0 use 1 word to drain MDB
 * - Stop on IBIR_UNF (read empty IBIR) to avoid garbage/spurious entries
 */
#define I3C_IBI_PAYLOAD_WORDS_MAX  8u  /* UG Table 92: xfer_bytes 5 bits, max 31 bytes */
#define I3C_IBI_DRAIN_MAX_ENTRIES  8u  /* safety limit */

static void i3c_master_ibi_queue_drain(void (*entry_cb)(uint32_t ibir, const uint32_t *data_words, unsigned num_words, void *ctx), void *ctx)
{
	unsigned count = 0u;
	unsigned garbage_streak = 0u; /* UG: slv_id=15 + xfer=0 = no-match/garbage; stop after 2 in a row */
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & I3C_MST_STATUS0_IBIR_EMP) == 0u && count < I3C_IBI_DRAIN_MAX_ENTRIES) {
		if ((i3c_hal_mst_isr_read(I3C_UNIT_ID) & I3C_MST_ISR_IBIR_UNF) != 0u) {
			i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ISR_IBIR_UNF);
			break;
		}
		uint32_t ibir = i3c_hal_ibir_read(I3C_UNIT_ID);
		unsigned n_bytes = (unsigned)((ibir & I3C_IBIR_XFER_BYTES_MASK) >> I3C_IBIR_XFER_BYTES_SHIFT);
		unsigned n_words = (n_bytes + 3u) / 4u;
		if (n_words == 0u && (ibir & I3C_IBIR_TYPE_MASK) == I3C_IBIR_TYPE_IBI)
			n_words = 1u; /* MDB at least 1 byte (UG 3.12) */
		uint32_t buf[I3C_IBI_PAYLOAD_WORDS_MAX];
		if (n_words > I3C_IBI_PAYLOAD_WORDS_MAX)
			n_words = I3C_IBI_PAYLOAD_WORDS_MAX;
		for (unsigned i = 0u; i < n_words; i++)
			buf[i] = i3c_hal_ibi_data_read(I3C_UNIT_ID);
		if (entry_cb != NULL)
			entry_cb(ibir, buf, n_words, ctx);
		count++;
		/* Stop when IBIR_OVF yields repeated slv_id=15 xfer=0 (garbage) to avoid log flood */
		{
			unsigned sid = (ibir & I3C_IBIR_SLAVE_ID_MASK) >> I3C_IBIR_SLAVE_ID_SHIFT;
			int is_garbage = (sid == 15u && n_bytes == 0u && (ibir & I3C_IBIR_RESP_BIT) == 0u) ? 1 : 0;
			if (is_garbage) {
				garbage_streak++;
				if (garbage_streak >= 2u)
					break;
			} else {
				garbage_streak = 0u;
			}
		}
	}
}

/* ---------- Standard Master API (bk_i3c_master_*) ---------- */
#define I3C_HDR_FIFO_WORDS        32u
#define I3C_HDR_MAX_DATA_PER_CHUNK 30u
#define I3C_SLV_HDR_MAX_BYTES_PER_CHUNK 62u
#define I3C_SLV_HDR_TX_BUF_MAX    256u

static beken_semaphore_t s_mst_sdr_sem;
static volatile uint8_t s_mst_sdr_sem_inited;
static volatile uint8_t s_mst_mode;  /* 0=none, 1=sdr_rx */
static i3c_master_mode_t s_mst_cfg_mode = I3C_MST_MODE_SDR;  /* from init; used by write/read */
static volatile uint8_t s_mst_int_registered;
/** Last SDR init target_hz (0 = unknown / clk_div path); used for RX THR heuristics. */
static uint32_t s_mst_sdr_target_hz;

static void i3c_mst_isr_handler(void)
{
	uint32_t st = i3c_hal_mst_isr_read(I3C_UNIT_ID);
	if (s_mst_mode == 1u && (st & I3C_MST_IER_RX_THR)) {
		i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_IER_RX_THR);
		rtos_set_semaphore(&s_mst_sdr_sem);
	}
}

static void i3c_mst_int_register(void)
{
	if (s_mst_int_registered)
		return;
	s_mst_int_registered = 1;
	bk_int_isr_register(INT_SRC_I3C, i3c_mst_isr_handler, NULL);
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_I3C, 1);
	I3C_MST_LOGI("i3c mst: interrupt handler registered\r\n");
}

void bk_i3c_master_devices_print(void)
{
	uint32_t ctrl = i3c_hal_devs_ctrl_read(I3C_UNIT_ID);
	unsigned n = 0u;

	I3C_MST_LOGI("i3c mst: device table (DEVS_CTRL=0x%08lx)\r\n", (unsigned long)ctrl);
	for (uint32_t i = 1u; i < 12u; i++) {
		if (!((ctrl >> i) & 1u))
			continue;
		n++;
		{
			uint32_t rr0 = i3c_hal_dev_rr0(I3C_UNIT_ID, i);
			uint32_t rr1 = i3c_hal_dev_rr1(I3C_UNIT_ID, i);
			uint32_t rr2 = i3c_hal_dev_rr2(I3C_UNIT_ID, i);
			uint8_t da = (uint8_t)((rr0 >> 1) & 0x7Fu);
			uint16_t pid_l = (uint16_t)((rr2 >> 16) & 0xFFFFu);
			uint8_t bcr = (uint8_t)((rr2 >> 8) & 0xFFu);
			uint8_t dcr = (uint8_t)(rr2 & 0xFFu);

			I3C_MST_LOGI("i3c mst:  slot %2u  DA=0x%02x  PID=%08lx:%04lx  BCR=0x%02x  DCR=0x%02x\r\n",
			             (unsigned)i, (unsigned)da,
			             (unsigned long)rr1, (unsigned long)pid_l,
			             (unsigned)bcr, (unsigned)dcr);
			I3C_MST_LOGD("i3c mst:  rr0=0x%08lx rr1=0x%08lx rr2=0x%08lx\r\n",
			             (unsigned long)rr0, (unsigned long)rr1, (unsigned long)rr2);
		}
	}
	if (n == 0u)
		I3C_MST_LOGI("i3c mst:  (no active device slots)\r\n");
}

bk_err_t bk_i3c_master_init(const i3c_master_config_t *cfg)
{
	if (!cfg)
		return BK_FAIL;
	s_mst_sdr_target_hz = 0u;
	s_mst_cfg_mode = (cfg->mode == I3C_MST_MODE_HDR) ? I3C_MST_MODE_HDR : I3C_MST_MODE_SDR;
	i3c_platform_init(cfg->platform);
	i3c_core_clk_src_apply(cfg->core_clk_src, cfg->core_hz);
	i3c_master_ps_set();
	if (s_mst_cfg_mode == I3C_MST_MODE_HDR) {
		if (cfg->target_hz != 0u)
			i3c_master_hdr_ctrl_set_freq(cfg->target_hz);
		else
			i3c_master_hdr_ctrl_set(cfg->clk_div ? cfg->clk_div : 1);
	} else {
		if (cfg->target_hz != 0u) {
			/* Run ENTDAA / CCC at most I3C_MST_SDR_ENUM_MAX_HZ; then raise to target_hz below. */
			if (cfg->target_hz > I3C_MST_SDR_ENUM_MAX_HZ)
				i3c_master_sdr_ctrl_set_freq(I3C_MST_SDR_ENUM_MAX_HZ);
			else
				i3c_master_sdr_ctrl_set_freq(cfg->target_hz);
		} else {
			i3c_master_sdr_ctrl_set(cfg->clk_div ? cfg->clk_div : 1);
		}
	}
	i3c_bus_init_config_t bus_cfg = {
		.mwl_bytes = cfg->mwl_bytes,
		.mrl_bytes = cfg->mrl_bytes,
	};
	if (s_mst_cfg_mode == I3C_MST_MODE_HDR) {
		if (i3c_master_hdr_bus_init(&bus_cfg) != 0)
			return BK_FAIL;
	} else {
		if (i3c_master_sdr_bus_init(&bus_cfg) != 0)
			return BK_FAIL;
		/* After successful enumeration, apply requested high SCL (push-pull capable rates). */
		if (cfg->target_hz != 0u && cfg->target_hz > I3C_MST_SDR_ENUM_MAX_HZ) {
			i3c_master_sdr_ctrl_set_freq(cfg->target_hz);
			I3C_DELAY_MS(10);
			i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ICR_CLEAR_MASK);
			i3c_master_mst_drain_cmdr_rx_bounded();
			i3c_master_sdr_reapply_mwl_mrl(cfg->mwl_bytes, cfg->mrl_bytes);
		}
	}
	if (s_mst_cfg_mode == I3C_MST_MODE_SDR && cfg->target_hz != 0u)
		s_mst_sdr_target_hz = cfg->target_hz;
	else
		s_mst_sdr_target_hz = 0u;
	bk_i3c_master_devices_print();
	return BK_OK;
}

bk_err_t bk_i3c_master_deinit(void)
{
	s_mst_sdr_target_hz = 0u;
	i3c_master_int_unregister();
	i3c_platform_deinit();
	return BK_OK;
}

void i3c_master_int_unregister(void)
{
	if (!s_mst_int_registered)
		return;
	s_mst_int_registered = 0;
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_I3C, 0);
	bk_int_isr_unregister(INT_SRC_I3C);
}

static bk_err_t bk_i3c_master_sdr_write(uint8_t dev_addr, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
static bk_err_t bk_i3c_master_sdr_read(uint8_t dev_addr, uint8_t *buf, uint32_t len, uint32_t *recv_len,
                                       uint32_t timeout_ms, uint32_t mode);
static bk_err_t bk_i3c_master_hdr_write(uint8_t dev_addr, uint8_t hdr_cmd, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
static bk_err_t bk_i3c_master_hdr_read(uint8_t dev_addr, uint8_t hdr_cmd, uint8_t *buf, uint32_t buf_size,
                                       uint32_t expect_len, uint32_t *recv_len, uint32_t timeout_ms);

bk_err_t bk_i3c_master_write(uint8_t dev_addr, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
	if (s_mst_cfg_mode == I3C_MST_MODE_HDR)
		return bk_i3c_master_hdr_write(dev_addr, 0x3Fu, data, len, timeout_ms);
	return bk_i3c_master_sdr_write(dev_addr, data, len, timeout_ms);
}

bk_err_t bk_i3c_master_read(uint8_t dev_addr, uint8_t *buf, uint32_t len, uint32_t *recv_len,
                            uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode)
{
	if (s_mst_cfg_mode == I3C_MST_MODE_HDR)
		return bk_i3c_master_hdr_read(dev_addr, 0x3Fu, buf, len, len, recv_len, timeout_ms);
	return bk_i3c_master_sdr_read(dev_addr, buf, len, recv_len, timeout_ms, (uint32_t)xfer_mode);
}

static bk_err_t bk_i3c_master_sdr_write(uint8_t dev_addr, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
	if (!data || len == 0u)
		return BK_FAIL;
	(void)timeout_ms;  /* TX is fire-and-forget; timeout not used for write */
	for (uint32_t widx = 0; widx < (len + 3u) / 4u; widx++) {
		uint32_t base = widx * 4u;
		uint32_t w = 0u;
		for (unsigned b = 0; b < 4u && (base + b) < len; b++)
			w |= (uint32_t)data[base + b] << (8u * b);
		i3c_hal_tx_fifo_write(I3C_UNIT_ID, w);
	}
	i3c_master_sdr_cmd0(dev_addr, len, 0u);
	return BK_OK;
}

static bk_err_t bk_i3c_master_sdr_read(uint8_t dev_addr, uint8_t *buf, uint32_t len, uint32_t *recv_len,
                                        uint32_t timeout_ms, uint32_t mode)
{
	if (!buf || !recv_len)
		return BK_FAIL;
	*recv_len = 0u;
	if (len == 0u)
		return BK_OK;
	if (timeout_ms == 0u)
		timeout_ms = 20000u;

	i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ICR_CLEAR_MASK);
	if (mode == (uint32_t)I3C_XFER_INT) {
		if (!s_mst_sdr_sem_inited) {
			s_mst_sdr_sem_inited = 1;
			rtos_init_semaphore_ex(&s_mst_sdr_sem, 1, 0);
		}
		i3c_mst_int_register();
		s_mst_mode = 1u;
		i3c_master_sdr_int_en();
	}

	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0u)
		(void)i3c_hal_cmdr_read(I3C_UNIT_ID);
	while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & (1u << 2)) == 0u)
		(void)i3c_hal_rx_fifo_read(I3C_UNIT_ID);

	/* Skip fixed THR when default RX works better: ~1M; exact 4M; 10M/12M class (final ~12.3M SCL).
	 * Else (e.g. 2M): use HDR reference 0x20001 so INT RX_THR asserts. */
	{
		uint32_t hz = s_mst_sdr_target_hz;
		int write_thr = 0;

		if (hz == 0u)
			write_thr = 1;
		else if (hz <= 1500000u)
			write_thr = 0;
		else if (hz == 4000000u)
			write_thr = 0;
		else if (hz >= 10000000u)
			write_thr = 0;
		else
			write_thr = 1;
		if (write_thr)
			i3c_hal_tx_rx_thr_write(I3C_UNIT_ID, I3C_MST_TX_RX_THR_DEFAULT);
	}

	i3c_master_sdr_cmd0(dev_addr, len, 1u);

	uint32_t timeout = (timeout_ms + 4u) / 5u;  /* ~5ms per outer iteration */
	uint32_t rx_len = 0u;
	while (rx_len < len && timeout-- > 0u) {
		uint32_t ms = i3c_hal_mst_status0_read(I3C_UNIT_ID);
		/* Drain while MST_STATUS0.RX_EMP (bit2) reports data (0 = not empty). */
		while (((ms & (1u << 2)) == 0u) && rx_len < len) {
			uint32_t w = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
			for (unsigned b = 0; b < 4u && rx_len < len; b++)
				buf[rx_len++] = (uint8_t)((w >> (8u * b)) & 0xFFu);
			ms = i3c_hal_mst_status0_read(I3C_UNIT_ID);
		}
		/* At high SCL, RX_EMP can clear before the last words are visible; drain by FIFO level too. */
		while (rx_len < len && i3c_hal_rx_fifo_status_read(I3C_UNIT_ID) != 0u) {
			uint32_t w = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
			for (unsigned b = 0; b < 4u && rx_len < len; b++)
				buf[rx_len++] = (uint8_t)((w >> (8u * b)) & 0xFFu);
		}
		if (rx_len >= len)
			break;
		ms = i3c_hal_mst_status0_read(I3C_UNIT_ID);
		if (ms & (1u << 17))
			i3c_hal_mst_status0_write(I3C_UNIT_ID, 1u << 17);
		if (mode == (uint32_t)I3C_XFER_INT)
			(void)rtos_get_semaphore(&s_mst_sdr_sem, 5u);
		else
			I3C_DELAY_MS(5);
	}
	if (mode == (uint32_t)I3C_XFER_INT)
		s_mst_mode = 0u;
	*recv_len = rx_len;
	return BK_OK;
}

static bk_err_t bk_i3c_master_hdr_write(uint8_t dev_addr, uint8_t hdr_cmd, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
	if (!data || (len & 1u) != 0u)
		return BK_FAIL;
	(void)timeout_ms;
	i3c_hal_tx_rx_thr_write(I3C_UNIT_ID, I3C_MST_TX_RX_THR_DEFAULT);
	i3c_hal_mst_icr_write(I3C_UNIT_ID, I3C_MST_ICR_CLEAR_MASK);

	unsigned data_words = len / 2u;
	uint8_t addr = (uint8_t)(dev_addr & 0x7Fu);
	uint8_t cmd = hdr_cmd;
	unsigned data_off = 0u;

	while (data_off < data_words) {
		unsigned data_in_chunk = data_words - data_off;
		if (data_in_chunk > I3C_HDR_MAX_DATA_PER_CHUNK)
			data_in_chunk = I3C_HDR_MAX_DATA_PER_CHUNK;
		unsigned chunk_words = 1u + data_in_chunk + 1u;

		uint32_t chunk[32];
		uint8_t crc = 0x1Fu;
		unsigned idx = 0;
		chunk[idx++] = i3c_hdr_tx_word(0x1u, cmd, (uint8_t)(addr << 1));
		crc = i3c_hdr_crc5(crc, (uint16_t)((cmd << 8) | (addr << 1)));
		for (unsigned w = 0; w < data_in_chunk; w++) {
			uint8_t b0 = data[(data_off + w) * 2u];
			uint8_t b1 = data[(data_off + w) * 2u + 1u];
			chunk[idx++] = i3c_hdr_tx_word((w == 0u) ? 0x2u : 0x3u, b0, b1);
			crc = i3c_hdr_crc5(crc, (uint16_t)((b0 << 8) | b1));
		}
		chunk[idx++] = (1u << 18) | (0xcu << 14) | ((uint32_t)crc << 9);

		for (unsigned i = 0; i < chunk_words; i++)
			i3c_hal_tx_fifo_write(I3C_UNIT_ID, chunk[i]);
		I3C_DELAY_MS(2);
		i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_HDR_DDR_PREAMBLE);
		i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_BROADCAST_CCC_PL0);
		i3c_hal_cmd1_write(I3C_UNIT_ID, I3C_MST_CMD1_ZERO);
		i3c_hal_cmd0_write(I3C_UNIT_ID, I3C_MST_CMD0_HDR_DDR_CHUNK(chunk_words));
		data_off += data_in_chunk;
		I3C_DELAY_MS(5);
	}
	I3C_DELAY_MS(100);
	return BK_OK;
}

static bk_err_t bk_i3c_master_hdr_read(uint8_t dev_addr, uint8_t hdr_cmd, uint8_t *buf, uint32_t buf_size,
                                       uint32_t expect_len, uint32_t *recv_len, uint32_t timeout_ms)
{
	if (!buf || !recv_len || (expect_len & 1u) != 0u)
		return BK_FAIL;
	*recv_len = 0u;
	(void)dev_addr;
	(void)hdr_cmd;
	(void)timeout_ms;

	unsigned num_chunks = (expect_len + I3C_SLV_HDR_MAX_BYTES_PER_CHUNK - 1u) / I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
	unsigned total_got = 0u;

	i3c_hal_tx_fifo_write(I3C_UNIT_ID, i3c_hdr_tx_word(0x1u, 0xffu, (uint8_t)((dev_addr & 0x7Fu) << 1)));
	I3C_DELAY_MS(2000);

	for (unsigned chunk = 0; chunk < num_chunks; chunk++) {
		unsigned chunk_bytes = expect_len - chunk * I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
		if (chunk_bytes > I3C_SLV_HDR_MAX_BYTES_PER_CHUNK)
			chunk_bytes = I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
		unsigned chunk_words = (chunk_bytes / 2u) + 1u;

		i3c_hal_tx_fifo_write(I3C_UNIT_ID, i3c_hdr_tx_word(0x1u, 0xffu, (uint8_t)((dev_addr & 0x7Fu) << 1)));
		I3C_DELAY_MS(chunk > 0 ? 2 : 0);
		i3c_master_hdr_rx_n((uint32_t)chunk_words);

		uint32_t timeout = 2000u;
		while ((i3c_hal_mst_status0_read(I3C_UNIT_ID) & 0x1u) == 0x1u && timeout-- > 0u)
			I3C_DELAY_MS(1);

		unsigned word_cnt = 0;
		uint32_t words[40];
		for (unsigned retry = 0; retry < 5u; retry++) {
			while (i3c_hal_rx_fifo_status_read(I3C_UNIT_ID) != 0u && word_cnt < 40u)
				words[word_cnt++] = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
			if (word_cnt >= chunk_words)
				break;
			for (volatile uint32_t spin = 0; spin < 1000u; spin++) (void)0;
		}
		for (unsigned i = 0; i < word_cnt && total_got < buf_size; i++) {
			uint32_t w = words[i];
			if (((w >> 18) & 3u) == 2u || ((w >> 18) & 3u) == 3u) {
				uint16_t d = (uint16_t)((w >> 2) & 0xFFFFu);
				if (total_got < buf_size) buf[total_got++] = (uint8_t)((d >> 8) & 0xFFu);
				if (total_got < buf_size) buf[total_got++] = (uint8_t)(d & 0xFFu);
			}
		}
	}
	while (i3c_hal_rx_fifo_status_read(I3C_UNIT_ID) != 0u && total_got < buf_size) {
		uint32_t w = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		if (((w >> 18) & 3u) == 2u || ((w >> 18) & 3u) == 3u) {
			uint16_t d = (uint16_t)((w >> 2) & 0xFFFFu);
			if (total_got < buf_size) buf[total_got++] = (uint8_t)((d >> 8) & 0xFFu);
			if (total_got < buf_size) buf[total_got++] = (uint8_t)(d & 0xFFu);
		}
	}
	*recv_len = total_got;
	return BK_OK;
}

