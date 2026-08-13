/*
 * Slimmed public header: forwards to the CherryUSB v1.6 tree (single source of
 * truth). The Beken OS includes that the ORIGINAL public header pulled
 * (os/os.h, os/mem.h, os/str.h) are preserved here on purpose: consumers reach
 * them transitively through this header (usb_types.h -> usbh_core.h ->
 * usb_osal.h) and rely on os_strcmp / os_strtoul being declared. Dropping them
 * broke e.g. uvc_display_example/uvc_test.c. See usb_list.h for the
 * relative-path rationale.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "../../../components/bk_usb/CherryUSB/common/usb_osal.h"
