#include "bk_cherryusb_adapter.h"

#if CONFIG_USB_HOST
#include <components/cherryusb/usbh_core.h>
#endif
#if CONFIG_USB_DEVICE
#include <components/cherryusb/usbd_core.h>
#endif
#if CONFIG_USB_HUB
#include <components/cherryusb/usbh_hub.h>
#endif
#if CONFIG_USBH_UAC
#include <components/cherryusb/usbh_audio.h>
#endif
#if CONFIG_USBH_UVC
#include <components/cherryusb/usbh_video.h>
#endif
#if CONFIG_USBH_SERIAL_CH340
#include "usbh_ch34x.h"
#endif

void bk_cherryusb_register_host_classes(void)
{
#if CONFIG_USB_HUB
	usbh_hub_class_register();
#endif

#if CONFIG_USBH_UVC
	usbh_uvc_class_register();
#endif

#if CONFIG_USBH_UAC
	usbh_uac_class_register();
#endif

#if CONFIG_USBH_MSC
	extern void usbh_msc_register(void);
	usbh_msc_register();
#endif

#if CONFIG_USB_CDC
	extern void usbh_cdc_data_class_register(void);
	usbh_cdc_data_class_register();
#endif

#if CONFIG_USBH_SERIAL_CH340
	usbh_class_serial_ch340_register_driver();
#endif
}

void bk_cherryusb_register_device_classes(void)
{
}

bk_err_t bk_cherryusb_host_open(void)
{
#if CONFIG_USB_HOST
	return usbh_initialize();
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_host_close(void)
{
#if CONFIG_USB_HOST
	return usbh_deinitialize();
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_device_open(void)
{
#if CONFIG_USB_DEVICE
	return usbd_initialize();
#else
	return BK_FAIL;
#endif
}

bk_err_t bk_cherryusb_device_close(void)
{
#if CONFIG_USB_DEVICE
	return usbd_deinitialize();
#else
	return BK_FAIL;
#endif
}
