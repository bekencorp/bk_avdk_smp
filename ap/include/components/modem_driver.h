// Copyright 2024-2025 Beken
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem communication protocol type
 *
 * Defines the upper-layer data-link / network protocol used between the host
 * MCU and the Modem module.
 */
typedef enum bk_modem_comm_proto_e
{
    INVALID_MODE,    /**< Invalid protocol, used as default or error indicator */
    PPP_MODE,        /**< PPP dial-up mode, establishes a network connection with the Modem over PPP */
    UART_NIC_MODE,   /**< UART NIC mode, treats UART as a virtual NIC to transport Ethernet frames */
}bk_modem_comm_proto;

/**
 * @brief Modem physical communication interface type
 *
 * Defines the physical hardware interface connecting the host MCU and the
 * Modem module.
 */
typedef enum bk_modem_comm_if_e
{
    INVALID_IF,      /**< Invalid interface, used as default or error indicator */
    USB_IF,          /**< USB interface, communicates with the Modem module over USB bus */
    UART_IF,         /**< UART interface, communicates with the Modem module over serial port */
    SPI_IF,          /**< SPI interface, communicates with the Modem module over SPI bus */
}bk_modem_comm_if;

/*******************************************************************************
*                      Function Declarations
*******************************************************************************/

/**
 * @brief     Modem Driver initialization
 *
 * Initialize the bk modem driver, then it will build a connection to modem modules.
 * Make sure to connect to a modem mudule by usb in hardware before call this API.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_modem_init(bk_modem_comm_proto comm_proto, bk_modem_comm_if comm_if);

/**
 * @brief     Modem Driver uninstallation
 *
 * Turn off the Modem driver, and it will disconnect to modem modules.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_modem_deinit(void);


