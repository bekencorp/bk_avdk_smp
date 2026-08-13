/*
 * Slimmed public header. The single source of truth is the CherryUSB v1.6 tree
 * under ap/components/bk_usb/CherryUSB/. This file only forwards to it so
 * that <components/cherryusb/usb_list.h> keeps resolving for consumers without
 * duplicating the header body. Relative path is used because external consumers
 * reach this file via the global ap/include search path, not bk_usb's -I dirs.
 */
#include "../../../components/bk_usb/CherryUSB/common/usb_list.h"
