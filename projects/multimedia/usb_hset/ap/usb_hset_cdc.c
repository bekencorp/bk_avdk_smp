/*
 * Project-side HS CDC-ACM device for USB HSET / USB 2.0 eye-diagram testing.
 *
 * On power-up this enumerates the MHDRC controller (SOC_USB_HS_BASE) as a USB
 * 2.0 HIGH-SPEED CDC-ACM device so a compliance host (e.g. USBHSET) can drive
 * the HS test modes via SET_FEATURE(TEST_MODE). That standard request is
 * decoded by the cherryusb device core, which after the control status stage
 * calls usbd_execute_test_mode() in the SDK MHDRC port
 * (ap/components/bk_usb/CherryUSB_v1_6/port/beken_musb/usb_dc_beken_musb_mhdrc.c),
 * gated by CONFIG_USB_HSET. That writes the MUSB TESTMODE register
 * (Test_J / Test_K / Test_SE0_NAK / Test_Packet).
 *
 * Only the descriptors + class wiring live here. The device-mode USB PHY /
 * clock / IRQ bring-up is done entirely by the port's usb_dc_low_level_init()
 * (invoked from usbd_initialize() -> usb_dc_init()), so unlike the ATE branch
 * this file does NOT poke any PHY registers.
 */
#include <common/bk_include.h>
#include <os/os.h>
#include <soc/soc.h>

#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#if CONFIG_USB_HSET

/* Device endpoint addresses. */
#define CDCDEV_CDC_IN_EP  0x81
#define CDCDEV_CDC_OUT_EP 0x02
#define CDCDEV_CDC_INT_EP 0x83

#define CDCDEV_USBD_VID       0xFFFF
#define CDCDEV_USBD_PID       0xFFFF
#define CDCDEV_USBD_MAX_POWER 100

/* High speed: 512-byte bulk max packet. */
#define CDCDEV_CDC_MAX_MPS 512

#define CDCDEV_DEV_BUSID 0

#define CDCDEV_USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

static const uint8_t s_device_descriptor[] = {
	USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, CDCDEV_USBD_VID,
		CDCDEV_USBD_PID, 0x0100, 0x01)
};

static const uint8_t s_config_descriptor[] = {
	USB_CONFIG_DESCRIPTOR_INIT(CDCDEV_USB_CONFIG_SIZE, 0x02, 0x01,
		USB_CONFIG_BUS_POWERED, CDCDEV_USBD_MAX_POWER),
	CDC_ACM_DESCRIPTOR_INIT(0x00, CDCDEV_CDC_INT_EP, CDCDEV_CDC_OUT_EP,
		CDCDEV_CDC_IN_EP, CDCDEV_CDC_MAX_MPS, 0x02)
};

/* Device qualifier: a HS-capable device must answer GET_DESCRIPTOR(qualifier)
 * so the host learns the other-speed (FS) capability during HS enumeration. */
static const uint8_t s_device_quality_descriptor[] = {
	0x0a,                                 /* bLength */
	USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, /* bDescriptorType */
	0x00, 0x02,                           /* bcdUSB 2.00 */
	0xEF,                                 /* bDeviceClass (misc / IAD) */
	0x02,                                 /* bDeviceSubClass */
	0x01,                                 /* bDeviceProtocol */
	0x40,                                 /* bMaxPacketSize0 = 64 */
	0x01,                                 /* bNumConfigurations */
	0x00,                                 /* bReserved */
};

static const char *s_string_descriptors[] = {
	(const char[]){ 0x09, 0x04 }, /* Langid: 0x0409 (US English) */
	"Beken",                      /* Manufacturer */
	"BK7259 USB HSET CDC",        /* Product */
	"7259USBHSET",                /* Serial Number */
};

static const uint8_t *device_descriptor_cb(uint8_t speed)
{
	(void)speed;
	return s_device_descriptor;
}

static const uint8_t *config_descriptor_cb(uint8_t speed)
{
	(void)speed;
	return s_config_descriptor;
}

static const uint8_t *device_quality_descriptor_cb(uint8_t speed)
{
	(void)speed;
	return s_device_quality_descriptor;
}

static const char *string_descriptor_cb(uint8_t speed, uint8_t index)
{
	(void)speed;
	if (index >= (sizeof(s_string_descriptors) / sizeof(char *))) {
		return NULL;
	}
	return s_string_descriptors[index];
}

static const struct usb_descriptor s_cdcdev_descriptor = {
	.device_descriptor_callback = device_descriptor_cb,
	.config_descriptor_callback = config_descriptor_cb,
	.device_quality_descriptor_callback = device_quality_descriptor_cb,
	.string_descriptor_callback = string_descriptor_cb,
};

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_dev_read_buffer[CDCDEV_CDC_MAX_MPS];

static volatile bool s_dev_configured;

static void cdcdev_usbd_event_handler(uint8_t busid, uint8_t event)
{
	switch (event) {
	case USBD_EVENT_RESET:
		s_dev_configured = false;
		break;
	case USBD_EVENT_CONFIGURED:
		s_dev_configured = true;
		/* Arm an OUT read so the device behaves like a real CDC port. */
		usbd_ep_start_read(busid, CDCDEV_CDC_OUT_EP, s_dev_read_buffer,
			sizeof(s_dev_read_buffer));
		USB_LOG_INFO("[usb-hset] device CONFIGURED (HS)\r\n");
		break;
	case USBD_EVENT_DISCONNECTED:
		s_dev_configured = false;
		break;
	default:
		break;
	}
}

static void cdcdev_cdc_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)ep;
	(void)nbytes;
	/* Re-arm the OUT read; content is irrelevant for the HSET use case. */
	usbd_ep_start_read(busid, CDCDEV_CDC_OUT_EP, s_dev_read_buffer,
		sizeof(s_dev_read_buffer));
}

static void cdcdev_cdc_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)busid;
	(void)ep;
	(void)nbytes;
}

static struct usbd_endpoint s_cdc_out_ep = {
	.ep_addr = CDCDEV_CDC_OUT_EP,
	.ep_cb = cdcdev_cdc_bulk_out,
};

static struct usbd_endpoint s_cdc_in_ep = {
	.ep_addr = CDCDEV_CDC_IN_EP,
	.ep_cb = cdcdev_cdc_bulk_in,
};

static struct usbd_interface s_intf0;
static struct usbd_interface s_intf1;

static bool s_cdcdev_up;

void bk_usb_hset_cdc_device_init(void)
{
	if (s_cdcdev_up) {
		USB_LOG_INFO("[usb-hset] already up\r\n");
		return;
	}

	usbd_desc_register(CDCDEV_DEV_BUSID, &s_cdcdev_descriptor);
	usbd_add_interface(CDCDEV_DEV_BUSID,
		usbd_cdc_acm_init_intf(CDCDEV_DEV_BUSID, &s_intf0));
	usbd_add_interface(CDCDEV_DEV_BUSID,
		usbd_cdc_acm_init_intf(CDCDEV_DEV_BUSID, &s_intf1));
	usbd_add_endpoint(CDCDEV_DEV_BUSID, &s_cdc_out_ep);
	usbd_add_endpoint(CDCDEV_DEV_BUSID, &s_cdc_in_ep);

	/* usbd_initialize() -> usb_dc_init() -> usb_dc_low_level_init() does the
	 * device-mode HS PHY / clock / IRQ bring-up for us. */
	usbd_initialize(CDCDEV_DEV_BUSID, SOC_USB_HS_BASE,
		cdcdev_usbd_event_handler);

	s_cdcdev_up = true;
	USB_LOG_INFO("[usb-hset] HS CDC device up; waiting for host SET_FEATURE(TEST_MODE)\r\n");
}

#endif /* CONFIG_USB_HSET */
