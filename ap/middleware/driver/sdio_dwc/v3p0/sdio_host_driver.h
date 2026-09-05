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

#pragma once

#include <common/bk_include.h>
#include <components/log.h>

#include "sdio_storage_driver.h"


#define SDIO_HOST_TAG "sdio_host"
#define SDIO_HOST_LOGI(...) BK_LOGI(SDIO_HOST_TAG, ##__VA_ARGS__)
#define SDIO_HOST_LOGW(...) BK_LOGW(SDIO_HOST_TAG, ##__VA_ARGS__)
#define SDIO_HOST_LOGE(...) BK_LOGE(SDIO_HOST_TAG, ##__VA_ARGS__)
#define SDIO_HOST_LOGD(...) BK_LOGD(SDIO_HOST_TAG, ##__VA_ARGS__)

#define SDIO_CMD_TIMEOUT_RESP        0xFFFFFFFFu

typedef enum
{
	SDIO_WIRE_WIDTH_SEL_1,
	SDIO_WIRE_WIDTH_SEL_4,	//4-wire
	SDIO_WIRE_WIDTH_SEL_8,	//8-wire
}sdio_wire_width_sel_t;

bk_err_t sdio_host_init(void);
void sdio_reset(void);
void sdio_switch_wire_width(sdio_wire_width_sel_t width_sel);
bool sdio_host_last_cmd_timeout(void);

bk_err_t sd_clk_change(uintptr_t addr,uint16 SD_FREQ_SEL);
void sd_card_interface_set(uintptr_t addr,uint8 UHS_MODE_SEL);
int send_cmd(uintptr_t addr, uint8 CMD_INDEX, uint8 RESP_TYPE, uint32 ARGUMENT);
bk_err_t send_mult_data(uintptr_t addr, const uint8_t *data, uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT);
int receive_mult_data(uintptr_t addr, uint8_t *data, uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT);
bk_err_t adma2_send_data(uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT);
bk_err_t adma2_receive_data(uintptr_t addr,uint32 SYS_ADDR,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT);

