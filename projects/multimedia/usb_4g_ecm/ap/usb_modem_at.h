/*
 * USB transport for the modem's vendor AT interface (intf3).
 *
 * This layer owns nothing but the USB side: claiming the interface, a receive
 * thread that pushes raw bytes up, and a serialised bulk write. It does not
 * parse AT at all - line assembly, OK/ERROR and URC handling live in modem_at.c.
 */
#ifndef _USB_MODEM_AT_H_
#define _USB_MODEM_AT_H_

#include <stdint.h>
#include <stdbool.h>

/* Raw bytes off the bulk IN endpoint. Called from the receive thread, so it
 * must not block: it is the only thing standing between the module and the
 * next bulk IN submit. */
typedef void (*usb_modem_at_rx_cb_t)(const uint8_t *data, uint32_t len);

/* Interface claimed (online=true) / released (online=false). On release the
 * callee must stop calling usb_modem_at_write() and cancel any wait; it runs in
 * the enumeration thread's disconnect path, so it must not block either. */
typedef void (*usb_modem_at_link_cb_t)(bool online);

/* Must be called once at startup, before enumeration can reach intf3. Besides
 * installing the callbacks it is the explicit link reference that keeps this
 * translation unit - and with it the CLASS_INFO_DEFINE registration record -
 * in the final image; nothing else in the project refers to it. */
void usb_modem_at_init(usb_modem_at_rx_cb_t rx_cb, usb_modem_at_link_cb_t link_cb);

/* True between the link callback's online and offline edges. */
bool usb_modem_at_is_online(void);

/* Write one command to the bulk OUT endpoint. Blocks until the transfer
 * completes, fails or times out; one writer at a time (serialised internally).
 * Returns the number of bytes written, or a negative CherryUSB error code.
 * len must be 1..USB_MODEM_AT_TX_MAX and timeout_ms must be non-zero. */
#define USB_MODEM_AT_TX_MAX 256U
int usb_modem_at_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms);

#endif /* _USB_MODEM_AT_H_ */
