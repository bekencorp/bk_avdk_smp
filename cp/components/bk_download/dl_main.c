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
#include "sdkconfig.h"
#include "driver/wdt.h"
#include "dl_uart.h"
#include "dl_protocol.h"
#include "bk_private/bk_wdt.h"

#define DL_QSPI_TAG "dl_qspi"
#define DL_QSPI_LOGI(...) BK_LOGI(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGW(...) BK_LOGW(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGE(...) BK_LOGE(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGD(...) BK_LOGD(DL_QSPI_TAG, ##__VA_ARGS__)

extern uint64_t bk_aon_rtc_get_ms(void);
extern void bk_set_printf_enable(uint8_t enable);
dl_uart_dev_t *dl_qspi = &dl_uart;
static beken_thread_t dl_qspi_flash_thread = NULL;
beken_semaphore_t dl_ack_semph = NULL;
u8 uart_link_check_flag = 0;

static void dl_process_qspi_data(void)
{
	int ret         = 0;
	u64 start_cnt   = bk_aon_rtc_get_ms(); 
	u64 current_cnt = 0;
	u32 diff_cnt    = 0;

	ret = dl_qspi->dev_drv->deinit(dl_qspi);
	if(ret != BK_TRUE)
	{
		DL_QSPI_LOGE("deinit fail \r\n");
		return ;
	}

	bk_set_printf_enable(0);

	ret = dl_qspi->dev_drv->init(dl_qspi);
	if(ret != BK_TRUE)
	{
		DL_QSPI_LOGE("init fail \r\n");
		return ;
	}

	ret = dl_qspi->dev_drv->open(dl_qspi);
	if(ret != BK_TRUE)
	{
		DL_QSPI_LOGE("open fail \r\n");
		return ;
	}
	//DL_QSPI_LOGD("open ok \r\n");

	while(diff_cnt < 10160)
	{
		#if CONFIG_WDT_EN
			bk_wdt_feed();
		#endif
		#if (CONFIG_INT_AON_WDT)
			bk_int_aon_wdt_feed();
		#endif
		#if (CONFIG_TASK_WDT)
			bk_task_wdt_feed();
		#endif
		boot_rx_frm_handler();
		if(uart_link_check_flag == 1){

		}
		else
		{
			current_cnt = bk_aon_rtc_get_ms();
			diff_cnt = (u32)(current_cnt - start_cnt);    // alway in while.
		}
	}
	DL_QSPI_LOGD("dl over \r\n");

    return ;
}

static void dl_qspi_flash_task(void *arg)
{
    DL_QSPI_LOGI("enter  \r\n",__FUNCTION__);
	rtos_get_semaphore(&dl_ack_semph, BEKEN_WAIT_FOREVER);
	//rtos_get_semaphore(&dl_ack_semph, 0); //clear the semaphore state
    dl_process_qspi_data();

}

void download_qspi_flash(void)
{
	rtos_init_semaphore(&dl_ack_semph, 1);
    rtos_create_thread(&dl_qspi_flash_thread,
                    4,
                    "dl_qspi_flash",
                    dl_qspi_flash_task,
                    2048,
                    NULL);
}