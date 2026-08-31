#ifndef _BOARD_UART_H_
#define _BOARD_UART_H_
#include <string.h>
/*********************************************************************
 * INCLUDES
 */
#include <driver/gpio.h>

/*********************************************************************
 * DEFINITIONS
 */

/* SCH-Robot V2: UART2 P35/P36 (NFC_RXD/NFC_TXD), IRQ P37 (NFC_IRQ). */
#define UART2_RXD_PIN                   GPIO_35
#define UART2_TXD_PIN                   GPIO_36
#define MFRC522_G_INT_PIN               GPIO_37

#define MFRC522_MX_PIN                  255
#define MFRC522_DTRQ_PIN                255
#define MFRC522_TXD_PIN                 UART2_RXD_PIN
#define MFRC522_RXD_PIN                 UART2_TXD_PIN
#define MFRC522_RST_GPIO_PIN            GPIO_53        // NFC复位
#define MFRC522_RST_LOW                 0x00
#define MFRC522_RST_HIGH                0x02
#define NFC_DEBUG_CODE                  (0)
#define MFRC522_WAIT_OUT_TIME           (20)
enum
{
    HOST_STATE_IDLE = 0,
    HOST_STATE_WAIT_DATA,
    HOST_STATE_WAIT_ACK,
} ;

/*********************************************************************
 * API FUNCTIONS
 */
void mfrc522_uart_init(void);
void mfrc522_uart_deinit(void);
void mfrc522_gpio_write(uint8_t mode);
uint8_t mfrc522_read_rawRc(uint8_t addr);
void mfrc522_write_rawRc(uint8_t addr, uint8_t writeData);
#endif /* _BOARD_UART_H_ */