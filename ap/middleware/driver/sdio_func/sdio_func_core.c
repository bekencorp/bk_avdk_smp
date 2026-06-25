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

/*
 * SDIO peripheral (I/O card) core.
 *
 * Depends ONLY on the generic SDIO host controller interface
 * (<driver/sdio_host.h>): CMD5/CMD52/CMD53 via bk_sdio_host_send_cmd() /
 * bk_sdio_host_xfer(), async interrupt via bk_sdio_host_register_sdio_irq().
 */

#include <os/os.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/sdio_host.h>
#include <driver/sdio_func.h>

#define SF_TAG "sdio_func"
#define SF_LOGI(...) BK_LOGI(SF_TAG, ##__VA_ARGS__)
#define SF_LOGW(...) BK_LOGW(SF_TAG, ##__VA_ARGS__)
#define SF_LOGE(...) BK_LOGE(SF_TAG, ##__VA_ARGS__)
#define SF_LOGD(...) BK_LOGD(SF_TAG, ##__VA_ARGS__)

#define SDIO_MAX_FUNCS               8

/* Commands */
#define SDIO_CMD_GO_IDLE_STATE       0   /* CMD0, no response */
#define SDIO_CMD_SEND_RELATIVE_ADDR  3   /* R6 */
#define SDIO_CMD_IO_SEND_OP_COND     5   /* R4 */
#define SDIO_CMD_SELECT_CARD         7   /* R1b */
#define SDIO_CMD_SEND_IF_COND        8   /* R7 (CMD8, SD 2.0 voltage/check) */
#define SDIO_CMD_IO_RW_DIRECT        52  /* R5 (CMD52) */
#define SDIO_CMD_IO_RW_EXTENDED      53  /* R5 (CMD53) */

/* CMD8 argument: VHS=1 (2.7~3.6V) + check pattern 0xAA -> 0x1AA */
#define SDIO_CMD8_VHS_27_36V         0x100u
#define SDIO_CMD8_CHECK_PATTERN      0xAAu
#define SDIO_CMD8_ARG                (SDIO_CMD8_VHS_27_36V | SDIO_CMD8_CHECK_PATTERN)

/* CCCR (function 0) register addresses */
#define SDIO_CCCR_IOEx               0x02 /* I/O enable */
#define SDIO_CCCR_IORx               0x03 /* I/O ready */
#define SDIO_CCCR_IENx               0x04 /* int enable */
#define SDIO_CCCR_INTx               0x05 /* int pending */
#define SDIO_CCCR_IO_ABORT           0x06 /* I/O abort */
#define SDIO_CCCR_IO_ABORT_RES       0x08 /* I/O abort bit3: RES = card soft reset */
#define SDIO_CCCR_CIS_PTR            0x09 /* 0x09..0x0B common CIS pointer */

/* Function Basic Register block */
#define SDIO_FBR_BASE(f)             ((uint32_t)(f) * 0x100u)
#define SDIO_FBR_BLKSIZE             0x10 /* 0x10..0x11 within FBR */

/* CIS tuple codes */
#define CISTPL_END                   0xFF
#define CISTPL_MANFID                0x20

/* CMD5 R4 response fields */
#define R4_READY(r)                  (((r) >> 31) & 0x1)
#define R4_NUM_FUNCS(r)              (((r) >> 28) & 0x7)
#define R4_OCR(r)                    ((r) & 0x00FFFFFFu)

/* CMD52/CMD53 R5 response: [15:8]=flags, [7:0]=data */
#define R5_DATA(r)                   ((r) & 0xFF)
#define R5_FLAGS(r)                  (((r) >> 8) & 0xFF)
#define R5_ERR_MASK                  0xCBu /* CRC|ILLEGAL|ERROR|FUNC_NUM|OUT_OF_RANGE */

typedef struct {
	uint16_t                blksz;
	bk_sdio_irq_handler_t   irq_handler;
	void                   *irq_arg;
} sdio_func_dev_t;

static bool             s_sf_inited = false;
static sdio_host_id_t   s_host_id = SDIO_HOST_ID_0;
static uint16_t         s_rca = 0;
static uint8_t          s_num_funcs = 0;
static sdio_func_dev_t  s_func[SDIO_MAX_FUNCS];
static sdio_func_cis_t  s_cis;
static beken_mutex_t    s_host_mutex = NULL;

/* Async interrupt worker: the host ISR cannot issue CMD52 (it would take the
 * host lock in interrupt context), so it just posts a semaphore and a worker
 * thread reads CCCR INTx and dispatches the per-function handlers. */
static beken_semaphore_t s_irq_sema = NULL;
static beken_thread_t    s_irq_thread = NULL;
static volatile bool     s_irq_thread_run = false;

/* ---------------- CMD52 / CMD53 primitives ---------------- */

static bk_err_t sdio_io_rw_direct(bool write, uint8_t func, uint32_t addr,
				  uint8_t in, uint8_t *out)
{
	sdio_host_cmd_t cmd;
	sdio_host_resp_t resp = {0};
	uint32_t arg;
	bk_err_t ret;

	arg = ((write ? 1u : 0u) << 31) |
	      (((uint32_t)func & 0x7u) << 28) |
	      ((write && out) ? (1u << 27) : 0u) |   /* RAW: read-after-write */
	      ((addr & 0x1FFFFu) << 9) |
	      (write ? in : 0u);

	cmd.index = SDIO_CMD_IO_RW_DIRECT;
	cmd.arg = arg;
	cmd.resp_type = SDIO_HOST_RESP_R5;

	ret = bk_sdio_host_send_cmd(s_host_id, &cmd, &resp);
	if (ret != BK_OK)
		return ret;
	if (R5_FLAGS(resp.resp[0]) & R5_ERR_MASK) {
		SF_LOGW("CMD52 %s f%d a0x%x flags=0x%x\r\n",
			write ? "W" : "R", func, (unsigned int)addr,
			(unsigned int)R5_FLAGS(resp.resp[0]));
		return BK_FAIL;
	}
	if (out)
		*out = (uint8_t)R5_DATA(resp.resp[0]);
	return BK_OK;
}

static bk_err_t sdio_io_rw_extended(bool write, uint8_t func, uint32_t addr,
				    uint8_t *buf, uint32_t len, bool incr)
{
	sdio_host_cmd_t cmd;
	sdio_host_data_t xfer;
	sdio_host_resp_t resp = {0};
	uint16_t blksz = s_func[func].blksz;
	bool block_mode;
	uint32_t count;
	bk_err_t ret;

	if (func >= SDIO_MAX_FUNCS || buf == NULL || len == 0)
		return BK_ERR_PARAM;

	block_mode = (blksz != 0) && (len >= blksz) && ((len % blksz) == 0);
	if (block_mode)
		count = len / blksz;            /* block count */
	else
		count = (len == 512) ? 0 : len; /* byte count, 0 == 512 */

	cmd.index = SDIO_CMD_IO_RW_EXTENDED;
	cmd.arg = ((write ? 1u : 0u) << 31) |
		  (((uint32_t)func & 0x7u) << 28) |
		  ((block_mode ? 1u : 0u) << 27) |
		  ((incr ? 1u : 0u) << 26) |
		  ((addr & 0x1FFFFu) << 9) |
		  (count & 0x1FFu);
	cmd.resp_type = SDIO_HOST_RESP_R5;

	xfer.dir = write ? SDIO_HOST_XFER_WRITE : SDIO_HOST_XFER_READ;
	xfer.mode = SDIO_HOST_XFER_PIO;
	xfer.buf = buf;
	xfer.byte_mode = !block_mode;
	if (block_mode) {
		xfer.block_size = blksz;
		xfer.block_cnt = count;
	} else {
		xfer.block_size = len;
		xfer.block_cnt = 1;
	}

	ret = bk_sdio_host_xfer(s_host_id, &cmd, &xfer, &resp);
	if (ret != BK_OK)
		return ret;
	if (R5_FLAGS(resp.resp[0]) & R5_ERR_MASK)
		return BK_FAIL;
	return BK_OK;
}

/* ---------------- CIS parsing ---------------- */

static void sdio_func_parse_cis(void)
{
	uint8_t b0 = 0, b1 = 0, b2 = 0;
	uint32_t addr;
	int guard;

	os_memset(&s_cis, 0, sizeof(s_cis));

	if (sdio_io_rw_direct(false, 0, SDIO_CCCR_CIS_PTR, 0, &b0) != BK_OK ||
	    sdio_io_rw_direct(false, 0, SDIO_CCCR_CIS_PTR + 1, 0, &b1) != BK_OK ||
	    sdio_io_rw_direct(false, 0, SDIO_CCCR_CIS_PTR + 2, 0, &b2) != BK_OK) {
		SF_LOGW("read CIS pointer failed\r\n");
		return;
	}
	addr = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16);

	for (guard = 0; guard < 256; guard++) {
		uint8_t code = 0, link = 0;
		if (sdio_io_rw_direct(false, 0, addr, 0, &code) != BK_OK)
			break;
		if (code == CISTPL_END)
			break;
		if (sdio_io_rw_direct(false, 0, addr + 1, 0, &link) != BK_OK)
			break;

		if (code == CISTPL_MANFID && link >= 4) {
			uint8_t m0 = 0, m1 = 0, c0 = 0, c1 = 0;
			sdio_io_rw_direct(false, 0, addr + 2, 0, &m0);
			sdio_io_rw_direct(false, 0, addr + 3, 0, &m1);
			sdio_io_rw_direct(false, 0, addr + 4, 0, &c0);
			sdio_io_rw_direct(false, 0, addr + 5, 0, &c1);
			s_cis.manf_id = (uint16_t)(m0 | (m1 << 8));
			s_cis.card_id = (uint16_t)(c0 | (c1 << 8));
			SF_LOGI("CIS manf=0x%04x card=0x%04x\r\n", s_cis.manf_id, s_cis.card_id);
		}
		addr += 2u + link;
	}
}

/* ---------------- async interrupt worker ---------------- */

static void sdio_func_host_isr(void *arg)
{
	(void)arg;
	if (s_irq_sema)
		rtos_set_semaphore(&s_irq_sema);
}

static void sdio_func_irq_worker(void *arg)
{
	(void)arg;
	while (s_irq_thread_run) {
		uint8_t pend = 0;
		uint8_t f;
		if (rtos_get_semaphore(&s_irq_sema, BEKEN_WAIT_FOREVER) != kNoErr)
			continue;
		if (!s_irq_thread_run)
			break;
		if (sdio_io_rw_direct(false, 0, SDIO_CCCR_INTx, 0, &pend) != BK_OK)
			continue;
		for (f = 1; f < SDIO_MAX_FUNCS; f++) {
			if ((pend & (1u << f)) && s_func[f].irq_handler)
				s_func[f].irq_handler(f, s_func[f].irq_arg);
		}
		/* Re-arm the host async interrupt for the next event. */
		bk_sdio_host_enable_sdio_irq(s_host_id, true);
	}
	rtos_deinit_semaphore(&s_irq_sema);
	s_irq_sema = NULL;
	s_irq_thread = NULL;
	rtos_delete_thread(NULL);
}

/* ---------------- public API ---------------- */

bk_err_t bk_sdio_func_init(sdio_host_id_t host_id)
{
	sdio_host_cfg_t cfg = {
		.is_emmc = false,
		.init_clock_hz = 0,
		.bus_width = SDIO_HOST_BUS_WIDTH_1,
	};
	sdio_host_resp_t resp = {0};
	uint32_t ocr;
	uint32_t retry = 0;
	bk_err_t ret;
	sdio_host_cmd_t cmd;

	if (host_id >= SDIO_HOST_ID_MAX) {
#if defined(CONFIG_SDIO_FUNC_HOST_ID)
		host_id = (sdio_host_id_t)CONFIG_SDIO_FUNC_HOST_ID;
#else
		host_id = SDIO_HOST_ID_0;
#endif
	}
	s_host_id = host_id;

	if (s_sf_inited)
		return BK_OK;

	os_memset(s_func, 0, sizeof(s_func));

	if (s_host_mutex == NULL)
		rtos_init_recursive_mutex(&s_host_mutex);

	ret = bk_sdio_host_init(host_id, &cfg);
	if (ret != BK_OK)
		return ret;

	/* Reset the card before identification (mirrors the bring-up sequence of
	 * common SDIO hosts; observed on the bus as CMD52 -> CMD52 -> CMD0 -> CMD5):
	 *   1) Read-modify-write the CCCR I/O Abort register to set the RES bit
	 *      (CMD52 read 0x06 then CMD52 write 0x06): soft-reset the SDIO card so
	 *      it drops any state left over from a warm host reset, while preserving
	 *      the other (ASx) bits as reference hosts do.
	 *   2) CMD0 GO_IDLE_STATE: return the card to the idle state.
	 * All are best-effort during bring-up: a card that is absent or not yet
	 * enumerated may NAK/timeout here, which is harmless, so the return codes
	 * are intentionally ignored and we still proceed to CMD5. */
	{
		uint8_t abort = 0;
		(void)sdio_io_rw_direct(false, 0, SDIO_CCCR_IO_ABORT, 0, &abort);
		abort |= SDIO_CCCR_IO_ABORT_RES;
		(void)sdio_io_rw_direct(true, 0, SDIO_CCCR_IO_ABORT, abort, NULL);
	}

	cmd.index = SDIO_CMD_GO_IDLE_STATE;
	cmd.arg = 0;
	cmd.resp_type = SDIO_HOST_RESP_NONE;
	(void)bk_sdio_host_send_cmd(host_id, &cmd, &resp);
	rtos_delay_milliseconds(2);

	/* CMD8 SEND_IF_COND: announce that the host is SD 2.0+ and exchange the
	 * voltage window / check pattern (the step Linux/Raspberry Pi run between
	 * CMD0 and CMD5). Best-effort: legacy SDIO-only cards do not respond, which
	 * the spec lets us ignore, so we proceed to CMD5 regardless. When the card
	 * does answer, the low byte must echo the 0xAA check pattern. */
	cmd.index = SDIO_CMD_SEND_IF_COND;
	cmd.arg = SDIO_CMD8_ARG;
	cmd.resp_type = SDIO_HOST_RESP_R7;
	if (bk_sdio_host_send_cmd(host_id, &cmd, &resp) == BK_OK &&
	    (resp.resp[0] & 0xFFu) != SDIO_CMD8_CHECK_PATTERN) {
		SF_LOGW("CMD8 echo mismatch: 0x%x\r\n", (unsigned int)resp.resp[0]);
	}

	/* CMD5 with arg 0: query OCR and number of I/O functions */
	cmd.index = SDIO_CMD_IO_SEND_OP_COND;
	cmd.arg = 0;
	cmd.resp_type = SDIO_HOST_RESP_R4;
	if (bk_sdio_host_send_cmd(host_id, &cmd, &resp) != BK_OK) {
		SF_LOGE("CMD5(probe) no response, no SDIO card\r\n");
		return BK_FAIL;
	}
	ocr = R4_OCR(resp.resp[0]);
	s_num_funcs = R4_NUM_FUNCS(resp.resp[0]);

	/* CMD5 with the voltage window: busy-wait until card ready */
	do {
		cmd.arg = ocr;
		if (bk_sdio_host_send_cmd(host_id, &cmd, &resp) != BK_OK)
			return BK_FAIL;
		rtos_delay_milliseconds(2);
		if (retry++ > 500) {
			SF_LOGE("CMD5 busy-wait timeout\r\n");
			return BK_FAIL;
		}
	} while (!R4_READY(resp.resp[0]));

	s_num_funcs = R4_NUM_FUNCS(resp.resp[0]);

	/* CMD3: ask the card to publish its RCA (R6) */
	cmd.index = SDIO_CMD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	cmd.resp_type = SDIO_HOST_RESP_R6;
	if (bk_sdio_host_send_cmd(host_id, &cmd, &resp) != BK_OK)
		return BK_FAIL;
	s_rca = (uint16_t)(resp.resp[0] >> 16);

	/* CMD7: select the card */
	cmd.index = SDIO_CMD_SELECT_CARD;
	cmd.arg = (uint32_t)s_rca << 16;
	cmd.resp_type = SDIO_HOST_RESP_R1B;
	if (bk_sdio_host_send_cmd(host_id, &cmd, &resp) != BK_OK)
		return BK_FAIL;

	sdio_func_parse_cis();

	s_sf_inited = true;
	SF_LOGI("sdio func init done: rca=0x%04x, %d io functions\r\n", s_rca, s_num_funcs);
	return BK_OK;
}

bk_err_t bk_sdio_func_deinit(void)
{
	s_irq_thread_run = false;
	if (s_irq_sema)
		rtos_set_semaphore(&s_irq_sema); /* wake worker to exit */
	s_sf_inited = false;
	return bk_sdio_host_deinit(s_host_id);
}

uint8_t bk_sdio_func_count(void)
{
	return s_num_funcs;
}

bk_err_t bk_sdio_func_get_cis(sdio_func_cis_t *cis)
{
	if (cis == NULL)
		return BK_ERR_PARAM;
	*cis = s_cis;
	return BK_OK;
}

/* ---------------- bus claim (Linux sdio_claim_host/release_host) -------- */

void bk_sdio_claim_host(uint8_t func)
{
	(void)func;
	if (s_host_mutex)
		rtos_lock_recursive_mutex(&s_host_mutex);
}

void bk_sdio_release_host(uint8_t func)
{
	(void)func;
	if (s_host_mutex)
		rtos_unlock_recursive_mutex(&s_host_mutex);
}

/* ---------------- function enable / block size ------------------------- */

bk_err_t bk_sdio_enable_func(uint8_t func)
{
	uint8_t ioe = 0, ior = 0;
	int i;

	if (func == 0 || func >= SDIO_MAX_FUNCS)
		return BK_ERR_PARAM;

	if (sdio_io_rw_direct(false, 0, SDIO_CCCR_IOEx, 0, &ioe) != BK_OK)
		return BK_FAIL;
	ioe |= (uint8_t)(1u << func);
	if (sdio_io_rw_direct(true, 0, SDIO_CCCR_IOEx, ioe, NULL) != BK_OK)
		return BK_FAIL;

	for (i = 0; i < 500; i++) {
		if (sdio_io_rw_direct(false, 0, SDIO_CCCR_IORx, 0, &ior) == BK_OK &&
		    (ior & (1u << func)))
			return BK_OK;
		rtos_delay_milliseconds(1);
	}
	SF_LOGW("func %d not ready after enable\r\n", func);
	return BK_FAIL;
}

bk_err_t bk_sdio_disable_func(uint8_t func)
{
	uint8_t ioe = 0;

	if (func == 0 || func >= SDIO_MAX_FUNCS)
		return BK_ERR_PARAM;
	if (sdio_io_rw_direct(false, 0, SDIO_CCCR_IOEx, 0, &ioe) != BK_OK)
		return BK_FAIL;
	ioe &= (uint8_t)~(1u << func);
	return sdio_io_rw_direct(true, 0, SDIO_CCCR_IOEx, ioe, NULL);
}

bk_err_t bk_sdio_set_block_size(uint8_t func, uint16_t blksz)
{
	uint32_t fbr = SDIO_FBR_BASE(func) + SDIO_FBR_BLKSIZE;

	if (func >= SDIO_MAX_FUNCS)
		return BK_ERR_PARAM;
	if (sdio_io_rw_direct(true, 0, fbr, (uint8_t)(blksz & 0xFF), NULL) != BK_OK)
		return BK_FAIL;
	if (sdio_io_rw_direct(true, 0, fbr + 1, (uint8_t)((blksz >> 8) & 0xFF), NULL) != BK_OK)
		return BK_FAIL;
	s_func[func].blksz = blksz;
	return BK_OK;
}

uint32_t bk_sdio_align_size(uint8_t func, uint32_t sz)
{
	uint16_t blksz = (func < SDIO_MAX_FUNCS) ? s_func[func].blksz : 0;

	if (blksz == 0)
		return sz;
	return ((sz + blksz - 1u) / blksz) * blksz;
}

/* ---------------- CMD52 byte / word / long IO -------------------------- */

uint8_t bk_sdio_readb(uint8_t func, uint32_t addr, int *err_ret)
{
	uint8_t val = 0;
	bk_err_t ret;

	if (func >= SDIO_MAX_FUNCS)
		ret = BK_ERR_PARAM;
	else
		ret = sdio_io_rw_direct(false, func, addr, 0, &val);
	if (err_ret)
		*err_ret = ret;
	return (ret == BK_OK) ? val : 0;
}

void bk_sdio_writeb(uint8_t func, uint8_t b, uint32_t addr, int *err_ret)
{
	bk_err_t ret;

	if (func >= SDIO_MAX_FUNCS)
		ret = BK_ERR_PARAM;
	else
		ret = sdio_io_rw_direct(true, func, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}

uint8_t bk_sdio_writeb_readb(uint8_t func, uint8_t write_byte, uint32_t addr, int *err_ret)
{
	uint8_t val = 0;
	bk_err_t ret;

	if (func >= SDIO_MAX_FUNCS)
		ret = BK_ERR_PARAM;
	else
		ret = sdio_io_rw_direct(true, func, addr, write_byte, &val); /* RAW */
	if (err_ret)
		*err_ret = ret;
	return (ret == BK_OK) ? val : 0;
}

uint16_t bk_sdio_readw(uint8_t func, uint32_t addr, int *err_ret)
{
	uint8_t buf[2] = {0};
	bk_err_t ret = sdio_io_rw_extended(false, func, addr, buf, sizeof(buf), true);

	if (err_ret)
		*err_ret = ret;
	if (ret != BK_OK)
		return 0;
	return (uint16_t)(buf[0] | (buf[1] << 8));
}

void bk_sdio_writew(uint8_t func, uint16_t b, uint32_t addr, int *err_ret)
{
	uint8_t buf[2];
	bk_err_t ret;

	buf[0] = (uint8_t)(b & 0xFF);
	buf[1] = (uint8_t)((b >> 8) & 0xFF);
	ret = sdio_io_rw_extended(true, func, addr, buf, sizeof(buf), true);
	if (err_ret)
		*err_ret = ret;
}

uint32_t bk_sdio_readl(uint8_t func, uint32_t addr, int *err_ret)
{
	uint8_t buf[4] = {0};
	bk_err_t ret = sdio_io_rw_extended(false, func, addr, buf, sizeof(buf), true);

	if (err_ret)
		*err_ret = ret;
	if (ret != BK_OK)
		return 0;
	return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
	       ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
}

void bk_sdio_writel(uint8_t func, uint32_t b, uint32_t addr, int *err_ret)
{
	uint8_t buf[4];
	bk_err_t ret;

	buf[0] = (uint8_t)(b & 0xFF);
	buf[1] = (uint8_t)((b >> 8) & 0xFF);
	buf[2] = (uint8_t)((b >> 16) & 0xFF);
	buf[3] = (uint8_t)((b >> 24) & 0xFF);
	ret = sdio_io_rw_extended(true, func, addr, buf, sizeof(buf), true);
	if (err_ret)
		*err_ret = ret;
}

/* ---------------- function-0 (CCCR/FBR) byte IO ------------------------ */

uint8_t bk_sdio_f0_readb(uint8_t func, uint32_t addr, int *err_ret)
{
	uint8_t val = 0;
	bk_err_t ret = sdio_io_rw_direct(false, 0, addr, 0, &val);

	(void)func;
	if (err_ret)
		*err_ret = ret;
	return (ret == BK_OK) ? val : 0;
}

void bk_sdio_f0_writeb(uint8_t func, uint8_t b, uint32_t addr, int *err_ret)
{
	bk_err_t ret = sdio_io_rw_direct(true, 0, addr, b, NULL);

	(void)func;
	if (err_ret)
		*err_ret = ret;
}

/* ---------------- CMD53 buffer IO -------------------------------------- */

bk_err_t bk_sdio_memcpy_fromio(uint8_t func, void *dst, uint32_t addr, uint32_t count)
{
	return sdio_io_rw_extended(false, func, addr, (uint8_t *)dst, count, true);
}

bk_err_t bk_sdio_memcpy_toio(uint8_t func, uint32_t addr, const void *src, uint32_t count)
{
	return sdio_io_rw_extended(true, func, addr, (uint8_t *)src, count, true);
}

bk_err_t bk_sdio_readsb(uint8_t func, void *dst, uint32_t addr, uint32_t count)
{
	return sdio_io_rw_extended(false, func, addr, (uint8_t *)dst, count, false);
}

bk_err_t bk_sdio_writesb(uint8_t func, uint32_t addr, const void *src, uint32_t count)
{
	return sdio_io_rw_extended(true, func, addr, (uint8_t *)src, count, false);
}

/* ---------------- async interrupt -------------------------------------- */

bk_err_t bk_sdio_claim_irq(uint8_t func, bk_sdio_irq_handler_t handler, void *arg)
{
	uint8_t ien = 0;
	bk_err_t ret;

	if (func == 0 || func >= SDIO_MAX_FUNCS || handler == NULL)
		return BK_ERR_PARAM;

	s_func[func].irq_handler = handler;
	s_func[func].irq_arg = arg;

	/* Lazily create the worker thread + signaling semaphore. */
	if (s_irq_sema == NULL) {
		ret = rtos_init_semaphore(&s_irq_sema, 1);
		if (ret != kNoErr)
			return ret;
	}
	if (s_irq_thread == NULL) {
		s_irq_thread_run = true;
		ret = rtos_create_thread(&s_irq_thread, BEKEN_DEFAULT_WORKER_PRIORITY,
					 "sdio_func_irq",
					 (beken_thread_function_t)sdio_func_irq_worker,
					 2048, NULL);
		if (ret != kNoErr) {
			SF_LOGE("create irq worker failed\r\n");
			return ret;
		}
	}

	/* Enable the function's interrupt in CCCR and the master IEN (bit0),
	 * then arm the host async interrupt line. */
	sdio_io_rw_direct(false, 0, SDIO_CCCR_IENx, 0, &ien);
	ien |= (uint8_t)((1u << func) | 0x01u);
	sdio_io_rw_direct(true, 0, SDIO_CCCR_IENx, ien, NULL);

	bk_sdio_host_register_sdio_irq(s_host_id, sdio_func_host_isr, NULL);
	bk_sdio_host_enable_sdio_irq(s_host_id, true);
	return BK_OK;
}

bk_err_t bk_sdio_release_irq(uint8_t func)
{
	uint8_t ien = 0;

	if (func == 0 || func >= SDIO_MAX_FUNCS)
		return BK_ERR_PARAM;

	s_func[func].irq_handler = NULL;
	s_func[func].irq_arg = NULL;

	if (sdio_io_rw_direct(false, 0, SDIO_CCCR_IENx, 0, &ien) == BK_OK) {
		ien &= (uint8_t)~(1u << func);
		sdio_io_rw_direct(true, 0, SDIO_CCCR_IENx, ien, NULL);
	}
	return BK_OK;
}
