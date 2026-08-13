/*
 * Slimmed public header: forwards to the CherryUSB v1.6 tree (single source of
 * truth). This also unifies the macro environment (__PACKED, USB_MEM_ALIGNX,
 * __ALIGNED, container_of, ...) with the tree so the slimmed descriptor headers
 * (usb_def.h, usb_audio.h, ...) can forward too. See usb_list.h for the
 * relative-path rationale.
 */
#include "../../../components/bk_usb/CherryUSB/common/usb_util.h"
