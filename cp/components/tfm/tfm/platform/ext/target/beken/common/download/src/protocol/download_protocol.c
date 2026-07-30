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

#pragma GCC push_options
#pragma GCC optimize ("O3")

enum
{
	FRM_TYPE_INVALID = 0,
	FRM_TYPE_BK_HCI,
	FRM_TYPE_PPP_LIKE,
};

enum
{
	HCI_WAIT_0X01 = 0,
	HCI_WAIT_0XE0,
	HCI_WAIT_0XFC,
};

enum
{
	HCI_RX_CMD_LEN = 0,
	HCI_RX_CMD_ID,
	HCI_RX_CMD_PARAM,
	HCI_RX_FLASH_CMD_LEN_LOW,
	HCI_RX_FLASH_CMD_LEN_HIGH,
	HCI_RX_FLASH_CMD_ID,
};

static rx_frm_ctrl_t	rx_frm_ctrl = {0};
rx_link_buf_t			rx_link_buf;

/*==============================================================================================
 *               ================                        ================ 
 *               ----------------     Data Link Layer    ----------------
 *               ================                        ================ 
 *==============================================================================================*/

static void reset_rx_ctrl(rx_frm_ctrl_t * frm_ctrl)
{
	// reset link type. -- link layer reset.
	frm_ctrl->frm_type = FRM_TYPE_INVALID;
	frm_ctrl->frm_done = 0;

	// reset HCI link state.  -- HCI link reset.
	frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
	frm_ctrl->hci_rx_state = HCI_RX_CMD_LEN;

	// reset PPP-like link state.  -- PPP-like link reset.
	//frm_ctrl->ppp_esc_next = 0;
	//frm_ctrl->ppp_rx_state = PPP_WAIT_SYNC;

	// reset app cmd type.  -- app layer reset.
	frm_ctrl->cmd_type = CMD_TYPE_INVALID;

	frm_ctrl->status = 0;		// used for concurrent process (cmd rx & sector write).
	frm_ctrl->read_idx = 0;		// used for concurrent process (cmd rx & sector write).
	frm_ctrl->write_idx = 0;	// should be reset for concurrent process ( indicates no param data received).

	frm_ctrl->cmd_len = 0;		// not necessary, will be set in state machine.

	return;
}

static void reset_rx_buff(rx_link_buf_t * link_buf)
{
	// discards all bytes in buffer.
	link_buf->read_idx = link_buf->write_idx;
}

static void rx_frm_bk_hci(rx_frm_ctrl_t * frm_ctrl, u8 rx_data)
{
	u8  * rx_cmd_buff = (u8 *)&frm_ctrl->cmd_param[0];
       
	if(frm_ctrl->hci_rx_state == HCI_RX_CMD_LEN)
	{
		frm_ctrl->cmd_len = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_CMD_ID;

		if(frm_ctrl->cmd_len == 0)  // frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		/*
		else if(frm_ctrl->cmd_len >= RX_CMD_BUFF_SIZE)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		*/  // rx_data < 256, RX_CMD_BUFF_SIZE > 256.
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_CMD_ID)
	{
		frm_ctrl->cmd_id = rx_data;

		if(frm_ctrl->cmd_len == 0xFF)
		{
			if(frm_ctrl->cmd_id == 0xF4)	// EXT_CMD, it is a command of FLASH.
			{
				frm_ctrl->cmd_type = CMD_TYPE_FLASH;
				frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_LEN_LOW;
			}
			else  // frame fault.
			{
				reset_rx_ctrl(frm_ctrl);
				return;
			}
		}
		else
		{
			frm_ctrl->cmd_type = CMD_TYPE_COMMON;
			frm_ctrl->hci_rx_state = HCI_RX_CMD_PARAM;
			frm_ctrl->write_idx = 0;
		}
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_CMD_PARAM)
	{
		rx_cmd_buff[frm_ctrl->write_idx] = rx_data;
		frm_ctrl->write_idx++;
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_LEN_LOW)
	{
		frm_ctrl->cmd_len = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_LEN_HIGH;
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_LEN_HIGH)
	{
		frm_ctrl->cmd_len += ((u16)rx_data << 8);
		frm_ctrl->hci_rx_state = HCI_RX_FLASH_CMD_ID;

		if(frm_ctrl->cmd_len == 0)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
		else if(frm_ctrl->cmd_len >= RX_CMD_BUFF_SIZE)	// frame fault.
		{
			reset_rx_ctrl(frm_ctrl);
			return;
		}
	}
	else if(frm_ctrl->hci_rx_state == HCI_RX_FLASH_CMD_ID)
	{
		frm_ctrl->cmd_id = rx_data;
		frm_ctrl->hci_rx_state = HCI_RX_CMD_PARAM;
		frm_ctrl->write_idx = 0;
	}
	else  // software fault.
	{
		reset_rx_ctrl(frm_ctrl);
		return;
	}

	// after HCI_RX_CMD_ID, HCI_RX_FLASH_CMD_ID, HCI_RX_CMD_PARAM process.
	if(frm_ctrl->hci_rx_state == HCI_RX_CMD_PARAM)
	{
		if((frm_ctrl->write_idx + 1) >= frm_ctrl->cmd_len)
		{
			frm_ctrl->frm_done = 1;
		}
	}

	return;
}

static void rx_frm_check_type(rx_frm_ctrl_t * frm_ctrl, u8 rx_data)
{
	if(frm_ctrl->hci_hdr_state == HCI_WAIT_0X01)
	{
		if(rx_data == 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XE0;
	}
	else if(frm_ctrl->hci_hdr_state == HCI_WAIT_0XE0)
	{
		if(rx_data == 0xE0)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XFC;
		else if(rx_data != 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
	}
	else if(frm_ctrl->hci_hdr_state == HCI_WAIT_0XFC)
	{
		if(rx_data == 0xFC)
		{
			//----->   frame type check completed   ------->
			frm_ctrl->frm_type = FRM_TYPE_BK_HCI;
			frm_ctrl->frm_done = 0;

			frm_ctrl->hci_rx_state = HCI_RX_CMD_LEN;
			frm_ctrl->write_idx = 0;
		}
		else if(rx_data != 0x01)
			frm_ctrl->hci_hdr_state = HCI_WAIT_0X01;
		else
			frm_ctrl->hci_hdr_state = HCI_WAIT_0XE0;
	}
	else  // software fault.
	{
		reset_rx_ctrl(frm_ctrl);
		return;
	}

	return;
}

u32 boot_rx_frm_handler(void)
{
	rx_frm_ctrl_t * frm_ctrl = &rx_frm_ctrl;
	rx_link_buf_t * link_buf = &rx_link_buf;
	
	u8	* rx_temp_buff = (u8 *)&link_buf->rx_buf[0];
	u8    rx_data;
	u16   rd_idx = link_buf->read_idx;
	
	while(rd_idx != link_buf->write_idx)
	{
		rx_data = rx_temp_buff[rd_idx];

		if(frm_ctrl->frm_type == FRM_TYPE_BK_HCI)
		{
			rx_frm_bk_hci(frm_ctrl, rx_data);
		}
		/*
		else if(frm_ctrl->frm_type == FRM_TYPE_PPP_LIKE)
		{
		}
		*/
		else  // if (frm_ctrl->frm_type == FRM_TYPE_INVALID)
		{
			rx_frm_check_type(frm_ctrl, rx_data);
		}

		rd_idx++;
		if(rd_idx >= sizeof(link_buf->rx_buf))
			rd_idx = 0;
		link_buf->read_idx = rd_idx;

		if(frm_ctrl->frm_done)
			break;

	}

	// handle commands, 
	// do not care what type of link frame the command is carried by.
	// do not care what type of hardware interface the command is transferred by.
	if(frm_ctrl->frm_done)
	{
		if(frm_ctrl->cmd_type == CMD_TYPE_COMMON)
		{
			common_cmd_process(frm_ctrl);
		}
		else if(frm_ctrl->cmd_type == CMD_TYPE_FLASH)
		{
			if(frm_ctrl->cmd_id == FLASH_CMD_SECTOR_WRITE)
				flash_cmd_sector_write_done(frm_ctrl);
			else
				flash_cmd_process(frm_ctrl);
		}
		else  // !!!!  FAULT  !!!!
		{
			/* reset rx frame! */
		}

		/* command handle complete, reset state machine for next cmd process. */
		reset_rx_ctrl(frm_ctrl);

		// discards all bytes in buffer.
		reset_rx_buff(link_buf);
	}
	else
	{
		// speed up the flash write process.
		if(frm_ctrl->cmd_type == CMD_TYPE_FLASH)
		{
			if(frm_ctrl->cmd_id == FLASH_CMD_SECTOR_WRITE)
			{
				if(frm_ctrl->write_idx > 0)  // some param data received.
				{
					flash_cmd_sector_write(frm_ctrl);
				}
			}
		}
	}

	return 0;
}


#pragma GCC pop_options

