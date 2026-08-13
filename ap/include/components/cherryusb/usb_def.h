/*
 * Slimmed public header: forwards to the CherryUSB v1.6 tree (single source of
 * truth). usb_util.h is pulled first because the tree descriptor header relies
 * on __PACKED (defined in usb_util.h) but does not include it itself. See
 * usb_list.h for the relative-path rationale.
 */
#include "usb_util.h"
#include "../../../components/bk_usb/CherryUSB/common/usb_def.h"
