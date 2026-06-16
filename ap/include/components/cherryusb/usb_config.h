/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <common/bk_include.h>
#include <os/mem.h>

#if !CONFIG_BK_USB_CHERRYUSB_V1_6
#define CHERRYUSB_VERSION 0x000700
#endif

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

#define usb_malloc(size) malloc(size)
#define usb_free(ptr)    free(ptr)

#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION //__attribute__((section(".noncacheable")))

/* ================= USB Device Stack Configuration ================ */

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 1024

/* Setup packet log for debug */
// #define CONFIG_USBDEV_SETUP_LOG_PRINT

/* Check if the input descriptor is correct */
// #define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
// #define CONFIG_USBDEV_TEST_MODE

#ifndef CONFIG_USBDEV_MSC_BLOCK_SIZE
#define CONFIG_USBDEV_MSC_BLOCK_SIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"
#endif

#define CONFIG_USBDEV_MSC_THREAD

#ifdef CONFIG_USBDEV_MSC_THREAD
#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE 2048
#endif

#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO 4
#endif
#endif

#ifndef CONFIG_USBDEV_AUDIO_VERSION
#define CONFIG_USBDEV_AUDIO_VERSION 0x0100
#endif

#ifndef CONFIG_USBDEV_AUDIO_MAX_CHANNEL
#define CONFIG_USBDEV_AUDIO_MAX_CHANNEL 8
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE 128
#endif

#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE 1536
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID 0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP

/* ================ USB HOST Stack Configuration ================== */
#ifndef CONFIG_USBHOST_MAX_RHPORTS
#define CONFIG_USBHOST_MAX_RHPORTS          1
#endif
#ifndef CONFIG_USBHOST_MAX_EXTHUBS
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#endif
#ifndef CONFIG_USBHOST_MAX_EHPORTS
#define CONFIG_USBHOST_MAX_EHPORTS          4
#endif
#ifndef CONFIG_USBHOST_MAX_INTERFACES
#define CONFIG_USBHOST_MAX_INTERFACES       10
#endif
#ifndef CONFIG_USBHOST_MAX_INTF_ALTSETTINGS
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 14
#endif
#ifndef CONFIG_USBHOST_MAX_ENDPOINTS
#define CONFIG_USBHOST_MAX_ENDPOINTS        14
#endif
#ifndef CONFIG_USBHOST_DEV_NAMELEN
#define CONFIG_USBHOST_DEV_NAMELEN 16
#endif
#ifndef CONFIG_USBHOST_PSC_PRIO
#define CONFIG_USBHOST_PSC_PRIO 2
#endif
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
#define CONFIG_USBHOST_PSC_STACKSIZE 2048
#endif

//#define CONFIG_USBHOST_GET_STRING_DESC

/* Ep0 max transfer buffer */
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 3072//512

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#ifndef CONFIG_USBHOST_MSC_TIMEOUT
#define CONFIG_USBHOST_MSC_TIMEOUT 5000
#endif

/* ================ USB Device Port Configuration ================*/

#if !CONFIG_BK_USB_CHERRYUSB_V1_6
#define USBD_IRQHandler USBD_IRQHandler
#define USB_BASE (SOC_USB_HS_BASE)
#endif
#define USB_NUM_BIDIR_ENDPOINTS 16

/* ================ USB Host Port Configuration ==================*/

#define CONFIG_USBHOST_PIPE_NUM 10

#if CONFIG_BK_USB_CHERRYUSB_V1_6
#undef CONFIG_USBHOST_MAX_INTERFACES
#define CONFIG_USBHOST_MAX_INTERFACES 4
#undef CONFIG_USBHOST_MAX_INTF_ALTSETTINGS
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 5
#undef CONFIG_USBHOST_MAX_ENDPOINTS
#define CONFIG_USBHOST_MAX_ENDPOINTS 4
#undef CONFIG_USBHOST_PIPE_NUM
#define CONFIG_USBHOST_PIPE_NUM 8
#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS 1
#endif
#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif
#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#endif
#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif
#ifndef CONFIG_USB_MUSB_EP_NUM
#if CONFIG_USB_RISCV_BRIDGE
/* g_musb_udc is published in-place to the RISC-V CP firmware, which reads it as
 * musb_udc_t with in_ep[16]/out_ep[16] (USB_NUM_BIDIR_ENDPOINTS). The AP struct
 * MUST size its EP arrays the same so out_ep[] lands at the offset the CP
 * expects. See ap/.../riscv_src/fw/common/riscv_usb_bridge.h. */
#define CONFIG_USB_MUSB_EP_NUM 16
#else
#define CONFIG_USB_MUSB_EP_NUM 8
#endif
#endif
#ifndef CONFIG_USB_MUSB_PIPE_NUM
#define CONFIG_USB_MUSB_PIPE_NUM 8
#endif
#ifndef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
#define CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
#endif
#ifndef CONFIG_USBHOST_MAX_SERIAL_CLASS
#define CONFIG_USBHOST_MAX_SERIAL_CLASS 1
#endif
#ifndef CONFIG_USBHOST_MAX_HID_CLASS
#define CONFIG_USBHOST_MAX_HID_CLASS 1
#endif
#ifndef CONFIG_USBHOST_MAX_MSC_CLASS
#define CONFIG_USBHOST_MAX_MSC_CLASS 1
#endif
#ifndef CONFIG_USBHOST_MAX_AUDIO_CLASS
#define CONFIG_USBHOST_MAX_AUDIO_CLASS 1
#endif
#ifndef CONFIG_USBHOST_MAX_VIDEO_CLASS
#define CONFIG_USBHOST_MAX_VIDEO_CLASS 1
#endif
#ifndef CONFIG_USBHOST_VIDEO_MAX_FORMATS
#define CONFIG_USBHOST_VIDEO_MAX_FORMATS 4
#endif
#ifndef CONFIG_USBHOST_VIDEO_MAX_FRAMES
#define CONFIG_USBHOST_VIDEO_MAX_FRAMES 8
#endif
#ifndef usb_phyaddr2ramaddr
#define usb_phyaddr2ramaddr(addr) (addr)
#endif
#ifndef usb_ramaddr2phyaddr
#define usb_ramaddr2phyaddr(addr) (addr)
#endif
#endif /* CONFIG_BK_USB_CHERRYUSB_V1_6 */

/* ================ EHCI Configuration ================ */

#define CONFIG_USB_EHCI_HCCR_BASE       (0x20072000)
#define CONFIG_USB_EHCI_HCOR_BASE       (0x20072000 + 0x10)
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE 1024
// #define CONFIG_USB_EHCI_INFO_ENABLE
// #define CONFIG_USB_ECHI_HCOR_RESERVED_DISABLE
// #define CONFIG_USB_EHCI_CONFIGFLAG
// #define CONFIG_USB_EHCI_PORT_POWER

#endif
