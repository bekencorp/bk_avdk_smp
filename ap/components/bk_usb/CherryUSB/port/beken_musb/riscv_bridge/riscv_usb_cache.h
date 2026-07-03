#ifndef RISCV_USB_CACHE_H
#define RISCV_USB_CACHE_H

#include <stddef.h>
#include <stdint.h>

#if CONFIG_USB_RISCV_BRIDGE && CONFIG_SRAM_DIRECT_ADDR
#include "cache.h"
#include <components/cherryusb/usb_hc.h>

static inline void riscv_usb_cache_clean(const void *addr, size_t len)
{
    if (addr != NULL && len > 0U) {
        arch_dcache_flush_range((void *)(uintptr_t)addr, len);
    }
}

static inline void riscv_usb_cache_invalidate(const void *addr, size_t len)
{
    if (addr != NULL && len > 0U) {
        arch_dcache_invd_range((void *)(uintptr_t)addr, len);
    }
}

static inline size_t riscv_usb_urb_meta_size(const struct usbh_urb *urb)
{
    size_t size = offsetof(struct usbh_urb, iso_packet);

    if (urb != NULL && urb->num_of_iso_packets > 1U) {
        size += (size_t)urb->num_of_iso_packets * sizeof(struct usbh_iso_frame_packet);
    }

    return size;
}

static inline void riscv_usb_urb_clean_for_riscv(const struct usbh_urb *urb)
{
    uint32_t i;

    if (urb == NULL) {
        return;
    }

    riscv_usb_cache_clean(urb, riscv_usb_urb_meta_size(urb));
    if (urb->setup != NULL) {
        riscv_usb_cache_clean(urb->setup, 8U);
    }

    if (urb->num_of_iso_packets > 1U) {
        for (i = 0U; i < urb->num_of_iso_packets; i++) {
            if (urb->iso_packet[i].transfer_buffer != NULL &&
                urb->iso_packet[i].transfer_buffer_length > 0U) {
                riscv_usb_cache_clean(urb->iso_packet[i].transfer_buffer,
                                      urb->iso_packet[i].transfer_buffer_length);
            }
        }
    } else if (urb->transfer_buffer != NULL && urb->transfer_buffer_length > 0U) {
        riscv_usb_cache_clean(urb->transfer_buffer, urb->transfer_buffer_length);
    }
}

static inline void riscv_usb_urb_invalidate_from_riscv(struct usbh_urb *urb)
{
    uint32_t i;

    if (urb == NULL) {
        return;
    }

    riscv_usb_cache_invalidate(urb, riscv_usb_urb_meta_size(urb));

    if (urb->num_of_iso_packets > 1U) {
        for (i = 0U; i < urb->num_of_iso_packets; i++) {
            uint32_t len = urb->iso_packet[i].actual_length;

            if (len == 0U) {
                continue;
            }
            if (urb->iso_packet[i].transfer_buffer != NULL) {
                riscv_usb_cache_invalidate(urb->iso_packet[i].transfer_buffer, len);
            }
        }
    } else if (urb->transfer_buffer != NULL && urb->actual_length > 0U) {
        riscv_usb_cache_invalidate(urb->transfer_buffer, urb->actual_length);
    }
}

static inline void riscv_usb_buffer_invalidate_from_riscv(uint8_t *buffer, uint32_t len)
{
    if (buffer != NULL && len > 0U) {
        riscv_usb_cache_invalidate(buffer, len);
    }
}

#else

struct usb_setup_packet;
struct usbh_urb;

static inline void riscv_usb_cache_clean(const void *addr, size_t len)
{
    (void)addr;
    (void)len;
}

static inline void riscv_usb_cache_invalidate(const void *addr, size_t len)
{
    (void)addr;
    (void)len;
}

static inline void riscv_usb_urb_clean_for_riscv(const struct usbh_urb *urb)
{
    (void)urb;
}

static inline void riscv_usb_urb_invalidate_from_riscv(struct usbh_urb *urb)
{
    (void)urb;
}

static inline void riscv_usb_buffer_invalidate_from_riscv(uint8_t *buffer, uint32_t len)
{
    (void)buffer;
    (void)len;
}

#endif /* CONFIG_USB_RISCV_BRIDGE && CONFIG_SRAM_DIRECT_ADDR */

#endif /* RISCV_USB_CACHE_H */
