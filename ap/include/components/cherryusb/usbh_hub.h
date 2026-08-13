/*
 * Slimmed public header: forwards to the CherryUSB v1.6 tree hub API (single
 * source of truth). usbh_core.h is kept first because consumers expect the hub
 * header to also provide the full struct usbh_hub (defined in usbh_core.h; the
 * tree hub header only forward-declares it). The former Beken-only declarations
 * (usbh_roothub_thread_*, usbh_hub_register/unregister, usbh_hub_class_register,
 * hub_class_head, usbh_hub_event_*_mutex, USBH_HUB_MAX_PORTS, ...) are dropped:
 * they had no consumers -- the hub-multiple-classes example locally #defines the
 * event mutex macros as no-ops. See usb_list.h for the relative-path rationale.
 */
#include "usbh_core.h"
#include "../../../components/bk_usb/CherryUSB/class/hub/usbh_hub.h"
