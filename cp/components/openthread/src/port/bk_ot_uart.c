// Copyright 2020-2025 Beken
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdkconfig.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/uart.h>
#include "openthread/error.h"
#include "utils/uart.h"
#include "bk_private/bk_uart.h"
#include <openthread/cli.h>
#include "common/code_utils.hpp"
#include "openthread-core-bk7239n-config.h"
#include "bk_private/bk_uart.h"

#if CONFIG_OPENTHREAD
#define BK_OT_UART_PORT UART_ID_0//UART_ID_1
#define BK_OT_UART_BPS  115200

#define RX_BUFF_SIZE    OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE

#define BK_OT_UART_LOG_EN 0
#if BK_OT_UART_LOG_EN
#define bk_ot_uart_log os_printf
#else
#define bk_ot_uart_log
#endif

#if !CONFIG_SHELL_ASYNCLOG
static beken_semaphore_t usrCliSemaphoreRx = NULL;
beken_thread_t usrOTCliRxHandle = NULL;


typedef struct
{
   uint8_t uart_id;

   uint8_t rx_buff[RX_BUFF_SIZE];
   uint8_t rx_over_flow;
}ot_uart_ext_t;
static ot_uart_ext_t ot_uart_ext_param;

static void ot_uart_rx_isr(uart_id_t id, ot_uart_ext_t *uart_ext)
{
	int   ret = -1;
	if(uart_get_interrupt_status(id)&0x02)
	{// max rx buffer size < 255
		uint8_t rx_data=0xFF;
		uint16_t rx_cnt=0;
		while(1)
		{
			ret = uart_read_byte_ex(ot_uart_ext_param.uart_id, &rx_data);
			if(ret == -1)
				break;
			ot_uart_ext_param.rx_buff[rx_cnt++] = rx_data;
		}
		otPlatUartReceived(ot_uart_ext_param.rx_buff,rx_cnt);
		return;
	}
	ret = rtos_set_semaphore(&usrCliSemaphoreRx);
	if(OT_ERROR_NONE !=ret)
	{
		os_printf("[Error]%s: set semaphore failed %#x\r\n",__func__,ret);
	}

}

static void usrOTCliRxMain(uint32_t data)
{
	int ret = -1;
	uint16_t   free_buff_len, rx_cnt = 0;
	uint8_t    rx_data;

	if(usrCliSemaphoreRx == NULL)
	{
	ret = rtos_init_semaphore(&usrCliSemaphoreRx, 1);
	if(ret != OT_ERROR_NONE)
	{
		os_printf("%s set semaphore failed\r\n",__func__);
		return ;
	}
	}
	while(1)
	{
		ret = rtos_get_semaphore(&usrCliSemaphoreRx, BEKEN_WAIT_FOREVER);
		
		if(OT_ERROR_NONE == ret)
		{
			free_buff_len = RX_BUFF_SIZE;
			rx_cnt = 0;
			while(1)  /* read all data from rx-FIFO. */
			{
			    ret = uart_read_byte_ex(ot_uart_ext_param.uart_id, &rx_data);
			    if (ret == -1)
			        break;

			    /* rx_buff_wr_idx == rx_buff_rd_idx means empty, so reserve one byte. */
			    if(rx_cnt < free_buff_len)  /* reserved one byte space. */
			    {
			        ot_uart_ext_param.rx_buff[rx_cnt] = rx_data;
			       rx_cnt++;
			    }
			    else
			    {
			        /* discard rx-data, rx overflow. */
			        ot_uart_ext_param.rx_over_flow = 1; //  bTRUE; // rx overflow, disable rx interrupt to stop rx.
			    }
			}
			otPlatUartReceived(ot_uart_ext_param.rx_buff,rx_cnt);
		}
	}

	if(usrCliSemaphoreRx!=NULL)
	{
		rtos_deinit_semaphore(&usrCliSemaphoreRx);
		usrCliSemaphoreRx=NULL;
	}
	os_printf("[Error] ot cli will be exited\r\n");
	rtos_delete_thread(usrOTCliRxHandle);
	usrOTCliRxHandle=NULL;
}

/**
 * Enable the UART.
 *
 * @retval OT_ERROR_NONE    Successfully enabled the UART.
 * @retval OT_ERROR_FAILED  Failed to enabled the UART.
 */
otError otPlatUartEnable(void)
{
   if(BK_OT_UART_PORT== CONFIG_UART_PRINT_PORT)
   {
      bk_ot_uart_log("[Warning] OpenThread uart port%d is the same as printf port%d\r\n",BK_OT_UART_PORT,CONFIG_UART_PRINT_PORT);
   }
   if(bk_uart_is_in_used(BK_OT_UART_PORT))
   		bk_uart_deinit(BK_OT_UART_PORT);
   memset(&ot_uart_ext_param, 0,sizeof(ot_uart_ext_t));
   ot_uart_ext_param.uart_id = BK_OT_UART_PORT;
   ot_uart_ext_param.rx_over_flow=0;

   uart_config_t sOtUartConfig={0};
   sOtUartConfig.baud_rate = UART_BAUD_RATE;
   sOtUartConfig.data_bits = UART_DATA_8_BITS;
   sOtUartConfig.parity    = UART_PARITY_NONE;
   sOtUartConfig.stop_bits = UART_STOP_BITS_1;
   sOtUartConfig.flow_ctrl = UART_FLOWCTRL_DISABLE;
   sOtUartConfig.src_clk   = UART_SCLK_XTAL_26M;


   bk_uart_init(BK_OT_UART_PORT,&sOtUartConfig);

   //bk_uart_isr_set_priority(BK_OT_UART_PORT, BK_PRINT_UART_ISR_DEFAULT_PRIORITY);
   bk_uart_disable_sw_fifo(BK_OT_UART_PORT);

   bk_uart_register_rx_isr(BK_OT_UART_PORT,(uart_isr_t)ot_uart_rx_isr,&ot_uart_ext_param);
   bk_uart_enable_rx_interrupt(BK_OT_UART_PORT);

	if(usrOTCliRxHandle == NULL)
	{
		if(OT_ERROR_NONE != rtos_create_thread(&usrOTCliRxHandle,
										BEKEN_DEFAULT_WORKER_PRIORITY,
										"OTCliRcv",
										(beken_thread_function_t)usrOTCliRxMain,
										2048,
										0))
		{
			os_printf("[Error]:Failed to create openthread cli rx\r\n");
			return OT_ERROR_NO_BUFS;
		}
	}

   return OT_ERROR_NONE;
}

/**
 * Disable the UART.
 *
 * @retval OT_ERROR_NONE    Successfully disabled the UART.
 * @retval OT_ERROR_FAILED  Failed to disable the UART.
 */
otError otPlatUartDisable(void)
{
   bk_uart_deinit(BK_OT_UART_PORT);
   bk_uart_disable_rx_interrupt(BK_OT_UART_PORT);
   memset(&ot_uart_ext_param, 0,sizeof(ot_uart_ext_t));
   return OT_ERROR_NONE;
}


/**
 * Send bytes over the UART.
 *
 * @param[in] aBuf        A pointer to the data buffer.
 * @param[in] aBufLength  Number of bytes to transmit.
 *
 * @retval OT_ERROR_NONE    Successfully started transmission.
 * @retval OT_ERROR_FAILED  Failed to start the transmission.
 */
otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
	bk_uart_write_bytes(ot_uart_ext_param.uart_id,aBuf,aBufLength);
	otPlatUartSendDone();

    return OT_ERROR_NONE;

}
#else
otError otPlatUartDisable(void)
{
   return OT_ERROR_NONE;
}
otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
	// bk_uart_write_bytes(BK_OT_UART_PORT, aBuf, aBufLength);
	shell_log_raw_data(aBuf, aBufLength);
	otPlatUartSendDone();
	shell_log_flush();
    return OT_ERROR_NONE;

}

#endif


/**
 * Flush the outgoing transmit buffer and wait for the data to be sent.
 * This is called when the CLI UART interface has a full buffer but still
 * wishes to send more data.
 *
 * @retval OT_ERROR_NONE                Flush succeeded, we can proceed to write more
 *                                      data to the buffer.
 *
 * @retval OT_ERROR_NOT_IMPLEMENTED     Driver does not support synchronous flush.
 * @retval OT_ERROR_INVALID_STATE       Driver has no data to flush.
 */
otError otPlatUartFlush(void)
{
#if CONFIG_SHELL_ASYNCLOG
   shell_log_flush();
#endif
   return OT_ERROR_NONE;
}

OT_TOOL_WEAK void otPlatUartSendDone(void){}
OT_TOOL_WEAK void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
   OT_UNUSED_VARIABLE(aBuf);
   OT_UNUSED_VARIABLE(aBufLength);
}


#endif //CONFIG_OPENTHREAD

