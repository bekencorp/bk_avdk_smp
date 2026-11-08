/*
 * Copyright (c) 2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7259 FS/LS USB IP CherryUSB host port -- external interface.
 *
 * This port is instantiated on host bus 1 (HS IP owns bus 0). The
 * core/hub layer calls the standard usb_hc.h contract entries which the
 * beken_musb HS port dispatches here via the CONFIG_USB_FS_HOST guards.
 */
#ifndef USB_HC_BEKEN_FS_H
#define USB_HC_BEKEN_FS_H

#include <stdbool.h>
#include "usb_hc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct usbh_bus;

/* Bus id owned by the FS port (HS = 0). */
#define BK_FS_USB_BUS_ID  1

/* Contract entries, FS-port side. The HS port (usb_hc_beken_musb.c) keeps
 * the original symbol names and forwards busid!=0 calls here. */
int usb_hc_beken_fs_init(struct usbh_bus *bus);
int usb_hc_beken_fs_deinit(struct usbh_bus *bus);
int usbh_beken_fs_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf);
int usbh_beken_fs_submit_urb(struct usbh_urb *urb);
int usbh_beken_fs_kill_urb(struct usbh_urb *urb);

/* Connection status for bk_usb_fs_get_device_connect_status(). */
bool usb_hc_beken_fs_port_connect_status(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_HC_BEKEN_FS_H */
