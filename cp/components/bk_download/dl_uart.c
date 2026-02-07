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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dl_protocol.h"

static bool dl_uart_init(dl_uart_dev_t *dl_uart_dev);
static bool dl_uart_open(dl_uart_dev_t *dl_uart_dev);
static bool dl_uart_set_baud_rate(dl_uart_dev_t *dl_uart_dev, u32 rate);
static bk_err_t dl_uart_read(dl_uart_dev_t *dl_uart_dev,  void *data, u32 BufLen);
static bk_err_t dl_uart_write(dl_uart_dev_t *dl_uart_dev, const void *data, u32 BufLen);
static bool dl_uart_deinit(dl_uart_dev_t *dl_uart_dev);
static bk_err_t dl_uart_read_byte(dl_uart_dev_t *dl_uart_dev, u8 *rx_data);
static bk_err_t dl_uart_read_ready(dl_uart_dev_t *dl_uart_dev);
void dl_uart_isr_handler(int uartn,void *param);

static const  dl_uart_dev_drv_t dl_uart_drv ={
	.init                   = dl_uart_init,
	.open                   = dl_uart_open,
	.set_baud_rate          = dl_uart_set_baud_rate,
	.write                  = dl_uart_write,
	.read_byte              = dl_uart_read_byte,
	.read_ready             = dl_uart_read_ready,
	.deinit                 = dl_uart_deinit,
};

dl_uart_dev_t dl_uart ={
	.dev_drv = (struct dl_uart_dev_drv *)&dl_uart_drv,
	.dev_id  = CONFIG_DL_QSPI_UART_PORT,
};

static const uart_config_t dl_uart_config =
{
	.baud_rate = UART_BAUD_RATE,
	.data_bits = UART_DATA_8_BITS,
	.parity = UART_PARITY_NONE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_FLOWCTRL_DISABLE,
	.src_clk = UART_SCLK_XTAL_26M
};

static bool dl_uart_init(dl_uart_dev_t *dl_uart_dev)
{
	u8 uart_id = 0;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;

	bk_uart_set_enable_rx(uart_id, 0);
	bk_uart_set_enable_tx(uart_id, 0);
	bk_uart_init(uart_id, &dl_uart_config);
	bk_uart_set_rx_full_threshold(uart_id, UART_RX_FIFO_THRESHOLD);
	bk_uart_set_rx_timeout(uart_id, UART_RX_STOP_DETECT_TIME_32_BITS);
	
	return BK_TRUE;
}

static bool dl_uart_open(dl_uart_dev_t *dl_uart_dev)
{
	u8 uart_id = 0;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	bk_uart_disable_sw_fifo(uart_id);

	bk_uart_register_rx_isr(uart_id, (uart_isr_t)dl_uart_isr_handler, NULL);
	bk_uart_enable_rx_interrupt(uart_id);

	return BK_TRUE;
}

static bool dl_uart_set_baud_rate(dl_uart_dev_t *dl_uart_dev, u32 rate)
{
	u8 uart_id = 0;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	bk_uart_set_baud_rate(uart_id, rate);

	return BK_TRUE;
}

static	bk_err_t dl_uart_write(dl_uart_dev_t *dl_uart_dev, const void *data, u32 BufLen)
{
	u8 uart_id = 0;
	bk_err_t ret;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	ret = bk_uart_write_bytes(uart_id, data, BufLen);
	if(ret != BK_OK)
		return BK_FAIL;

	return BK_OK;
}

static bk_err_t dl_uart_read_byte(dl_uart_dev_t *dl_uart_dev, u8 *rx_data)
{	
	u8 uart_id  = 0;
	bk_err_t ret_val;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	ret_val = uart_read_byte_ex(uart_id, rx_data);

	return ret_val;
}

static bk_err_t dl_uart_read_ready(dl_uart_dev_t *dl_uart_dev)
{
	u8  uart_id = 0;
	bk_err_t ready_val;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	ready_val = uart_read_ready(uart_id);

	return ready_val;
}

static bool dl_uart_deinit(dl_uart_dev_t *dl_uart_dev)
{
	u8 uart_id = 0;

	if(dl_uart_dev == NULL)
		return BK_FALSE;

	uart_id = dl_uart_dev->dev_id;
	bk_uart_disable_rx_interrupt(uart_id);
	bk_uart_register_rx_isr(uart_id, NULL, NULL);
	bk_uart_deinit(uart_id);

	return BK_TRUE;
}

//----------------------------------------------
// UART0 Interrupt Service Rountine
//----------------------------------------------
extern rx_link_buf_t rx_link_buf;
extern dl_uart_dev_t *dl_qspi;

void dl_uart_isr_handler(int uartn,void *param) 
{
	(void)uartn;
	u8       value ;
	u16      next_wr_idx;
	int      ret;

	rx_link_buf_t  * link_buf = &rx_link_buf;
	u8	* rx_temp_buff = (u8 *)&link_buf->rx_buf[0];

	while (dl_uart.dev_drv->read_ready(&dl_uart) == 0)
	{
		ret = dl_qspi->dev_drv->read_byte(dl_qspi, &value);
		if (ret == -1)
			break;

		next_wr_idx = (link_buf->write_idx + 1);
		if(next_wr_idx >= sizeof(link_buf->rx_buf))
			next_wr_idx = 0;

		if(next_wr_idx != link_buf->read_idx)
		{
			rx_temp_buff[link_buf->write_idx] = value;
			link_buf->write_idx = next_wr_idx;
		}
		else
		{
			
		}
			//uart0_send((u8 *)&value, 1);  // for test
		//os_printf("cccc value :0x%x \r\n",value);    
	}
}