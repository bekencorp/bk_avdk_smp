/*
 * USB transport for the modem's vendor AT interface (intf3). See usb_modem_at.h
 * for the contract; this file is USB only and never looks at AT syntax.
 */
#include "usbh_core.h"
#include "usb_modem_at.h"

#undef USB_DBG_TAG
#define USB_DBG_TAG "usbh_modem_at"
#include "usb_log.h"

#define DEV_FORMAT "/dev/modem_at"

/* The module reports MPS 512 on both bulk endpoints. One MPS per submit keeps
 * the short-packet case trivial: any transfer shorter than the buffer is a
 * complete USB transfer, nothing is left half-read in the controller. */
#define MODEM_AT_RX_BUF_SIZE 512U

/* intf3 is vendor class; protocol is what separates it from the module's other
 * vendor interfaces (intf2 is 0x04, intf4 is 0x05). Interface number 3 is only
 * a consistency check - the match itself does not use it. */
#define MODEM_AT_INTF_PROTOCOL 0x03
#define MODEM_AT_EXPECT_INTF   3

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_at_rx_buffer[USB_ALIGN_UP(MODEM_AT_RX_BUF_SIZE, CONFIG_USB_ALIGN_SIZE)];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_at_tx_buffer[USB_ALIGN_UP(USB_MODEM_AT_TX_MAX, CONFIG_USB_ALIGN_SIZE)];

struct modem_at_port {
    struct usbh_hubport *hport;
    uint8_t intf;
    struct usb_endpoint_descriptor *bulkin;
    struct usb_endpoint_descriptor *bulkout;
    struct usbh_urb bulkin_urb;
    struct usbh_urb bulkout_urb;
};

static struct modem_at_port g_at_port;

/* Same shape as the CDC-ECM stop handshake, and for the same reason: killing
 * the rx thread's urb only makes its blocking submit return, the thread still
 * runs and still dereferences g_at_port afterwards. Disconnect must know it has
 * actually left before clearing any of that.
 *   IDLE     nobody owns g_at_port - safe to (re)connect
 *   RUNNING  the rx thread owns it
 *   EXITING  the thread is past its last access to it
 */
enum {
    MODEM_AT_RX_IDLE = 0,
    MODEM_AT_RX_RUNNING,
    MODEM_AT_RX_EXITING,
};

static volatile uint8_t g_at_rx_state = MODEM_AT_RX_IDLE;
static volatile bool g_at_rx_running = false;
static volatile bool g_at_online = false;
static usb_osal_sem_t g_at_rx_exit_sem = NULL;
/* One writer at a time: a second usbh_submit_urb() on the same urb while the
 * first still waits would overwrite its fields and lose its completion. */
static usb_osal_mutex_t g_at_tx_lock = NULL;

static usb_modem_at_rx_cb_t g_at_rx_cb = NULL;
static usb_modem_at_link_cb_t g_at_link_cb = NULL;

void usb_modem_at_init(usb_modem_at_rx_cb_t rx_cb, usb_modem_at_link_cb_t link_cb)
{
    g_at_rx_cb = rx_cb;
    g_at_link_cb = link_cb;
    USB_LOG_INFO("modem AT transport ready, waiting for intf%u\r\n", MODEM_AT_EXPECT_INTF);
}

bool usb_modem_at_is_online(void)
{
    return g_at_online;
}

int usb_modem_at_write(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    struct modem_at_port *port = &g_at_port;
    int ret;

    /* timeout 0 means "submit and return" in CherryUSB, which with a NULL
     * completion callback would leave the transfer unobserved and the buffer
     * reused under it. This path is synchronous by contract, so refuse it. */
    if (!data || !len || len > USB_MODEM_AT_TX_MAX || !timeout_ms) {
        return -USB_ERR_INVAL;
    }
    if (!g_at_online || !g_at_tx_lock) {
        return -USB_ERR_NODEV;
    }

    usb_osal_mutex_take(g_at_tx_lock);
    /* Re-checked under the lock: the offline edge may have landed while we
     * waited, and port->bulkout/hport are cleared on disconnect. */
    if (!g_at_online || !port->bulkout) {
        usb_osal_mutex_give(g_at_tx_lock);
        return -USB_ERR_NODEV;
    }

    memcpy(g_at_tx_buffer, data, len);
    usbh_bulk_urb_fill(&port->bulkout_urb, port->hport, port->bulkout,
                       g_at_tx_buffer, len, timeout_ms, NULL, NULL);
    ret = usbh_submit_urb(&port->bulkout_urb);
    if (ret == 0) {
        ret = (int)port->bulkout_urb.actual_length;
    }
    usb_osal_mutex_give(g_at_tx_lock);
    return ret;
}

static void modem_at_rx_thread(CONFIG_USB_OSAL_THREAD_SET_ARGV)
{
    struct modem_at_port *port = &g_at_port;
    int ret;

    USB_LOG_INFO("modem AT rx thread start\r\n");

    while (g_at_rx_running) {
        usbh_bulk_urb_fill(&port->bulkin_urb, port->hport, port->bulkin,
                           g_at_rx_buffer, MODEM_AT_RX_BUF_SIZE,
                           USB_OSAL_WAITING_FOREVER, NULL, NULL);
        ret = usbh_submit_urb(&port->bulkin_urb);
        if (ret < 0) {
            /* A kill from disconnect lands here too, and g_at_rx_running is
             * already false by then, so the loop condition is the exit. Other
             * errors (stall, babble) are retried: the module may just have
             * nothing to say yet, and bailing out would silently kill the AT
             * channel for the rest of the session. */
            if (g_at_rx_running) {
                USB_LOG_DBG("modem AT bulk in ret:%d\r\n", ret);
            }
            continue;
        }

        if (port->bulkin_urb.actual_length && g_at_rx_cb) {
            g_at_rx_cb(g_at_rx_buffer, port->bulkin_urb.actual_length);
        }
    }

    /* Published before the sem: disconnect uses the state to decide whether it
     * has to wait at all, and must not wait on a thread that already left. */
    g_at_rx_state = MODEM_AT_RX_EXITING;
    if (g_at_rx_exit_sem) {
        usb_osal_sem_give(g_at_rx_exit_sem);
    }
    USB_LOG_INFO("modem AT rx thread exit\r\n");
    usb_osal_thread_delete(NULL);
}

static int modem_at_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct modem_at_port *port = &g_at_port;
    struct usb_endpoint_descriptor *ep_desc;
    struct usbh_interface_altsetting *alt;
    uint8_t ep_type;

    /* One static instance. A re-plug reaching here while the previous rx
     * thread still owns it must be refused - the memset() below would pull the
     * endpoints out from under that thread. */
    if (g_at_rx_state != MODEM_AT_RX_IDLE) {
        USB_LOG_ERR("modem AT instance still busy (rx state:%d)\r\n", g_at_rx_state);
        return -USB_ERR_BUSY;
    }

    if (intf != MODEM_AT_EXPECT_INTF) {
        /* Consistency check only: the class match keys on vendor class +
         * protocol 0x03, so a different layout is worth a log but not a
         * refusal. Claiming it anyway keeps a renumbered module working. */
        USB_LOG_INFO("modem AT on intf%u, expected intf%u\r\n", intf, MODEM_AT_EXPECT_INTF);
    }

    memset(port, 0, sizeof(struct modem_at_port));
    port->hport = hport;
    port->intf = intf;

    /* Endpoint addresses come from the descriptor, never from the table in the
     * plan: alt0 is the only altsetting this interface has. */
    alt = &hport->config.intf[intf].altsetting[0];
    for (uint8_t i = 0; i < alt->intf_desc.bNumEndpoints; i++) {
        ep_desc = &alt->ep[i].ep_desc;
        ep_type = ep_desc->bmAttributes & USB_ENDPOINT_TYPE_MASK;

        if (ep_type == USB_ENDPOINT_TYPE_BULK) {
            if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
                USBH_EP_INIT(port->bulkin, ep_desc);
            } else {
                USBH_EP_INIT(port->bulkout, ep_desc);
            }
        } else {
            /* The module also exposes an interrupt IN here. The AT channel
             * needs no notifications, so it is logged and left unsubmitted
             * rather than costing a pipe slot and an idle urb. */
            USB_LOG_INFO("modem AT skip ep=%02x attr=%02u\r\n",
                         ep_desc->bEndpointAddress, ep_desc->bmAttributes);
        }
    }

    if (!port->bulkin || !port->bulkout) {
        USB_LOG_ERR("modem AT needs bulk in+out, got in:%p out:%p\r\n", port->bulkin, port->bulkout);
        memset(port, 0, sizeof(struct modem_at_port));
        return -USB_ERR_NOTSUPP;
    }

    hport->config.intf[intf].priv = port;

    /* Created per connection and destroyed on disconnect, so a module reset
     * cannot leave a stale count or a held mutex behind. Order matters: the
     * locks and the handshake must exist before the thread can run. */
    g_at_rx_exit_sem = usb_osal_sem_create(0);
    g_at_tx_lock = usb_osal_mutex_create();
    if (!g_at_rx_exit_sem || !g_at_tx_lock) {
        USB_LOG_ERR("modem AT osal alloc failed\r\n");
        goto errout;
    }

    /* Both published before the thread exists: it reads g_at_rx_running on its
     * first loop test, and disconnect reads the state to decide whether to
     * wait. A thread that starts between the two would see a stale value. */
    g_at_rx_running = true;
    g_at_rx_state = MODEM_AT_RX_RUNNING;
    if (usb_osal_thread_create("modem_at_rx", 2048, CONFIG_USBHOST_PSC_PRIO + 1,
                               modem_at_rx_thread, NULL) == NULL) {
        USB_LOG_ERR("modem AT rx thread create failed\r\n");
        g_at_rx_running = false;
        g_at_rx_state = MODEM_AT_RX_IDLE;
        goto errout;
    }

    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT);
    USB_LOG_INFO("Register modem AT Class:%s\r\n", hport->config.intf[intf].devname);

    /* Last: the callee may call usb_modem_at_write() straight from here, so
     * the tx lock and the endpoints have to be in place already. */
    g_at_online = true;
    if (g_at_link_cb) {
        g_at_link_cb(true);
    }
    return 0;

errout:
    if (g_at_tx_lock) {
        usb_osal_mutex_delete(g_at_tx_lock);
        g_at_tx_lock = NULL;
    }
    if (g_at_rx_exit_sem) {
        usb_osal_sem_delete(g_at_rx_exit_sem);
        g_at_rx_exit_sem = NULL;
    }
    hport->config.intf[intf].priv = NULL;
    memset(port, 0, sizeof(struct modem_at_port));
    return -USB_ERR_NOMEM;
}

static int modem_at_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    struct modem_at_port *port = (struct modem_at_port *)hport->config.intf[intf].priv;

    if (!port) {
        return 0;
    }

    /* Both before the kills, not after. A writer that wakes up between the
     * kill and the flag going down would submit another urb on an endpoint
     * that is already gone, and the rx thread would re-arm a WAITING_FOREVER
     * submit and become unreachable. */
    g_at_online = false;
    g_at_rx_running = false;

    /* Tell the AT layer first so it stops issuing writes and cancels whatever
     * it is waiting on; it is contractually non-blocking. */
    if (g_at_link_cb) {
        g_at_link_cb(false);
    }

    if (port->bulkin) {
        usbh_kill_urb(&port->bulkin_urb);
    }
    if (port->bulkout) {
        usbh_kill_urb(&port->bulkout_urb);
    }

    /* Nothing below may run before the thread confirms it is out: the memset()
     * clears the endpoints it reads, and our caller then deletes hport->mutex
     * and recycles the whole hubport. The wait is bounded because the thread's
     * only block is the urb we just killed. */
    if (g_at_rx_state == MODEM_AT_RX_RUNNING && g_at_rx_exit_sem) {
        usb_osal_sem_take(g_at_rx_exit_sem, USB_OSAL_WAITING_FOREVER);
    }

    if (g_at_tx_lock) {
        /* After the online flag went down and after the kill, so any writer is
         * already out of the critical section. */
        usb_osal_mutex_take(g_at_tx_lock);
        usb_osal_mutex_give(g_at_tx_lock);
        usb_osal_mutex_delete(g_at_tx_lock);
        g_at_tx_lock = NULL;
    }
    if (g_at_rx_exit_sem) {
        usb_osal_sem_delete(g_at_rx_exit_sem);
        g_at_rx_exit_sem = NULL;
    }

    memset(port, 0, sizeof(struct modem_at_port));
    hport->config.intf[intf].priv = NULL;
    memset(hport->config.intf[intf].devname, 0, CONFIG_USBHOST_DEV_NAMELEN);

    /* Published here rather than by the thread: the thread must not write
     * anything once a new connection may already own the state. */
    g_at_rx_state = MODEM_AT_RX_IDLE;
    USB_LOG_INFO("Unregister modem AT Class\r\n");
    return 0;
}

static const uint16_t modem_at_id_table[][2] = {
    { 0x3505, 0x0003 },
    { 0, 0 },
};

const struct usbh_class_driver modem_at_class_driver = {
    .driver_name = "modem_at",
    .connect = modem_at_connect,
    .disconnect = modem_at_disconnect
};

/* Vendor class plus protocol 0x03 plus the module's VID/PID. The module's other
 * vendor interfaces differ only in protocol (intf2 0x04, intf4 0x05), so the
 * protocol bit is what keeps this driver off the diagnostic ports. */
CLASS_INFO_DEFINE const struct usbh_class_info modem_at_class_info = {
    .match_flags = USB_CLASS_MATCH_VID_PID | USB_CLASS_MATCH_INTF_CLASS |
                   USB_CLASS_MATCH_INTF_SUBCLASS | USB_CLASS_MATCH_INTF_PROTOCOL,
    .bInterfaceClass = USB_DEVICE_CLASS_VEND_SPECIFIC,
    .bInterfaceSubClass = 0x00,
    .bInterfaceProtocol = MODEM_AT_INTF_PROTOCOL,
    .id_table = modem_at_id_table,
    .class_driver = &modem_at_class_driver
};
