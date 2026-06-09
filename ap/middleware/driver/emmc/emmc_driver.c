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
 * eMMC device protocol driver.
 *
 * Depends ONLY on the generic SDIO host controller interface
 * (<driver/sdio_host.h>); no controller register access, so it is portable
 * across the two BK7259 SDIO controllers (selected by CONFIG_EMMC_HOST_ID).
 */

#include <os/os.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include <components/log.h>
#include <driver/sdio_host.h>
#include <driver/emmc.h>

#define EMMC_TAG "emmc"
#define EMMC_LOGI(...) BK_LOGI(EMMC_TAG, ##__VA_ARGS__)
#define EMMC_LOGW(...) BK_LOGW(EMMC_TAG, ##__VA_ARGS__)
#define EMMC_LOGE(...) BK_LOGE(EMMC_TAG, ##__VA_ARGS__)
#define EMMC_LOGD(...) BK_LOGD(EMMC_TAG, ##__VA_ARGS__)

#if defined(CONFIG_EMMC_HOST_ID)
#define EMMC_HOST_ID   ((sdio_host_id_t)CONFIG_EMMC_HOST_ID)
#else
#define EMMC_HOST_ID   SDIO_HOST_ID_0
#endif

#if defined(CONFIG_EMMC_BUS_WIDTH)
#define EMMC_TARGET_BUS_WIDTH  CONFIG_EMMC_BUS_WIDTH
#else
#define EMMC_TARGET_BUS_WIDTH  8
#endif

/* eMMC command set (JESD84) */
#define EMMC_CMD_GO_IDLE_STATE        0
#define EMMC_CMD_SEND_OP_COND         1   /* R3 */
#define EMMC_CMD_ALL_SEND_CID         2   /* R2 */
#define EMMC_CMD_SET_RELATIVE_ADDR    3   /* R1, host assigns RCA */
#define EMMC_CMD_SWITCH               6   /* R1b */
#define EMMC_CMD_SELECT_CARD          7   /* R1b */
#define EMMC_CMD_SEND_EXT_CSD         8   /* R1 + 512B data read */
#define EMMC_CMD_SEND_CSD             9   /* R2 */
#define EMMC_CMD_SEND_STATUS          13  /* R1 */
#define EMMC_CMD_SET_BLOCKLEN         16  /* R1 */
#define EMMC_CMD_READ_MULTIPLE_BLOCK  18  /* R1 */
#define EMMC_CMD_WRITE_MULTIPLE_BLOCK 25  /* R1 */

#define EMMC_BLOCK_LEN                512
#define EMMC_XFER_RETRY_CNT          3   /* local retries on transient data-phase failure */

/* OCR bits */
#define EMMC_OCR_BUSY                 (1u << 31)
#define EMMC_OCR_SECTOR_MODE          (1u << 30)
#define EMMC_OCR_VOLTAGE_WINDOW       0x00FF8000u

/* CMD6 (SWITCH) access mode: write byte */
#define EMMC_SWITCH_ACCESS_WRITE_BYTE (3u << 24)
/* EXT_CSD field indices */
#define EXT_CSD_BUS_WIDTH             183
#define EXT_CSD_HS_TIMING             185
#define EXT_CSD_REV                   192
#define EXT_CSD_DEVICE_TYPE           196
#define EXT_CSD_SEC_COUNT             212

/* CMD13 card status: current state field [12:9], READY_FOR_DATA bit8 */
#define EMMC_STATUS_READY_FOR_DATA    (1u << 8)
#define EMMC_STATUS_CURRENT_STATE(s)  (((s) >> 9) & 0x0f)
#define EMMC_STATE_TRANS              4

#define EMMC_RCA                      0x0001

static bool s_emmc_is_init = false;
static emmc_info_t s_emmc_info;
/* EXT_CSD scratch buffer (cache-line friendly). */
static uint8_t s_ext_csd[EMMC_BLOCK_LEN] __attribute__((aligned(32)));

static bk_err_t emmc_send(uint8_t index, sdio_host_resp_type_t rt, uint32_t arg,
			  sdio_host_resp_t *resp)
{
	sdio_host_cmd_t cmd = {
		.index = index,
		.arg = arg,
		.resp_type = rt,
	};
	return bk_sdio_host_send_cmd(EMMC_HOST_ID, &cmd, resp);
}

/* Poll CMD13 until the card returns to transfer state and is ready for data. */
static bk_err_t emmc_wait_ready(uint32_t timeout_ms)
{
	sdio_host_resp_t resp = {0};
	uint32_t i;

	for (i = 0; i < timeout_ms; i++) {
		if (emmc_send(EMMC_CMD_SEND_STATUS, SDIO_HOST_RESP_R1,
			      ((uint32_t)s_emmc_info.rca << 16), &resp) != BK_OK)
			return BK_FAIL;
		if ((resp.resp[0] & EMMC_STATUS_READY_FOR_DATA) &&
		    EMMC_STATUS_CURRENT_STATE(resp.resp[0]) == EMMC_STATE_TRANS)
			return BK_OK;
		rtos_delay_milliseconds(1);
	}
	EMMC_LOGW("wait ready timeout (status=0x%x)\r\n", resp.resp[0]);
	return BK_FAIL;
}

/* CMD6 SWITCH: write @value to EXT_CSD[@index], then wait until not busy. */
static bk_err_t emmc_switch(uint8_t index, uint8_t value)
{
	uint32_t arg = EMMC_SWITCH_ACCESS_WRITE_BYTE |
		       ((uint32_t)index << 16) | ((uint32_t)value << 8);
	bk_err_t ret;

	ret = emmc_send(EMMC_CMD_SWITCH, SDIO_HOST_RESP_R1B, arg, NULL);
	if (ret != BK_OK)
		return ret;
	return emmc_wait_ready(500);
}

static bk_err_t emmc_read_ext_csd(void)
{
	sdio_host_cmd_t cmd = {
		.index = EMMC_CMD_SEND_EXT_CSD,
		.arg = 0,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_READ,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = s_ext_csd,
		.block_size = EMMC_BLOCK_LEN,
		.block_cnt = 1,
	};
	bk_err_t ret;

	os_memset(s_ext_csd, 0, sizeof(s_ext_csd));
	ret = bk_sdio_host_xfer(EMMC_HOST_ID, &cmd, &xfer, NULL);
	if (ret != BK_OK) {
		EMMC_LOGE("read EXT_CSD failed, ret=%d\r\n", ret);
		return ret;
	}

	s_emmc_info.sector_count =
		((uint32_t)s_ext_csd[EXT_CSD_SEC_COUNT]) |
		((uint32_t)s_ext_csd[EXT_CSD_SEC_COUNT + 1] << 8) |
		((uint32_t)s_ext_csd[EXT_CSD_SEC_COUNT + 2] << 16) |
		((uint32_t)s_ext_csd[EXT_CSD_SEC_COUNT + 3] << 24);
	s_emmc_info.ext_csd_rev = s_ext_csd[EXT_CSD_REV];
	EMMC_LOGI("EXT_CSD rev=%d, sec_count=0x%x (%u MB), dev_type=0x%x\r\n",
		  s_emmc_info.ext_csd_rev, s_emmc_info.sector_count,
		  (unsigned int)(s_emmc_info.sector_count / 2048),
		  s_ext_csd[EXT_CSD_DEVICE_TYPE]);
	return BK_OK;
}

static bk_err_t emmc_identify(void)
{
	sdio_host_resp_t resp = {0};
	uint32_t ocr = 0;
	uint32_t retry = 0;

	/* CMD0: reset to idle */
	emmc_send(EMMC_CMD_GO_IDLE_STATE, SDIO_HOST_RESP_NONE, 0, NULL);
	rtos_delay_milliseconds(2);

	/* CMD1: send operating conditions; busy-wait until power-up done. The
	 * sector-mode bit requests >2GB sector addressing. */
	do {
		if (emmc_send(EMMC_CMD_SEND_OP_COND, SDIO_HOST_RESP_R3,
			      (EMMC_OCR_SECTOR_MODE | EMMC_OCR_VOLTAGE_WINDOW), &resp) != BK_OK) {
			EMMC_LOGE("CMD1 no response, no eMMC present\r\n");
			return BK_FAIL;
		}
		ocr = resp.resp[0];
		rtos_delay_milliseconds(2);
		if (retry++ > 500) {
			EMMC_LOGE("CMD1 busy-wait timeout (ocr=0x%x)\r\n", ocr);
			return BK_FAIL;
		}
	} while (!(ocr & EMMC_OCR_BUSY));

	s_emmc_info.is_high_capacity = (ocr & EMMC_OCR_SECTOR_MODE) ? true : false;

	/* CMD2: ALL_SEND_CID (R2) */
	if (emmc_send(EMMC_CMD_ALL_SEND_CID, SDIO_HOST_RESP_R2, 0, &resp) != BK_OK)
		return BK_FAIL;
	s_emmc_info.cid[0] = resp.resp[0];
	s_emmc_info.cid[1] = resp.resp[1];
	s_emmc_info.cid[2] = resp.resp[2];
	s_emmc_info.cid[3] = resp.resp[3];

	/* CMD3: host assigns RCA (unlike SD, the host picks it) */
	s_emmc_info.rca = EMMC_RCA;
	if (emmc_send(EMMC_CMD_SET_RELATIVE_ADDR, SDIO_HOST_RESP_R1,
		      ((uint32_t)s_emmc_info.rca << 16), &resp) != BK_OK)
		return BK_FAIL;

	/* CMD9: SEND_CSD (R2) */
	if (emmc_send(EMMC_CMD_SEND_CSD, SDIO_HOST_RESP_R2,
		      ((uint32_t)s_emmc_info.rca << 16), &resp) != BK_OK)
		return BK_FAIL;
	s_emmc_info.csd[0] = resp.resp[0];
	s_emmc_info.csd[1] = resp.resp[1];
	s_emmc_info.csd[2] = resp.resp[2];
	s_emmc_info.csd[3] = resp.resp[3];

	/* CMD7: select the card (R1b) */
	if (emmc_send(EMMC_CMD_SELECT_CARD, SDIO_HOST_RESP_R1B,
		      ((uint32_t)s_emmc_info.rca << 16), NULL) != BK_OK)
		return BK_FAIL;
	if (emmc_wait_ready(500) != BK_OK)
		return BK_FAIL;

	return BK_OK;
}

static bk_err_t emmc_setup_bus(void)
{
	uint8_t width_val;
	sdio_host_bus_width2_t hw_width;

#if (EMMC_TARGET_BUS_WIDTH == 8)
	width_val = 2; hw_width = SDIO_HOST_BUS_WIDTH_8;
#elif (EMMC_TARGET_BUS_WIDTH == 4)
	width_val = 1; hw_width = SDIO_HOST_BUS_WIDTH_4;
#else
	width_val = 0; hw_width = SDIO_HOST_BUS_WIDTH_1;
#endif

	/* Switch the card bus width, then match it on the controller. */
	if (emmc_switch(EXT_CSD_BUS_WIDTH, width_val) != BK_OK) {
		EMMC_LOGW("set bus width %d on card failed, fall back to 1-bit\r\n", EMMC_TARGET_BUS_WIDTH);
		hw_width = SDIO_HOST_BUS_WIDTH_1;
	} else {
		s_emmc_info.bus_width = EMMC_TARGET_BUS_WIDTH;
	}
	bk_sdio_host_set_bus_width(EMMC_HOST_ID, hw_width);

	/* Switch to high-speed SDR timing (HS_TIMING = 1).
	 * TODO[HW]: HS200/HS400 need controller delay-line tuning and 1.8V
	 * signaling; enable here after board bring-up. */
	if (emmc_switch(EXT_CSD_HS_TIMING, 1) == BK_OK) {
		bk_sdio_host_set_timing(EMMC_HOST_ID, SDIO_HOST_TIMING_MMC_HS);
		bk_sdio_host_set_clock(EMMC_HOST_ID, 40000000);
	} else {
		EMMC_LOGW("HS timing switch failed, stay at default speed\r\n");
	}

	return BK_OK;
}

bk_err_t bk_emmc_init(void)
{
	bk_err_t ret;
	sdio_host_cfg_t cfg = {
		.is_emmc = true,
		.init_clock_hz = 0,
		.bus_width = SDIO_HOST_BUS_WIDTH_1,
	};

	if (s_emmc_is_init) {
		EMMC_LOGI("emmc already inited\r\n");
		return BK_OK;
	}

	os_memset(&s_emmc_info, 0, sizeof(s_emmc_info));
	s_emmc_info.bus_width = 1;

	ret = bk_sdio_host_init(EMMC_HOST_ID, &cfg);
	if (ret != BK_OK)
		return ret;

	ret = emmc_identify();
	if (ret != BK_OK) {
		EMMC_LOGE("emmc identify failed\r\n");
		return ret;
	}

	/* Force 512-byte block length (no effect on sector-addressed parts). */
	emmc_send(EMMC_CMD_SET_BLOCKLEN, SDIO_HOST_RESP_R1, EMMC_BLOCK_LEN, NULL);

	ret = emmc_read_ext_csd();
	if (ret != BK_OK)
		return ret;

	emmc_setup_bus();

	s_emmc_is_init = true;
	EMMC_LOGI("emmc init done: %u sectors, %u-bit bus\r\n",
		  s_emmc_info.sector_count, s_emmc_info.bus_width);
	return BK_OK;
}

bk_err_t bk_emmc_deinit(void)
{
	s_emmc_is_init = false;
	return bk_sdio_host_deinit(EMMC_HOST_ID);
}

bk_err_t bk_emmc_read_blocks(uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t arg = s_emmc_info.is_high_capacity ? block_addr : (block_addr << 9);
	sdio_host_cmd_t cmd = {
		.index = EMMC_CMD_READ_MULTIPLE_BLOCK,
		.arg = arg,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_READ,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = data,
		.block_size = EMMC_BLOCK_LEN,
		.block_cnt = block_num,
	};
	bk_err_t ret = BK_FAIL;

	if (!s_emmc_is_init)
		return BK_FAIL;

	/* Re-issue on transient data-phase failures: the host driver aborts the
	 * stuck transfer (DAT/CMD soft-reset + CMD12) before returning, so a
	 * fresh multi-block transfer almost always succeeds (mirrors the SD card
	 * driver's local retry). */
	for (uint32_t i = 0; i < EMMC_XFER_RETRY_CNT; i++) {
		ret = bk_sdio_host_xfer(EMMC_HOST_ID, &cmd, &xfer, NULL);
		if (ret == BK_OK)
			break;
		EMMC_LOGW("read blocks retry %u (addr=%u, cnt=%u)\r\n",
			  i, block_addr, block_num);
	}
	return ret;
}

bk_err_t bk_emmc_write_blocks(const uint8_t *data, uint32_t block_addr, uint32_t block_num)
{
	uint32_t arg = s_emmc_info.is_high_capacity ? block_addr : (block_addr << 9);
	sdio_host_cmd_t cmd = {
		.index = EMMC_CMD_WRITE_MULTIPLE_BLOCK,
		.arg = arg,
		.resp_type = SDIO_HOST_RESP_R1,
	};
	sdio_host_data_t xfer = {
		.dir = SDIO_HOST_XFER_WRITE,
		.mode = SDIO_HOST_XFER_PIO,
		.buf = (uint8_t *)data,
		.block_size = EMMC_BLOCK_LEN,
		.block_cnt = block_num,
	};
	bk_err_t ret = BK_FAIL;

	if (!s_emmc_is_init)
		return BK_FAIL;

	/* Re-issue on transient data-phase failures (see bk_emmc_read_blocks). */
	for (uint32_t i = 0; i < EMMC_XFER_RETRY_CNT; i++) {
		ret = bk_sdio_host_xfer(EMMC_HOST_ID, &cmd, &xfer, NULL);
		if (ret == BK_OK)
			break;
		EMMC_LOGW("write blocks retry %u (addr=%u, cnt=%u)\r\n",
			  i, block_addr, block_num);
	}
	return ret;
}

uint32_t bk_emmc_get_sector_count(void)
{
	return s_emmc_info.sector_count;
}

bk_err_t bk_emmc_get_info(emmc_info_t *info)
{
	if (info == NULL)
		return BK_ERR_PARAM;
	*info = s_emmc_info;
	return BK_OK;
}
