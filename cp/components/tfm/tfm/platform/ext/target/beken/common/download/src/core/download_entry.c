#include <stdio.h>
#include "../../include/download.h"
#include "../download_internal.h"
#include "bl_bk_reg.h"

enum {
	DOWNLOAD_UART_DEFAULT_BAUD_RATE = 115200u,
	DOWNLOAD_UART_RX_FIFO_THRESHOLD = 32u,
	DOWNLOAD_UART_TX_FIFO_THRESHOLD = 64u,
};

static const download_uart_config_t s_download_uart_default_config = {
	.baud_rate = DOWNLOAD_UART_DEFAULT_BAUD_RATE,
	.rx_fifo_threshold = DOWNLOAD_UART_RX_FIFO_THRESHOLD,
	.tx_fifo_threshold = DOWNLOAD_UART_TX_FIFO_THRESHOLD,
};

static void download_flash_bus_init(void)
{
	SPI0_ENABLE;
}

static void download_flash_bus_deinit(void)
{
	download_uart_disable();
	SPI0_DISABLE;
	SET_FLASHCTRL_RW_FLASH;	
}

extern u8 uart_link_check_flag ;
void legacy_boot_main(void)
{
	int restart_t = 10;

	download_flash_bus_init();
	download_uart_init(&s_download_uart_default_config);

	u32 i = timer_init(restart_t * 30);
	while(i--)
	{
		boot_rx_frm_handler();
		wdt_time_set(DOWNLOAD_WDT_VALUE);
		if(uart_link_check_flag == 1)
		{
			while(1)
			{
				boot_rx_frm_handler();
				wdt_time_set(DOWNLOAD_WDT_VALUE);
			}
		}
	}

	download_flash_bus_deinit();
	wdt_time_set(DOWNLOAD_WDT_VALUE);
    return ;
}
