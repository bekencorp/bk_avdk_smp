/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_CDC_ECM_H
#define USBH_CDC_ECM_H

#include "usb_cdc.h"

struct usbh_cdc_ecm {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *bulkin;  /* Bulk IN endpoint */
    struct usb_endpoint_descriptor *bulkout; /* Bulk OUT endpoint */
    struct usb_endpoint_descriptor *intin;   /* Interrupt IN endpoint */
    struct usbh_urb bulkout_urb; /* Bulk out endpoint */
    struct usbh_urb bulkin_urb; /* Bulk IN endpoint */
    struct usbh_urb intin_urb; /* Interrupt IN endpoint */

    uint8_t ctrl_intf; /* Control interface number */
    uint8_t data_intf; /* Data interface number */
    uint8_t minor;

    uint8_t mac[6];
    bool connect_status;
    uint16_t max_segment_size;
    uint32_t speed[2];

    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int usbh_cdc_ecm_get_connect_status(struct usbh_cdc_ecm *cdc_ecm_class);

/* Returns < 0 if the interface could not be brought up, with everything it had
 * built already rolled back; usbh_cdc_ecm_connect() then fails too instead of
 * registering a device that carries no traffic. */
int usbh_cdc_ecm_run(struct usbh_cdc_ecm *cdc_ecm_class);
void usbh_cdc_ecm_stop(struct usbh_cdc_ecm *cdc_ecm_class);

uint8_t *usbh_cdc_ecm_get_eth_txbuf(void);
int usbh_cdc_ecm_eth_output(uint32_t buflen);
void usbh_cdc_ecm_eth_input(uint8_t *buf, uint32_t buflen);
void usbh_cdc_ecm_rx_thread(CONFIG_USB_OSAL_THREAD_SET_ARGV);

/* Arm/undo the rx thread stop handshake. Call prepare() before creating the
 * thread and abort() if that creation fails; disconnect then waits for the
 * thread to confirm it has exited before tearing the instance down. */
int usbh_cdc_ecm_rx_thread_prepare(void);
void usbh_cdc_ecm_rx_thread_abort(void);

#ifdef __cplusplus
}
#endif

#endif /* USBH_CDC_ECM_H */
