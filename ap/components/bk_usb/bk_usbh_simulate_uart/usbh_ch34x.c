/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usbh_ch34x.h"

#define DEV_FORMAT "/dev/ttyUSB%d"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_ch34x_buf[64];

#define CONFIG_USBHOST_MAX_CH34X_CLASS 2

static struct usbh_ch34x g_ch34x_class[CONFIG_USBHOST_MAX_CH34X_CLASS];
static uint32_t g_devinuse = 0;

static struct usbh_ch34x *usbh_ch34x_class_alloc(void)
{
    uint8_t devno;

    for (devno = 0; devno < CONFIG_USBHOST_MAX_CH34X_CLASS; devno++) {
        if ((g_devinuse & (1U << devno)) == 0) {
            g_devinuse |= (1U << devno);
            memset(&g_ch34x_class[devno], 0, sizeof(struct usbh_ch34x));
            g_ch34x_class[devno].minor = devno;
            return &g_ch34x_class[devno];
        }
    }
    return NULL;
}

static void usbh_ch34x_class_free(struct usbh_ch34x *ch34x_class)
{
    uint8_t devno = ch34x_class->minor;

    if (devno < 32) {
        g_devinuse &= ~(1U << devno);
    }
    memset(ch34x_class, 0, sizeof(struct usbh_ch34x));
}

/* first bound (in-use) ch34x instance, for CLI data-plane testing on a
 * single-device bus; NULL when no ch34x is attached */
struct usbh_ch34x *usbh_ch34x_get_dev(void)
{
    for (uint8_t devno = 0; devno < CONFIG_USBHOST_MAX_CH34X_CLASS; devno++) {
        if ((g_devinuse & (1U << devno)) && g_ch34x_class[devno].hport) {
            return &g_ch34x_class[devno];
        }
    }
    return NULL;
}

/* first bound ch34x instance on the given bus (0=HS, 1=FS); NULL when no
 * ch34x is attached there. Dual-bus tests must use this instead of
 * usbh_ch34x_get_dev() so the traffic really goes over the intended bus. */
struct usbh_ch34x *usbh_ch34x_get_dev_by_bus(uint8_t busid)
{
    for (uint8_t devno = 0; devno < CONFIG_USBHOST_MAX_CH34X_CLASS; devno++) {
        if ((g_devinuse & (1U << devno)) && g_ch34x_class[devno].hport &&
            (g_ch34x_class[devno].hport->bus->busid == busid)) {
            return &g_ch34x_class[devno];
        }
    }
    return NULL;
}

static int usbh_ch34x_get_baudrate_div(uint32_t baudrate, uint8_t *factor, uint8_t *divisor)
{
    uint8_t a;
    uint8_t b;
    uint32_t c;

    switch (baudrate) {
        case 921600:
            a = 0xf3;
            b = 7;
            break;

        case 307200:
            a = 0xd9;
            b = 7;
            break;

        default:
            if (baudrate > 6000000 / 255) {
                b = 3;
                c = 6000000;
            } else if (baudrate > 750000 / 255) {
                b = 2;
                c = 750000;
            } else if (baudrate > 93750 / 255) {
                b = 1;
                c = 93750;
            } else {
                b = 0;
                c = 11719;
            }
            a = (uint8_t)(c / baudrate);
            if (a == 0 || a == 0xFF) {
                return -USB_ERR_INVAL;
            }
            if ((c / a - baudrate) > (baudrate - c / (a + 1))) {
                a++;
            }
            a = (uint8_t)(256 - a);
            break;
    }

    *factor = a;
    *divisor = b;

    return 0;
}

static int usbh_ch34x_get_version(struct usbh_ch34x *ch34x_class)
{
    struct usb_setup_packet *setup;
    int ret;

    if (!ch34x_class || !ch34x_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = ch34x_class->hport->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_READ_VERSION;
    setup->wValue = 0;
    setup->wIndex = 0;
    setup->wLength = 2;

    ret = usbh_control_transfer(ch34x_class->hport, setup, g_ch34x_buf);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/* ep0 control-transfer test with NON-multiple-of-4 data lengths.
 * Exercises the FIFO byte-tail paths (fs_read_packet/fs_write_packet
 * tails on the FS port) on endpoint 0:
 *  1. IN  wLength=2 (READ_VERSION) three times -- result must be stable
 *     and the buffer actually written (a 2-byte tail read bypasses the
 *     32-bit FIFO window entirely).
 *  2. OUT wLength=5 with a 01..05 pattern (fabricated vendor request;
 *     the device may STALL or ignore it -- either is acceptable, the
 *     point is the OUT data-phase state machine survives a 4+1 packet
 *     and pushes EXACTLY 5 bytes into the ep0 FIFO).
 *  3. IN  wLength=2 again -- ep0 must still be healthy afterwards.
 * Returns 0 when all steps completed (a STALL on step 2 still counts). */
int usbh_ch34x_ep0_unaligned_test(struct usbh_ch34x *ch34x_class)
{
    struct usb_setup_packet *setup;
    uint8_t buf[8];
    uint8_t v0 = 0;
    uint8_t v1 = 0;
    int ret;

    if (!ch34x_class || !ch34x_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = ch34x_class->hport->setup;

    /* step 1: 3x IN wLength=2 (tail-only path, no 32-bit word loop) */
    for (int i = 0; i < 3; i++) {
        setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
        setup->bRequest = CH34X_READ_VERSION;
        setup->wValue = 0;
        setup->wIndex = 0;
        setup->wLength = 2;
        memset(buf, 0xEE, sizeof(buf));
        ret = usbh_control_transfer(ch34x_class->hport, setup, buf);
        if (ret < 0) {
            USB_LOG_RAW("ep0 unaligned: IN(2) #%d failed ret=%d\r\n", i, ret);
            return ret;
        }
        USB_LOG_RAW("ep0 unaligned: IN(2) #%d ret=%d ver=%02x %02x\r\n",
                    i, ret, buf[0], buf[1]);
        if ((buf[0] == 0xEE) || (buf[1] == 0xEE)) {
            USB_LOG_RAW("ep0 unaligned: IN(2) buffer untouched (FIFO tail read broken)\r\n");
            return -USB_ERR_IO;
        }
        if (i == 0) {
            v0 = buf[0];
            v1 = buf[1];
        } else if ((buf[0] != v0) || (buf[1] != v1)) {
            USB_LOG_RAW("ep0 unaligned: IN(2) unstable (%02x %02x vs %02x %02x)\r\n",
                        v0, v1, buf[0], buf[1]);
            return -USB_ERR_IO;
        }
    }

    /* step 2: OUT wLength=5 (4-byte word + 1-byte tail on the write side) */
    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_WRITE_REG;
    setup->wValue = 0;
    setup->wIndex = 0;
    setup->wLength = 5;
    for (int i = 0; i < 5; i++) {
        buf[i] = (uint8_t)(i + 1);
    }
    ret = usbh_control_transfer(ch34x_class->hport, setup, buf);
    /* ret<0 (e.g. STALL) is acceptable: the request is fabricated and the
     * device is free to reject it; the data phase still exercised the
     * 5-byte FIFO write path and the ep0 state machine recovery. */
    USB_LOG_RAW("ep0 unaligned: OUT(5) ret=%d%s\r\n", ret,
                (ret < 0) ? " (device rejected, acceptable)" : "");

    /* step 3: ep0 health re-check */
    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_READ_VERSION;
    setup->wValue = 0;
    setup->wIndex = 0;
    setup->wLength = 2;
    memset(buf, 0xEE, sizeof(buf));
    ret = usbh_control_transfer(ch34x_class->hport, setup, buf);
    if (ret < 0) {
        USB_LOG_RAW("ep0 unaligned: health re-check failed ret=%d\r\n", ret);
        return ret;
    }
    USB_LOG_RAW("ep0 unaligned: health re-check OK ver=%02x %02x\r\n", buf[0], buf[1]);
    return 0;
}

static int usbh_ch34x_flow_ctrl(struct usbh_ch34x *ch34x_class)
{
    struct usb_setup_packet *setup;

    if (!ch34x_class || !ch34x_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = ch34x_class->hport->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_WRITE_REG;
    setup->wValue = 0x2727;
    setup->wIndex = 0;
    setup->wLength = 0;

    return usbh_control_transfer(ch34x_class->hport, setup, NULL);
}

int usbh_ch34x_set_line_coding(struct usbh_ch34x *ch34x_class, struct cdc_line_coding *line_coding)
{
    struct usb_setup_packet *setup;
    uint16_t reg_value = 0;
    uint16_t value = 0;
    uint8_t factor = 0;
    uint8_t divisor = 0;

    if (!ch34x_class || !ch34x_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = ch34x_class->hport->setup;

    memcpy((uint8_t *)&ch34x_class->line_coding, line_coding, sizeof(struct cdc_line_coding));

    /* refer to https://github.com/WCHSoftGroup/ch341ser_linux/blob/main/driver/ch341.c */

    switch (line_coding->bParityType) {
        case 0:
            break;
        case 1:
            reg_value |= CH341_L_PO;
            break;
        case 2:
            reg_value |= CH341_L_PE;
            break;
        case 3:
            reg_value |= CH341_L_PM;
            break;
        case 4:
            reg_value |= CH341_L_PS;
            break;
        default:
            return -USB_ERR_INVAL;
    }

    switch (line_coding->bDataBits) {
        case 5:
            reg_value |= CH341_L_D5;
            break;
        case 6:
            reg_value |= CH341_L_D6;
            break;
        case 7:
            reg_value |= CH341_L_D7;
            break;
        case 8:
            reg_value |= CH341_L_D8;
            break;
        default:
            return -USB_ERR_INVAL;
    }

    if (line_coding->bCharFormat == 2) {
        reg_value |= CH341_L_SB;
    }

    reg_value |= 0xC0;

    value |= 0x9c;
    value |= reg_value << 8;

    usbh_ch34x_get_baudrate_div(line_coding->dwDTERate, &factor, &divisor);

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_SERIAL_INIT;
    setup->wValue = value;
    setup->wIndex = (factor << 8) | 0x80 | divisor;
    setup->wLength = 0;

    return usbh_control_transfer(ch34x_class->hport, setup, NULL);
}

int usbh_ch34x_get_line_coding(struct usbh_ch34x *ch34x_class, struct cdc_line_coding *line_coding)
{
    memcpy(line_coding, (uint8_t *)&ch34x_class->line_coding, sizeof(struct cdc_line_coding));
    return 0;
}

int usbh_ch34x_set_line_state(struct usbh_ch34x *ch34x_class, bool dtr, bool rts)
{
    struct usb_setup_packet *setup;

    if (!ch34x_class || !ch34x_class->hport) {
        return -USB_ERR_INVAL;
    }
    setup = ch34x_class->hport->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_VENDOR | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = CH34X_MODEM_CTRL;
    setup->wValue = 0x0f | (dtr << 5) | (rts << 6);
    setup->wIndex = 0;
    setup->wLength = 0;

    return usbh_control_transfer(ch34x_class->hport, setup, NULL);
}

static usbh_simulate_uart_con_disc_cb_t s_con_disc_cb = NULL;
void usbh_class_serial_ch340_register_disc_cb(usbh_simulate_uart_con_disc_cb_t callback)
{
    s_con_disc_cb = callback;
}

static int usbh_ch34x_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    int ret = 0;

    struct usbh_ch34x *ch34x_class = usbh_ch34x_class_alloc();
    if (ch34x_class == NULL) {
        USB_LOG_ERR("Fail to alloc ch34x_class\r\n");
        return -USB_ERR_NOMEM;
    }

    ch34x_class->hport = hport;
    ch34x_class->intf = intf;

    hport->config.intf[intf].priv = ch34x_class;

    usbh_ch34x_get_version(ch34x_class);
    usbh_ch34x_flow_ctrl(ch34x_class);

    ch34x_class->line_coding.dwDTERate = 115200;
    ch34x_class->line_coding.bDataBits = 8;
    ch34x_class->line_coding.bCharFormat = 0;
    ch34x_class->line_coding.bParityType = 0;
    

    ret = usbh_ch34x_set_line_coding(ch34x_class, &ch34x_class->line_coding);
    usbh_ch34x_set_line_state(ch34x_class, true, false);
    for (uint8_t i = 0; i < hport->config.intf[intf].altsetting[0].intf_desc.bNumEndpoints; i++) {
        ep_desc = &hport->config.intf[intf].altsetting[0].ep[i].ep_desc;
        if (ep_desc->bmAttributes == USB_ENDPOINT_TYPE_INTERRUPT) {
            continue;
        } else {
            if (ep_desc->bEndpointAddress & 0x80) {
                ch34x_class->bulkin_ep_d = ep_desc;
            } else {
                ch34x_class->bulkout_ep_d = ep_desc;
            }
        }
    }

    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, ch34x_class->minor);

    USB_LOG_DBG("Register CH34X Class:%s\r\n", hport->config.intf[intf].devname);
    if(s_con_disc_cb) {
        s_con_disc_cb(1, (void *)ch34x_class);
    }
#if 0
    USB_LOG_DBG("Test ch34x rx and tx and rx for 5 times, baudrate is 115200\r\n");

    struct cdc_line_coding linecoding;
    uint8_t count = 5;

    linecoding.dwDTERate = 115200;
    linecoding.bDataBits = 8;
    linecoding.bParityType = 0;
    linecoding.bCharFormat = 0;
    usbh_ch34x_set_line_coding(ch34x_class, &linecoding);
    usbh_ch34x_set_line_state(ch34x_class, true, false);

    memset(g_ch34x_buf, 'a', sizeof(g_ch34x_buf));
    ret = usbh_ch34x_bulk_out_transfer(ch34x_class, g_ch34x_buf, sizeof(g_ch34x_buf), 0xfffffff);
    USB_LOG_RAW("out ret:%d\r\n", ret);
    while (count--) {
        ret = usbh_ch34x_bulk_in_transfer(ch34x_class, g_ch34x_buf, sizeof(g_ch34x_buf), 0xfffffff);
        USB_LOG_RAW("in ret:%d\r\n", ret);
        if (ret > 0) {
            for (uint32_t i = 0; i < ret; i++) {
                USB_LOG_RAW("%02x ", g_ch34x_buf[i]);
            }
            USB_LOG_RAW("\r\n");
        }
    }
#endif
    usbh_ch34x_run(ch34x_class);
    return ret;
}

static int usbh_ch34x_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    int ret = 0;

    struct usbh_ch34x *ch34x_class = (struct usbh_ch34x *)hport->config.intf[intf].priv;

    if(s_con_disc_cb) {
        s_con_disc_cb(0, (void *)ch34x_class);
    }
    if (ch34x_class) {
        if (ch34x_class->bulkout_ep_d) {
            usbh_kill_urb(&ch34x_class->bulkout_urb);
        }

        if (ch34x_class->bulkin_ep_d) {
            usbh_kill_urb(&ch34x_class->bulkin_urb);
        }

        if (hport->config.intf[intf].devname[0] != '\0') {
            USB_LOG_DBG("Unregister CH34X Class:%s\r\n", hport->config.intf[intf].devname);
            usbh_ch34x_stop(ch34x_class);
        }

        usbh_ch34x_class_free(ch34x_class);
    }

    return ret;
}

int usbh_ch34x_bulk_in_transfer(struct usbh_ch34x *ch34x_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
    int ret;
    struct usbh_urb *urb = &ch34x_class->bulkin_urb;

    usbh_bulk_urb_fill(urb, ch34x_class->hport, ch34x_class->bulkin_ep_d, buffer, buflen, timeout, NULL, NULL);
    ret = usbh_submit_urb(urb);
    if (ret == 0) {
        ret = urb->actual_length;
    }
    return ret;
}

int usbh_ch34x_bulk_out_transfer(struct usbh_ch34x *ch34x_class, uint8_t *buffer, uint32_t buflen, uint32_t timeout)
{
    int ret;
    struct usbh_urb *urb = &ch34x_class->bulkout_urb;

    usbh_bulk_urb_fill(urb, ch34x_class->hport, ch34x_class->bulkout_ep_d, buffer, buflen, timeout, NULL, NULL);
    ret = usbh_submit_urb(urb);
    if (ret == 0) {
        ret = urb->actual_length;
    }
    return ret;
}

__WEAK void usbh_ch34x_run(struct usbh_ch34x *ch34x_class)
{
    (void)ch34x_class;
}

__WEAK void usbh_ch34x_stop(struct usbh_ch34x *ch34x_class)
{
    (void)ch34x_class;
}

const struct usbh_class_driver ch34x_class_driver = {
    .driver_name = "ch34x",
    .connect = usbh_ch34x_connect,
    .disconnect = usbh_ch34x_disconnect
};

/* Known WCH chips (same table as the upstream class/serial ch34x driver).
 * Required in addition to the 0xFF interface-class match: without a VID/PID
 * filter ANY vendor-specific interface (4G modems, FTDI, UVC vendor intf...)
 * would be claimed by this driver. */
static const uint16_t ch34x_id_table[][2] = {
    { 0x1A86, 0x7523 }, /* ch340 chip */
    { 0x1A86, 0x7522 }, /* ch340k chip */
    { 0x1A86, 0x5523 }, /* ch341 chip */
    { 0x1A86, 0xe523 }, /* ch330 chip */
    { 0x4348, 0x5523 }, /* ch340 custom chip */
    { 0, 0 },
};

CLASS_INFO_DEFINE const struct usbh_class_info ch34x_class_info = {
    .match_flags = USB_CLASS_MATCH_VID_PID | USB_CLASS_MATCH_INTF_CLASS,
    .bInterfaceClass = USB_DEVICE_CLASS_VEND_SPECIFIC,
    .bInterfaceSubClass = 0x00,
    .bInterfaceProtocol = 0x00,
    .bInterfaceNumber = 0,
    .id_table = ch34x_id_table,
    .class_driver = &ch34x_class_driver
};

static bool serial_ch340_register_flag = 0;
void usbh_class_serial_ch340_register_driver()
{
    /* class driver is registered via the CLASS_INFO_DEFINE linker section,
     * nothing to do at runtime anymore */
    if(serial_ch340_register_flag) return;

    serial_ch340_register_flag = 1;
}