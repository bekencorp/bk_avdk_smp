/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usbh_cdc_ecm.h"

#undef USB_DBG_TAG
#define USB_DBG_TAG "usbh_cdc_ecm"
#include "usb_log.h"

#define DEV_FORMAT "/dev/cdc_ether"

/* general descriptor field offsets */
#define DESC_bLength            0 /** Length offset */
#define DESC_bDescriptorType    1 /** Descriptor type offset */
#define DESC_bDescriptorSubType 2 /** Descriptor subtype offset */

/* interface descriptor field offsets */
#define INTF_DESC_bInterfaceNumber  2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting 3 /** Alternate setting offset */

#define CONFIG_USBHOST_CDC_ECM_PKT_FILTER   0x000C
#define CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE 1514U

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_cdc_ecm_rx_buffer[USB_ALIGN_UP(CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE, CONFIG_USB_ALIGN_SIZE)];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_cdc_ecm_tx_buffer[USB_ALIGN_UP(CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE, CONFIG_USB_ALIGN_SIZE)];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_cdc_ecm_inttx_buffer[USB_ALIGN_UP(16, CONFIG_USB_ALIGN_SIZE)];

static struct usbh_cdc_ecm g_cdc_ecm_class;

/* Stop handshake for the rx thread. Killing its URBs only makes the blocking
 * submit return; the thread still runs and still touches g_cdc_ecm_class and the
 * netif afterwards, so disconnect has to know whether it has actually left
 * before tearing any of that down.
 *
 * The state says who owns g_cdc_ecm_class right now:
 *   IDLE     nobody - safe to (re)connect
 *   RUNNING  the rx thread
 *   EXITING  the rx thread has made its last access and given the exit
 *            semaphore; disconnect does the teardown
 * The wait is unconditional: disconnect must not return while the thread can
 * still touch a USB object, because usbh_hubport_release() deletes hport->mutex
 * as soon as we return and the next enumeration memsets the whole hubport,
 * descriptors and endpoints included. Deferring only the class teardown would
 * not save a thread that is holding those.
 *
 * What bounds the wait is the disconnect path itself, not a timeout: hport
 * ->connected is already false before CLASS_DISCONNECT runs - see
 * usbh_hubport_release() in usbh_core.c, its only caller - and usbh_submit_urb()
 * refuses a disconnected port with -ENODEV. So an urb in flight is ended by the kills
 * below, and one submitted just after them fails immediately instead of
 * blocking. The thread's remaining delays are its own msleep(100)/msleep(128),
 * so it reaches the exit within roughly 128ms.
 *
 * The mutex covers the RUNNING->EXITING handoff, the one transition the rx
 * thread and the enumeration thread can reach at once, and orders the thread's
 * last shared write before disconnect publishes IDLE. */
enum {
    CDC_ECM_RX_IDLE = 0,
    CDC_ECM_RX_RUNNING,
    CDC_ECM_RX_EXITING,
};

static volatile bool g_cdc_ecm_rx_running;
static volatile uint8_t g_cdc_ecm_rx_state;
static usb_osal_sem_t g_cdc_ecm_rx_exit_sem;
static usb_osal_mutex_t g_cdc_ecm_rx_lock;

int usbh_cdc_ecm_rx_thread_prepare(void)
{
    /* Created once and reused: deleting either per disconnect would reintroduce
     * a race with a thread that has not woken up yet. */
    if (g_cdc_ecm_rx_lock == NULL) {
        g_cdc_ecm_rx_lock = usb_osal_mutex_create();
        if (g_cdc_ecm_rx_lock == NULL) {
            USB_LOG_ERR("Failed to create cdc ecm rx lock\r\n");
            return -USB_ERR_NOMEM;
        }
    }

    if (g_cdc_ecm_rx_exit_sem == NULL) {
        g_cdc_ecm_rx_exit_sem = usb_osal_sem_create(0);
        if (g_cdc_ecm_rx_exit_sem == NULL) {
            USB_LOG_ERR("Failed to create cdc ecm rx exit sem\r\n");
            return -USB_ERR_NOMEM;
        }
    } else {
        /* Backstop. Disconnect already drains the count under the lock after the
         * thread's last write, so a stale count should not reach here; this only
         * keeps one from surviving into the next connection if it somehow does.
         * Drained rather than usb_osal_sem_reset(), which is an empty stub in
         * this SDK's OSAL adaptor. Zero timeout does not block. */
        while (usb_osal_sem_take(g_cdc_ecm_rx_exit_sem, 0) == 0) {
        }
    }

    g_cdc_ecm_rx_running = true;
    g_cdc_ecm_rx_state = CDC_ECM_RX_RUNNING;
    return 0;
}

void usbh_cdc_ecm_rx_thread_abort(void)
{
    g_cdc_ecm_rx_running = false;
    g_cdc_ecm_rx_state = CDC_ECM_RX_IDLE;
}

static int usbh_cdc_ecm_set_eth_packet_filter(struct usbh_cdc_ecm *cdc_ecm_class, uint16_t filter_value)
{
    struct usb_setup_packet *setup;

    if (!cdc_ecm_class || !cdc_ecm_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = cdc_ecm_class->hport->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_INTERFACE;
    setup->bRequest = CDC_REQUEST_SET_ETHERNET_PACKET_FILTER;
    setup->wValue = filter_value;
    setup->wIndex = cdc_ecm_class->ctrl_intf;
    setup->wLength = 0;

    return usbh_control_transfer(cdc_ecm_class->hport, setup, NULL);
}

int usbh_cdc_ecm_get_connect_status(struct usbh_cdc_ecm *cdc_ecm_class)
{
    int ret;

    usbh_int_urb_fill(&cdc_ecm_class->intin_urb, cdc_ecm_class->hport, cdc_ecm_class->intin, g_cdc_ecm_inttx_buffer, 16, USB_OSAL_WAITING_FOREVER, NULL, NULL);
    ret = usbh_submit_urb(&cdc_ecm_class->intin_urb);
    if (ret < 0) {
        return ret;
    }

    if (g_cdc_ecm_inttx_buffer[1] == CDC_ECM_NOTIFY_CODE_NETWORK_CONNECTION) {
        if (g_cdc_ecm_inttx_buffer[2] == CDC_ECM_NET_CONNECTED) {
            cdc_ecm_class->connect_status = true;
        } else {
            cdc_ecm_class->connect_status = false;
        }
    } else if (g_cdc_ecm_inttx_buffer[1] == CDC_ECM_NOTIFY_CODE_CONNECTION_SPEED_CHANGE) {
        memcpy(cdc_ecm_class->speed, &g_cdc_ecm_inttx_buffer[8], 8);
    }
    return 0;
}

static int usbh_cdc_ecm_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    int ret;
    uint8_t altsetting = 0;
    char mac_buffer[12];
    uint8_t *p;
    uint8_t cur_iface = 0xff;
    uint8_t mac_str_idx = 0xff;

    struct usbh_cdc_ecm *cdc_ecm_class = &g_cdc_ecm_class;

    /* There is one static instance, so a re-plug that lands here while the
     * previous rx thread still owns it must be refused: the memset() below
     * would pull the endpoints out from under that thread. Cleared again once
     * the thread finishes its own teardown, so a later re-plug gets through. */
    if (g_cdc_ecm_rx_state != CDC_ECM_RX_IDLE) {
        USB_LOG_ERR("CDC ECM instance still busy (rx state:%d)\r\n", g_cdc_ecm_rx_state);
        return -USB_ERR_BUSY;
    }

    memset(cdc_ecm_class, 0, sizeof(struct usbh_cdc_ecm));

    cdc_ecm_class->hport = hport;
    cdc_ecm_class->ctrl_intf = intf;
    cdc_ecm_class->data_intf = intf + 1;

    hport->config.intf[intf].priv = cdc_ecm_class;
    hport->config.intf[intf + 1].priv = NULL;

    p = hport->raw_config_desc;
    while (p[DESC_bLength]) {
        switch (p[DESC_bDescriptorType]) {
            case USB_DESCRIPTOR_TYPE_INTERFACE:
                cur_iface = p[INTF_DESC_bInterfaceNumber];
                //cur_alt_setting = p[INTF_DESC_bAlternateSetting];
                break;
            case CDC_CS_INTERFACE:
                if ((cur_iface == cdc_ecm_class->ctrl_intf) && p[DESC_bDescriptorSubType] == CDC_FUNC_DESC_ETHERNET_NETWORKING) {
                    struct cdc_eth_descriptor *desc = (struct cdc_eth_descriptor *)p;
                    mac_str_idx = desc->iMACAddress;
                    cdc_ecm_class->max_segment_size = desc->wMaxSegmentSize;
                    goto get_mac;
                }
                break;

            default:
                break;
        }
        /* skip to next descriptor */
        p += p[DESC_bLength];
    }

get_mac:
    if (mac_str_idx == 0xff) {
        USB_LOG_ERR("Do not find cdc ecm mac string\r\n");
        return -1;
    }

    memset(mac_buffer, 0, 12);
    ret = usbh_get_string_desc(cdc_ecm_class->hport, mac_str_idx, (uint8_t *)mac_buffer, 12);
    if (ret < 0) {
        USB_LOG_ERR("Failed to read cdc ecm mac string %u, ret:%d\r\n", mac_str_idx, ret);
        return ret;
    }

    for (int i = 0, j = 0; i < 12; i += 2, j++) {
        char byte_str[3];
        byte_str[0] = mac_buffer[i];
        byte_str[1] = mac_buffer[i + 1];
        byte_str[2] = '\0';

        uint32_t byte = strtoul(byte_str, NULL, 16);
        cdc_ecm_class->mac[j] = (unsigned char)byte;
    }

    USB_LOG_INFO("CDC ECM MAC address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                 cdc_ecm_class->mac[0],
                 cdc_ecm_class->mac[1],
                 cdc_ecm_class->mac[2],
                 cdc_ecm_class->mac[3],
                 cdc_ecm_class->mac[4],
                 cdc_ecm_class->mac[5]);

    if (cdc_ecm_class->max_segment_size > CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE) {
        USB_LOG_ERR("CDC ECM Max Segment Size is overflow, default is %u, but now %u\r\n", CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE, cdc_ecm_class->max_segment_size);
    } else {
        USB_LOG_INFO("CDC ECM Max Segment Size:%u\r\n", cdc_ecm_class->max_segment_size);
    }

    /* enable int ep */
    ep_desc = &hport->config.intf[intf].altsetting[0].ep[0].ep_desc;
    USBH_EP_INIT(cdc_ecm_class->intin, ep_desc);

    if (hport->config.intf[intf + 1].altsetting_num > 1) {
        altsetting = hport->config.intf[intf + 1].altsetting_num - 1;

        for (uint8_t i = 0; i < hport->config.intf[intf + 1].altsetting[altsetting].intf_desc.bNumEndpoints; i++) {
            ep_desc = &hport->config.intf[intf + 1].altsetting[altsetting].ep[i].ep_desc;

            if (ep_desc->bEndpointAddress & 0x80) {
                USBH_EP_INIT(cdc_ecm_class->bulkin, ep_desc);
            } else {
                USBH_EP_INIT(cdc_ecm_class->bulkout, ep_desc);
            }
        }

        USB_LOG_INFO("Select cdc ecm altsetting: %d\r\n", altsetting);
        /* Bail out instead of falling through: the endpoints above were taken
         * from the alt1 descriptor, so if the device stays on alt0 its data
         * endpoints are not active and no netif must be created. */
        ret = usbh_set_interface(cdc_ecm_class->hport, cdc_ecm_class->data_intf, altsetting);
        if (ret < 0) {
            USB_LOG_ERR("Failed to select cdc ecm altsetting %d, ret:%d\r\n", altsetting, ret);
            return ret;
        }
    } else {
        for (uint8_t i = 0; i < hport->config.intf[intf + 1].altsetting[0].intf_desc.bNumEndpoints; i++) {
            ep_desc = &hport->config.intf[intf + 1].altsetting[0].ep[i].ep_desc;

            if (ep_desc->bEndpointAddress & 0x80) {
                USBH_EP_INIT(cdc_ecm_class->bulkin, ep_desc);
            } else {
                USBH_EP_INIT(cdc_ecm_class->bulkout, ep_desc);
            }
        }
    }

    /* bit0 Promiscuous
    * bit1 ALL Multicast
    * bit2 Directed
    * bit3 Broadcast
    * bit4 Multicast
    */
    ret = usbh_cdc_ecm_set_eth_packet_filter(cdc_ecm_class, CONFIG_USBHOST_CDC_ECM_PKT_FILTER);
    if (ret < 0) {
        return ret;
    }
    USB_LOG_INFO("Set CDC ECM packet filter:%04x\r\n", CONFIG_USBHOST_CDC_ECM_PKT_FILTER);

    strncpy(hport->config.intf[intf].devname, DEV_FORMAT, CONFIG_USBHOST_DEV_NAMELEN);

    USB_LOG_INFO("Register CDC ECM Class:%s\r\n", hport->config.intf[intf].devname);

    ret = usbh_cdc_ecm_run(cdc_ecm_class);
    if (ret < 0) {
        /* run() rolled back whatever it had built, so leave no devname behind
         * either: an interface that carries no traffic is worse than no
         * interface, and disconnect keys its teardown off this name. */
        USB_LOG_ERR("Failed to start CDC ECM, ret:%d\r\n", ret);
        memset(hport->config.intf[intf].devname, 0, CONFIG_USBHOST_DEV_NAMELEN);
        return ret;
    }

    return ret;
}

static int usbh_cdc_ecm_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    int ret = 0;

    struct usbh_cdc_ecm *cdc_ecm_class = (struct usbh_cdc_ecm *)hport->config.intf[intf].priv;

    if (cdc_ecm_class) {
        /* Before the kills, not after: a thread that wakes up between the kill
         * and the flag going down just submits another WAITING_FOREVER urb and
         * is then unreachable. */
        g_cdc_ecm_rx_running = false;
        /* Same reason on the tx side: usbh_cdc_ecm_eth_output() gates on this
         * alone, and the netif stays up until stop() below. Without it lwIP can
         * keep handing us frames while we are tearing the instance down. */
        cdc_ecm_class->connect_status = false;

        if (cdc_ecm_class->bulkin) {
            usbh_kill_urb(&cdc_ecm_class->bulkin_urb);
        }

        if (cdc_ecm_class->bulkout) {
            usbh_kill_urb(&cdc_ecm_class->bulkout_urb);
        }

        if (cdc_ecm_class->intin) {
            usbh_kill_urb(&cdc_ecm_class->intin_urb);
        }

        if (hport->config.intf[intf].devname[0] != '\0') {
            USB_LOG_INFO("Unregister CDC ECM Class:%s\r\n", hport->config.intf[intf].devname);

            /* Wait for the thread to confirm it has stopped touching the USB
             * objects. Nothing below may run before that: stop() removes the
             * netif it feeds, the memset() clears the endpoints it reads, and
             * our caller then deletes hport->mutex and later reuses the whole
             * hubport. No timeout - see the handshake comment at the top for why
             * the thread is already bounded, and why returning early would hand
             * a live thread pointers that are about to be recycled. */
            if (g_cdc_ecm_rx_state == CDC_ECM_RX_RUNNING && g_cdc_ecm_rx_exit_sem) {
                usb_osal_sem_take(g_cdc_ecm_rx_exit_sem, USB_OSAL_WAITING_FOREVER);
            }

            if (g_cdc_ecm_rx_lock) {
                /* Taking the lock puts us after the thread's last shared write,
                 * whether we observed EXITING above or raced it. Then drain, so
                 * a count the thread left behind after exiting on its own cannot
                 * satisfy the next disconnect's wait, and publish IDLE here
                 * rather than letting the thread do it - it must not write
                 * anything once a new connection may already own the state. */
                usb_osal_mutex_take(g_cdc_ecm_rx_lock);
                if (g_cdc_ecm_rx_exit_sem) {
                    while (usb_osal_sem_take(g_cdc_ecm_rx_exit_sem, 0) == 0) {
                    }
                }
                usbh_cdc_ecm_stop(cdc_ecm_class);
                memset(cdc_ecm_class, 0, sizeof(struct usbh_cdc_ecm));
                g_cdc_ecm_rx_state = CDC_ECM_RX_IDLE;
                usb_osal_mutex_give(g_cdc_ecm_rx_lock);
            } else {
                /* No lock means run() never got as far as creating one, so no
                 * thread was ever started and there is nothing to order against. */
                usbh_cdc_ecm_stop(cdc_ecm_class);
                memset(cdc_ecm_class, 0, sizeof(struct usbh_cdc_ecm));
                g_cdc_ecm_rx_state = CDC_ECM_RX_IDLE;
            }
        } else {
            memset(cdc_ecm_class, 0, sizeof(struct usbh_cdc_ecm));
        }
    }

    return ret;
}

void usbh_cdc_ecm_rx_thread(CONFIG_USB_OSAL_THREAD_SET_ARGV)
{
    uint32_t g_cdc_ecm_rx_length;
    int ret;

    (void)CONFIG_USB_OSAL_THREAD_GET_ARGV;
    USB_LOG_INFO("Create cdc ecm rx thread\r\n");
    // clang-format off
find_class:
    // clang-format on
    /* Every path that loops back here must re-test the stop flag, otherwise a
     * disconnect that kills the URBs just sends the thread round again. */
    if (!g_cdc_ecm_rx_running) {
        goto delete;
    }
    g_cdc_ecm_class.connect_status = false;
    if (usbh_find_class_instance("/dev/cdc_ether") == NULL) {
        goto delete;
    }

    while (g_cdc_ecm_rx_running && g_cdc_ecm_class.connect_status == false) {
        ret = usbh_cdc_ecm_get_connect_status(&g_cdc_ecm_class);
        if (ret < 0) {
            usb_osal_msleep(100);
            goto find_class;
        }
        usb_osal_msleep(128);
    }

    g_cdc_ecm_rx_length = 0;
    while (g_cdc_ecm_rx_running) {
        usbh_bulk_urb_fill(&g_cdc_ecm_class.bulkin_urb, g_cdc_ecm_class.hport, g_cdc_ecm_class.bulkin, g_cdc_ecm_rx_buffer, CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE, USB_OSAL_WAITING_FOREVER, NULL, NULL);
        ret = usbh_submit_urb(&g_cdc_ecm_class.bulkin_urb);
        if (ret < 0) {
            goto find_class;
        }

        g_cdc_ecm_rx_length = g_cdc_ecm_class.bulkin_urb.actual_length;

        /* A transfer is complete because last packet is a short packet.
         * Short packet is not zero, match g_cdc_ecm_rx_length % USB_GET_MAXPACKETSIZE(g_cdc_ecm_class.bulkin->wMaxPacketSize).
         * Short packet is zero, check if g_cdc_ecm_class.bulkin_urb.actual_length < transfer_size, for example transfer is complete with size is 512 < 1514.
         * This case is always true
        */
        if (g_cdc_ecm_rx_length % USB_GET_MAXPACKETSIZE(g_cdc_ecm_class.bulkin->wMaxPacketSize) ||
            (g_cdc_ecm_class.bulkin_urb.actual_length < CONFIG_USBHOST_CDC_ECM_ETH_MAX_SIZE)) {
            USB_LOG_DBG("rxlen:%d\r\n", g_cdc_ecm_rx_length);

            usbh_cdc_ecm_eth_input(g_cdc_ecm_rx_buffer, g_cdc_ecm_rx_length);

            g_cdc_ecm_rx_length = 0;
        } else {
            /* There's no way to run here. */
        }
    }
    // clang-format off
delete:
    USB_LOG_INFO("Delete cdc ecm rx thread\r\n");
    g_cdc_ecm_rx_running = false;

    /* Publish the exit and signal it as one step, so a disconnect that takes the
     * lock afterwards is guaranteed to be past both. Everything this thread will
     * ever touch has been touched by now; it does not write IDLE and does not
     * come back, which is what keeps a late signal from this connection out of
     * the next one. */
    if (g_cdc_ecm_rx_lock) {
        usb_osal_mutex_take(g_cdc_ecm_rx_lock);
    }
    g_cdc_ecm_rx_state = CDC_ECM_RX_EXITING;
    if (g_cdc_ecm_rx_exit_sem) {
        usb_osal_sem_give(g_cdc_ecm_rx_exit_sem);
    }
    if (g_cdc_ecm_rx_lock) {
        usb_osal_mutex_give(g_cdc_ecm_rx_lock);
    }
    usb_osal_thread_delete(NULL);
    // clang-format on
}

uint8_t *usbh_cdc_ecm_get_eth_txbuf(void)
{
    return g_cdc_ecm_tx_buffer;
}

int usbh_cdc_ecm_eth_output(uint32_t buflen)
{
    if (g_cdc_ecm_class.connect_status == false) {
        return -USB_ERR_NOTCONN;
    }

    USB_LOG_DBG("txlen:%d\r\n", buflen);

    usbh_bulk_urb_fill(&g_cdc_ecm_class.bulkout_urb, g_cdc_ecm_class.hport, g_cdc_ecm_class.bulkout, g_cdc_ecm_tx_buffer, buflen, USB_OSAL_WAITING_FOREVER, NULL, NULL);
    return usbh_submit_urb(&g_cdc_ecm_class.bulkout_urb);
}

const struct usbh_class_driver cdc_ecm_class_driver = {
    .driver_name = "cdc_ecm",
    .connect = usbh_cdc_ecm_connect,
    .disconnect = usbh_cdc_ecm_disconnect
};

CLASS_INFO_DEFINE const struct usbh_class_info cdc_ecm_class_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS | USB_CLASS_MATCH_INTF_SUBCLASS | USB_CLASS_MATCH_INTF_PROTOCOL,
    .bInterfaceClass = USB_DEVICE_CLASS_CDC,
    .bInterfaceSubClass = CDC_ETHERNET_NETWORKING_CONTROL_MODEL,
    .bInterfaceProtocol = CDC_COMMON_PROTOCOL_NONE,
    .id_table = NULL,
    .class_driver = &cdc_ecm_class_driver
};