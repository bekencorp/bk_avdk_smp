#include "bk_cherryusb_adapter.h"

#include <soc/soc.h>

#if CONFIG_USB_HOST
#include "usbh_core.h"
#include "usbh_msc.h"
#endif
#if CONFIG_USB_DEVICE
#include "usbd_core.h"
#include "usbd_msc.h"
#endif

#define BK_CHERRYUSB_BUS_ID 0

#define BK_V16_MSC_IN_EP      0x81
#define BK_V16_MSC_OUT_EP     0x02
#define BK_V16_USBD_VID       0x0000
#define BK_V16_USBD_PID       0x0000
#define BK_V16_USBD_MAX_POWER 100

#ifdef CONFIG_USB_HS
#define BK_V16_MSC_MAX_MPS 512
#else
#define BK_V16_MSC_MAX_MPS 64
#endif

#if CONFIG_USB_HOST && CONFIG_USBH_MSC
static struct usbh_msc *s_bk_v16_msc;
static uint8_t s_bk_v16_msc_ready;
#endif

void bk_cherryusb_register_host_classes(void)
{
	/* CherryUSB v1.6 host classes are registered through CLASS_INFO_DEFINE. */
}

void bk_cherryusb_register_device_classes(void)
{
}

bk_err_t bk_cherryusb_host_open(void)
{
#if CONFIG_USB_HOST
	return (usbh_initialize(BK_CHERRYUSB_BUS_ID, SOC_USB_HS_BASE, NULL) == 0) ? BK_OK : BK_FAIL;
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_host_close(void)
{
#if CONFIG_USB_HOST
	return (usbh_deinitialize(BK_CHERRYUSB_BUS_ID) == 0) ? BK_OK : BK_FAIL;
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_device_open(void)
{
#if CONFIG_USB_DEVICE
	return (usbd_initialize(BK_CHERRYUSB_BUS_ID, SOC_USB_HS_BASE, NULL) == 0) ? BK_OK : BK_FAIL;
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_device_close(void)
{
#if CONFIG_USB_DEVICE
	return (usbd_deinitialize(BK_CHERRYUSB_BUS_ID) == 0) ? BK_OK : BK_FAIL;
#else
	return BK_FAIL;
#endif
}

#if CONFIG_USB_HOST && CONFIG_USBH_MSC
void usbh_msc_run(struct usbh_msc *msc_class)
{
	/* v1.6 usbh_msc_connect() only does GET_MAX_LUN and defers the SCSI
	 * INQUIRY / READ_CAPACITY to the application hook. Run it here so the
	 * adapter exposes a valid capacity (legacy read capacity during connect). */
	int ret = usbh_msc_scsi_init(msc_class);
	if (ret < 0) {
		USB_LOG_ERR("[bk_v1_6] MSC scsi_init failed: %d\r\n", ret);
		s_bk_v16_msc = NULL;
		s_bk_v16_msc_ready = 0;
		return;
	}

	USB_LOG_INFO("[bk_v1_6] MSC host media ready: %u blocks x %u bytes\r\n",
		(unsigned int)msc_class->blocknum, (unsigned int)msc_class->blocksize);
	s_bk_v16_msc = msc_class;
	s_bk_v16_msc_ready = 1;
}

void usbh_msc_stop(struct usbh_msc *msc_class)
{
	if (s_bk_v16_msc == msc_class) {
		s_bk_v16_msc = NULL;
		s_bk_v16_msc_ready = 0;
	}
}

uint8_t usbh_ms_media_get_status(void)
{
	return s_bk_v16_msc_ready;
}

int usbh_device_write(uint32_t first_block, const uint8_t *dest, uint32_t block_num)
{
	return s_bk_v16_msc ? usbh_msc_scsi_write10(s_bk_v16_msc, first_block, dest, block_num) : BK_FAIL;
}

int usbh_device_read(uint32_t first_block, const uint8_t *dest, uint32_t block_num)
{
	return s_bk_v16_msc ? usbh_msc_scsi_read10(s_bk_v16_msc, first_block, dest, block_num) : BK_FAIL;
}

void usbh_msc_register(void)
{
}
#endif

#if CONFIG_USB_DEVICE && CONFIG_USBD_MSC
static struct usbd_interface s_bk_v16_msc_intf;
static uint8_t s_bk_v16_msc_inited;

static const uint8_t s_bk_v16_device_descriptor[] = {
	USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00,
		BK_V16_USBD_VID, BK_V16_USBD_PID, 0x0200, 0x01)
};

static const uint8_t s_bk_v16_config_descriptor[] = {
	USB_CONFIG_DESCRIPTOR_INIT(9 + MSC_DESCRIPTOR_LEN, 0x01, 0x01,
		USB_CONFIG_BUS_POWERED, BK_V16_USBD_MAX_POWER),
	MSC_DESCRIPTOR_INIT(0x00, BK_V16_MSC_OUT_EP, BK_V16_MSC_IN_EP, BK_V16_MSC_MAX_MPS, 0x02)
};

static const uint8_t s_bk_v16_device_quality_descriptor[] = {
	0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
};

static const char *s_bk_v16_string_descriptors[] = {
	(const char[]){ 0x09, 0x04 },
	"Beken-USB",
	"Beken-USB MSC DEMO",
	"2022123456",
};

static const uint8_t *bk_v16_device_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return s_bk_v16_device_descriptor;
}

static const uint8_t *bk_v16_config_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return s_bk_v16_config_descriptor;
}

static const uint8_t *bk_v16_device_quality_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return s_bk_v16_device_quality_descriptor;
}

static const char *bk_v16_string_descriptor_callback(uint8_t speed, uint8_t index)
{
	(void)speed;
	if (index >= (sizeof(s_bk_v16_string_descriptors) / sizeof(s_bk_v16_string_descriptors[0]))) {
		return NULL;
	}
	return s_bk_v16_string_descriptors[index];
}

static const struct usb_descriptor s_bk_v16_msc_descriptor = {
	.device_descriptor_callback = bk_v16_device_descriptor_callback,
	.config_descriptor_callback = bk_v16_config_descriptor_callback,
	.device_quality_descriptor_callback = bk_v16_device_quality_descriptor_callback,
	.string_descriptor_callback = bk_v16_string_descriptor_callback,
};

static void bk_v16_usbd_event_handler(uint8_t busid, uint8_t event)
{
	(void)busid;
	(void)event;
}

int msc_storage_init(void)
{
	if (s_bk_v16_msc_inited) {
		USB_LOG_INFO("[bk_v1_6] device MSC already initialized\r\n");
		return 0;
	}

	USB_LOG_INFO("[bk_v1_6] initializing device MSC adapter\r\n");
	usbd_desc_register(BK_CHERRYUSB_BUS_ID, &s_bk_v16_msc_descriptor);
	usbd_add_interface(BK_CHERRYUSB_BUS_ID,
		usbd_msc_init_intf(BK_CHERRYUSB_BUS_ID, &s_bk_v16_msc_intf, BK_V16_MSC_OUT_EP, BK_V16_MSC_IN_EP));
	s_bk_v16_msc_inited = 1;

	return (usbd_initialize(BK_CHERRYUSB_BUS_ID, SOC_USB_HS_BASE, bk_v16_usbd_event_handler) == 0) ? 0 : BK_FAIL;
}

int msc_storage_deinit(void)
{
	if (!s_bk_v16_msc_inited) {
		return 0;
	}

	s_bk_v16_msc_inited = 0;
	return bk_cherryusb_device_close() == BK_OK ? 0 : BK_FAIL;
}

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
	extern uint32_t bk_sd_card_get_card_size(void);
	(void)busid;
	(void)lun;
	*block_num = bk_sd_card_get_card_size();
	*block_size = 512;
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
	extern bk_err_t bk_sd_card_read_blocks(uint8_t *data, uint32_t block_addr, uint32_t block_num);
	(void)busid;
	(void)lun;
	return bk_sd_card_read_blocks(buffer, sector, length / 512);
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
	extern bk_err_t bk_sd_card_write_blocks(const uint8_t *data, uint32_t block_addr, uint32_t block_num);
	(void)busid;
	(void)lun;
	return bk_sd_card_write_blocks(buffer, sector, length / 512);
}
#endif
