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

#ifndef _dl_uart_h_
#define _dl_uart_h_

#ifdef __cplusplus
extern "C" {
#endif
#include <common/bk_include.h>
#include "dl_uart.h"
#include "driver/uart.h"

struct dl_uart_dev_drv;
typedef struct
{
	struct dl_uart_dev_drv *dev_drv;
	u8      		  		dev_id;
} dl_uart_dev_t;

typedef struct dl_uart_dev_drv
{
	bool		(*init)(dl_uart_dev_t *dl_uart_dev);
	bool 		(*open)(dl_uart_dev_t *dl_uart_dev);
	bool 		(*set_baud_rate)(dl_uart_dev_t *dl_uart_dev, u32 rate);
	bk_err_t 	(*write)(dl_uart_dev_t *dl_uart_dev, const void *data, u32 BufLen);
	bk_err_t 	(*read_byte)(dl_uart_dev_t *dl_uart_dev, u8 *rx_data);
	bk_err_t    (*read_ready)(dl_uart_dev_t *dl_uart_dev);
	bool    	(*deinit)(dl_uart_dev_t *dl_uart_dev);

}dl_uart_dev_drv_t;

extern dl_uart_dev_t	dl_uart;

extern bk_err_t uart_read_ready(uart_id_t id);
extern uint32_t uart_get_interrupt_status(uart_id_t id);
extern void uart_clear_interrupt_status(uart_id_t id, uint32_t int_status);
extern int uart_read_byte_ex(uart_id_t id, uint8_t *ch);

#define portMAX_DELAY    (0xFFFF)

#ifdef __cplusplus
}
#endif

#endif /* _dl_protocol_h_ */

