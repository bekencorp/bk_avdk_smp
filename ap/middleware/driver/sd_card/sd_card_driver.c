// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * SD memory card protocol driver.
 *
 * This layer implements the SD card identification/transfer state machine using
 * the generic SDIO host controller interface (<driver/sdio_host.h>). It does
 * not touch controller registers, controller private headers or controller base
 * addresses directly, so it is decoupled from the underlying DesignWare MSHC
 * host driver and from which of the two BK7259 controllers the card happens to
 * be wired to.
 */

#include <os/os.h>
#include <driver/gpio.h>
#include <driver/sdio_host.h>
#include <driver/sd_card.h>
#include "sd_card_driver.h"
#include "gpio_driver.h"

/* Which physical SDIO host controller the SD card is wired to. The single
 * source of truth is CONFIG_SDCARD_HOST_ID (0 = SDIO0, 1 = SDIO1), an int
 * Kconfig that always has a value when SDCARD is enabled. The selection is
 * applied at runtime by passing SDCARD_HOST_ID to the generic bk_sdio_host_*()
 * API. */
#if defined(CONFIG_SDCARD_HOST_ID)
#define SDCARD_HOST_ID   ((sdio_host_id_t)CONFIG_SDCARD_HOST_ID)
#else
#define SDCARD_HOST_ID   SDIO_HOST_ID_0
#endif

/* Board card-detect predicate registered via bk_sd_card_set_present_cb() (see
 * sd_card.h). NULL means no board hook: presence is treated as "unknown" and the
 * stack keeps the original retry behavior. A board registers a callback that
 * reads the actual SDCD GPIO. */
static bk_sd_card_present_cb_t s_present_cb = NULL;

bk_err_t bk_sd_card_set_present_cb(bk_sd_card_present_cb_t cb)
{
	s_present_cb = cb;
	return BK_OK;
}

uint8_t bk_sd_card_is_present(void)
{
	return s_present_cb ? s_present_cb() : 1;
}

#define SDIO_CARD_STABLE_TIMEOUT_MS  50
/* Number of times we re-issue CMD0+CMD8 during sd_card_identify() before
 * declaring the slot empty. Covers warm-reboot cases where the card is still
 * finishing an internal op from the previous power session and swallows the
 * first CMD0. */
#define SD_INIT_HANDSHAKE_RETRY      4

#define SD_BLOCK_LEN                 0x200   /* 512-byte data block */
#define SD_CARD_XFER_RETRY_CNT       3       /* local retries on transient data-phase failure */

typedef struct {
	sd_card_info_t sd_card; /**< sd card information */
	uint32_t cid[4];        /**< sd card CID register */
	sd_card_csd_t csd;      /**< sd card CSD register */
	sdio_host_clock_freq_t clock_freq;
} sd_card_obj_t;

static bool s_sd_card_is_init = false;
static sd_card_obj_t s_sd_card_obj = {0};
static uint32_t sdio_rca = 0xAAAA0000;

#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
static gpio_dev_t sd_card_data3_func(sdio_host_id_t host_id)
{
	return (host_id == SDIO_HOST_ID_1) ?
		GPIO_DEV_SDIO1_HOST_DATA3 : GPIO_DEV_SDIO_HOST_DATA3;
}

/* SD cards sample CD/DAT3 (CS) when CMD0 is received. Some boards wire an
 * on-board SD-NAND to the 4-bit bus and rely on the host DAT3 pad to idle high;
 * on BK7259 this is not always strong enough during 1-bit identification. Keep
 * the project pinmux table standard (DATA3 is configured as SDIO in
 * usr_gpio_cfg.h), but temporarily detach that pad and drive it high until the
 * card is enumerated. The pad is restored to SDIO DATA3 before ACMD6 switches
 * the card to 4-bit mode. */
static bk_err_t sd_card_force_data3_high(sdio_host_id_t host_id,
					 gpio_dev_t *data3_func)
{
	gpio_dev_t func = sd_card_data3_func(host_id);
	gpio_id_t gpio_id = gpio_get_id_by_func(func);
	bk_err_t ret;

	*data3_func = func;

	if (gpio_id >= SOC_GPIO_NUM) {
		SD_CARD_LOGE("DATA3 function %u is not configured in usr_gpio_cfg.h\r\n",
			     (unsigned)func);
		return BK_FAIL;
	}

	ret = gpio_dev_unprotect_map(gpio_id, GPIO_DEV_GPIO_OUTPUT);
	if (ret != BK_OK)
		return ret;

	return bk_gpio_set_output_high(gpio_id);
}

static bk_err_t sd_card_restore_data3_func(gpio_dev_t data3_func)
{
	if (data3_func == GPIO_DEV_INVALID)
		return BK_OK;

	return gpio_dev_map_by_func(data3_func);
}
#endif

/* Convenience wrapper: issue a command on the SD card host controller. The
 * caller-visible response code is preserved in *resp (may be NULL). */
static bk_err_t sd_send(uint8_t index, sdio_host_resp_type_t rt, uint32_t arg,
			sdio_host_resp_t *resp)
{
	sdio_host_cmd_t cmd = {
		.index = index,
		.arg = arg,
		.resp_type = rt,
	};
	return bk_sdio_host_send_cmd(SDCARD_HOST_ID, &cmd, resp);
}

static bk_err_t sd_card_identify(void)
{
	sdio_host_resp_t resp = {0};
	uint32_t ocr = 0;
	uint32_t retry_cnt = 0;

	/* ---- 1: host card-detect pre-check ---- */
	{
		int i;
		bool present = false;
		for (i = 0; i < SDIO_CARD_STABLE_TIMEOUT_MS; i++) {
			if (bk_sdio_host_card_present(SDCARD_HOST_ID)) {
				present = true;
				break;
			}
			rtos_delay_milliseconds(1);
		}
		if (!present) {
#if CONFIG_SDCARD_CHECK_INSERTION_EN
			SD_CARD_LOGW("no card present, skip sd init\r\n");
			return BK_FAIL;
#else
			SD_CARD_LOGW("card-detect bit is low, continue init because insertion check is disabled\r\n");
#endif
		}
	}

	/* ---- 2: SD power-on / 74-clock wait ----
	 *
	 * The card's internal power-on reset doesn't start until SD_CLK is
	 * driven; its completion time depends on the card's internal LDO /
	 * oscillator ramp (0~10ms, observed up to ~100ms on some SD-NAND parts
	 * after a hard power cycle). If CMD0 arrives before POR finishes the
	 * card silently drops the command and the host sees a CMD timeout.
	 * SD Physical Layer Spec mandates >=74 SD_CLK cycles after VDD ramp-up
	 * before the first CMD; 50ms comfortably covers that plus typical POR
	 * time observed on SD-NAND parts. */
	rtos_delay_milliseconds(50);

	/* ---- 2b: defensive STOP_TRANSMISSION ----
	 *
	 * On a warm reboot the card's VDD is never removed, so if a previous
	 * session aborted a multi-block read/write (CMD18/CMD25) without a
	 * STOP, the card can still be sitting in the data (sending/receiving)
	 * state and will ignore CMD0/CMD8, looking exactly like an absent card
	 * (CMD timeout) until a physical power cycle. Issue one CMD12 up front
	 * to pull such a card back to the transfer/standby state. It is expected
	 * to time out on a freshly powered card, so the result is ignored. */
	sd_send(SD_CMD_STOP_TRANSMISSION, SDIO_HOST_RESP_R1, 0, NULL); /* CMD12 */
	rtos_delay_milliseconds(2);

	/* ---- 3: CMD0 -> CMD8 -> ACMD41 handshake with whole-sequence retry ----
	 *
	 * On warm reboot the card's VDD is never removed, so the card may still
	 * be finishing an internal op or have its bus state machine half
	 * initialized. We wrap the entire CMD0+CMD8+ACMD41 sequence in an outer
	 * retry loop; any timeout restarts the whole sequence from a fresh batch
	 * of CMD0s. We burst three CMD0s before each attempt to guarantee enough
	 * bus activity so cards that swallow the first CMD0 catch a later one. */
	{
		uint32_t i;
		bool acmd41_done = false;
		for (i = 0; i < SD_INIT_HANDSHAKE_RETRY; i++) {
			uint32_t j;
			for (j = 0; j < 3; j++) {
				sd_send(SD_CMD_GO_IDLE_STATE, SDIO_HOST_RESP_NONE, 0, NULL); /* CMD0 */
				rtos_delay_milliseconds(2);
			}

			sd_send(SD_CMD_SEND_IF_COND, SDIO_HOST_RESP_R7, 0x1aa, &resp); /* CMD8 */
			rtos_delay_milliseconds(1);
			if (resp.timeout) {
				SD_CARD_LOGW("CMD8 timeout, retry %u/%u\r\n", i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}

			/* CMD8 ok: try the ACMD41 sequence with the FULL voltage
			 * window (0xff8000 = 2.7-3.6V) and HCS=bit30. */
			sd_send(SD_CMD_APP_CMD, SDIO_HOST_RESP_R1, 0, &resp); /* CMD55 */
			rtos_delay_milliseconds(1);
			if (resp.timeout) {
				SD_CARD_LOGW("CMD55(first) timeout, retry %u/%u\r\n", i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}
			sd_send(SD_CMD_SD_SEND_OP_COND, SDIO_HOST_RESP_R3, (0xff8000 | 0x40000000), &resp); /* ACMD41 */
			if (resp.timeout) {
				SD_CARD_LOGW("ACMD41(first) timeout, retry %u/%u\r\n", i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}
			ocr = resp.resp[0];

			/* ACMD41 busy-wait until the card sets the power-up done
			 * bit (OCR[31]). Spec allows up to 1s after a valid voltage
			 * window; ~1000 * 7ms gives generous margin for slow parts. */
			retry_cnt = 0;
			while (!((ocr != 0xFFFFFFFFu) && ((ocr >> 31) & 0x01))) {
				rtos_delay_milliseconds(5);
				sd_send(SD_CMD_APP_CMD, SDIO_HOST_RESP_R1, 0, &resp); /* CMD55 */
				rtos_delay_milliseconds(1);
				if (resp.timeout)
					break;
				sd_send(SD_CMD_SD_SEND_OP_COND, SDIO_HOST_RESP_R3,
					(0xff8000 | 0x40000000), &resp); /* ACMD41 */
				if (resp.timeout)
					break;
				ocr = resp.resp[0];
				if (retry_cnt++ > 1000) {
					SD_CARD_LOGE("ACMD41 busy wait timed out (ocr=0x%x)\r\n", ocr);
					return BK_FAIL;
				}
			}
			if (resp.timeout) {
				SD_CARD_LOGW("ACMD41(busy-wait) timeout, retry %u/%u\r\n", i + 1, SD_INIT_HANDSHAKE_RETRY);
				rtos_delay_milliseconds(30);
				continue;
			}

			if (i > 0)
				SD_CARD_LOGI("ACMD41 done after %u sequence retries\r\n", i);
			acmd41_done = true;
			break;
		}
		if (!acmd41_done)
			goto no_card;
	}
	s_sd_card_obj.sd_card.card_type = ((ocr >> 30) & 1) ? SD_CARD_TYPE_SDHC_SDXC : SD_CARD_TYPE_SDSC;
	rtos_delay_milliseconds(1);

	/* CMD2: ALL_SEND_CID (R2 long response) */
	if (sd_send(SD_CMD_ALL_SEND_CID, SDIO_HOST_RESP_R2, 0, &resp) != BK_OK)
		goto no_card;
	s_sd_card_obj.cid[0] = resp.resp[0];
	s_sd_card_obj.cid[1] = resp.resp[1];
	s_sd_card_obj.cid[2] = resp.resp[2];
	s_sd_card_obj.cid[3] = resp.resp[3];
	rtos_delay_milliseconds(1);

	/* CMD3: SEND_RELATIVE_ADDR (R6) */
	if (sd_send(SD_CMD_SEND_RELATIVE_ADDR, SDIO_HOST_RESP_R6, 0, &resp) != BK_OK)
		goto no_card;
	rtos_delay_milliseconds(1);
	sdio_rca = resp.resp[0] & 0xFFFF0000;

	/* CMD9: SEND_CSD (R2 long response) */
	if (sd_send(SD_CMD_SEND_CSD, SDIO_HOST_RESP_R2, sdio_rca, &resp) != BK_OK)
		goto no_card;
	s_sd_card_obj.csd.csd_3.v = resp.resp[0];
	s_sd_card_obj.csd.csd_2.v = resp.resp[1];
	s_sd_card_obj.csd.csd_1.v = resp.resp[2];
	s_sd_card_obj.csd.csd_0.v = resp.resp[3];
	SD_CARD_LOGV("csd[0]=0x%x, csd[1]=0x%x, csd[2]=0x%x, csd[3]=0x%x\r\n",
		s_sd_card_obj.csd.csd_0.v, s_sd_card_obj.csd.csd_1.v,
		s_sd_card_obj.csd.csd_2.v, s_sd_card_obj.csd.csd_3.v);
	rtos_delay_milliseconds(1);

	/* CMD7: SELECT_CARD (R1b) */
	if (sd_send(SD_CMD_SELECT_DESELECT_CARD, SDIO_HOST_RESP_R1B, sdio_rca, &resp) != BK_OK)
		goto no_card;
	rtos_delay_milliseconds(1);

	return BK_OK;

no_card:
	SD_CARD_LOGW("sd init aborted: no card response (CMD timeout)\r\n");
	return BK_FAIL;
}

static uint32_t sd_card_configured_clock_hz(void)
{
#if defined(CONFIG_SDCARD_CLOCK_FREQ_HZ)
	return (uint32_t)CONFIG_SDCARD_CLOCK_FREQ_HZ;
#elif CONFIG_SDCARD_HIGH_SPEED
	return SD_HS_DEFAULT_CLOCK_HZ;
#else
	return 20000000u;
#endif
}

static bk_err_t sd_card_apply_transfer_clock(bool high_speed)
{
	uint32_t hz = sd_card_configured_clock_hz();
	bk_err_t ret;

	if (high_speed) {
		ret = bk_sdio_host_set_timing(SDCARD_HOST_ID, SDIO_HOST_TIMING_SDR25);
		if (hz > SD_HS_MAX_CLOCK_HZ)
			hz = SD_HS_MAX_CLOCK_HZ;
	} else {
		ret = bk_sdio_host_set_timing(SDCARD_HOST_ID, SDIO_HOST_TIMING_SDR12);
		if (hz > SD_DS_MAX_CLOCK_HZ)
			hz = SD_DS_MAX_CLOCK_HZ;
	}
	if (ret != BK_OK)
		return ret;

	ret = bk_sdio_host_set_clock(SDCARD_HOST_ID, hz);
	if (ret != BK_OK)
		return ret;

	SD_CARD_LOGI("%s, clock=%u Hz\r\n",
		     high_speed ? "High Speed" : "Default Speed", (unsigned)hz);
	return BK_OK;
}

#if CONFIG_SDCARD_HIGH_SPEED
typedef enum {
	SD_HS_SWITCH_NOT_ENABLED = 0,
	SD_HS_SWITCH_ENABLED,
	SD_HS_SWITCH_INDETERMINATE,
} sd_hs_switch_result_t;

static bk_err_t sd_card_cmd6(uint32_t arg, uint8_t *status)
{
	sdio_host_cmd_t cmd = {
		.index = SD_CMD_SWITCH_FUNC,
		.arg = arg,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_READ,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = status,
		.block_size = SD_SWITCH_STATUS_SIZE,
		.block_cnt = 1,
	};
	sdio_host_resp_t resp = {0};
	bk_err_t ret;

	ret = bk_sdio_host_xfer(SDCARD_HOST_ID, &cmd, &xfer, &resp);
	if (ret != BK_OK)
		return ret;
	if (resp.timeout || resp.crc_err ||
	    (resp.resp[0] & SD_R1_CMD6_ERROR_MASK)) {
		SD_CARD_LOGW("CMD6 R1 error: status=0x%08x timeout=%u crc=%u\r\n",
			     (unsigned)resp.resp[0], (unsigned)resp.timeout,
			     (unsigned)resp.crc_err);
		return BK_FAIL;
	}
	return BK_OK;
}

/* CMD6 must complete while the bus is still at Default Speed (<=25MHz). */
static sd_hs_switch_result_t sd_card_switch_high_speed(void)
{
	uint8_t status[SD_SWITCH_STATUS_SIZE] = {0};
	uint32_t retry;

	for (retry = 0; retry < SD_SWITCH_BUSY_RETRY_CNT; retry++) {
		if (sd_card_cmd6(SD_SWITCH_FUNC_CHECK_HS, status) != BK_OK) {
			SD_CARD_LOGW("CMD6 High Speed check failed\r\n");
			return SD_HS_SWITCH_NOT_ENABLED;
		}
		if ((status[13] & SD_SWITCH_GROUP1_HS_SUPPORT) == 0) {
			SD_CARD_LOGW("card does not support High Speed\r\n");
			return SD_HS_SWITCH_NOT_ENABLED;
		}
		if ((status[43] & SD_SWITCH_GROUP1_HS_BUSY) == 0)
			break;
		rtos_delay_milliseconds(1);
	}
	if (retry == SD_SWITCH_BUSY_RETRY_CNT) {
		SD_CARD_LOGW("card High Speed function remains busy\r\n");
		return SD_HS_SWITCH_NOT_ENABLED;
	}

	if (sd_card_cmd6(SD_SWITCH_FUNC_SET_HS, status) != BK_OK) {
		SD_CARD_LOGE("CMD6 High Speed set result is indeterminate\r\n");
		return SD_HS_SWITCH_INDETERMINATE;
	}
	if ((status[16] & SD_SWITCH_GROUP1_FUNC_MASK) != SD_SWITCH_GROUP1_FUNC_HS) {
		SD_CARD_LOGW("CMD6 High Speed not selected (group1=%u)\r\n",
			     (unsigned)(status[16] & SD_SWITCH_GROUP1_FUNC_MASK));
		return SD_HS_SWITCH_NOT_ENABLED;
	}

	/* Spec: wait at least 8 clocks after a successful switch. */
	rtos_delay_milliseconds(1);
	return SD_HS_SWITCH_ENABLED;
}
#endif

sd_card_state_t bk_sd_card_get_card_state(void)
{
	sdio_host_resp_t resp = {0};

	/* CMD13: SEND_STATUS (R1). Card status BIT[12:9] = current_state. */
	sd_send(SD_CMD_SEND_STATUS, SDIO_HOST_RESP_R1, sdio_rca, &resp);
	return (sd_card_state_t)((resp.resp[0] >> 0x9) & 0x0f);
}

bk_err_t bk_sd_card_init(void)
{
	bk_err_t ret = BK_OK;
#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
	gpio_dev_t data3_func = GPIO_DEV_INVALID;
#endif
	sdio_host_cfg_t cfg = {
		.is_emmc = false,
		.init_clock_hz = 0,   /* host default identification clock */
		.bus_width = SDIO_HOST_BUS_WIDTH_1,
	};

	if (s_sd_card_is_init) {
		SD_CARD_LOGI("sd card has inited\r\n");
		return BK_OK;
	}

#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
	ret = sd_card_force_data3_high(SDCARD_HOST_ID, &data3_func);
	if (ret)
		return ret;
#endif

	ret = bk_sdio_host_init(SDCARD_HOST_ID, &cfg);
	if (ret) {
#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
		(void)sd_card_restore_data3_func(data3_func);
#endif
		return ret;
	}

	ret = sd_card_identify();
	if (ret) {
#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
		(void)sd_card_restore_data3_func(data3_func);
#endif
		(void)bk_sdio_host_deinit(SDCARD_HOST_ID);
		return ret;
	}

#if CONFIG_SDCARD_BUSWIDTH_4LINE && CONFIG_USR_GPIO_CFG_EN
	ret = sd_card_restore_data3_func(data3_func);
	if (ret) {
		(void)bk_sdio_host_deinit(SDCARD_HOST_ID);
		return ret;
	}
#endif

	rtos_delay_milliseconds(1);

#if CONFIG_SDCARD_BUSWIDTH_4LINE
	/* 4-bit bus (opt-in via Kconfig; default build stays 1-bit). The card is
	 * now fully enumerated and latched in SD mode, and DATA3 has been restored
	 * to its SDIO function. Switch the card (ACMD6 arg=2 => 4-bit) and match
	 * the controller's DAT_XFER_WIDTH. */
	sd_send(SD_CMD_APP_CMD, SDIO_HOST_RESP_R1, sdio_rca, NULL);    /* CMD55 */
	rtos_delay_milliseconds(1);
	sd_send(SD_CMD_APP_CMD6_SET_BUS_WIDTH, SDIO_HOST_RESP_R1, 2, NULL); /* ACMD6 arg=2 => 4-bit */
	rtos_delay_milliseconds(1);
	bk_sdio_host_set_bus_width(SDCARD_HOST_ID, SDIO_HOST_BUS_WIDTH_4);
	rtos_delay_milliseconds(1);
#endif

	/* CMD16: set block length to 512. The legacy driver issued this with the
	 * controller long-response code; preserve that exact behavior. */
	sd_send(SD_CMD_SET_BLOCKLEN, SDIO_HOST_RESP_R2, SD_BLOCK_LEN, NULL);
	rtos_delay_milliseconds(1);

	/* Speed mode is independent of DMA. Default Speed (SDR12, <=25MHz) is
	 * the safe 3.3V path used by identification. High Speed is opt-in via
	 * CONFIG_SDCARD_HIGH_SPEED: CMD6 first, then host HIGH_SPEED_EN / SDR25,
	 * then the configured clock (default 40MHz). UHS-I SDR50/SDR104 are not
	 * used here (1.8V + tuning). */
#if CONFIG_SDCARD_HIGH_SPEED
	{
		sd_hs_switch_result_t hs_result = sd_card_switch_high_speed();

		if (hs_result == SD_HS_SWITCH_INDETERMINATE) {
			/* The card may already be in High Speed. Stop the host and
			 * require a fresh CMD0 enumeration instead of continuing
			 * with mismatched card/host timing. */
			(void)bk_sdio_host_deinit(SDCARD_HOST_ID);
			return BK_FAIL;
		}
		ret = sd_card_apply_transfer_clock(hs_result == SD_HS_SWITCH_ENABLED);
	}
#else
	ret = sd_card_apply_transfer_clock(false);
#endif
	if (ret != BK_OK) {
		(void)bk_sdio_host_deinit(SDCARD_HOST_ID);
		return ret;
	}
	rtos_delay_milliseconds(1);

	s_sd_card_is_init = true;
	return BK_OK;
}

bk_err_t bk_sd_card_deinit(void)
{
	bk_sdio_host_reset(SDCARD_HOST_ID);
	s_sd_card_is_init = false;
	return bk_sdio_host_deinit(SDCARD_HOST_ID);
}

bk_err_t bk_sd_card_write_blocks(const uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t arg = (s_sd_card_obj.sd_card.card_type == SD_CARD_TYPE_SDSC) ? (block_addr << 9) : block_addr;
	sdio_host_cmd_t cmd = {
		.index = SD_CMD_WRITE_MULTIPLE_BLOCK,
		.arg = arg,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_WRITE,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = (uint8_t *)data,
		.block_size = SD_BLOCK_LEN,
		.block_cnt = block_num,
	};
	bk_err_t ret = BK_FAIL;

	/* Re-issue on transient data-phase failures. The host driver aborts the
	 * stuck transfer (DAT/CMD soft-reset + CMD12) before returning, leaving
	 * the card back in TRAN, so a fresh multi-block transfer almost always
	 * succeeds. This keeps rare glitches local instead of escalating to the
	 * FATFS layer's heavy deinit/reinit retry. */
	for (uint32_t i = 0; i < SD_CARD_XFER_RETRY_CNT; i++) {
		ret = bk_sdio_host_xfer(SDCARD_HOST_ID, &cmd, &xfer, NULL);
		if (ret == BK_OK)
			break;
		/* Card physically removed -> further re-issues just burn doomed timeouts. */
		if (!bk_sd_card_is_present()) {
			SD_CARD_LOGW("card absent, abort write retries (addr=%d, cnt=%d)\r\n", block_addr, block_num);
			break;
		}
		SD_CARD_LOGW("write blocks retry %d (addr=%d, cnt=%d)\r\n", i, block_addr, block_num);
	}
	return ret;
}

static bk_err_t sd_card_read_blocks_once(uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t arg = (s_sd_card_obj.sd_card.card_type == SD_CARD_TYPE_SDSC) ? (block_addr << 9) : block_addr;
	sdio_host_cmd_t cmd = {
		/* Keep protocol and host transfer semantics aligned: CMD17 for one
		 * block, CMD18 only when a real multi-block transfer is requested. */
		.index = (block_num == 1) ? SD_CMD_READ_SINGLE_BLOCK : SD_CMD_READ_MULTIPLE_BLOCK,
		.arg = arg,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_READ,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = data,
		.block_size = SD_BLOCK_LEN,
		.block_cnt = block_num,
	};
	bk_err_t ret = BK_FAIL;

	/* Re-issue on transient data-phase failures (see bk_sd_card_write_blocks). */
	for (uint32_t i = 0; i < SD_CARD_XFER_RETRY_CNT; i++) {
		ret = bk_sdio_host_xfer(SDCARD_HOST_ID, &cmd, &xfer, NULL);
		if (ret == BK_OK)
			break;
		/* Card physically removed -> further re-issues just burn doomed timeouts. */
		if (!bk_sd_card_is_present()) {
			SD_CARD_LOGW("card absent, abort read retries (addr=%d, cnt=%d)\r\n", block_addr, block_num);
			break;
		}
		SD_CARD_LOGW("read blocks retry %d (addr=%d, cnt=%d)\r\n", i, block_addr, block_num);
	}
	return ret;
}

bk_err_t bk_sd_card_read_blocks(uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
#if defined(CONFIG_SDCARD_MAX_BLOCKS_PER_XFER) && (CONFIG_SDCARD_MAX_BLOCKS_PER_XFER > 0)
	/*
	 * Split large reads into smaller CMD18 transfers. On boards with
	 * marginal SDIO SI, long multi-block PIO reads are more likely to hit
	 * DATA_END_BIT_ERR; limiting the per-xfer block count is a software
	 * workaround (see CONFIG_SDCARD_MAX_BLOCKS_PER_XFER).
	 */
	const uint32_t max_chunk = (uint32_t)CONFIG_SDCARD_MAX_BLOCKS_PER_XFER;
	uint32_t done = 0;

	while (done < block_num) {
		uint32_t chunk = block_num - done;
		bk_err_t ret;

		if (chunk > max_chunk)
			chunk = max_chunk;
		ret = sd_card_read_blocks_once(data + (done * SD_BLOCK_LEN),
					       block_addr + done, chunk);
		if (ret != BK_OK)
			return ret;
		done += chunk;
	}
	return BK_OK;
#else
	return sd_card_read_blocks_once(data, block_addr, block_num);
#endif
}

bk_err_t bk_sd_card_get_card_info(sd_card_info_t *card_info)
{
	*card_info = s_sd_card_obj.sd_card;
	return BK_OK;
}

/* size unit: sector counts, default sector size is 512 bytes */
uint32_t bk_sd_card_get_card_size(void)
{
	sd_card_csd_t *csd_p = (sd_card_csd_t *)&s_sd_card_obj.csd;
	uint32_t ver = 0, size = 0;
	uint32_t csd_struct_raw = csd_p->csd_3.csd_structure;
	uint32_t card_type = s_sd_card_obj.sd_card.card_type;

	/*
	 * SD NAND chips may report a wrong csd_structure (e.g. v3.0) while
	 * ACMD41 CCS=0 indicates SDSC. CCS from OCR is authoritative for the
	 * capacity class, so force v1.0 parsing for SDSC cards.
	 */
	if (card_type == SD_CARD_TYPE_SDSC) {
		ver = 1;
		if (csd_struct_raw != 0)
			SD_CARD_LOGW("SDSC(CCS=0) but csd_structure=%d, force v1.0 parse\r\n", csd_struct_raw);
	} else {
		ver = csd_struct_raw + 1;
	}

	switch (ver) {
	case 1: { /* ver1.0 */
		uint32_t c_size, c_size_mul, block_nr, read_bl_len;
		c_size = (csd_p->csd_2.v1p0.c_size_high << 2) + csd_p->csd_1.v1p0.c_size_low;
		c_size_mul = 1 << (csd_p->csd_1.v1p0.c_size_mult + 2);
		block_nr = (c_size + 1) * c_size_mul;
		size = block_nr;

		read_bl_len = csd_p->csd_2.v1p0.read_bl_len;
		if (read_bl_len > 9 && read_bl_len <= 11) {
			size = block_nr << (read_bl_len - 9);
			SD_CARD_LOGW("card ver=%d.0, block_len=%d != 512bytes\r\n", ver, 1 << read_bl_len);
		} else if (read_bl_len > 11) {
			SD_CARD_LOGW("SDSC: invalid read_bl_len=%d (>11), treat as 9\r\n", read_bl_len);
		}
		break;
	}
	case 2: /* ver2.0: card_size == (c_size + 1) * 512K bytes */
		size = ((csd_p->csd_2.v2p0.c_size_high << 16) + (csd_p->csd_1.v2p0_v3p0.c_size_low) + 1) << 10;
		break;
	case 3: /* ver3.0: card_size == (c_size + 1) * 512K bytes */
		size = ((csd_p->csd_2.v3p0.c_size_high << 16) + (csd_p->csd_1.v2p0_v3p0.c_size_low) + 1) << 10;
		break;
	default:
		SD_CARD_LOGE("unsupported card ver=%d.0\r\n", ver);
		break;
	}

	SD_CARD_LOGI("card ver=%d.0, size:0x%08x sector(sector=512bytes)\r\n", ver, (uint32_t)size);
	return size;
}
