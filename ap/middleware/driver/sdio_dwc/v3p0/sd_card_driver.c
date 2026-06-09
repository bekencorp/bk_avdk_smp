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

#include <os/mem.h>
#include <os/os.h>
#include <driver/sdio_host.h>
#include <driver/sd_card.h>
#include "sd_card_driver.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/io_matrix.h>
#include <driver/hal/hal_gpio_types.h>
#include <hal/sys_hal.h>
#include "sdio_storage_driver.h"
//#include "sys_a35_ll.h"
//#include "sys_ana_ll.h"
//#include <driver/gicv2.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include <os/mem.h>
#include "cache.h"
#include "sys_driver.h"

#define INIT_400K	 1
#define MAX_WAIT_STATE_TRANS_TIMES	200

/* Returned by send_cmd() when the CMD line timed out (no response from card). */
#define SDIO_CMD_TIMEOUT_RESP        0xFFFFFFFFu
/* Max time waiting for the PSTATE_REG card-detect logic to stabilize on boot. */
#define SDIO_CARD_STABLE_TIMEOUT_MS  50
/* Number of times we re-issue CMD0+CMD8 during sd_card_init() before declaring
 * the slot empty. Covers warm-reboot cases where the card is still finishing
 * an internal op from the previous power session and swallows the first CMD0. */
#define SD_INIT_HANDSHAKE_RETRY      4

/* PSTATE_REG bit definitions not provided by mshc_regs.h. */
#ifndef CARD_STATE_STABLE
#define CARD_STATE_STABLE            BIT(17)
#endif
#ifndef CARD_DETECT_PIN_LEVEL
#define CARD_DETECT_PIN_LEVEL        BIT(18)
#endif


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
uint32_t sdio_rca = 0xAAAA0000;


static beken_semaphore_t s_sdio_cmd_done_sema = NULL;
static beken_semaphore_t s_sdio_wr_buf_ready_sema = NULL;
static beken_semaphore_t s_sdio_rd_buf_ready_sema = NULL;
static beken_semaphore_t s_sdio_data_xfer_done_sema = NULL;


typedef enum
{
	SDCARD_OPS_READ,
	SDCARD_OPS_WRITE,
	SDCARD_OPS_SYNC_RW,	//sync read or write
}sdcard_rw_ops_t;

typedef enum
{
	SDCARD_RW_STATE_INITIALIZED,
	SDCARD_RW_STATE_READING,
	SDCARD_RW_STATE_WRITING,
	SDCARD_RW_STATE_ENDED,
}sdcard_rw_state_t;

typedef struct {
	sd_card_info_t sd_card; /**< sd card information */
	uint32_t cid[4]; /**< sd card CID register, it contains the card identification information */
	sd_card_csd_t csd;
	sdio_host_clock_freq_t clock_freq;
} sd_card_obj_t;

typedef enum
{
	SDIO_WIRE_WIDTH_SEL_1,
	SDIO_WIRE_WIDTH_SEL_4,	//4-wire
	SDIO_WIRE_WIDTH_SEL_8,	//8-wire
}sdio_wire_width_sel_t;

#define CONFIG_SDCARD_OPS_TRACE_EN (0)
#if (CONFIG_SDCARD_OPS_TRACE_EN)
typedef struct
{
	uint32_t ops	: 3;	//last operation, init:1, deinit:2, read:3, write:4
	uint32_t init	: 4;	//init step trace
	uint32_t deinit	: 4;
	uint32_t read 	: 3;
	uint32_t write 	: 3;
}sd_card_sw_status_t;
static volatile sd_card_sw_status_t s_sdcard_sw_status;
#endif
static bool s_sd_card_is_init = false;
/* Set by send_cmd() when the last command timed out on the CMD line.
 * sd_card_init() polls this between commands to implement fail-fast. */
static volatile bool s_last_cmd_timeout = false;

static sd_card_obj_t s_sd_card_obj = {0};

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


static sdcard_rw_state_t sd_card_check_continious_rw(sdcard_rw_ops_t ops, uint32_t addr, uint32_t blk_cnt);


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
	uint32_t int_level = rtos_disable_int();

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

	return resp01;
}

bk_err_t sd_card_init(uintptr_t addr)
{
	bk_err_t ret = BK_OK;
	uint32_t resp[4] = {0};
	uint32_t retry_cnt = 0;

	/* ----  1: SDHCI hardware card-detect pre-check ----
	 * Wait for CARD_STATE_STABLE then read CARD_INSERTED. If the slot CD
	 * pin is not routed to the SDIO controller this check is unreliable
	 * (the bit may be hard-wired 0 or 1) - fail-fast in send_cmd() is
	 * the second line of defense.
	 */
	{
		uint32_t pstate = 0;
		int i;
		for (i = 0; i < SDIO_CARD_STABLE_TIMEOUT_MS; i++) {
			pstate = PSTATE_REG_R(addr);
			if (pstate & CARD_STATE_STABLE) {
				break;
			}
			rtos_delay_milliseconds(1);
		}
		if (!(pstate & CARD_INSERTED)) {
#if CONFIG_SDCARD_CHECK_INSERTION_EN
			SDIOD_LOGW("no card present (PSTATE=0x%08x), skip sd init\r\n",
				   pstate);
			return BK_FAIL;
#else
			SDIOD_LOGW("card-detect bit is low (PSTATE=0x%08x), continue init because insertion check is disabled\r\n",
				   pstate);
#endif
		}
	}

	/* ----  2: SD power-on / 74-clock wait ----
	 *
	 * Root cause of probabilistic mount failure on power-on:
	 *   sdio_host_init() -> mshc_host_init() enables SD bus power
	 *   (PWR_CTRL_R.SD_BUS_PWR_VDD1) and SD_CLK output (CLK_CTRL_R.
	 *   SD_CLK_EN at ~200kHz) and immediately falls through to
	 *   sd_card_init() which sends CMD0 with no further wait.
	 *
	 *   The SD card's internal power-on reset (POR) doesn't start
	 *   until SD_CLK is driven; its completion time depends on the
	 *   card's internal LDO / oscillator ramp (0~10ms, observed up
	 *   to ~100ms on some SD-NAND parts after a hard power cycle).
	 *   If CMD0 arrives before POR finishes the card silently drops
	 *   the command and the host sees a CMD line timeout. The 30-50us
	 *   DAT1 toggle observed on a logic analyzer right after the
	 *   failed CMD0/CMD8 is the card's IO buffer settling during POR
	 *   (DAT lines are not yet stably pulled high).
	 *
	 *   SD Physical Layer Spec 7.10 6.4.1 mandates >=74 SD_CLK
	 *   cycles after VDD ramp-up before the first CMD; 50ms
	 *   comfortably covers that (~370us at 200kHz) plus typical
	 *   POR time observed on SD-NAND parts (up to ~100ms worst).
	 *   Linux mmc-host mmc_power_up() uses a similar policy. 
	 *
	 * ----  3: fail-fast on CMD line timeout ----
	 * send_cmd() sets s_last_cmd_timeout=true and performs SW_RST_CMD
	 * when the controller reports CMD_TOUT_ERR. We bail out of init on
	 * the very first timeout so we never spin in the ACMD41 retry loop
	 * when no card is responding.
	 */
	/* 50ms POR window. On a hard power cycle the card's internal LDO
	 * /oscillator can take up to ~100ms to stabilize on some SD-NAND
	 * parts; on a warm reboot the card's VDD never drops (EVB nSD is
	 * hard-wired to 3V3) but SD_CLK was interrupted while the SoC reset
	 * its GPIOs, so the card may have entered a low-power state and
	 * needs >=8 SD_CLK cycles + several ms to re-stabilize. 50ms covers
	 * both cases plus comfortable margin over the 74-clock minimum
	 * (~370us @ 200kHz). */
	rtos_delay_milliseconds(50);

	/* CMD0 -> CMD8 -> ACMD41 handshake with whole-sequence retry.
	 *
	 * On warm reboot the card's VDD is never removed (EVB nSD is hard-
	 * wired to 3V3), so the card may still be in transfer/data state
	 * from the previous power session, finishing an internal program/
	 * erase op, or have its bus state machine half-initialized. CMD0
	 * itself (GO_IDLE_STATE, no response) just clocks the bus; the
	 * earliest signal "card is actually alive" is CMD8's R7 reply.
	 *
	 * Empirically we observe that even *after* CMD8 succeeds (~17ms
	 * after a burst of CMD0s), the very next CMD55/CMD41 sometimes
	 * times out: the card's CMD line state machine accepted CMD8 but
	 * is not yet ready for ACMD41 because it is still doing internal
	 * power/clock domain bring-up. Treating CMD55/41 timeout as a
	 * hard failure aborts init and loses the recovery opportunity.
	 *
	 * Fix: wrap the entire CMD0+CMD8+ACMD41 sequence in an outer retry
	 * loop. Any timeout (CMD8 or CMD55/41) restarts the whole sequence
	 * from a fresh batch of CMD0s. Linux's mmc-core does effectively
	 * the same via mmc_attach_sd() -> mmc_send_app_op_cond() retries.
	 *
	 * We burst three CMD0s before each attempt to guarantee >=300us of
	 * bus activity (each CMD0 ~240us at 200kHz with its 80-clock dummy)
	 * so cards that swallow the first CMD0 (still busy) catch a later
	 * one. */
	{
		uint32_t i;
		bool acmd41_done = false;
		for (i = 0; i < SD_INIT_HANDSHAKE_RETRY; i++) {
			uint32_t j;
			for (j = 0; j < 3; j++) {
				send_cmd(addr, CMD0, 0, 0); //send CMD0
				rtos_delay_milliseconds(2);
			}

			send_cmd(addr, CMD8, 2, 0x1aa); //send CMD8
			rtos_delay_milliseconds(1);
			if (s_last_cmd_timeout) {
				SDIOD_LOGW("CMD8 timeout, retry %u/%u\r\n",
					   i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}

			/* CMD8 ok: try the ACMD41 sequence. First ACMD41 uses
			 * voltage-window arg, subsequent retries use HCS-only
			 * continuation arg matching the original code. */
			send_cmd(addr, CMD55, 2, 0); //CMD55
			rtos_delay_milliseconds(1);
			if (s_last_cmd_timeout) {
				SDIOD_LOGW("CMD55(first) timeout, retry %u/%u\r\n",
					   i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}
			resp[0] = send_cmd(addr, CMD41, 2, (0xff8000|0x40000000)); //ACMD41
			if (s_last_cmd_timeout) {
				SDIOD_LOGW("ACMD41(first) timeout, retry %u/%u\r\n",
					   i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}

			/* ACMD41 busy-wait. Keep using the FULL voltage window
			 * (0xff8000 = 2.7-3.6V) for every retry - the original
			 * code shrank it to a single bit (0x200000 ≈ 3.5-3.6V)
			 * on retries, which some SD cards refuse and they then
			 * never set the busy-done bit. Linux mmc_send_op_cond()
			 * passes the same OCR mask on every retry. HCS=bit30
			 * indicates host supports SDHC.
			 *
			 * Spec allows up to 1s for the card to finish power-up
			 * after the first ACMD41 with a valid voltage window;
			 * 1000 iterations * ~7ms = ~7s gives generous margin
			 * for SD-NAND and industrial cards that boot slowly. */
			retry_cnt = 0;
			while (!((SDIO_CMD_TIMEOUT_RESP != resp[0]) && ((resp[0] >> 31) & 0x01))) {
				rtos_delay_milliseconds(5);
				send_cmd(addr, CMD55, 2, 0); //CMD55
				rtos_delay_milliseconds(1);
				if (s_last_cmd_timeout) break;
				resp[0] = send_cmd(addr, CMD41, 2, (0xff8000|0x40000000)); //ACMD41 continuation
				if (s_last_cmd_timeout) break;
				if (retry_cnt++ > 1000) {
					SDIOD_LOGE("ACMD41 busy wait timed out (resp=0x%x)\r\n", resp[0]);
					return BK_FAIL;
				}
			}
			if (s_last_cmd_timeout) {
				SDIOD_LOGW("ACMD41(busy-wait) timeout, retry %u/%u\r\n",
					   i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}

			if (i > 0) {
				SDIOD_LOGI("ACMD41 done after %u sequence retries\r\n", i);
			}
			acmd41_done = true;
			break;
		}
		if (!acmd41_done) goto no_card;
	}
	s_sd_card_obj.sd_card.card_type = ((resp[0] >> 30) & 1) ? SD_CARD_TYPE_SDHC_SDXC : SD_CARD_TYPE_SDSC;
	rtos_delay_milliseconds(1);

	resp[0] = send_cmd(addr, CMD2, 1, 0); //send CMD2
	if (s_last_cmd_timeout) goto no_card;
	s_sd_card_obj.cid[0] = RESP01_R(addr);
	s_sd_card_obj.cid[1] = RESP23_R(addr);
	s_sd_card_obj.cid[2] = RESP45_R(addr);
	s_sd_card_obj.cid[3] = RESP67_R(addr);
	rtos_delay_milliseconds(1);

	resp[0] = send_cmd(addr, CMD3, 2, 0); //send CMD3
	rtos_delay_milliseconds(1);
	if (s_last_cmd_timeout) goto no_card;

	sdio_rca = resp[0] & 0xFFFF0000;
	resp[0] = send_cmd(addr, CMD9, 1, sdio_rca);	//send CMD9
	if (s_last_cmd_timeout) goto no_card;
#if 1
	s_sd_card_obj.csd.csd_3.v = RESP01_R(addr);
	s_sd_card_obj.csd.csd_2.v = RESP23_R(addr);
	s_sd_card_obj.csd.csd_1.v = RESP45_R(addr);
	s_sd_card_obj.csd.csd_0.v = RESP67_R(addr);
	SD_CARD_LOGD("csd[0]=0x%x, csd[1]=0x%x, csd[2]=0x%x, csd[3]=0x%x\r\n",
		s_sd_card_obj.csd.csd_0.v,s_sd_card_obj.csd.csd_1.v,s_sd_card_obj.csd.csd_2.v,s_sd_card_obj.csd.csd_3.v);
#else
s_sd_card_obj.csd.csd_0.v = RESP01_R(addr);
s_sd_card_obj.csd.csd_1.v = RESP23_R(addr);
s_sd_card_obj.csd.csd_2.v = RESP45_R(addr);
s_sd_card_obj.csd.csd_3.v = RESP67_R(addr);
SD_CARD_LOGD("csd[0]=0x%x, csd[1]=0x%x, csd[2]=0x%x, csd[3]=0x%x\r\n",
	s_sd_card_obj.csd.csd_0.v,s_sd_card_obj.csd.csd_1.v,s_sd_card_obj.csd.csd_2.v,s_sd_card_obj.csd.csd_3.v);
#endif
	rtos_delay_milliseconds(1);

	resp[0] = send_cmd(addr, CMD7, 3, sdio_rca);	//send CMD7
	rtos_delay_milliseconds(1);
	if (s_last_cmd_timeout) goto no_card;

	return ret;

no_card:
	SDIOD_LOGW("sd init aborted: no card response (CMD timeout)\r\n");
	return BK_FAIL;
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


sd_card_state_t bk_sd_card_get_card_state(void)
{
	sd_card_state_t card_state = SD_CARD_TRANSFER;
	uint32_t resp1 = 0;

	/* Send status command(CMD13) */
	resp1 = send_cmd(SDIO_ACTIVE_BASE, CMD13, 2, sdio_rca);

	/* The response format R1 contains a 32-bit field named card status.
	 * BIT[12:9] current_state
	 */
	card_state = (sd_card_state_t)((resp1 >> 0x9) & 0x0f);

	return card_state;
}


bk_err_t send_mult_data(uintptr_t addr, const uint8_t *data, uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
	uint32 num=0;
	uint16 block_num =0;

	sd_card_clear_transfer_error_flags();

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
		ret = rtos_get_semaphore(&s_sdio_wr_buf_ready_sema, 2000);
		if((ret != 0) || sd_card_has_transfer_error()) {
			sd_card_log_transfer_wait_error(__func__, "s_sdio_wr_buf_ready_sema", ret);
			return BK_FAIL;
		}
		while(num<(128*(block_num+1)))
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

	sd_card_clear_transfer_error_flags();

	NORMAL_INT_STAT_EN_R(addr) = CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_RD_READY_STAT_EN;
	ERROR_INT_STAT_EN_R(addr) = 0x870;
	NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_RD_READY_STAT_EN;
	ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

	BLOCKSIZE_R(addr) = BLOCK_SIZE;
	BLOCKCOUNT_R(addr)= BLOCK_CNT;
	ARGUMENT_R(addr)  = ARGUMENT;

	XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN;//0xb2;
	CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;


	int ret = 0;
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, 2000);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_cmd_done_sema", ret);
		return BK_FAIL;
	}

	for (block_num=0; block_num<BLOCK_CNT; block_num++)
	{
		ret = rtos_get_semaphore(&s_sdio_rd_buf_ready_sema, 2000);
		if((ret != 0) || sd_card_has_transfer_error()) {
			sd_card_log_transfer_wait_error(__func__, "s_sdio_rd_buf_ready_sema", ret);
			return BK_FAIL;
		}

		while(num<128*(block_num+1))
		{
			read_data=BUF_DATA_R(addr);
			sd_card_store_unaligned_le32(data + (num << 2), read_data);
			//SDIOD_LOGI("Receive: BUF_DATA = %x, num =%x \r\n",read_data,num);
			num = num+1;
		}
	}

	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 2000);
	if((ret != 0) || sd_card_has_transfer_error()) {
		sd_card_log_transfer_wait_error(__func__, "s_sdio_data_xfer_done_sema", ret);
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t adma2_send_data(uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
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
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, 2000);
	if(ret != 0) {
		SDIOD_LOGE("func %s get sem s_sdio_cmd_done_sema timeout\r\n", __func__);
	}

	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 2000);
	if(ret != 0) {
		SDIOD_LOGE("func %s get sem s_sdio_data_xfer_done_sema timeout\r\n", __func__);
	}
	SDIOD_LOGD("*****Data(ADMA2) Write End*****\r\n");


	return ret;
}

bk_err_t adma2_receive_data (uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
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
	ret = rtos_get_semaphore(&s_sdio_cmd_done_sema, 2000);
	if(ret != 0) {
		SDIOD_LOGE("func %s: get sem s_sdio_cmd_done_sema timeout\r\n", __func__);
	}
	ret = rtos_get_semaphore(&s_sdio_data_xfer_done_sema, 2000);
	if(ret != 0) {
		SDIOD_LOGE("func %s: get sem s_sdio_data_xfer_done_sema timeout\r\n", __func__);
	}

	SDIOD_LOGD("*****Data(ADMA2) Read End*****\r\n");

	return ret;
}


#define  GPIO_CFG(port)	*((volatile unsigned int *) (0x44000400+port*4))

void sdio_gpio_init(uint8_t io_pos, sdio_wire_width_sel_t width_sel)
{
	(void)io_pos;
	(void)width_sel;
}

void tuning_cfg(uintptr_t addr,uint8 tuning_rx_sel0,uint8 tuning_rx_sel1,uint8 sample_rx_sel0,uint8 sample_rx_sel1,uint8 tuning_tx_sel0,uint8 tuning_tx_sel1,uint8 sample_tx_sel0,uint8 sample_tx_sel1,uint8 clk_drv_inv_sel)
{
	#if 0	//TODO:V1 chip uses two steps tuning, V2 chip uses one step tuning
	sdio_reg5(addr) |= tuning_rx_sel0 <<5 | tuning_rx_sel1<<8 | sample_rx_sel0 <<14 | sample_rx_sel1 <<15;
	sdio_reg5(addr) |= tuning_tx_sel0 <<17 | tuning_tx_sel1<<20 | sample_tx_sel0 <<26 | sample_tx_sel1 <<27;
	sdio_reg5(addr) |= clk_drv_inv_sel << 29;
	#else
	sdio_reg5(addr) |= (tuning_rx_sel0 & 0x0f) <<5 | sample_rx_sel0 <<14;
	sdio_reg5(addr) |= (tuning_tx_sel0 & 0x0f) <<17 | sample_tx_sel0 <<26;
	sdio_reg5(addr) |= clk_drv_inv_sel << 29;
	#endif
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
		ret = sd_card_init(addr);
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
#if SDIO_VERIFY_USE_SDIO1
	sys_hal_sdio1_set_cken(0);
	sys_hal_sdio1_set_cken(1);
#else
	sys_hal_sdio0_set_cken(0);
	sys_hal_sdio0_set_cken(1);
#endif
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

	sdio_gpio_init(0, SDIO_WIRE_WIDTH_SEL_1);
	PWR_CTRL_R(SDIO_ACTIVE_BASE) = 0x01;

	//tuning_cfg(SDIO_ACTIVE_BASE,0x0,0x0,0,0,0x0,0x0,0,0,1);
	tuning_cfg(SDIO_ACTIVE_BASE,0x0,0x0,0,0,0x0,0x0,0,0,0);	//TODO: V2 chip positive edge tuning

	ret = mshc_host_init(SDIO_ACTIVE_BASE,0x3,300,0xff,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH1);	  //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
	if(BK_OK != ret)
		return ret;

	rtos_delay_milliseconds(1);

	send_cmd(SDIO_ACTIVE_BASE, CMD55, 2, sdio_rca);
	rtos_delay_milliseconds(1);
	//send_cmd(sdio_mshc_1_base, CMD6, 2, 2); //send CMD6 4 line
	send_cmd(SDIO_ACTIVE_BASE, CMD6, 2, 0); //send CMD6 1 line
	rtos_delay_milliseconds(1);

	// sdio_gpio_init(0, SDIO_WIRE_WIDTH_SEL_4);
	// HOST_CTRL1_R(SDIO_ACTIVE_BASE) = (HOST_CTRL1_R(SDIO_ACTIVE_BASE) & ~((1<<1)|(1<<5))) | (1 << 1);

	send_cmd(SDIO_ACTIVE_BASE, CMD16, 1, 0x200);	//send CMD16 set block size
	rtos_delay_milliseconds(1);
	sd_card_interface_set(SDIO_ACTIVE_BASE,UHS_MODE_SDR50);
	sd_clk_change(SDIO_ACTIVE_BASE, 3);  //时钟分频: 80/(3+1)=20MHz
	rtos_delay_milliseconds(1);

	return ret;
}

void sdio_switch_wire_width(sdio_wire_width_sel_t width_sel)
{
	sdio_gpio_init(0, width_sel);
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
		rtos_set_semaphore(&s_sdio_cmd_done_sema);
		//SDIOD_LOGD("CMD_COMP\r\n");
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_CMD_COMPLETE_STAT;
	}
	if(normal_int & XFER_COMPLETE_STAT_EN)
	{
		//XFER_COMPLETE_STATE = 1;
		//SDIOD_LOGD("XFER_COMP\r\n");
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
		rtos_set_semaphore(&s_sdio_wr_buf_ready_sema);
		NORMAL_INT_STAT_R(SDIO_ACTIVE_BASE)= CLR_BUF_WR_READY_STAT;
	}
	if(normal_int & BUF_RD_READY_STAT_EN)
	{
		//BUF_RD_READY_STATE = 1;
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
}

bk_err_t bk_sdio_storage_driver_init(void)
{
	SDIOD_LOGI("bk_sdio_storage_driver_init \r\n");
		bk_int_isr_register(INT_SRC_SDIO0, (int_group_isr_t)sdio_dwc_isr0, NULL);
		bk_int_isr_register(INT_SRC_SDIO1, (int_group_isr_t)sdio_dwc_isr0, NULL);	//just chip verify

#if CONFIG_SDIO_DWC_TEST
		int bk_sdio_host_register_cli_test_feature(void);
		bk_sdio_host_register_cli_test_feature();
#endif

		return BK_OK;
}

bk_err_t bk_sdio_storage_driver_deinit(void)
{
		bk_int_isr_unregister(INT_SRC_SDIO0);
		bk_int_isr_unregister(INT_SRC_SDIO1);

		return BK_OK;
}



bk_err_t bk_sd_card_init(void)
{
	bk_err_t ret = BK_OK;

	if (s_sd_card_is_init) {
		SDIOD_LOGI("sd card has inited\r\n");
		return BK_OK;
	}


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

	ret = sdio_host_init();
	if(ret)
		return ret;

	s_sd_card_is_init = true;
	return BK_OK;
}

bk_err_t bk_sd_card_deinit(void)
{
	uint32_t ret = 0;
	sdio_reset(); //reset


	s_sd_card_is_init = false;


	if(s_sdio_cmd_done_sema)
	{
		ret = rtos_deinit_semaphore(&s_sdio_cmd_done_sema);
		if (kNoErr != ret)
		{
			SDIOD_LOGE("s_sdio_cmd_done_sema deinit\r\n");
		}
		s_sdio_cmd_done_sema = NULL;
	}

	if(s_sdio_wr_buf_ready_sema)
	{
		ret = rtos_deinit_semaphore(&s_sdio_wr_buf_ready_sema);
		if (kNoErr != ret)
		{
			SDIOD_LOGE("s_sdio_wr_buf_ready_sema deinit\r\n");
		}
		s_sdio_wr_buf_ready_sema = NULL;
	}

	if(s_sdio_rd_buf_ready_sema)
	{
		ret = rtos_deinit_semaphore(&s_sdio_rd_buf_ready_sema);
		if (kNoErr != ret)
		{
			SDIOD_LOGE("s_sdio_rd_buf_ready_sema deinit\r\n");
		}
		s_sdio_rd_buf_ready_sema = NULL;
	}

	if(s_sdio_data_xfer_done_sema)
	{
		ret = rtos_deinit_semaphore(&s_sdio_data_xfer_done_sema);
		if (kNoErr != ret)
		{
			SDIOD_LOGE("s_sdio_data_xfer_done_sema deinit\r\n");
		}
		s_sdio_data_xfer_done_sema = NULL;
	}

	return BK_OK;
}



bk_err_t bk_sd_card_write_blocks(const uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t addr = (s_sd_card_obj.sd_card.card_type == SD_CARD_TYPE_SDSC) ? (block_addr << 9) : block_addr;
	//CPU buffer write
	bk_err_t ret = send_mult_data(SDIO_ACTIVE_BASE, data, 0x200, block_num,25,addr);

	//SDMA write
	
	//ADMA2 write
	//arch_dcache_flush_and_invd_range((void*)((uintptr_t)data - 64), block_num*512 + 128);
	//adma2_send_data(SDIO_ACTIVE_BASE,(uintptr_t)data,0x200,block_num,CMD25,block_addr);

	return ret;
}

bk_err_t bk_sd_card_read_blocks(uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t addr = (s_sd_card_obj.sd_card.card_type == SD_CARD_TYPE_SDSC) ? (block_addr << 9) : block_addr;
	//CPU buffer read
	bk_err_t ret = receive_mult_data(SDIO_ACTIVE_BASE, data, 0x200,block_num,18,addr);


	return ret;
}


bk_err_t bk_sd_card_get_card_info(sd_card_info_t *card_info)
{
	*card_info = s_sd_card_obj.sd_card;
	return BK_OK;
}

/* size unit: sector counts,default sector size is 512 bytes */
uint32_t bk_sd_card_get_card_size(void)
{
	sd_card_csd_t *csd_p = (sd_card_csd_t *)&s_sd_card_obj.csd;
	uint32_t ver = 0, size = 0;

	// #region agent log CSD parse diagnostics (H-A)
	uint32_t csd_struct_raw = csd_p->csd_3.csd_structure;
	uint32_t card_type = s_sd_card_obj.sd_card.card_type;
	SD_CARD_LOGI("[DBG] csd_structure=%d, card_type=%d (0=SDSC,1=SDHC)\r\n", csd_struct_raw, card_type);
	// #endregion

	/*
	 * SD NAND chips may report wrong csd_structure (e.g. v3.0) while
	 * ACMD41 CCS=0 indicates SDSC.  CCS from OCR is authoritative for
	 * capacity class, so force v1.0 parsing for SDSC cards.
	 */
	if (card_type == SD_CARD_TYPE_SDSC) {
		ver = 1;
		if (csd_struct_raw != 0) {
			SD_CARD_LOGW("SDSC(CCS=0) but csd_structure=%d, force v1.0 parse\r\n", csd_struct_raw);
		}
	} else {
		ver = csd_struct_raw + 1;
	}

	switch(ver)
	{
		case 1:	//ver1.0
		{
			uint32_t c_size, c_size_mul, block_nr, read_bl_len;
			c_size = (csd_p->csd_2.v1p0.c_size_high<<2) + csd_p->csd_1.v1p0.c_size_low;
			c_size_mul = 1 << (csd_p->csd_1.v1p0.c_size_mult + 2);
			block_nr = (c_size + 1) * c_size_mul;
			size = block_nr;

			read_bl_len = csd_p->csd_2.v1p0.read_bl_len;
			// #region agent log v1.0 parse details (H-A, H-B)
			SD_CARD_LOGI("[DBG] v1.0: c_size=%u, c_size_mult=%u, block_nr=%u, read_bl_len=%u\r\n",
				c_size, csd_p->csd_1.v1p0.c_size_mult, block_nr, read_bl_len);
			// #endregion
			if (read_bl_len > 9 && read_bl_len <= 11) {
				size = block_nr << (read_bl_len - 9);
				SD_CARD_LOGW("card ver=%d.0,block_len=%d != 512bytes\r\n", ver, 1 << read_bl_len);
			} else if (read_bl_len > 11) {
				SD_CARD_LOGW("SDSC: invalid read_bl_len=%d (>11), treat as 9\r\n", read_bl_len);
			}

			break;
		}

		case 2:	//ver2.0:(c_size:22bits == c_size_low << 16 + c_size_high), card_size == (c_size + 1) * 512 K bytes
		{
			size = ((csd_p->csd_2.v2p0.c_size_high<<16) + (csd_p->csd_1.v2p0_v3p0.c_size_low) + 1) << 10;
			break;
		}

		case 3:	//3.0:(c_size:28bits == c_size_low << 16 + c_size_high), card_size == (c_size + 1) * 512 K bytes
		{
			size = ((csd_p->csd_2.v3p0.c_size_high<<16) + (csd_p->csd_1.v2p0_v3p0.c_size_low) + 1) << 10;
			break;
		}

		default:
			SD_CARD_LOGE("card ver=%d.0\r\n", ver);
			break;
	}

	SD_CARD_LOGI("card ver=%d.0,size:0x%08x sector(sector=512bytes)\r\n", ver, (uint32_t)size);
	// #region agent log final capacity (H-C)
	SD_CARD_LOGI("[DBG] final capacity: %u sectors = %u MB\r\n", size, size / 2048);
	// #endregion
	return size;
}