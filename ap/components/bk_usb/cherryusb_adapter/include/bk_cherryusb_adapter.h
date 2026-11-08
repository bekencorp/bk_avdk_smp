#pragma once

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void bk_cherryusb_register_host_classes(void);
void bk_cherryusb_register_device_classes(void);

bk_err_t bk_cherryusb_host_open(void);
bk_err_t bk_cherryusb_host_close(void);
bk_err_t bk_cherryusb_device_open(void);
bk_err_t bk_cherryusb_device_close(void);

/**
 * @brief Bring the FS/LS USB IP up as CherryUSB host bus 1
 *        (dual-IP mode, CONFIG_USB_FS_HOST).
 *
 * Only available when CONFIG_USB_FS_HOST=y; returns BK_FAIL otherwise
 * (no symbol is linked in that case, callers must be guarded too).
 */
bk_err_t bk_cherryusb_fs_host_open(void);

/**
 * @brief Tear down the FS/LS CherryUSB host bus 1.
 */
bk_err_t bk_cherryusb_fs_host_close(void);

/**
 * @brief Present the USB device to the host by asserting the D+ pull-up
 *        (soft-connect), without re-initialising the controller.
 *
 * @return BK_OK on success, BK_FAIL if USB device support is not compiled in.
 */
bk_err_t bk_cherryusb_device_connect(void);

/**
 * @brief Disconnect the USB device from the host by removing the D+ pull-up
 *        (soft-disconnect). Counterpart of bk_cherryusb_device_connect().
 *
 * @return BK_OK on success, BK_FAIL if USB device support is not compiled in.
 */
bk_err_t bk_cherryusb_device_disconnect(void);

#ifdef __cplusplus
}
#endif
