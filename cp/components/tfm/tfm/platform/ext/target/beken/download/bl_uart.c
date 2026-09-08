#include "system.h"
#include "bl_config.h"
#include "bl_uart.h"
#include "cmsis_gcc.h"
#define TX_FIFO_THRD                0x40
#define RX_FIFO_THRD                0x40


volatile char uart_rx_done_state = 0;
volatile unsigned long uart_rx_index = 0;
volatile unsigned long uart_rx_index_cur = 0;

unsigned char uart_rx_end_flag = 0;



char buf[128 / 2];
u16 COUNT;
u32 g_baud_rate;

uint32_t uart_get_clock_freq(void)
{
#if CONFIG_XTAL_40M
	if (AON_PMU_IS_XTAL_40M) {
		return UART_CLOCK_FREQ_40M;
	} else {
		return UART_CLOCK_FREQ_26M;
	}
#else
	return UART_CLOCK_FREQ_26M;
#endif
}

void boot_uart_init(uint32_t baud_rate,uint8_t rx_fifo_thr)
{
    uint32_t baud_divisor;
    g_baud_rate = baud_rate;
    baud_divisor           = UART_CLOCK_FREQ/baud_rate;
    baud_divisor           = baud_divisor-1;

    COUNT = UART_TX_FIFO_COUNT;

    REG_APB3_UART_CFG      = (   (DEF_STOP_BIT    << sft_UART_CONF_STOP_LEN)
                                 | (DEF_PARITY_MODE << sft_UART_CONF_PAR_MODE)
                                 | (DEF_PARITY_EN   << sft_UART_CONF_PAR_EN)
                                 | (DEF_DATA_LEN    << sft_UART_CONF_UART_LEN)
                                 | (baud_divisor    << sft_UART_CONF_CLK_DIVID)
                                 | (DEF_IRDA_MODE   << sft_UART_CONF_IRDA)
                                 | (DEF_RX_EN       << sft_UART_CONF_RX_ENABLE)
                                 | (DEF_TX_EN       << sft_UART_CONF_TX_ENABLE));

    REG_APB3_UART_FIFO_THRESHOLD = ((rx_fifo_thr << sft_UART_FIFO_CONF_RX_FIFO)
                                    | (TX_FIFO_THRD << sft_UART_FIFO_CONF_TX_FIFO));
    REG_APB3_UART_INT_ENABLE=0;             /* Disable UART Interrupts */
    REG_APB3_UART_INT_ENABLE = bit_UART_INT_RX_NEED_READ | bit_UART_INT_RX_STOP_END ; //enable Rx interrupt

}


void boot_uart_init_tx(uint32_t baud_rate,uint8_t rx_fifo_thr)
{
    uint32_t baud_divisor;
    g_baud_rate = baud_rate;
    baud_divisor           = UART_CLOCK_FREQ/baud_rate;
    baud_divisor           = baud_divisor-1;

    COUNT = UART_TX_FIFO_COUNT;

    REG_APB3_UART_CFG      = (   (DEF_STOP_BIT    << sft_UART_CONF_STOP_LEN)
                                 | (DEF_PARITY_MODE << sft_UART_CONF_PAR_MODE)
                                 | (DEF_PARITY_EN   << sft_UART_CONF_PAR_EN)
                                 | (DEF_DATA_LEN    << sft_UART_CONF_UART_LEN)
                                 | (baud_divisor    << sft_UART_CONF_CLK_DIVID)
                                 | (DEF_IRDA_MODE   << sft_UART_CONF_IRDA)
//                                 | (DEF_RX_EN       << sft_UART_CONF_RX_ENABLE)
                                 | (DEF_TX_EN       << sft_UART_CONF_TX_ENABLE));

    REG_APB3_UART_FIFO_THRESHOLD = ((rx_fifo_thr << sft_UART_FIFO_CONF_RX_FIFO)
                                    | (TX_FIFO_THRD << sft_UART_FIFO_CONF_TX_FIFO));
    REG_APB3_UART_INT_ENABLE=0;             /* Disable UART Interrupts */
//    REG_APB3_UART_INT_ENABLE = bit_UART_INT_RX_NEED_READ | bit_UART_INT_RX_STOP_END ; //enable Rx interrupt

}



void debug_uart_send_poll( uint8_t *buff, int len )
{
//	return;
    while (len--) {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(*buff++);
    }
}
void debug2_uart_send_poll( UINT8 *buff, int len )
{
    while (len--) {
        while(!UART_TX_WRITE_READY);
        UART_WRITE_BYTE(*buff++);
    }
}


int bl_printf(const char *fmt, ...)
{
	return 0;
    PRINT_BUF_PREPARE(rc, buf, fmt);
    debug_uart_send_poll((uint8_t *)&buf[0], rc);
    return rc;
}

int ad_printf(const char *fmt, ...)
{
		return 0;
    PRINT_BUF_PREPARE(rc, buf, fmt);
    debug_uart_send_poll((uint8_t *)&buf[0], rc);
    return rc;
}

void *bl_memcpy(void *d, const void *s, size_t n)
{
        /* attempt word-sized copying only if buffers have identical alignment */

        unsigned char *d_byte = (unsigned char *)d;
        const unsigned char *s_byte = (const unsigned char *)s;
        const uint32_t mask = sizeof(uint32_t) - 1;

        if ((((uint32_t)d ^ (uint32_t)s_byte) & mask) == 0) {

                /* do byte-sized copying until word-aligned or finished */

                while (((uint32_t)d_byte) & mask) {
                        if (n == 0) {
                                return d;
                        }
                        *(d_byte++) = *(s_byte++);
                        n--;
                };

                /* do word-sized copying as long as possible */

                uint32_t *d_word = (uint32_t *)d_byte;
                const uint32_t *s_word = (const uint32_t *)s_byte;

                while (n >= sizeof(uint32_t)) {
                        *(d_word++) = *(s_word++);
                        n -= sizeof(uint32_t);
                }

                d_byte = (unsigned char *)d_word;
                s_byte = (unsigned char *)s_word;
        }

        /* do byte-sized copying until finished */

        while (n > 0) {
                *(d_byte++) = *(s_byte++);
                n--;
        }

        return d;
}


void *bl_memset(void *s, int c, size_t n)
{
    unsigned char *ss = s;
    while (n--) {
        *ss++ = c;
    }
    return s;
}

void bl_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	// uint32_t int_level = bl_disable_irq();
	bl_memcpy((char*)user_buf, (const char*)(0x02000000+address),size);
	// bl_enable_irq(int_level);
}

void bl_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	// uint32_t int_level = bl_disable_irq();
	bl_memcpy((char*)(0x02000000+address), (const char*)user_buf,size);
	// bl_enable_irq(int_level);
}

void uart_send(unsigned char *buff, int len)
{
	 while (len--) {
	        while(!UART_TX_WRITE_READY);
	        UART_WRITE_BYTE(*buff++);
	    }
}
