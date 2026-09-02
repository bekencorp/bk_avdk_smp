// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	 http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include <driver/sdio_host.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/io_matrix.h>
#include <driver/hal/hal_gpio_types.h>
#include <hal/sys_hal.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include "cache.h"
#include "sys_driver.h"
#include "sdio_host_driver.h"

/* ===================== multi-controller instance layer =====================
 * The legacy code selected the active controller at compile time through the
 * SDIO_ACTIVE_BASE macro (defined in sdio_storage_driver.h). To make the host
 * usable on either of the two BK7259 controllers at runtime, we capture the
 * compile-time default into a runtime variable and redirect the macro to it.
 * Every existing register access (reset / init / ISR) then follows whichever
 * controller the generic bk_sdio_host_*() API has selected - with no churn to
 * the low-level register code below.
 *
 * Concurrency: transactions on the two controllers are serialized by an
 * internal mutex (s_host_lock) because the controller error-state globals and
 * the wait semaphores below are shared. Both controllers are fully usable;
 * truly concurrent in-flight transfers are a future enhancement. */
static volatile uintptr_t s_active_base = SDIO_ACTIVE_BASE;
static sdio_host_id_t      s_active_id   =
#if SDIO_VERIFY_USE_SDIO1
	SDIO_HOST_ID_1;
#else
	SDIO_HOST_ID_0;
#endif
#undef  SDIO_ACTIVE_BASE
#define SDIO_ACTIVE_BASE   (s_active_base)

void sdio_host_dispatch_card_irq(void); /* defined in the generic API section */

#define INIT_400K     1
#define MAX_WAIT_STATE_TRANS_TIMES  200

/* Keep the generic host budget unchanged; only SD CMD17 uses the shorter
 * surprise-removal budget below. */
#define SDIO_HOST_DEFAULT_TIMEOUT_MS   2000
#define SDIO_HOST_SD_CMD17_TIMEOUT_MS  500
#define SDIO_HOST_R1B_TIMEOUT_MS       2000
#define SDIO_HOST_BUF_READY_SLICE_MS   10

uint32 adma3_wr_descriptor_addr[42];
uint32 adma3_rd_descriptor_addr[42];
volatile uint8_t CMD_COMPLETE_STATE =0;
volatile uint8_t XFER_COMPLETE_STATE =0;
volatile uint8_t BGAP_EVENT_STATE =0;
volatile uint8_t DMA_INTERRUPT_STATE =0;
volatile uint8_t BUF_WR_READY_STATE =0;
volatile uint8_t BUF_RD_READY_STATE =0;
volatile uint8_t CARD_INSERTION_STATE =0;
volatile uint8_t CARD_REMOVAL_STATE =0;
volatile uint8_t CARD_INTERRUPT_STATE =0;
volatile uint8_t INT_A_STATE =0;
volatile uint8_t INT_B_STATE =0;
volatile uint8_t INT_C_STATE =0;
volatile uint8_t RE_TUNE_EVENT_STATE =0;
volatile uint8_t FX_EVENT_STATE =0;
volatile uint8_t CQE_EVENT_STATE =0;
volatile uint8_t ERROR_INTERRUPT_STATE=0;
volatile uint8_t CMD_TOUT_ERR_STATE =0;
volatile uint8_t CMD_CRC_ERR_STATE =0;
volatile uint8_t CMD_END_BIT_ERR_STATE =0;
volatile uint8_t CMD_IDX_ERR_STATE =0;
volatile uint8_t DATA_TOUT_ERR_STATE =0;
volatile uint8_t DATA_CRC_ERR_STATE =0;
volatile uint8_t DATA_END_BIT_ERR_STATE =0;
volatile uint8_t CUR_LMT_ERR_STATE =0;
volatile uint8_t AUTO_CMD_ERR_STATE =0;
volatile uint8_t ADMA_ERR_STATE =0;
volatile uint8_t TUNING_ERR_STATE =0;
volatile uint8_t RESP_ERR_STATE =0;
volatile uint8_t BOOT_ACK_ERR_STATE =0;
volatile uint8_t VENDOR_ERR1_STATE =0;
volatile uint8_t VENDOR_ERR2_STATE =0;
volatile uint8_t VENDOR_ERR3_STATE =0;

static beken_semaphore_t s_sdio_cmd_done_sema = NULL;
static beken_semaphore_t s_sdio_wr_buf_ready_sema = NULL;
static beken_semaphore_t s_sdio_rd_buf_ready_sema = NULL;
static beken_semaphore_t s_sdio_data_xfer_done_sema = NULL;

/* Set by send_cmd() when the last command timed out on the CMD line.
 * sd_card_init() polls this between commands to implement fail-fast. */
static volatile bool s_last_cmd_timeout = false;

static bk_err_t sdio_host_shared_resource_init(void)
{
	bk_err_t ret = BK_OK;

	if (s_sdio_cmd_done_sema == NULL)
	{
		ret = rtos_init_semaphore(&(s_sdio_cmd_done_sema), 1);
		if (kNoErr != ret) {
			SDIOD_LOGE("s_sdio_cmd_done_sema init fail\r\n");
			return ret;
		}
	}

	if (s_sdio_wr_buf_ready_sema == NULL)
	{
		ret = rtos_init_semaphore(&(s_sdio_wr_buf_ready_sema), 1);
		if (kNoErr != ret) {
			SDIOD_LOGE("s_sdio_wr_buf_ready_sema init fail\r\n");
			return ret;
		}
	}

	if (s_sdio_rd_buf_ready_sema == NULL)
	{
		ret = rtos_init_semaphore(&(s_sdio_rd_buf_ready_sema), 1);
		if (kNoErr != ret) {
			SDIOD_LOGE("s_sdio_rd_buf_ready_sema init fail\r\n");
			return ret;
		}
	}

	if (s_sdio_data_xfer_done_sema == NULL)
	{
		ret = rtos_init_semaphore(&(s_sdio_data_xfer_done_sema), 1);
		if (kNoErr != ret) {
			SDIOD_LOGE("s_sdio_data_xfer_done_sema init fail\r\n");
			return ret;
		}
	}

	return BK_OK;
}

static void sdio_host_drain_completion_semas(void)
{
	if (s_sdio_cmd_done_sema)
		rtos_get_semaphore(&s_sdio_cmd_done_sema, 0);
	if (s_sdio_wr_buf_ready_sema)
		rtos_get_semaphore(&s_sdio_wr_buf_ready_sema, 0);
	if (s_sdio_rd_buf_ready_sema)
		rtos_get_semaphore(&s_sdio_rd_buf_ready_sema, 0);
	if (s_sdio_data_xfer_done_sema)
		rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 0);
}

bool sdio_host_last_cmd_timeout(void)
{
	return s_last_cmd_timeout;
}

static void sd_card_clear_transfer_error_flags(void)
{
	CMD_TOUT_ERR_STATE = 0;
	CMD_CRC_ERR_STATE = 0;
	CMD_END_BIT_ERR_STATE = 0;
	CMD_IDX_ERR_STATE = 0;
	DATA_TOUT_ERR_STATE = 0;
	DATA_CRC_ERR_STATE = 0;
	DATA_END_BIT_ERR_STATE = 0;
	AUTO_CMD_ERR_STATE = 0;
	ADMA_ERR_STATE = 0;
	RESP_ERR_STATE = 0;
	ERROR_INTERRUPT_STATE = 0;
}

static bool sd_card_has_transfer_error(void)
{
	return CMD_TOUT_ERR_STATE || CMD_CRC_ERR_STATE ||
		CMD_END_BIT_ERR_STATE || CMD_IDX_ERR_STATE ||
		DATA_TOUT_ERR_STATE || DATA_CRC_ERR_STATE ||
		DATA_END_BIT_ERR_STATE || AUTO_CMD_ERR_STATE ||
		ADMA_ERR_STATE || RESP_ERR_STATE;
}

static void sd_card_log_transfer_wait_error(const char *func, const char *sema_name, int ret)
{
	SDIOD_LOGE("func %s: wait %s failed, ret=%d, err_int=%u, cmd[tout/crc/end/idx]=%u/%u/%u/%u, data[tout/crc/end]=%u/%u/%u, auto=%u, adma=%u, resp=%u\r\n",
		func, sema_name, ret, (unsigned int)ERROR_INTERRUPT_STATE,
		(unsigned int)CMD_TOUT_ERR_STATE, (unsigned int)CMD_CRC_ERR_STATE,
		(unsigned int)CMD_END_BIT_ERR_STATE, (unsigned int)CMD_IDX_ERR_STATE,
		(unsigned int)DATA_TOUT_ERR_STATE, (unsigned int)DATA_CRC_ERR_STATE,
		(unsigned int)DATA_END_BIT_ERR_STATE, (unsigned int)AUTO_CMD_ERR_STATE,
		(unsigned int)ADMA_ERR_STATE, (unsigned int)RESP_ERR_STATE);
}

static uint32_t sd_card_load_unaligned_le32(const uint8_t *data)
{
	return ((uint32_t)data[0]) |
		((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) |
		((uint32_t)data[3] << 24);
}

static void sd_card_store_unaligned_le32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
	data[2] = (uint8_t)(value >> 16);
	data[3] = (uint8_t)(value >> 24);
}

void host_ctrl_set(uintptr_t addr,uint8 SD_BUS_VOL_VDD1,uint8 TOUT_CNT,uint8 CARD_IS_EMMC,uint8 DAT_XFER_WIDTH)
{
	uint16_t vers;
	uint8 data_width;

	PWR_CTRL_R(addr)= SD_BUS_VOL_VDD1;//PWR_CTRL_R.SD_BUS_VOL_VDD1=3.3v: 0x0e
	TOUT_CTRL_R(addr)= TOUT_CNT;	  //TOUT_CTRL_R.TOUT_CNT=TMCLK x 2^13

	SNPS_EMMC_CTRL_R(addr) = (SNPS_EMMC_CTRL_R(addr) & 0xfffe) | CARD_IS_EMMC;
	vers = HOST_CNTRL_VERS_R(addr);

	switch(DAT_XFER_WIDTH)
	{
		case 1:
			data_width = 0<<1;
			break;
		case 4:
			data_width = 1<<1;
			break;
		case 8:
			data_width = 1<<5;
			break;
		default :
			data_width = 0<<1;
	}

	if(vers >=3)
	{
		SDIOD_LOGD("VERS>=3\r\n");
		CLK_CTRL_R(addr)= 0x00;//base clk
		HOST_CTRL2_R(addr)=0x1000;//HOST_CTRL2_R.HOST_VER4_ENABLE=1,HOST_CTRL2_R.ADDRESSING=0(32bit),HOST_CTRL2_R.ASYNC_INT_ENABLE=0;EXEC_TUNING=0
		HOST_CTRL1_R(addr)= data_width;

	}
	else
		CLK_CTRL_R(addr)=0x00;

}

void card_clk_supply(uintptr_t addr)
{
	CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) | SD_CLK_EN;
}

void card_clk_stop(uintptr_t addr)
{
	CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) & 0xfffffffb;
}

void sd_clk_change(uintptr_t addr,uint16 SD_FREQ_SEL)
{
	uint16 internal_clk_stable;
	card_clk_stop(addr);

	CLK_CTRL_R(addr)= CLK_CTRL_R(addr) & 0xfffffff7;//Set CLK_CTRL_R.PLL_ENABLE to 0
	CLK_CTRL_R(addr)= (CLK_CTRL_R(addr)& 0x003f) | ((SD_FREQ_SEL&0xff)<<8) | ((SD_FREQ_SEL & 0x0300)>>2);
	CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) & 0xffffffdf;
	CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) | INTERNAL_CLK_EN | PLL_ENABLE;//Set INTERNAL_CLK_EN

	internal_clk_stable = CLK_CTRL_R(addr) & 0x0002;
	//wait internal clk stable
	for(int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
		if(internal_clk_stable != 0) {
			break;
		}
		rtos_delay_milliseconds(1);
	}

	card_clk_supply(addr);
}

void sd_card_interface_set(uintptr_t addr,uint8 UHS_MODE_SEL)
{
	HOST_CTRL2_R(addr) = (HOST_CTRL2_R(addr) & 0xfff8) | UHS_MODE_SEL;
	PWR_CTRL_R(addr) = PWR_CTRL_R(addr) | SD_BUS_PWR_VDD1;
}

void emmc_card_interface_set(uintptr_t addr,uint8 UHS_MODE_SEL)
{
	HOST_CTRL2_R(addr) = (HOST_CTRL2_R(addr) & 0xfffc)| UHS_MODE_SEL;//HOST_CTRL2_R.UHS2_IF_ENABLE=0,HOST_CTRL2.R.UHS_MODE_SEL=0
	PWR_CTRL_R(addr) = PWR_CTRL_R(addr) | SD_BUS_PWR_VDD1;
}

int send_cmd(uintptr_t addr, uint8 CMD_INDEX, uint8 RESP_TYPE, uint32 ARGUMENT)
{
	uint32 pstate;
	uint32 resp01;
	bool wait_busy = (RESP_TYPE == 3);

	s_last_cmd_timeout = false;

	for(int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
		pstate = PSTATE_REG_R(addr);
		// BIT(0) means cmd ready when it is 0
		if((pstate & BIT(0)) == 0) {
			break;
		}
		rtos_delay_milliseconds(1);
	}

	//SDIOD_LOGD("send cmd[%d], pstate:0x%x\r\n", CMD_INDEX, pstate);

	/* Drain any stale CMD_COMPLETE token left in the binary semaphore by a
	 * previous command (e.g. a late/extra completion after the defensive
	 * CMD12 timeout, or a double CMD_COMPLETE+CMD_TOUT interrupt). Without
	 * this, a leftover token makes the rtos_get_semaphore() below return
	 * *before* THIS command completes, so we latch the PREVIOUS command's
	 * RESP register -> a persistent one-command response shift (observed:
	 * CMD8 reads 0, CMD8's 0x1aa lands on CMD55, ... RCA parsed from CID,
	 * then the data read stalls on BUF_RD_READY). Same self-contained
	 * pattern already used in receive_mult_data(). */
	if (rtos_get_semaphore(&s_sdio_cmd_done_sema, 0) == 0)
		SDIOD_LOGW("send_cmd: drained stale cmd_done token before CMD%u\r\n", CMD_INDEX);

	uint32_t int_level = rtos_disable_int();

	if (wait_busy) {
		/* Mask and clear an old XFER_COMPLETE before draining its software
		 * token. Masking at the controller also closes the cross-core ISR
		 * window that local interrupt disable alone cannot protect. */
		NORMAL_INT_SIGNAL_EN_R(addr) &= ~XFER_COMPLETE_SIGNAL_EN;
		NORMAL_INT_STAT_R(addr) = CLR_XFER_COMPLETE_STAT;
		__DSB();
		rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 0);
		DATA_TOUT_ERR_STATE = 0;
		DATA_CRC_ERR_STATE = 0;
		DATA_END_BIT_ERR_STATE = 0;
	}

	/* Clear stale software flags set by the previous command's ISR but
	 * never cleared (the existing code only clears CMD_TOUT_ERR_STATE
	 * on the error path of send_cmd). Without this, a CMD_TOUT_ERR
	 * that fires _after_ a CMD_COMPLETE in the same command leaves
	 * CMD_TOUT_ERR_STATE=1, and the *next* otherwise-successful
	 * send_cmd() then falsely reports timeout at the check below.
	 * Observed: CMD8 ok then CMD55 fails 2ms later with no bus traffic.
	 *
	 * We intentionally do NOT write ERROR_INT_STAT_R here - the ISR is
	 * responsible for clearing its own status bits, and writing it
	 * pre-emptively can race with the controller (it may discard a
	 * legitimate timeout pending for the new command). */
	CMD_TOUT_ERR_STATE = 0;
	CMD_CRC_ERR_STATE = 0;
	CMD_END_BIT_ERR_STATE = 0;
	CMD_IDX_ERR_STATE = 0;
	ERROR_INTERRUPT_STATE = 0;

	NORMAL_INT_STAT_EN_R(addr)   = NORMAL_INT_STAT_EN_R(addr) | CMD_COMPLETE_STAT_EN;
	NORMAL_INT_SIGNAL_EN_R(addr) = NORMAL_INT_SIGNAL_EN_R(addr) | CMD_COMPLETE_SIGNAL_EN;
	if (wait_busy) {
		/* SDHCI reports the 48-bit R1 response through CMD_COMPLETE, but
		 * reports release of the R1b DAT0 busy signal through
		 * XFER_COMPLETE. Arm both before issuing the command so a short busy
		 * interval cannot complete before the second interrupt is enabled. */
		NORMAL_INT_STAT_EN_R(addr)   |= XFER_COMPLETE_STAT_EN;
		NORMAL_INT_SIGNAL_EN_R(addr) |= XFER_COMPLETE_SIGNAL_EN;
	}
	ERROR_INT_STAT_EN_R(addr)	= ERROR_INT_STAT_EN_R(addr) | 0x80f;
	ERROR_INT_SIGNAL_EN_R(addr)  = ERROR_INT_SIGNAL_EN_R(addr) | 0x80f;

	ARGUMENT_R(addr) = ARGUMENT;
	CMD_R(addr) = (CMD_INDEX<<8) | RESP_TYPE;
	XFER_MODE_R(addr) = XFER_MODE_R(addr) & 0xfffffeff;
	rtos_enable_int(int_level);

	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, 2000);
	if(ret != 0) {
		SDIOD_LOGE("func %s: get sem s_sdio_cmd_done_sema timeout\r\n", __func__);
		s_last_cmd_timeout = true;
		return SDIO_CMD_TIMEOUT_RESP;
	}

	/* CMD line timeout: the ISR has set CMD_TOUT_ERR_STATE, cleared the
	 * status, masked the CMD_TOUT_ERR signal-enable bit and released the
	 * semaphore. We must reset the CMD line here before any next command
	 * can be issued, then re-arm the masked signal. */
	if (CMD_TOUT_ERR_STATE) {
		CMD_TOUT_ERR_STATE = 0;
		SW_RST_R(addr) |= SW_RST_CMD;
		for (int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
			if ((SW_RST_R(addr) & SW_RST_CMD) == 0) {
				break;
			}
			rtos_delay_milliseconds(1);
		}
		ERROR_INT_SIGNAL_EN_R(addr) |= CMD_TOUT_ERR_STAT_EN;
		s_last_cmd_timeout = true;
		return SDIO_CMD_TIMEOUT_RESP;
	}

	resp01 = RESP01_R(addr);
	//SDIOD_LOGD("resp01[0x%x]\r\n", resp01);

	if (wait_busy) {
		bool busy_done = false;

		/* For a command with an R1b response, CMD_COMPLETE only means the R1
		 * bits were received. The operation is not complete until the card
		 * releases DAT0. On this SDHCI-style controller that is signalled by
		 * XFER_COMPLETE and reflected by CMD_INHIBIT_DAT clearing. Poll the
		 * live state as a fallback for a missed interrupt. */
		for (int i = 0; i < SDIO_HOST_R1B_TIMEOUT_MS; i++) {
			(void)rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 1);
			if (sd_card_has_transfer_error())
				break;
			if (!(PSTATE_REG_R(addr) & CMD_INHIBIT_DAT)) {
				busy_done = true;
				break;
			}
		}

		if (!busy_done) {
			SDIOD_LOGE("CMD%u R1b busy timeout: resp=0x%08x pstate=0x%08x normal=0x%04x error=0x%04x\r\n",
				(unsigned int)CMD_INDEX, (unsigned int)resp01,
				(unsigned int)PSTATE_REG_R(addr),
				(unsigned int)NORMAL_INT_STAT_R(addr),
				(unsigned int)ERROR_INT_STAT_R(addr));
			SW_RST_R(addr) |= SW_RST_DAT | SW_RST_CMD;
			for (int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
				if ((SW_RST_R(addr) & (SW_RST_DAT | SW_RST_CMD)) == 0)
					break;
				rtos_delay_milliseconds(1);
			}
			s_last_cmd_timeout = true;
			return SDIO_CMD_TIMEOUT_RESP;
		}
	}

	return resp01;
}

void emmc_card_init(uintptr_t addr,uint8 ddr_mode)
{
	uint32_t resp[4] = {0};

	send_cmd(addr, CMD0, 0, 0); //send CMD0

	resp[0] = send_cmd(addr, CMD1, 2, 0xC0000080);  //send CMD1
	while(!((0xFFFFFFFF != resp[0]) && ((resp[0] >> 31) & 0x01)))
	{
		resp[0] = send_cmd(addr, CMD1, 2, 0xC0000080);  //send CMD1
		rtos_delay_milliseconds(10);
	}

	send_cmd(addr, CMD2, 1, 0); //send CMD2
	send_cmd(addr, CMD3, 2, 0x00010000);	//send CMD3
	send_cmd(addr, CMD9, 1, 0x00010000);	//send CMD9
	send_cmd(addr, CMD7, 2, 0x00010000);	//send CMD7
	send_cmd(addr, CMD8, 2, 0x00000000);	//send CMD8
}

/*
 * Recover the controller and card after a multi-block transfer was aborted
 * before all blocks moved. Because the data phase is set up with AUTOCMD12,
 * the controller only auto-issues CMD12 (STOP_TRANSMISSION) on *normal*
 * completion; an early bail-out (buffer-ready timeout, CRC/data error) leaves
 * both the controller's DAT/CMD lines and the card stuck in the data state.
 * The card then ignores every subsequent command - including a full
 * CMD0/CMD8 re-init - until it is physically power-cycled. Resetting the
 * DAT/CMD lines and manually issuing CMD12 brings the card back to the
 * transfer state so the disk_read/disk_write retry (or a soft re-init) can
 * succeed without a power cycle.
 */
static void sd_card_abort_data_transfer(uintptr_t addr, bool send_stop)
{
	uint32_t stop_resp;

	SW_RST_R(addr) |= SW_RST_DAT | SW_RST_CMD;
	for (int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
		if ((SW_RST_R(addr) & (SW_RST_DAT | SW_RST_CMD)) == 0)
			break;
		rtos_delay_milliseconds(1);
	}
	/* CMD12 is required only for an interrupted multi-block command
	 * (CMD18/CMD25). Sending it after CMD17 is both unnecessary and can move a
	 * recovering card/controller into another unexpected command state. */
	if (send_stop) {
		stop_resp = (uint32_t)send_cmd(addr, 12, 3, 0);
		if (stop_resp == SDIO_CMD_TIMEOUT_RESP)
			SDIOD_LOGE("CMD12 abort failed\r\n");
	}
}

bk_err_t send_mult_data(uintptr_t addr, const uint8_t *data, uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
	uint32 num=0;
	uint16 block_num =0;

	sd_card_clear_transfer_error_flags();

	/* Drain stale completion signals from a prior aborted transfer (see
	 * receive_mult_data for the detailed rationale). */
	sdio_host_drain_completion_semas();

	NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_WR_READY_STAT_EN;
	ERROR_INT_STAT_EN_R(addr) = 0x870;
	NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN;
	ERROR_INT_SIGNAL_EN_R(addr) = 0x870;


	BLOCKSIZE_R(addr) = BLOCK_SIZE;
	BLOCKCOUNT_R(addr)= BLOCK_CNT;
	ARGUMENT_R(addr)  = ARGUMENT;
	XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN; //0xa6

	CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;

	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, 2000);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_cmd_done_sema", ret);
		return BK_FAIL;
	}

	for (block_num = 0; block_num < BLOCK_CNT; block_num = block_num+1)
	{
		/* Wait for write-buffer space: interrupt semaphore (fast path) with a
		 * BUF_WR_ENABLE poll fallback to recover a missed BUF_WR_READY
		 * interrupt (see receive_mult_data for rationale). */
		int ready = 0, slice;
		for (slice = 0; slice < 200; slice++) {
			if (rtos_get_semaphore(&s_sdio_wr_buf_ready_sema, 10) == 0) {
				ready = 1;
				break;
			}
			if (sd_card_has_transfer_error())
				break;
			if (PSTATE_REG_R(addr) & BUF_WR_ENABLE) {
				ready = 1;
				break;
			}
		}
		if((!ready) || sd_card_has_transfer_error()) {
			sd_card_log_transfer_wait_error(__func__, "s_sdio_wr_buf_ready_sema", ready ? 0 : -1);
			sd_card_abort_data_transfer(addr, true);
			return BK_FAIL;
		}
		while(num<((BLOCK_SIZE>>2)*(block_num+1)))
		{
			uint32_t write_data = sd_card_load_unaligned_le32(data + (num << 2));
			BUF_DATA_R(addr) = write_data;
			///SDIOD_LOGI("func %s, LINE=%d, num=%d, data = 0x%x.\r\n", __func__, __LINE__, num, *((uint32_t *)data + num));
			num++;
		}
	}


	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 2000);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_data_xfer_done_sema", ret);
		sd_card_abort_data_transfer(addr, true);
		return BK_FAIL;
	}

#if 0   //As CMD23 is added, CMD12 is not necessary
	ARGUMENT_R(addr) = 0x0;
	CMD_R(addr) = 12<<8;	//CMD12:Card stop transmission
	SDIOD_LOGD("func %s, LINE=%d.\r\n", __func__, __LINE__);
	while(CMD_COMPLETE_STATE==0);
	CMD_COMPLETE_STATE=0;

	while (bk_sd_card_get_card_state() != SD_CARD_TRANSFER)
	{
		SDIOD_LOGI("===> bk_sd_card_get_card_state = 0x%x.\r\n", bk_sd_card_get_card_state());
	}
#endif
	//SDIOD_LOGD("**Mul Data Buf Write End**\r\n");

	return BK_OK;
}

int receive_mult_data(uintptr_t addr, uint8_t *data, uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
	uint32 read_data;
	uint32 num=0;
	uint16 block_num =0;
	uint32_t timeout_ms = SDIO_HOST_DEFAULT_TIMEOUT_MS;

	sd_card_clear_transfer_error_flags();

	/*
	 * Drain any stale completion signals left over from a previously
	 * aborted/timed-out transfer. After such a failure the controller may
	 * still raise BUF_RD_READY / XFER_COMPLETE late, leaving these binary
	 * semaphores at count 1; the next transfer would then consume that
	 * stale token, read BUF_DATA before the block is actually ready and
	 * desync (data CRC error followed by a buffer-ready timeout). Clearing
	 * them here keeps each transfer (and disk_read retries) self-contained.
	 */
	sdio_host_drain_completion_semas();

	NORMAL_INT_STAT_EN_R(addr) = CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_RD_READY_STAT_EN;
	ERROR_INT_STAT_EN_R(addr) = 0x870;
	NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_RD_READY_STAT_EN;
	ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

	BLOCKSIZE_R(addr) = BLOCK_SIZE;
	BLOCKCOUNT_R(addr)= BLOCK_CNT;
	ARGUMENT_R(addr)  = ARGUMENT;

	if (CMD == CMD17 && BLOCK_CNT == 1) {
		/* Single-block read: no MULTBLK, block-count or Auto-CMD12 state
		 * machine. This is the protocol-correct counterpart of CMD17. */
		XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_DATA_READ;
		timeout_ms = SDIO_HOST_SD_CMD17_TIMEOUT_MS;
	} else {
		/* This is a generic Host PIO entry. eMMC CMD8 and SDIO CMD53 also
		 * carry read data, so preserve their legacy transfer configuration
		 * instead of rejecting every command other than SD CMD17/CMD18. */
		XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL |
			XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN;
	}
	CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;


	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, timeout_ms);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_cmd_done_sema", ret);
		sd_card_abort_data_transfer(addr, CMD == CMD18);
		return BK_FAIL;
	}

	for (block_num=0; block_num<BLOCK_CNT; block_num++)
	{
		/*
		 * Wait until a block is available in the read buffer. The per-block
		 * BUF_RD_READY interrupt can occasionally be missed on this MSHC
		 * controller, which used to stall a whole multi-block read until the
		 * 2 s timeout and then fail. To be robust we wait on the interrupt
		 * semaphore (fast common path) but also poll the live BUF_RD_ENABLE
		 * status bit (maintained by hardware, independent of the interrupt
		 * status) so a missed interrupt is recovered within ~10 ms.
		 */
		int ready = 0, slice;
		for (slice = 0; slice < (int)(timeout_ms / SDIO_HOST_BUF_READY_SLICE_MS); slice++) {
			if (rtos_get_semaphore(&s_sdio_rd_buf_ready_sema, SDIO_HOST_BUF_READY_SLICE_MS) == 0) {
				ready = 1;
				break;
			}
			if (sd_card_has_transfer_error())
				break;
			if (PSTATE_REG_R(addr) & BUF_RD_ENABLE) {
				ready = 1;
				break;
			}
		}
		if(!ready || sd_card_has_transfer_error()) {
			sd_card_log_transfer_wait_error(__func__, "s_sdio_rd_buf_ready_sema", ready ? 0 : -1);
			sd_card_abort_data_transfer(addr, CMD == CMD18);
			return BK_FAIL;
		}

		while(num<((BLOCK_SIZE>>2)*(block_num+1)))
		{
			read_data=BUF_DATA_R(addr);
			sd_card_store_unaligned_le32(data + (num << 2), read_data);
			//SDIOD_LOGI("Receive: BUF_DATA = %x, num =%x \r\n",read_data,num);
			num = num+1;
		}
	}

	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, timeout_ms);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_data_xfer_done_sema", ret);
		sd_card_abort_data_transfer(addr, CMD == CMD18);
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t adma2_send_data(uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
	sd_card_clear_transfer_error_flags();
	sdio_host_drain_completion_semas();
	send_cmd(SDIO_ACTIVE_BASE, CMD23, 2, BLOCK_CNT);  //CMD23 to set card block cnt

	uint32_t int_level = rtos_disable_int();
	uint32 adma2_wr_descriptor_tbl[2];
	adma2_wr_descriptor_tbl[0] = ((BLOCK_SIZE * BLOCK_CNT) << 16) | (ADMA2_ATTRIBUTE_ACT_TRAN << 3) | ADMA2_ATTRIBUTE_VALID_EN | ADMA2_ATTRIBUTE_END_EN | ADMA2_ATTRIBUTE_INT_EN;   //0x8000027;
	adma2_wr_descriptor_tbl[1] = SYS_ADDR;
	arch_dcache_flush_and_invd_range((void*)adma2_wr_descriptor_tbl, sizeof(adma2_wr_descriptor_tbl));

	HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA2;
	HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | 1<<12;	//HOST_VER4_ENABLE

	ADMA_SA_LOW_R(addr)= (uintptr_t)adma2_wr_descriptor_tbl;   //Set ADMA System AddressRegister (ADMA_SA_LOW_R)
	ADMA_SA_HIGH_R(addr)= 0;	//Set ADMA System AddressRegister (ADMA_SA_HIGH_R)

	NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
	ERROR_INT_STAT_EN_R(addr)= 0xb7f;

	NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
	ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

	BLOCKCOUNT_R(addr)= BLOCK_CNT;
	BLOCKSIZE_R(addr)= BLOCK_SIZE;

	ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 DISABLE; transfer :wirte
	XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;
	CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;
	rtos_enable_int(int_level);

	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, SDIO_HOST_DEFAULT_TIMEOUT_MS);
	if((ret != 0) || sd_card_has_transfer_error()) {
		SDIOD_LOGE("func %s get sem s_sdio_cmd_done_sema timeout\r\n", __func__);
		sd_card_abort_data_transfer(addr, CMD == CMD25);
		return BK_FAIL;
	}

	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, SDIO_HOST_DEFAULT_TIMEOUT_MS);
	if((ret != 0) || sd_card_has_transfer_error()) {
		SDIOD_LOGE("func %s get sem s_sdio_data_xfer_done_sema timeout\r\n", __func__);
		sd_card_abort_data_transfer(addr, CMD == CMD25);
		return BK_FAIL;
	}
	SDIOD_LOGD("*****Data(ADMA2) Write End*****\r\n");


	return ret;
}

bk_err_t adma2_receive_data (uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
	sd_card_clear_transfer_error_flags();
	sdio_host_drain_completion_semas();
	uint32_t int_level = rtos_disable_int();
	uint32 adma2_rd_descriptor_tbl[2];
	adma2_rd_descriptor_tbl[0] = ((BLOCK_SIZE * BLOCK_CNT) << 16) | (ADMA2_ATTRIBUTE_ACT_TRAN << 3) | ADMA2_ATTRIBUTE_VALID_EN | ADMA2_ATTRIBUTE_END_EN | ADMA2_ATTRIBUTE_INT_EN;;  //0x8000027;
	adma2_rd_descriptor_tbl[1] = SYS_ADDR;

	arch_dcache_flush_and_invd_range((void*)adma2_rd_descriptor_tbl, sizeof(adma2_rd_descriptor_tbl));

	HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA2;
	HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | 1<<12;	//HOST_VER4_ENABLE

	ADMA_SA_LOW_R(addr)= (uintptr_t)adma2_rd_descriptor_tbl;   //Set ADMA System AddressRegister (ADMA_SA_LOW_R)
	ADMA_SA_HIGH_R(addr)= 0;	//Set ADMA System AddressRegister (ADMA_SA_HIGH_R)

	NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
	ERROR_INT_STAT_EN_R(addr)= 0xb7f;

	NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
	ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

	BLOCKCOUNT_R(addr)= BLOCK_CNT;
	BLOCKSIZE_R(addr)= BLOCK_SIZE;

	ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 ENABLE; transfer :wirte
	XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;

	CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;
	rtos_enable_int(int_level);

	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, SDIO_HOST_DEFAULT_TIMEOUT_MS);
	if((ret != 0) || sd_card_has_transfer_error()) {
		SDIOD_LOGE("func %s: get sem s_sdio_cmd_done_sema timeout\r\n", __func__);
		sd_card_abort_data_transfer(addr, CMD == CMD18);
		return BK_FAIL;
	}
	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, SDIO_HOST_DEFAULT_TIMEOUT_MS);
	if((ret != 0) || sd_card_has_transfer_error()) {
		SDIOD_LOGE("func %s: get sem s_sdio_data_xfer_done_sema timeout\r\n", __func__);
		sd_card_abort_data_transfer(addr, CMD == CMD18);
		return BK_FAIL;
	}

	SDIOD_LOGD("*****Data(ADMA2) Read End*****\r\n");

	return ret;
}

/* Runtime pinmux setup. io_pos selects the controller instance:
 *   0 -> SDIO0 group: GPIO2/3/4/5/10/11 (+GPIO6~9 for 8-bit)
 *   1 -> SDIO1 group: GPIO14~23
 * Replaces the former compile-time SDIO0/SDIO1 selection so both BK7259
 * controllers can be brought up at runtime. */
void sdio_gpio_init(uint8_t io_pos, sdio_wire_width_sel_t width_sel)
{
	(void)io_pos;
	(void)width_sel;
}

void tuning_cfg(uintptr_t addr,uint8 tuning_rx_sel0, uint8 sample_rx_sel0,uint8 tuning_tx_sel0,uint8 sample_tx_sel0,uint8 clk_drv_inv_sel)
{
	sdio_reg5(addr) |= (tuning_rx_sel0 & 0x0f) <<5 | sample_rx_sel0 <<14;
	sdio_reg5(addr) |= (tuning_tx_sel0 & 0x0f) <<17 | sample_tx_sel0 <<26;
	sdio_reg5(addr) |= clk_drv_inv_sel << 29;
}

bk_err_t mshc_host_init(uintptr_t addr,uint16 sysclk_div,uint16 sdclk_div,uint8 tmclk_div,uint8 cqetmclk_div,uint8 CARD_IS_EMMC,uint8 UHS_MODE_SEL,uint8 DAT_XFER_WIDTH)
{
	bk_err_t ret = BK_OK;
	uint8 ddr_mode;
	sdio_reg2(addr)= 3<<0;
	sdio_reg4(addr)= (tmclk_div<<8)|(cqetmclk_div);
	if(addr == sdio_mshc_0_base)
	{
		sys_hal_sdio0_set_src_clk_div(sysclk_div);
		sys_hal_sdio0_set_cken(1);
	}
	if(addr == sdio_mshc_1_base)
	{
		sys_hal_sdio1_set_src_clk_div(sysclk_div);
		sys_hal_sdio1_set_cken(1);
	}

	host_ctrl_set(addr,SD_BUS_PWR_VDD1,0x0e,CARD_IS_EMMC,DAT_XFER_WIDTH);//addr,SD_BUS_VOL_VDD1,TOUT_CNT,DAT_XFER_WIDTH
	sd_clk_change(addr,sdclk_div);

	if(CARD_IS_EMMC == 1)  //EMMC CARD INIT
	{
		emmc_card_interface_set(addr,UHS_MODE_SEL);
		if(UHS_MODE_SEL == UHS_MODE_EMMC_HSDDR)
		{
			SDIOD_LOGI("EMMC Init DDR\r\n");
			ddr_mode =1;
			emmc_card_init(addr,ddr_mode);
		}
		else
		{
			SDIOD_LOGI("EMMC Init SDR\r\n");
			ddr_mode =0;
			emmc_card_init(addr,ddr_mode);
		}
	}
	else
	{
		sd_card_interface_set(addr,UHS_MODE_SEL);
	}

	return ret;
}


void sdio_reset(void)
{
	SDIOD_LOGI("func %s .\r\n", __func__);

	SW_RST_R(SDIO_ACTIVE_BASE)=0xff;
	CLK_CTRL_R(SDIO_ACTIVE_BASE) = 0;
	HOST_CTRL2_R(SDIO_ACTIVE_BASE) = 0;
	PWR_CTRL_R(SDIO_ACTIVE_BASE) = 0;
	sdio_reg2(SDIO_ACTIVE_BASE)= 0<<0;
	sdio_reg2(SDIO_ACTIVE_BASE)= 3<<0;
	card_clk_stop(SDIO_ACTIVE_BASE);
	/* Enable the clock of whichever controller the generic API selected at
	 * runtime (s_active_id), consistent with sdio_gpio_init() below. */
	if (s_active_id == SDIO_HOST_ID_1) {
		sys_hal_sdio1_set_cken(0);
		sys_hal_sdio1_set_cken(1);
	} else {
		sys_hal_sdio0_set_cken(0);
		sys_hal_sdio0_set_cken(1);
	}
}

bk_err_t sdio_host_init()
{
	bk_err_t ret = BK_OK;

	SDIOD_LOGI("SDIO init Start...\r\n");

	//sys_hal_sdio0_set_int_en(1);
	//sys_hal_sdio1_set_int_en(1);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_SDIO0, 1);
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_SDIO1, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_SDIO0, 1);
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_SDIO1, 1);
#endif
//	sys_a35_ll_set_clk_ctrl0_ckdiv_cpu(1);

//	sys_ana_ll_set_reg0_spitrig(1);
//	sys_ana_ll_set_reg0_spitrig(0);
//	sys_ana_ll_set_reg5_en_vout(1);

	sdio_gpio_init(s_active_id, SDIO_WIRE_WIDTH_SEL_1);
	PWR_CTRL_R(SDIO_ACTIVE_BASE) = 0x01;

	ret = mshc_host_init(SDIO_ACTIVE_BASE,0x3,300,0xff,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH1);	  //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
	if(BK_OK != ret)
		return ret;

	/* Flush the CMD/DAT state machines now that mshc_host_init()->
	 * sd_clk_change() has SD_CLK running again. The teardown in sdio_reset()
	 * issues SW_RST_ALL but immediately stops SD_CLK afterwards; SW_RST_ALL
	 * needs the clock to complete, so with the clock gated it does not
	 * reliably flush the DAT data-path state machine on this MSHC. The stale
	 * DAT state survives into the re-init and makes the first multi-block
	 * read after a soft re-init (unmount->mount without power-cycle) complete
	 * its command phase but never assert BUF_RD_READY/BUF_RD_ENABLE (no
	 * CRC/timeout/error int), so receive_mult_data() times out until
	 * disk_read() retries with a full reset. Resetting CMD/DAT here with the
	 * clock live - the same thing sd_card_abort_data_transfer() does on the
	 * recovery path - makes the first read after re-init reliable. */
	SW_RST_R(SDIO_ACTIVE_BASE) |= SW_RST_DAT | SW_RST_CMD;
	for (int i = 0; i < MAX_WAIT_STATE_TRANS_TIMES; i++) {
		if ((SW_RST_R(SDIO_ACTIVE_BASE) & (SW_RST_DAT | SW_RST_CMD)) == 0)
			break;
		rtos_delay_milliseconds(1);
	}

	return ret;
}

void sdio_switch_wire_width(sdio_wire_width_sel_t width_sel)
{
	sdio_gpio_init(s_active_id, width_sel);
}

void sdio_dwc_isr0(void)
{
	volatile  uint16  normal_int;
	volatile  uint16  error_int;

	normal_int = NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE);
	error_int  = ERROR_INT_STAT_R(SDIO_ACTIVE_BASE);

	if(normal_int & CMD_COMPLETE_STAT_EN)
	{
		//CMD_COMPLETE_STATE = 1;
		if (s_sdio_cmd_done_sema)
			rtos_set_semaphore(&s_sdio_cmd_done_sema);
		//SDIOD_LOGD("CMD_COMP\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_COMPLETE_STAT;
	}
	if(normal_int & XFER_COMPLETE_STAT_EN)
	{
		//XFER_COMPLETE_STATE = 1;
		//SDIOD_LOGD("XFER_COMP\r\n");
		if (s_sdio_data_xfer_done_sema)
			rtos_set_semaphore(&s_sdio_data_xfer_done_sema);
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_XFER_COMPLETE_STAT;
	}
	if(normal_int & BGAP_EVENT_STAT_EN)
	{
		BGAP_EVENT_STATE = 1;
		SDIOD_LOGD("BGAP_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BGAP_EVENT_STAT;
	}
	if(normal_int & DMA_INTERRUPT_STAT_EN)
	{
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DMA_INTERRUPT_STAT;
		SDIOD_LOGD("DMA_INT\r\n");
		DMA_INTERRUPT_STATE = 1;
	}
	if(normal_int & BUF_WR_READY_STAT_EN)
	{
		//BUF_WR_READY_STATE = 1;
		//SDIOD_LOGD("BUF_WR_READY\r\n");
		if (s_sdio_wr_buf_ready_sema)
			rtos_set_semaphore(&s_sdio_wr_buf_ready_sema);
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BUF_WR_READY_STAT;
	}
	if(normal_int & BUF_RD_READY_STAT_EN)
	{
		//BUF_RD_READY_STATE = 1;
		if (s_sdio_rd_buf_ready_sema)
			rtos_set_semaphore(&s_sdio_rd_buf_ready_sema);
		//SDIOD_LOGD("BUF_RD_READY\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BUF_RD_READY_STAT;
	}
	if(normal_int & CARD_INSERTION_STAT_EN)
	{
		CARD_INSERTION_STATE = 1;
		SDIOD_LOGD("CARD_INSERTION\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_INSERTION_STAT;//clear the card insertion bit
	}
	if(normal_int & CARD_REMOVAL_STAT_EN)
	{
		CARD_REMOVAL_STATE = 1;
		SDIOD_LOGD("CARD_REMOVAL\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_REMOVAL_STAT;
	}
	if(normal_int & CARD_INTERRUPT_STAT_EN)
	{
		CARD_INTERRUPT_STATE = 1;
		SDIOD_LOGD("CARD_INTERRUPT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_INTERRUPT_STAT;
		sdio_host_dispatch_card_irq();
	}
	if(normal_int & INT_A_STAT_EN)
	{
		INT_A_STATE = 1;
		SDIOD_LOGD("INT_A\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_A_STAT;
	}
	if(normal_int & INT_B_STAT_EN)
	{
		INT_B_STATE = 1;
		SDIOD_LOGD("INT_B\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_B_STAT;
	}
	if(normal_int & INT_C_STAT_EN)
	{
		INT_C_STATE = 1;
		SDIOD_LOGD("INT_C\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_C_STAT;
	}
	if(normal_int & RE_TUNE_EVENT_STAT_EN)
	{
		RE_TUNE_EVENT_STATE = 1;
		SDIOD_LOGD("RE_TUNE_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_RE_TUNE_EVENT_STAT;
	}
	if(normal_int & FX_EVENT_STAT_EN)
	{
		FX_EVENT_STATE = 1;
		SDIOD_LOGD("FX_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_FX_EVENT_STAT;
	}
	if(normal_int & CQE_EVENT_STAT_EN)
	{
		CQE_EVENT_STATE = 1;
		SDIOD_LOGD("CQE_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CQE_EVENT_STAT;
	}
#if 1
	if(error_int & ERROR_INTERRUPT_STAT_EN)
	{
		ERROR_INTERRUPT_STATE = 1;
		SDIOD_LOGD("ERROR_INTERRUPT\r\n");
	}
#else
	if(normal_int & ERROR_INTERRUPT_STAT_EN)
	{
		ERROR_INTERRUPT_STATE = 1;
		SDIOD_LOGD("ERROR_INTERRUPT\r\n");
	}
#endif

	if(error_int & CMD_TOUT_ERR_STAT_EN)
	{
		CMD_TOUT_ERR_STATE = 1;
		SDIOD_LOGD("CMD_TOUT_ERR, command timeout error\r\n");
		/* Mask the signal so the controller stops retriggering this ISR
		 * before software resets the CMD line. send_cmd() error path
		 * re-arms this bit after SW_RST_CMD completes. */
		ERROR_INT_SIGNAL_EN_R(SDIO_ACTIVE_BASE) &= ~CMD_TOUT_ERR_STAT_EN;
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_TOUT_ERR_STAT;
		/* Wake up the send_cmd() waiter so fail-fast actually fails
		 * fast rather than waiting the full 2000ms semaphore timeout. */
		if (s_sdio_cmd_done_sema)
			rtos_set_semaphore(&s_sdio_cmd_done_sema);
	}
	if(error_int & CMD_CRC_ERR_STAT_EN)
	{
		CMD_CRC_ERR_STATE = 1;
		SDIOD_LOGD("CMD_CRC_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_CRC_ERR_STAT;
	}
	if(error_int & CMD_END_BIT_ERR_STAT_EN)
	{
		CMD_END_BIT_ERR_STATE = 1;
		SDIOD_LOGD("CMD_END_BIT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_END_BIT_ERR_STAT;
	}
	if(error_int & CMD_IDX_ERR_STAT_EN)
	{
		CMD_IDX_ERR_STATE = 1;
		SDIOD_LOGD("CMD_IDX_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_IDX_ERR_STAT;
	}
	if(error_int & DATA_TOUT_ERR_STAT_EN)
	{
		DATA_TOUT_ERR_STATE = 1;
		SDIOD_LOGD("DATA_TOUT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_TOUT_ERR_STAT;
	}
	if(error_int & DATA_CRC_ERR_STAT_EN)
	{
		DATA_CRC_ERR_STATE = 1;
		SDIOD_LOGD("DATA_CRC_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_CRC_ERR_STAT;
	}
	if(error_int & DATA_END_BIT_ERR_STAT_EN)
	{
		DATA_END_BIT_ERR_STATE = 1;
		SDIOD_LOGD("DATA_END_BIT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_END_BIT_ERR_STAT;
	}
	if(error_int & CUR_LMT_ERR_STAT_EN)
	{
		CUR_LMT_ERR_STATE = 1;
		SDIOD_LOGD("CUR_LMT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CUR_LMT_ERR_STAT;
	}
	if(error_int & AUTO_CMD_ERR_STAT_EN)
	{
		AUTO_CMD_ERR_STATE = 1;
		SDIOD_LOGD("AUTO_CMD_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_AUTO_CMD_ERR_STAT;
	}
	if(error_int & ADMA_ERR_STAT_EN)
	{
		ADMA_ERR_STATE = 1;
		SDIOD_LOGD("ADMA_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_ADMA_ERR_STAT;
	}
	if(error_int & TUNING_ERR_STAT_EN)
	{
		TUNING_ERR_STATE = 1;
		SDIOD_LOGD("TUNING_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_TUNING_ERR_STAT;
	}
	if(error_int & RESP_ERR_STAT_EN)
	{
		//uint32_t xfer_mode = UHS2_XFER_MODE_R(SDIO_ACTIVE_BASE);
		uint32_t resp_err = RESP01_R(SDIO_ACTIVE_BASE);
		uint32_t resp_err2 = RESP23_R(SDIO_ACTIVE_BASE);
		uint32_t resp_err3 = RESP45_R(SDIO_ACTIVE_BASE);
		uint32_t resp_err6 = RESP67_R(SDIO_ACTIVE_BASE);
		//SDIOD_LOGD("RESP_ERR, xfer_mode=0x%08x\r\n", xfer_mode);
		RESP_ERR_STATE = 1;
		SDIOD_LOGD("RESP_ERR, resp_err=0x%08x, resp_err2=0x%08x, resp_err3=0x%08x, resp_err6=0x%08x\r\n", resp_err, resp_err2, resp_err3, resp_err6);
		(void)resp_err;
		(void)resp_err2;
		(void)resp_err3;
		(void)resp_err6;
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_RESP_ERR_STAT;
	}
	if(error_int & BOOT_ACK_ERR_STAT_EN)
	{
		BOOT_ACK_ERR_STATE = 1;
		SDIOD_LOGD("BOOT_ACK_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BOOT_ACK_ERR_STAT;
	}
	if(error_int & VENDOR_ERR1_STAT_EN)
	{
		VENDOR_ERR1_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR1\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR1_STAT;
	}
	if(error_int & VENDOR_ERR2_STAT_EN)
	{
		VENDOR_ERR2_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR2\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR2_STAT;
	}
	if(error_int & VENDOR_ERR3_STAT_EN)
	{
		VENDOR_ERR3_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR3\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR3_STAT;
	}

	if (error_int & (DATA_TOUT_ERR_STAT_EN | DATA_CRC_ERR_STAT_EN |
	    DATA_END_BIT_ERR_STAT_EN | ADMA_ERR_STAT_EN)) {
		if (s_sdio_data_xfer_done_sema)
			rtos_set_semaphore(&s_sdio_data_xfer_done_sema);
	}

	normal_int = NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE);
	error_int  = ERROR_INT_STAT_R(SDIO_ACTIVE_BASE);

}

void sdio_dwc_isr1(void)
{
	volatile  uint16	normal_int;
	volatile  uint16	error_int;

	normal_int = NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE);
	error_int  = ERROR_INT_STAT_R(SDIO_ACTIVE_BASE);

	if(normal_int & CMD_COMPLETE_STAT_EN)
	{
		CMD_COMPLETE_STATE = 1;
		SDIOD_LOGD("CMD_COMP\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_COMPLETE_STAT;
	}
	if(normal_int & XFER_COMPLETE_STAT_EN)
	{
		XFER_COMPLETE_STATE = 1;
		//SDIOD_LOGD("XFER_COMP\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_XFER_COMPLETE_STAT;
	}
	if(normal_int & BGAP_EVENT_STAT_EN)
	{
		BGAP_EVENT_STATE = 1;
		SDIOD_LOGD("BGAP_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BGAP_EVENT_STAT;
	}
	if(normal_int & DMA_INTERRUPT_STAT_EN)
	{
		DMA_INTERRUPT_STATE = 1;
		SDIOD_LOGD("DMA_INT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DMA_INTERRUPT_STAT;
	}
	if(normal_int & BUF_WR_READY_STAT_EN)
	{
		BUF_WR_READY_STATE = 1;
		SDIOD_LOGD("BUF_WR_READY\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BUF_WR_READY_STAT;
	}
	if(normal_int & BUF_RD_READY_STAT_EN)
	{
		BUF_RD_READY_STATE = 1;
		//SDIOD_LOGD("BUF_RD_READY\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BUF_RD_READY_STAT;
	}
	if(normal_int & CARD_INSERTION_STAT_EN)
	{
		CARD_INSERTION_STATE = 1;
		SDIOD_LOGD("CARD_INSERTION\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_INSERTION_STAT;//clear the card insertion bit
	}
	if(normal_int & CARD_REMOVAL_STAT_EN)
	{
		CARD_REMOVAL_STATE = 1;
		SDIOD_LOGD("CARD_REMOVAL\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_REMOVAL_STAT;
	}
	if(normal_int & CARD_INTERRUPT_STAT_EN)
	{
		CARD_INTERRUPT_STATE = 1;
		SDIOD_LOGD("CARD_INTERRUPT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CARD_INTERRUPT_STAT;
		sdio_host_dispatch_card_irq();
	}
	if(normal_int & INT_A_STAT_EN)
	{
		INT_A_STATE = 1;
		SDIOD_LOGD("INT_A\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_A_STAT;
	}
	if(normal_int & INT_B_STAT_EN)
	{
		INT_B_STATE = 1;
		SDIOD_LOGD("INT_B\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_B_STAT;
	}
	if(normal_int & INT_C_STAT_EN)
	{
		INT_C_STATE = 1;
		SDIOD_LOGD("INT_C\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_INT_C_STAT;
	}
	if(normal_int & RE_TUNE_EVENT_STAT_EN)
	{
		RE_TUNE_EVENT_STATE = 1;
		SDIOD_LOGD("RE_TUNE_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_RE_TUNE_EVENT_STAT;
	}
	if(normal_int & FX_EVENT_STAT_EN)
	{
		FX_EVENT_STATE = 1;
		SDIOD_LOGD("FX_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_FX_EVENT_STAT;
	}
	if(normal_int & CQE_EVENT_STAT_EN)
	{
		CQE_EVENT_STATE = 1;
		SDIOD_LOGD("CQE_EVENT\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CQE_EVENT_STAT;
	}
	if(error_int & ERROR_INTERRUPT_STAT_EN)
	{
		ERROR_INTERRUPT_STATE = 1;
		SDIOD_LOGD("ERROR_INTERRUPT\r\n");
	}

	if(error_int & CMD_TOUT_ERR_STAT_EN)
	{
		CMD_TOUT_ERR_STATE = 1;
		SDIOD_LOGD("CMD_TOUT_ERR\r\n");
		ERROR_INT_SIGNAL_EN_R(SDIO_ACTIVE_BASE) &= ~CMD_TOUT_ERR_STAT_EN;
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_TOUT_ERR_STAT;
		if (s_sdio_cmd_done_sema)
			rtos_set_semaphore(&s_sdio_cmd_done_sema);
	}
	if(error_int & CMD_CRC_ERR_STAT_EN)
	{
		CMD_CRC_ERR_STATE = 1;
		SDIOD_LOGD("CMD_CRC_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_CRC_ERR_STAT;
	}
	if(error_int & CMD_END_BIT_ERR_STAT_EN)
	{
		CMD_END_BIT_ERR_STATE = 1;
		SDIOD_LOGD("CMD_END_BIT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_END_BIT_ERR_STAT;
	}
	if(error_int & CMD_IDX_ERR_STAT_EN)
	{
		CMD_IDX_ERR_STATE = 1;
		SDIOD_LOGD("CMD_IDX_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_IDX_ERR_STAT;
	}
	if(error_int & DATA_TOUT_ERR_STAT_EN)
	{
		DATA_TOUT_ERR_STATE = 1;
		SDIOD_LOGD("DATA_TOUT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_TOUT_ERR_STAT;
	}
	if(error_int & DATA_CRC_ERR_STAT_EN)
	{
		DATA_CRC_ERR_STATE = 1;
		SDIOD_LOGD("DATA_CRC_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_CRC_ERR_STAT;
	}
	if(error_int & DATA_END_BIT_ERR_STAT_EN)
	{
		DATA_END_BIT_ERR_STATE = 1;
		SDIOD_LOGD("DATA_END_BIT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_DATA_END_BIT_ERR_STAT;
	}
	if(error_int & CUR_LMT_ERR_STAT_EN)
	{
		CUR_LMT_ERR_STATE = 1;
		SDIOD_LOGD("CUR_LMT_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CUR_LMT_ERR_STAT;
	}
	if(error_int & AUTO_CMD_ERR_STAT_EN)
	{
		AUTO_CMD_ERR_STATE = 1;
		SDIOD_LOGD("AUTO_CMD_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_AUTO_CMD_ERR_STAT;
	}
	if(error_int & ADMA_ERR_STAT_EN)
	{
		ADMA_ERR_STATE = 1;
		SDIOD_LOGD("ADMA_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_ADMA_ERR_STAT;
	}
	if(error_int & TUNING_ERR_STAT_EN)
	{
		TUNING_ERR_STATE = 1;
		SDIOD_LOGD("TUNING_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_TUNING_ERR_STAT;
	}
	if(error_int & RESP_ERR_STAT_EN)
	{
		RESP_ERR_STATE = 1;
		SDIOD_LOGD("RESP_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_RESP_ERR_STAT;
	}
	if(error_int & BOOT_ACK_ERR_STAT_EN)
	{
		BOOT_ACK_ERR_STATE = 1;
		SDIOD_LOGD("BOOT_ACK_ERR\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BOOT_ACK_ERR_STAT;
	}
	if(error_int & VENDOR_ERR1_STAT_EN)
	{
		VENDOR_ERR1_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR1\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR1_STAT;
	}
	if(error_int & VENDOR_ERR2_STAT_EN)
	{
		VENDOR_ERR2_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR2\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR2_STAT;
	}
	if(error_int & VENDOR_ERR3_STAT_EN)
	{
		VENDOR_ERR3_STATE = 1;
		SDIOD_LOGD("VENDOR_ERR3\r\n");
		ERROR_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_VENDOR_ERR3_STAT;
	}

	if (error_int & (DATA_TOUT_ERR_STAT_EN | DATA_CRC_ERR_STAT_EN |
	    DATA_END_BIT_ERR_STAT_EN | ADMA_ERR_STAT_EN)) {
		if (s_sdio_data_xfer_done_sema)
			rtos_set_semaphore(&s_sdio_data_xfer_done_sema);
	}
}

/* =========================================================================
 *                  Generic SDIO host controller interface
 *  Thin, controller-agnostic layer over the low-level register driver above.
 *  Upper protocol drivers (sd_card / emmc / sdio_func) use ONLY these APIs.
 * ====================================================================== */

typedef struct {
	uintptr_t          base;
	bool               initialized;
	bool               is_emmc;
	sdio_host_irq_cb_t sdio_irq_cb;
	void              *sdio_irq_arg;
} sdio_host_inst_t;

static sdio_host_inst_t s_host_inst[SDIO_HOST_ID_MAX] = {
	{ .base = sdio_mshc_0_base },
	{ .base = sdio_mshc_1_base },
};

/* Serializes transactions across the two controllers (shared low-level
 * error-state globals + wait semaphores). */
static beken_mutex_t s_host_lock = NULL;

static void sdio_host_lock(void)   { if (s_host_lock) rtos_lock_mutex(&s_host_lock); }
static void sdio_host_unlock(void) { if (s_host_lock) rtos_unlock_mutex(&s_host_lock); }

static void sdio_host_select(sdio_host_id_t id)
{
	s_active_base = s_host_inst[id].base;
	s_active_id   = id;
}

/* Map generic response type to the controller RESP_TYPE field used by
 * send_cmd(): 0=none, 1=long(136-bit R2), 2=short(48-bit), 3=short+busy. */
static uint8_t sdio_host_hw_resp_type(sdio_host_resp_type_t t)
{
	switch (t) {
	case SDIO_HOST_RESP_NONE: return 0;
	case SDIO_HOST_RESP_R2:   return 1;
	case SDIO_HOST_RESP_R1B:  return 3;
	default:                  return 2;
	}
}

bk_err_t bk_sdio_host_init(sdio_host_id_t id, const sdio_host_cfg_t *cfg)
{
	bk_err_t ret;
	bool is_emmc = (cfg && cfg->is_emmc);

	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;

	if (s_host_lock == NULL) {
		ret = rtos_init_mutex(&s_host_lock);
		if (ret != kNoErr)
			return ret;
	}

	sdio_host_lock();
	sdio_host_select(id);

	ret = sdio_host_shared_resource_init();   /* idempotent: allocate semaphores */
	if (ret != BK_OK) {
		sdio_host_unlock();
		return ret;
	}

	if (is_emmc) {
		/* eMMC controller bring-up (identification clock, 1-bit, eMMC card
		 * type bit). Speed-mode (HS/HS200/HS400) is applied later by the
		 * emmc driver via bk_sdio_host_set_timing()/set_bus_width(). */
		sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_SDIO0, 1);
		sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_SDIO1, 1);
		sdio_gpio_init(id, SDIO_WIRE_WIDTH_SEL_1);
		PWR_CTRL_R(s_active_base) = 0x01;
		tuning_cfg(s_active_base, 0, 0, 0, 0, 0);
		ret = mshc_host_init(s_active_base, 0x3, 300, 0xff, 0xa,
				     1 /*CARD_IS_EMMC*/, UHS_MODE_EMMC_DS, DATA_WIDTH1);
	} else {
		tuning_cfg(s_active_base, 0, 0, 0x8, 1, 0);
		ret = sdio_host_init();         /* SD path, behavior preserved */
	}

	s_host_inst[id].initialized = (ret == BK_OK);
	if (ret == BK_OK)
		s_host_inst[id].is_emmc = is_emmc;
	sdio_host_unlock();
	return ret;
}

bk_err_t bk_sdio_host_deinit(sdio_host_id_t id)
{
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	sdio_host_lock();
	sdio_host_select(id);
	NORMAL_INT_SIGNAL_EN_R(s_active_base) = 0;
	ERROR_INT_SIGNAL_EN_R(s_active_base) = 0;
	__DSB();
	sdio_reset();
	NORMAL_INT_STAT_R(s_active_base) = 0xffff;
	ERROR_INT_STAT_R(s_active_base) = 0xffff;
	__DSB();
	sdio_host_drain_completion_semas();
	s_host_inst[id].initialized = false;
	sdio_host_unlock();

	/* The completion semaphores are shared by both host instances and touched
	 * from ISR context. Keep them allocated for the driver lifetime. Destroying
	 * them on every card unmount created an SMP race: after releasing
	 * s_host_lock another core could enter init/xfer while this deinit destroyed
	 * the semaphore, leading to xQueueSemaphoreTake(NULL). */
	return BK_OK;
}

bk_err_t bk_sdio_host_reset(sdio_host_id_t id)
{
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	sdio_host_lock();
	sdio_host_select(id);
	sdio_reset();
	sdio_host_unlock();
	return BK_OK;
}

bk_err_t bk_sdio_host_set_clock(sdio_host_id_t id, uint32_t freq_hz)
{
	uint32_t div;
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	if (freq_hz == 0)
		freq_hz = 400000;
	div = (80000000u + freq_hz - 1) / freq_hz;   /* base clock 80MHz */
	if (div)
		div -= 1;
	if (div > 0x3ff)
		div = 0x3ff;
	sdio_host_lock();
	sdio_host_select(id);
	sd_clk_change(s_active_base, (uint16)div);
	sdio_host_unlock();
	return BK_OK;
}

bk_err_t bk_sdio_host_set_bus_width(sdio_host_id_t id, sdio_host_bus_width2_t width)
{
	uint8_t bits;
	sdio_wire_width_sel_t gw;
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	switch (width) {
	case SDIO_HOST_BUS_WIDTH_8: bits = (1 << 5); gw = SDIO_WIRE_WIDTH_SEL_8; break;
	case SDIO_HOST_BUS_WIDTH_4: bits = (1 << 1); gw = SDIO_WIRE_WIDTH_SEL_4; break;
	default:                    bits = 0;        gw = SDIO_WIRE_WIDTH_SEL_1; break;
	}
	sdio_host_lock();
	sdio_host_select(id);
	HOST_CTRL1_R(s_active_base) = (HOST_CTRL1_R(s_active_base) & ~((1 << 1) | (1 << 5))) | bits;
	sdio_gpio_init(id, gw);
	sdio_host_unlock();
	return BK_OK;
}

bk_err_t bk_sdio_host_set_timing(sdio_host_id_t id, sdio_host_timing_t timing)
{
	uint8_t uhs;
	bool is_emmc;
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	is_emmc = s_host_inst[id].is_emmc;
	switch (timing) {
	case SDIO_HOST_TIMING_SDR25:     uhs = UHS_MODE_SDR25; break;
	case SDIO_HOST_TIMING_SDR50:     uhs = UHS_MODE_SDR50; break;
	case SDIO_HOST_TIMING_SDR104:    uhs = UHS_MODE_SDR104; break;
	case SDIO_HOST_TIMING_DDR50:     uhs = UHS_MODE_DDR50; break;
	case SDIO_HOST_TIMING_MMC_HS:    uhs = UHS_MODE_EMMC_HS; break;
	case SDIO_HOST_TIMING_MMC_DDR:   uhs = UHS_MODE_EMMC_HSDDR; break;
	case SDIO_HOST_TIMING_MMC_HS200: uhs = UHS_MODE_EMMC_HS200; break;
	case SDIO_HOST_TIMING_MMC_HS400: uhs = UHS_MODE_EMMC_HS400; break;
	default:                         uhs = is_emmc ? UHS_MODE_EMMC_DS : UHS_MODE_SDR12; break;
	}
	sdio_host_lock();
	sdio_host_select(id);
	if (is_emmc)
		emmc_card_interface_set(s_active_base, uhs);
	else
		sd_card_interface_set(s_active_base, uhs);
	sdio_host_unlock();
	return BK_OK;
}

bk_err_t bk_sdio_host_set_signal_voltage(sdio_host_id_t id, sdio_host_signal_voltage_t volt)
{
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	/* TODO[HW]: 1.8V signaling (HOST_CTRL2.SIGNALING_EN + board level-shifter)
	 * needs hardware bring-up; current EVB only wires 3.3V. */
	(void)volt;
	return BK_OK;
}

bool bk_sdio_host_card_present(sdio_host_id_t id)
{
	if (id >= SDIO_HOST_ID_MAX)
		return false;
	return (PSTATE_REG_R(s_host_inst[id].base) & CARD_INSERTED) ? true : false;
}

bk_err_t bk_sdio_host_get_caps(sdio_host_id_t id, sdio_host_caps_t *caps)
{
	if (id >= SDIO_HOST_ID_MAX || caps == NULL)
		return BK_ERR_PARAM;
	caps->support_8bit     = true;
	caps->support_adma2    = true;
	caps->support_sdio_irq = true;
	caps->max_clock_hz     = 80000000u;
	return BK_OK;
}

bk_err_t bk_sdio_host_send_cmd(sdio_host_id_t id, const sdio_host_cmd_t *cmd,
			      sdio_host_resp_t *resp)
{
	uint32_t r;
	bool timeout;

	if (id >= SDIO_HOST_ID_MAX || cmd == NULL)
		return BK_ERR_PARAM;

	sdio_host_lock();
	sdio_host_select(id);
	if (!s_host_inst[id].initialized ||
	    s_sdio_cmd_done_sema == NULL ||
	    (cmd->resp_type == SDIO_HOST_RESP_R1B &&
	     s_sdio_data_xfer_done_sema == NULL)) {
		sdio_host_unlock();
		return BK_ERR_SDIO_HOST_NOT_INIT;
	}
	r = (uint32_t)send_cmd(s_active_base, cmd->index,
			       sdio_host_hw_resp_type(cmd->resp_type), cmd->arg);
	timeout = sdio_host_last_cmd_timeout();
	bool crc_err = (CMD_CRC_ERR_STATE != 0);
	if (resp) {
		resp->timeout = timeout;
		resp->crc_err = crc_err;
		if (cmd->resp_type == SDIO_HOST_RESP_R2) {
			/*
			 * R2 (136-bit CID/CSD): this SDHCI-style MSHC controller stores
			 * only the 120-bit payload (card register bits [127:8], the low
			 * 8 CRC/stop bits are stripped) right-aligned across the four
			 * RESPxx registers:
			 *   RESP01 = bits[39:8]   RESP23 = bits[71:40]
			 *   RESP45 = bits[103:72] RESP67 = bits[127:104] (in [23:0])
			 * Upper protocol drivers (sd_card/emmc) expect the conventional
			 * full 128-bit layout (resp[0]=bits[127:96] ... resp[3]=[31:0]).
			 * Re-align by shifting in the missing low byte so csd_structure
			 * and capacity fields land in the right place; otherwise a normal
			 * SDHC/SDXC card is mis-parsed as CSD v3.0 with a bogus ~2TB size.
			 */
			uint32_t r01 = RESP01_R(s_active_base);
			uint32_t r23 = RESP23_R(s_active_base);
			uint32_t r45 = RESP45_R(s_active_base);
			uint32_t r67 = RESP67_R(s_active_base);
			resp->resp[0] = (r67 << 8) | (r45 >> 24);
			resp->resp[1] = (r45 << 8) | (r23 >> 24);
			resp->resp[2] = (r23 << 8) | (r01 >> 24);
			resp->resp[3] = (r01 << 8);
		} else {
			resp->resp[0] = r;
			resp->resp[1] = resp->resp[2] = resp->resp[3] = 0;
		}
	}
	sdio_host_unlock();

	SDIOD_LOGD("send_cmd: CMD%u arg=0x%08x rt=%d tout=%d crc=%d resp0=0x%08x\r\n",
		   cmd->index, cmd->arg, cmd->resp_type, timeout, crc_err, r);

	return timeout ? BK_ERR_SDIO_HOST_CMD_RSP_TIMEOUT : BK_OK;
}

bk_err_t bk_sdio_host_xfer(sdio_host_id_t id, const sdio_host_cmd_t *cmd,
			   sdio_host_data_t *data, sdio_host_resp_t *resp)
{
	bk_err_t ret;

	if (id >= SDIO_HOST_ID_MAX || cmd == NULL || data == NULL || data->buf == NULL)
		return BK_ERR_PARAM;

	sdio_host_lock();
	sdio_host_select(id);
	if (!s_host_inst[id].initialized ||
	    s_sdio_cmd_done_sema == NULL ||
	    s_sdio_wr_buf_ready_sema == NULL ||
	    s_sdio_rd_buf_ready_sema == NULL ||
	    s_sdio_data_xfer_done_sema == NULL) {
		sdio_host_unlock();
		return BK_ERR_SDIO_HOST_NOT_INIT;
	}
	if (data->dir == SDIO_HOST_XFER_WRITE) {
		if (data->mode == SDIO_HOST_XFER_DMA_ADMA2)
			ret = adma2_send_data(s_active_base, (uint32)(uintptr_t)data->buf,
					      data->block_size, data->block_cnt, cmd->index, cmd->arg);
		else
			ret = send_mult_data(s_active_base, data->buf,
					     data->block_size, data->block_cnt, cmd->index, cmd->arg);
	} else {
		if (data->mode == SDIO_HOST_XFER_DMA_ADMA2)
			ret = adma2_receive_data(s_active_base, (uint32)(uintptr_t)data->buf,
						 data->block_size, data->block_cnt, cmd->index, cmd->arg);
		else
			ret = (bk_err_t)receive_mult_data(s_active_base, data->buf,
							  data->block_size, data->block_cnt, cmd->index, cmd->arg);
	}
	if (resp) {
		resp->timeout = sdio_host_last_cmd_timeout();
		resp->crc_err = (CMD_CRC_ERR_STATE != 0);
		resp->resp[0] = RESP01_R(s_active_base);
		resp->resp[1] = resp->resp[2] = resp->resp[3] = 0;
	}
	sdio_host_unlock();
	return ret;
}

bk_err_t bk_sdio_host_register_sdio_irq(sdio_host_id_t id, sdio_host_irq_cb_t cb, void *arg)
{
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	s_host_inst[id].sdio_irq_cb  = cb;
	s_host_inst[id].sdio_irq_arg = arg;
	return BK_OK;
}

bk_err_t bk_sdio_host_enable_sdio_irq(sdio_host_id_t id, bool enable)
{
	if (id >= SDIO_HOST_ID_MAX)
		return BK_ERR_PARAM;
	sdio_host_lock();
	sdio_host_select(id);
	if (enable) {
		NORMAL_INT_STAT_EN_R(s_active_base)   |= CARD_INTERRUPT_STAT_EN;
		NORMAL_INT_SIGNAL_EN_R(s_active_base) |= CARD_INTERRUPT_SIGNAL_EN;
	} else {
		NORMAL_INT_SIGNAL_EN_R(s_active_base) &= ~CARD_INTERRUPT_SIGNAL_EN;
	}
	sdio_host_unlock();
	return BK_OK;
}

/* Dispatch the async SDIO card-interrupt to the registered callback. Called
 * from the controller ISR when a CARD_INTERRUPT event fires. */
void sdio_host_dispatch_card_irq(void)
{
	sdio_host_inst_t *inst = &s_host_inst[s_active_id];
	if (inst->sdio_irq_cb)
		inst->sdio_irq_cb(inst->sdio_irq_arg);
}

bk_err_t sdio_dwc_interrupt_init(void)
{
	SDIOD_LOGI("sdio_dwc_interrupt_init \r\n");
		bk_int_isr_register(INT_SRC_SDIO0, (int_group_isr_t)sdio_dwc_isr0, NULL);
		bk_int_isr_register(INT_SRC_SDIO1, (int_group_isr_t)sdio_dwc_isr0, NULL);	//just chip verify

#if CONFIG_SDIO_DWC_TEST
		int bk_sdio_host_register_cli_test_feature(void);
		bk_sdio_host_register_cli_test_feature();
#endif

		return BK_OK;
}

bk_err_t sdio_dwc_interrupt_deinit(void)
{
		bk_int_isr_unregister(INT_SRC_SDIO0);
		bk_int_isr_unregister(INT_SRC_SDIO1);

		return BK_OK;
}
