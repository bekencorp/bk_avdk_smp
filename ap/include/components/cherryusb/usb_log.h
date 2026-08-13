/*
 * Slimmed public header: forwards to the CherryUSB v1.6 tree (single source of
 * truth). The Beken <components/log.h> include that the ORIGINAL public header
 * pulled is preserved here, because consumers reach it transitively through
 * this header. See usb_list.h for the relative-path rationale.
 */
#include <components/log.h>
#include "../../../components/bk_usb/CherryUSB/common/usb_log.h"
