// Copyright 2020-2022 Beken
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

#include "../download_internal.h"

typedef struct {
	u8 rsp_hdr_0x04;
	u8 rsp_hdr_0x0e;
	u8 rsp_len;
	u8 cmd_hdr_0x01;
	u8 cmd_hdr_0xe0;
	u8 cmd_hdr_0xfc;
	u8 cmd_id;
	u8 rsp_param[0];
} bk_hci_rsp_frm_t;

typedef struct {
	u8 rsp_hdr_0x04;
	u8 rsp_hdr_0x0e;
	u8 rsp_len;
	u8 cmd_hdr_0x01;
	u8 cmd_hdr_0xe0;
	u8 cmd_hdr_0xfc;
	u8 cmd_id;
	u8 flash_rsp_len_low;
	u8 flash_rsp_len_high;
	u8 flash_cmd_id;
	u8 flash_cmd_result;
	u8 flash_rsp_param[0];
} bk_flash_rsp_frm_t;

const u8 build_version[] = " " __DATE__ " " __TIME__;

void tx_rsp_data(u8 *buf, u16 len)
{
	download_uart_write(buf, len);
}

void tx_rsp_for_common_cmd(u16 cmd_id, u8 *cmd_param, u16 param_len)
{
	bk_hci_rsp_frm_t rsp_frm;

	rsp_frm.rsp_hdr_0x04 = 0x04;
	rsp_frm.rsp_hdr_0x0e = 0x0e;
	rsp_frm.rsp_len = param_len + 4;
	rsp_frm.cmd_hdr_0x01 = 0x01;
	rsp_frm.cmd_hdr_0xe0 = 0xe0;
	rsp_frm.cmd_hdr_0xfc = 0xfc;
	rsp_frm.cmd_id = (u8)cmd_id;

	tx_rsp_data((u8 *)&rsp_frm, sizeof(rsp_frm));
	tx_rsp_data(cmd_param, param_len);
}

void tx_rsp_for_flash_cmd_hdr(u16 cmd_id, u8 status, u16 param_len)
{
	bk_flash_rsp_frm_t rsp_frm;

	rsp_frm.rsp_hdr_0x04 = 0x04;
	rsp_frm.rsp_hdr_0x0e = 0x0e;
	rsp_frm.rsp_len = 0xff;
	rsp_frm.cmd_hdr_0x01 = 0x01;
	rsp_frm.cmd_hdr_0xe0 = 0xe0;
	rsp_frm.cmd_hdr_0xfc = 0xfc;
	rsp_frm.cmd_id = 0xf4;
	rsp_frm.flash_rsp_len_low = (u8)(param_len + 2);
	rsp_frm.flash_rsp_len_high = (u8)((param_len + 2) >> 8);
	rsp_frm.flash_cmd_id = (u8)cmd_id;
	rsp_frm.flash_cmd_result = status;

	tx_rsp_data((u8 *)&rsp_frm, sizeof(rsp_frm));
}

void tx_rsp_for_flash_cmd(u16 cmd_id, u8 status, u8 *cmd_param, u16 param_len)
{
	tx_rsp_for_flash_cmd_hdr(cmd_id, status, param_len);
	tx_rsp_data(cmd_param, param_len);
}

void boot_tx_startup_indication(void)
{
#if 0
	u8 temp[2] = {0x95, 0x27};

	// 04 0e 06 01 e0 fc fe 95 27
	tx_rsp_for_common_cmd(COMMON_CMD_STARTUP, &temp[0], 2);
#else
	// 04 0e nn 01 e0 fc fe 20 xx xx xx xx ......
	tx_rsp_for_common_cmd(COMMON_CMD_STARTUP, (u8 *)build_version, sizeof(build_version) - 1);
#endif
}
