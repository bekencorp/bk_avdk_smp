#ifndef __USB_VFS_H_
#define __USB_VFS_H_

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* USB MSC <-> local-FS arbitration helpers.
 *
 * usbd_msc.c (CherryUSB MSC class on this SDK) invokes these from
 * the MSC bulk-out worker thread on the MSC_THREAD_OP_RESET /
 * MSC_THREAD_OP_SUSPEND transitions so the on-device FatFs mount of
 * the SD card is dropped while the PC has the device claimed as a
 * USB Mass Storage volume.
 *
 * Why we cycle the local mount around USB activity:
 *   1) FatFs cluster cache on the device side and the host OS-level
 *      block cache would otherwise both believe they own the FAT
 *      metadata, yielding silent corruption on the first PC write.
 *   2) bk_sd_card_init() (called from msc_storage_init() and also
 *      from the MSC RESET path on the legacy BK7258 SDIO stack)
 *      needs exclusive access to the SDIO controller while it
 *      (re)idents the card, so any active FatFs handle has to go
 *      first.
 *
 * Historical note:
 *   The original BK SDK shipped these under the names
 *   `usb_vfs_init / usb_vfs_deinit` (see
 *   ref/bk_ai_release_2.0.1/.../usb_vfs.{h,c}). The BK7259 SDK
 *   renamed the call sites in usbd_msc.c to `lv_vfs_init /
 *   lv_vfs_deinit` but dropped the implementation file, which is
 *   why this header + source pair has to exist. The names here MUST
 *   match the call-site names in usbd_msc.c exactly or the link
 *   step fails with "undefined reference to lv_vfs_*".
 */
bk_err_t lv_vfs_init(void);
bk_err_t lv_vfs_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_VFS_H_ */
