/*
 * Slimmed public header. The official CherryUSB v1.6 MSC host API (struct
 * usbh_msc, usbh_msc_scsi_*, usbh_msc_run/stop, ...) is the single source of
 * truth in the tree and is forwarded below. Only the Beken-specific block
 * device wrappers -- implemented in
 * cherryusb_adapter/v1_6/bk_cherryusb_adapter_v1_6.c and used by fatfs /
 * usb_example -- are kept here. See usb_list.h for the relative-path rationale.
 */
#include "usb_util.h"
#include "../../../components/bk_usb/CherryUSB/class/msc/usbh_msc.h"

/* Beken block-device convenience wrappers over usbh_msc_scsi_* (not upstream). */
int usbh_device_write(uint32_t first_block, const uint8_t *dest, uint32_t block_num);
int usbh_device_read(uint32_t first_block, const uint8_t *dest, uint32_t block_num);
uint8_t usbh_ms_media_get_status(void);
