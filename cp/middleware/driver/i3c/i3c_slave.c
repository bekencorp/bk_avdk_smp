/**
 * @file i3c_slave.c
 * @brief I3C Slave-only: strap, SDR/HDR tx prepare. Standard API (bk_i3c_slave_*) encapsulates transfer + ISR.
 */
#include "i3c_common.h"
#include "i3c_slave.h"
#include <os/os.h>
#include <driver/int.h>
#include "sys_driver.h"

#define I3C_SLV_HDR_MAX_BYTES_PER_CHUNK 62u
#define I3C_SLV_DEFAULT_MAX_BYTES  128u

static uint16_t s_slv_max_mrl = I3C_SLV_DEFAULT_MAX_BYTES;
static i3c_slave_mode_t s_slv_mode = I3C_SLV_MODE_SDR;

void i3c_slave_ps_set(i3c_slv_device_t slv_device)
{
	i3c_beken_set_ps_device_role(I3C_BEKEN_PS_DEVICE_ROLE_SLAVE);
	i3c_beken_set_ps_dcr(0x5a);
	i3c_beken_set_ps_mrl(0xffffff);
	i3c_beken_set_ps_mwl(0xffff);
	i3c_beken_set_ps_mxds_limited(0);
	i3c_beken_set_ps_mxds_maxwr(0);
	i3c_beken_set_ps_mxds_maxrd(0);

	if (slv_device == 0)
		i3c_beken_set_ps_pid_mfr_id(0x5a78);
	else if (slv_device == 1)
		i3c_beken_set_ps_pid_mfr_id(0x3cab);
	else if (slv_device == 2)
		i3c_beken_set_ps_pid_mfr_id(0x1e06);

	i3c_beken_set_ps_pid_instance_id(0x5);
	i3c_beken_set_ps_bus_avail_timer(0x3);
	i3c_beken_set_ps_bus_idle_timer(0x28d);
	i3c_beken_set_ps_stat_addr(0x7);
	i3c_beken_set_ps_flow_ctrl_pr_dis(1);
	i3c_beken_set_ps_flow_ctrl_pw_dis(1);
	i3c_beken_set_ps_fpf_pw_sel(0);
	i3c_beken_set_ps_alt_mode_en(1);
	i3c_beken_set_ps_hj_in_use(0);
	i3c_beken_set_ps_ibi_mdb_prn(0);
	i3c_beken_set_ps_rx_data_fifo_mode(0);
	i3c_beken_set_ps_periph_rst_ret_time(0);
	i3c_beken_set_ps_chip_rst_ret_time(0);
	i3c_beken_set_ps_xtime_freq_byte(0);
	i3c_beken_set_ps_xtime_inacc_byte(0);
	i3c_beken_set_tgt_tcam0_t_c1_xdel(0);

	i3c_beken_set_sw_reset(1);
	I3C_DELAY_MS(6);
}

void i3c_slave_sdr_tx_prepare(void)
{
	{ uint32_t v = i3c_hal_ctrl_read(I3C_UNIT_ID); i3c_hal_ctrl_write(I3C_UNIT_ID, v | (0x3u << 24)); }
	i3c_hal_slv_ctrl_write(I3C_UNIT_ID, (uint32_t)s_slv_max_mrl);
	i3c_hal_slv_status1_write(I3C_UNIT_ID, (0x1u << 29) | (0x1u << 28));
	/* App fills addI3C_TX_FIFO next */
}

void i3c_slave_hdr_tx_prepare(void)
{
	i3c_hal_slv_ctrl_write(I3C_UNIT_ID, 0x8u);
	i3c_hal_slv_status1_write(I3C_UNIT_ID, (0x1u << 29) | (0x1u << 28));
	/* App fills addI3C_SLV_DDR_TX_FIFO with i3c_hdr_tx_word / i3c_hdr_crc5 */
}

void i3c_slave_sdr_int_en(void)
{
	i3c_hal_slv_ier_write(I3C_UNIT_ID, 0x3u);
}

void i3c_slave_hdr_int_en(void)
{
	i3c_hal_slv_ier_write(I3C_UNIT_ID, 0x3u << 2);
}

/* IBI: Slave send. Per UG Table 100 (SLV_IBI_CTRL): bit 8 = ibi_req (self-clearing trigger),
 * bit 9 = ibi_sel (0=regular IBI, 1=TCAM0), bits 24:16 = ibi_pl (payload size bytes),
 * bits 3:0 = ibi_id (identifier for GETSTATUS). No MDB field in register; MDB is first
 * payload byte and may be provided by HW from ibi_id or a payload path.
 * Bus-side IBI enable: master ENEC CCC; there is no separate "enable IBI" bit in SLV_IBI_CTRL. */
#define I3C_SLV_IBI_CTRL_REQ    (1u << 8)
#define I3C_SLV_IBI_CTRL_IBI_SEL (1u << 9)
#define I3C_SLV_IBI_CTRL_IBI_PL_SHIFT  16u
#define I3C_SLV_IBI_CTRL_IBI_ID_MASK   0xFu

void i3c_slave_ibi_request(uint8_t mdb_byte)
{
	/* UG: bits 3:0 = ibi_id (4-bit). Use low 4 bits of mdb_byte for backward compatibility. */
	uint32_t v = i3c_hal_slv_ibi_ctrl_read(I3C_UNIT_ID);
	v = (v & ~I3C_SLV_IBI_CTRL_IBI_ID_MASK) | (mdb_byte & I3C_SLV_IBI_CTRL_IBI_ID_MASK);
	i3c_hal_slv_ibi_ctrl_write(I3C_UNIT_ID, v | I3C_SLV_IBI_CTRL_REQ);
}

void i3c_slave_ibi_request_with_pl(uint8_t ibi_id, uint16_t payload_len)
{
	uint32_t v = (payload_len << I3C_SLV_IBI_CTRL_IBI_PL_SHIFT) & 0x1FF00u;
	v |= (ibi_id & I3C_SLV_IBI_CTRL_IBI_ID_MASK);
	i3c_hal_slv_ibi_ctrl_write(I3C_UNIT_ID, v | I3C_SLV_IBI_CTRL_REQ);
}

/* ---------- Standard Slave API (bk_i3c_slave_*) ---------- */
#define I3C_SLV_DEFAULT_TIMEOUT_MS  20000u

/* Forward declarations for static helpers called from bk_i3c_slave_write/read */
static bk_err_t bk_i3c_slave_sdr_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms, i3c_xfer_mode_t mode);
static bk_err_t bk_i3c_slave_hdr_write(const uint8_t *data, uint32_t payload_bytes, uint32_t timeout_ms, i3c_xfer_mode_t mode);
static bk_err_t bk_i3c_slave_sdr_read(uint8_t *buf, uint32_t buf_size, uint32_t *recv_len, uint32_t timeout_ms, i3c_xfer_mode_t mode);
static bk_err_t bk_i3c_slave_hdr_read(uint8_t *buf, uint32_t buf_size, uint32_t expect_bytes, uint32_t *recv_len, uint32_t timeout_ms, i3c_xfer_mode_t mode);

static beken_semaphore_t s_slv_sdr_sem;
static beken_semaphore_t s_slv_hdr_sem;
static volatile uint8_t s_slv_int_registered;

static void i3c_slv_isr_handler(void)
{
	uint32_t st = i3c_hal_slv_isr_read(I3C_UNIT_ID);
	if (st & 1u) {
		i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u);
		rtos_set_semaphore(&s_slv_sdr_sem);
	}
	if (st & (1u << 1)) {
		i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 1);
		rtos_set_semaphore(&s_slv_sdr_sem);
	}
	if (st & (1u << 2)) {
		i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 2);
		rtos_set_semaphore(&s_slv_hdr_sem);
	}
	if (st & (1u << 3)) {
		i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 3);
		rtos_set_semaphore(&s_slv_hdr_sem);
	}
}

static void i3c_slv_int_register(void)
{
	if (s_slv_int_registered)
		return;
	s_slv_int_registered = 1;
	rtos_init_semaphore_ex(&s_slv_sdr_sem, 1, 0);
	rtos_init_semaphore_ex(&s_slv_hdr_sem, 1, 0);
	bk_int_isr_register(INT_SRC_I3C, i3c_slv_isr_handler, NULL);
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_I3C, 1);
}

bk_err_t bk_i3c_slave_init(const i3c_slave_config_t *cfg)
{
	if (!cfg)
		return BK_FAIL;
	s_slv_max_mrl = (cfg->max_mrl != 0u) ? cfg->max_mrl : I3C_SLV_DEFAULT_MAX_BYTES;
	s_slv_mode = (cfg->mode == I3C_SLV_MODE_HDR) ? I3C_SLV_MODE_HDR : I3C_SLV_MODE_SDR;
	i3c_platform_init(cfg->platform);
	i3c_core_clk_src_apply(cfg->core_clk_src, cfg->core_hz);
	i3c_slave_ps_set(cfg->slv_device);
	return BK_OK;
}

bk_err_t bk_i3c_slave_deinit(void)
{
	i3c_slave_int_unregister();
	i3c_platform_deinit();
	return BK_OK;
}

void i3c_slave_int_unregister(void)
{
	if (!s_slv_int_registered)
		return;
	s_slv_int_registered = 0;
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_I3C, 0);
	bk_int_isr_unregister(INT_SRC_I3C);
}

bk_err_t bk_i3c_slave_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode)
{
	if (s_slv_mode == I3C_SLV_MODE_HDR)
		return bk_i3c_slave_hdr_write(data, len, timeout_ms, xfer_mode);
	return bk_i3c_slave_sdr_write(data, len, timeout_ms, xfer_mode);
}

static bk_err_t bk_i3c_slave_sdr_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms, i3c_xfer_mode_t mode)
{
	if (!data || len == 0u || len > s_slv_max_mrl)
		return BK_FAIL;
	uint32_t deadline = (timeout_ms != 0u) ? timeout_ms : I3C_SLV_DEFAULT_TIMEOUT_MS;
	i3c_slave_sdr_tx_prepare();
	if (mode == I3C_XFER_INT)
		i3c_slv_int_register();
	if (mode == I3C_XFER_INT)
		i3c_slave_sdr_int_en();

	i3c_hal_slv_icr_write(I3C_UNIT_ID, 0xFFFFFFFFu);
	const uint32_t total_words = (len + 3u) / 4u;
	uint32_t widx = 0;

	while (widx < total_words && ((i3c_hal_slv_status1_read(I3C_UNIT_ID) >> 2) & 0x1u) == 0u) {
		uint32_t base = widx * 4u;
		uint32_t w = 0u;
		for (unsigned b = 0; b < 4u && (base + b) < len; b++)
			w |= (uint32_t)data[base + b] << (8u * b);
		i3c_hal_tx_fifo_write(I3C_UNIT_ID, w);
		widx++;
		if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & (1u << 4))
			i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 4);
	}

	while (deadline > 0u) {
		uint32_t st = i3c_hal_slv_isr_read(I3C_UNIT_ID);
		uint32_t s1 = i3c_hal_slv_status1_read(I3C_UNIT_ID);
		if (st & (1u << 1)) {
			i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 1);
			break;
		}
		if (st & (1u << 4))
			i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 4);
		if (st & (1u << 8)) {
			i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 8);
			while (widx < total_words && ((s1 >> 2) & 0x1u) == 0u) {
				uint32_t base = widx * 4u;
				uint32_t w = 0u;
				for (unsigned b = 0; b < 4u && (base + b) < len; b++)
					w |= (uint32_t)data[base + b] << (8u * b);
				i3c_hal_tx_fifo_write(I3C_UNIT_ID, w);
				widx++;
				s1 = i3c_hal_slv_status1_read(I3C_UNIT_ID);
				if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & (1u << 4)) {
					i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 4);
					break;
				}
			}
		}
		if (widx < total_words && ((s1 >> 2) & 0x1u) == 0u) {
			uint32_t base = widx * 4u;
			uint32_t w = 0u;
			for (unsigned b = 0; b < 4u && (base + b) < len; b++)
				w |= (uint32_t)data[base + b] << (8u * b);
			i3c_hal_tx_fifo_write(I3C_UNIT_ID, w);
			widx++;
			if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & (1u << 4))
				i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 4);
		}
		if (mode == I3C_XFER_INT) {
			if (rtos_get_semaphore(&s_slv_sdr_sem, 1u) == kNoErr)
				break;
		} else {
			if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & (1u << 1)) {
				i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 1);
				break;
			}
		}
		deadline--;
	}
	return (deadline > 0u) ? BK_OK : BK_FAIL;
}

bk_err_t bk_i3c_slave_read(uint8_t *buf, uint32_t buf_size, uint32_t *recv_len, uint32_t expect_bytes,
                            uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode)
{
	if (s_slv_mode == I3C_SLV_MODE_HDR)
		return bk_i3c_slave_hdr_read(buf, buf_size, expect_bytes, recv_len, timeout_ms, xfer_mode);
	return bk_i3c_slave_sdr_read(buf, buf_size, recv_len, timeout_ms, xfer_mode);
}

static bk_err_t bk_i3c_slave_sdr_read(uint8_t *buf, uint32_t buf_size, uint32_t *recv_len, uint32_t timeout_ms, i3c_xfer_mode_t mode)
{
	if (!buf || !recv_len)
		return BK_FAIL;
	uint32_t deadline = (timeout_ms != 0u) ? timeout_ms : I3C_SLV_DEFAULT_TIMEOUT_MS;
	{ uint32_t v = i3c_hal_ctrl_read(I3C_UNIT_ID); i3c_hal_ctrl_write(I3C_UNIT_ID, v | (0x3u << 24)); }
	i3c_hal_slv_ctrl_write(I3C_UNIT_ID, 0x0u);
	i3c_hal_slv_icr_write(I3C_UNIT_ID, 0xFFFFFFFFu);

	if (mode == I3C_XFER_INT) {
		i3c_slv_int_register();
		i3c_slave_sdr_int_en();
	}
	while (deadline > 0u) {
		if (mode == I3C_XFER_INT) {
			if (rtos_get_semaphore(&s_slv_sdr_sem, 1u) == kNoErr)
				break;
		} else {
			if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & 1u) {
				i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u);
				break;
			}
		}
		deadline--;
	}
	if (deadline == 0u)
		return BK_FAIL;

	uint32_t st0 = i3c_hal_slv_status0_read(I3C_UNIT_ID);
	unsigned xfer = (unsigned)(st0 & 0xFFFFu);
	if (xfer > buf_size)
		xfer = buf_size;

	unsigned got = 0;
	for (unsigned w = 0; w < (xfer + 3u) / 4u; w++) {
		uint32_t v = i3c_hal_rx_fifo_read(I3C_UNIT_ID);
		for (unsigned b = 0; b < 4u && got < xfer; b++)
			buf[got++] = (uint8_t)((v >> (8u * b)) & 0xFFu);
	}
	*recv_len = got;
	return BK_OK;
}

static bk_err_t bk_i3c_slave_hdr_write(const uint8_t *data, uint32_t payload_bytes, uint32_t timeout_ms, i3c_xfer_mode_t mode)
{
	if (!data || (payload_bytes & 1u) != 0u)
		return BK_FAIL;
	uint32_t chunk_deadline = (timeout_ms != 0u) ? timeout_ms : I3C_SLV_DEFAULT_TIMEOUT_MS;
	i3c_slave_hdr_tx_prepare();
	i3c_hal_slv_ddr_thr_write(I3C_UNIT_ID, 0x00020001u);
	i3c_hal_slv_icr_write(I3C_UNIT_ID, 0xFFFFFFFFu);

	if (mode == I3C_XFER_INT)
		i3c_slv_int_register();
	if (mode == I3C_XFER_INT)
		i3c_slave_hdr_int_en();

	unsigned num_chunks = (payload_bytes + I3C_SLV_HDR_MAX_BYTES_PER_CHUNK - 1u) / I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
	i3c_hal_slv_ctrl_write(I3C_UNIT_ID, (uint32_t)payload_bytes);

	for (unsigned chunk = 0; chunk < num_chunks; chunk++) {
		unsigned byte_off = chunk * I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
		unsigned n = payload_bytes - byte_off;
		if (n > I3C_SLV_HDR_MAX_BYTES_PER_CHUNK)
			n = I3C_SLV_HDR_MAX_BYTES_PER_CHUNK;
		unsigned data_words = n / 2u;
		uint8_t crc = 0x1Fu;
		for (unsigned w = 0; w < data_words; w++) {
			uint8_t b0 = data[byte_off + w * 2u];
			uint8_t b1 = data[byte_off + w * 2u + 1u];
			i3c_hal_slv_ddr_tx_write(I3C_UNIT_ID, i3c_hdr_tx_word((w == 0u) ? 0x2u : 0x3u, b0, b1));
			crc = i3c_hdr_crc5(crc, (uint16_t)((b0 << 8) | b1));
		}
		i3c_hal_slv_ddr_tx_write(I3C_UNIT_ID, (1u << 18) | (0xcu << 14) | ((uint32_t)crc << 9));

		uint32_t deadline = chunk_deadline;
		while (deadline > 0u) {
			if (mode == I3C_XFER_INT) {
				if (rtos_get_semaphore(&s_slv_hdr_sem, 1u) == kNoErr)
					break;
			} else {
				if (i3c_hal_slv_isr_read(I3C_UNIT_ID) & (1u << 3)) {
					i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 3);
					break;
				}
			}
			deadline--;
		}
		if (deadline == 0u)
			return BK_FAIL;
	}
	return BK_OK;
}

static bk_err_t bk_i3c_slave_hdr_read(uint8_t *buf, uint32_t buf_size, uint32_t expect_bytes, uint32_t *recv_len, uint32_t timeout_ms, i3c_xfer_mode_t mode)
{
	if (!buf || !recv_len || (expect_bytes & 1u) != 0u)
		return BK_FAIL;
	uint32_t deadline = (timeout_ms != 0u) ? timeout_ms : (3u * I3C_SLV_DEFAULT_TIMEOUT_MS);  /* HDR needs longer */
	i3c_slave_hdr_tx_prepare();
	i3c_hal_slv_ddr_thr_write(I3C_UNIT_ID, 0x00020001u);
	i3c_hal_slv_icr_write(I3C_UNIT_ID, 0xFFFFFFFFu);

	unsigned expect_words;
	if (expect_bytes > 60u) {
		expect_words = 0;
		unsigned data_off = 0;
		while (data_off < expect_bytes / 2u) {
			unsigned n = expect_bytes / 2u - data_off;
			if (n > 30u) n = 30u;
			expect_words += 1u + n + 1u;
			data_off += n;
		}
	} else {
		expect_words = 1u + expect_bytes / 2u + 1u;
	}
	if (expect_words > 80u) expect_words = 80u;

	uint32_t words[80];
	unsigned word_cnt = 0;
	int got_comp = 0;

	while (deadline-- > 0u && word_cnt < expect_words) {
		uint32_t isr = i3c_hal_slv_isr_read(I3C_UNIT_ID);
		for (;;) {
			uint32_t s1 = i3c_hal_slv_status1_read(I3C_UNIT_ID);
			if ((s1 & (1u << 5)) != 0u)
				break;
			if (word_cnt >= expect_words)
				break;
			words[word_cnt++] = i3c_hal_slv_ddr_rx_read(I3C_UNIT_ID);
		}
		if (isr & (1u << 2)) {
			got_comp = 1;
			i3c_hal_slv_icr_write(I3C_UNIT_ID, 1u << 2);
		}
		if (word_cnt >= expect_words)
			break;
		if (mode == I3C_XFER_INT)
			(void)rtos_get_semaphore(&s_slv_hdr_sem, 1u);
		else
			I3C_DELAY_MS(1);
	}

	if (got_comp && word_cnt < expect_words) {
		for (uint32_t extra = 0u; extra < 300u && word_cnt < expect_words; extra++) {
			while (word_cnt < expect_words && ((i3c_hal_slv_status1_read(I3C_UNIT_ID) & (1u << 5)) == 0u))
				words[word_cnt++] = i3c_hal_slv_ddr_rx_read(I3C_UNIT_ID);
			I3C_DELAY_MS(1);
		}
	}

	unsigned got = 0;
	for (unsigned i = 0; i < word_cnt && got < buf_size; i++) {
		uint32_t w = words[i];
		if (((w >> 18) & 3u) == 2u || ((w >> 18) & 3u) == 3u) {
			uint16_t d = (uint16_t)((w >> 2) & 0xFFFFu);
			if (got < buf_size) buf[got++] = (uint8_t)((d >> 8) & 0xFFu);
			if (got < buf_size) buf[got++] = (uint8_t)(d & 0xFFu);
		}
	}
	*recv_len = got;
	return BK_OK;
}

