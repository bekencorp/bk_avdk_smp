/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_core.h"
#include "usbh_hub.h"
#include "usb_musb_reg.h"

#if CONFIG_USB_RISCV_BRIDGE
#include <os/os.h>
#include "sys_driver.h"
#include <driver/int_types.h>
#include "riscv_usb_bridge.h"
#include "riscv_usb_probe_defs.h"
#include "modules/bk_riscv_types.h"
#include "hspl_driver.h"
#include "hspl_res_lock.h"
#if CONFIG_IPI
#include "ipi_driver.h"
#endif

/* The EPIDX cross-core spin lock uses BK_HSPL_RES_USB, which the hspl resource
 * map routes to HSPL_ID_1 channel RISCV_USB_HSPL_CHANNEL. Keep both views in
 * sync at build time. */
_Static_assert((int)RISCV_USB_HSPL_CHANNEL == (int)(BK_HSPL_RES_USB - 16),
               "RISCV_USB_HSPL_CHANNEL must match BK_HSPL_RES_USB mapping");

/* forward declarations: the bridge engine is defined further down but is
 * referenced from usbh_submit_urb() above it. */
struct usbh_urb;
extern volatile uint8_t usb_ep0_state;
static bool bk_v16_bridge_active(void);
static int bk_v16_bridge_arm_xfer(uint8_t chidx, uint8_t dir, struct usbh_urb *urb);
static void bk_v16_bridge_kill_xfer(struct usbh_urb *urb);
/* AP-side EPIDX serialisation against the CP (RISC-V) USB ISR using the BK7259
 * hardware spin lock. It also serialises the two AP (M55 SMP) cores against each
 * other, since the audio submit path and the USB IPI/poll completion path can run
 * on different cores concurrently.
 *
 * CRITICAL: local interrupts MUST be masked across the whole HSPL window. An
 * earlier comment here claimed callers already sit inside
 * usb_osal_enter_critical_section() with IRQs off -- that is FALSE on this port,
 * where usb_osal_enter/leave_critical_section() are no-ops. With IRQs left
 * enabled, an AP core can take this HSPL inside isoc_init/arm_xfer (task context)
 * and then be preempted ON THE SAME CORE by the USB IPI ISR, whose
 * bk_v16_bridge_complete_one() path takes this very HSPL. The ISR then spins
 * forever on a lock held by the task it just preempted (self-deadlock); the CP
 * USB ISR also blocks on the HSPL and stops servicing USB, so the ISO stream
 * wedges (dseq/dipi -> 0) after a few seconds. Masking IRQs makes the (short,
 * register-only) EPIDX holder un-preemptible and removes the deadlock. Flags are
 * saved per-core because both AP cores can be in the acquire path at once (one
 * owns the channel, the other spins).
 *
 * The spin is BOUNDED with a last-resort steal. The USB IPI completion path takes
 * this lock from hard-IRQ at ~2kHz, and its urb completion callback re-arms
 * (arm_xfer) which takes it again. If the current holder -- the peer AP core, or
 * more often the RISC-V CP USB ISR that holds it for its whole ISR -- is wedged or
 * dies still holding the lock, an UNBOUNDED spin keeps the AP core spinning with
 * IRQs masked forever -> the core hangs -> the CP raises an IPC heartbeat-timeout
 * assert (the doorbell death seen repeatedly). So if the holder has not released
 * within BK_V16_EPIDX_HSPL_STEAL_US -- ~100x a healthy tens-of-us hold, so it never
 * trips under normal contention -- force-release the channel and re-acquire. A
 * forced steal can momentarily race the (already-wedged) holder's EPIDX/CSR window,
 * i.e. at worst one corrupted transfer, which is vastly better than hanging the
 * whole AP. Steals are counted so the poll timer can surface that the peer/CP
 * wedged. */
#ifndef BK_V16_EPIDX_MAX_CPU
#define BK_V16_EPIDX_MAX_CPU 4
#endif
#define BK_V16_EPIDX_HSPL_STEAL_US 4000U
static volatile uint32_t s_v16_epidx_hspl_flags[BK_V16_EPIDX_MAX_CPU];
static volatile uint32_t s_v16_epidx_hspl_steal_cnt;
extern uint64_t bk_aon_rtc_get_us(void);

static inline void bk_v16_epidx_hspl_lock(void)
{
    uint32_t flags = rtos_disable_int();

    /* Acquire through the canonical resource lock (BK_HSPL_RES_USB -> HSPL_ID_1 /
     * RISCV_USB_HSPL_CHANNEL), so the AP side shares the recursion + owner
     * accounting and central mapping used by every other subsystem and hits the
     * exact channel the RISC-V CP drives. bk_hspl_res_try_lock() tries once (and
     * is hard-IRQ safe: no must_lock wait-forever/assert), so the bounded spin +
     * last-resort steal below stays owned here. */
    if (bk_hspl_res_try_lock(BK_HSPL_RES_USB) != BK_OK) {
        uint64_t t0 = bk_aon_rtc_get_us();
        uint32_t n = 0;

        /* contended: spin, but never forever (see the steal rationale above). The
         * time check is sampled every 16384 try_locks so it adds ~no overhead. */
        do {
            if ((((++n) & 0x3FFFU) == 0U) &&
                ((uint32_t)(bk_aon_rtc_get_us() - t0) >= BK_V16_EPIDX_HSPL_STEAL_US)) {
                /* last-resort steal: raw-release the wedged holder's HW channel
                 * (any master may write the unlock MAGIC). The steal only fires
                 * after ~100x a healthy hold, i.e. the holder is already wedged;
                 * its stale res rec_count is left as-is (same accepted trade-off
                 * as before). */
                bk_hspl_unlock(BK_HSPL_ID_1, RISCV_USB_HSPL_CHANNEL);
                s_v16_epidx_hspl_steal_cnt++;
                t0 = bk_aon_rtc_get_us();
                n = 0;
            }
        } while (bk_hspl_res_try_lock(BK_HSPL_RES_USB) != BK_OK);
    }
    s_v16_epidx_hspl_flags[rtos_get_core_id() & (BK_V16_EPIDX_MAX_CPU - 1)] = flags;
}

static inline void bk_v16_epidx_hspl_unlock(void)
{
    uint32_t flags = s_v16_epidx_hspl_flags[rtos_get_core_id() & (BK_V16_EPIDX_MAX_CPU - 1)];

    bk_hspl_res_unlock(BK_HSPL_RES_USB);
    rtos_enable_int(flags);
}
#endif

#define HWREG(x) \
    (*((volatile uint32_t *)(x)))
#define HWREGH(x) \
    (*((volatile uint16_t *)(x)))
#define HWREGB(x) \
    (*((volatile uint8_t *)(x)))

#define USB_BASE (bus->hcd.reg_base)

#if defined(CONFIG_USB_MUSB_SUNXI)
#define MUSB_FADDR_OFFSET 0x98
#define MUSB_POWER_OFFSET 0x40
#define MUSB_TXIS_OFFSET  0x44
#define MUSB_RXIS_OFFSET  0x46
#define MUSB_TXIE_OFFSET  0x48
#define MUSB_RXIE_OFFSET  0x4A
#define MUSB_IS_OFFSET    0x4C
#define MUSB_IE_OFFSET    0x50
#define MUSB_EPIDX_OFFSET 0x42

#define MUSB_IND_TXMAP_OFFSET      0x80
#define MUSB_IND_TXCSRL_OFFSET     0x82
#define MUSB_IND_TXCSRH_OFFSET     0x83
#define MUSB_IND_RXMAP_OFFSET      0x84
#define MUSB_IND_RXCSRL_OFFSET     0x86
#define MUSB_IND_RXCSRH_OFFSET     0x87
#define MUSB_IND_RXCOUNT_OFFSET    0x88
#define MUSB_IND_TXTYPE_OFFSET     0x8C
#define MUSB_IND_TXINTERVAL_OFFSET 0x8D
#define MUSB_IND_RXTYPE_OFFSET     0x8E
#define MUSB_IND_RXINTERVAL_OFFSET 0x8F

#define MUSB_FIFO_OFFSET 0x00

#define MUSB_DEVCTL_OFFSET 0x41

#define MUSB_TXFIFOSZ_OFFSET  0x90
#define MUSB_RXFIFOSZ_OFFSET  0x94
#define MUSB_TXFIFOADD_OFFSET 0x92
#define MUSB_RXFIFOADD_OFFSET 0x96

#define MUSB_TXFUNCADDR0_OFFSET 0x98
#define MUSB_TXHUBADDR0_OFFSET  0x9A
#define MUSB_TXHUBPORT0_OFFSET  0x9B
#define MUSB_TXFUNCADDRx_OFFSET 0x98
#define MUSB_TXHUBADDRx_OFFSET  0x9A
#define MUSB_TXHUBPORTx_OFFSET  0x9B
#define MUSB_RXFUNCADDRx_OFFSET 0x9C
#define MUSB_RXHUBADDRx_OFFSET  0x9E
#define MUSB_RXHUBPORTx_OFFSET  0x9F

#define USB_TXMAP_BASE(ep_idx)      (USB_BASE + MUSB_IND_TXMAP_OFFSET)
#define USB_TXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXCSRL_OFFSET)
#define USB_TXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXCSRH_OFFSET)
#define USB_RXMAP_BASE(ep_idx)      (USB_BASE + MUSB_IND_RXMAP_OFFSET)
#define USB_RXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXCSRL_OFFSET)
#define USB_RXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXCSRH_OFFSET)
#define USB_RXCOUNT_BASE(ep_idx)    (USB_BASE + MUSB_IND_RXCOUNT_OFFSET)
#define USB_TXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXTYPE_OFFSET)
#define USB_TXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_IND_TXINTERVAL_OFFSET)
#define USB_RXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXTYPE_OFFSET)
#define USB_RXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_IND_RXINTERVAL_OFFSET)

#define USB_TXADDR_BASE(ep_idx)    (USB_BASE + MUSB_TXFUNCADDRx_OFFSET)
#define USB_TXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_TXHUBADDRx_OFFSET)
#define USB_TXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_TXHUBPORTx_OFFSET)
#define USB_RXADDR_BASE(ep_idx)    (USB_BASE + MUSB_RXFUNCADDRx_OFFSET)
#define USB_RXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_RXHUBADDRx_OFFSET)
#define USB_RXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_RXHUBPORTx_OFFSET)

#elif defined(CONFIG_USB_MUSB_CUSTOM)
#include "musb_custom.h"
#else
#define MUSB_FADDR_OFFSET 0x00
#define MUSB_POWER_OFFSET 0x01
#define MUSB_TXIS_OFFSET  0x02
#define MUSB_RXIS_OFFSET  0x04
#define MUSB_TXIE_OFFSET  0x06
#define MUSB_RXIE_OFFSET  0x08
#define MUSB_IS_OFFSET    0x0A
#define MUSB_IE_OFFSET    0x0B

#define MUSB_EPIDX_OFFSET 0x0E

#define MUSB_IND_TXMAP_OFFSET      0x10
#define MUSB_IND_TXCSRL_OFFSET     0x12
#define MUSB_IND_TXCSRH_OFFSET     0x13
#define MUSB_IND_RXMAP_OFFSET      0x14
#define MUSB_IND_RXCSRL_OFFSET     0x16
#define MUSB_IND_RXCSRH_OFFSET     0x17
#define MUSB_IND_RXCOUNT_OFFSET    0x18
#define MUSB_IND_TXTYPE_OFFSET     0x1A
#define MUSB_IND_TXINTERVAL_OFFSET 0x1B
#define MUSB_IND_RXTYPE_OFFSET     0x1C
#define MUSB_IND_RXINTERVAL_OFFSET 0x1D

#define MUSB_FIFO_OFFSET 0x20

#define MUSB_DEVCTL_OFFSET 0x60

#define MUSB_TXFIFOSZ_OFFSET  0x62
#define MUSB_RXFIFOSZ_OFFSET  0x63
#define MUSB_TXFIFOADD_OFFSET 0x64
#define MUSB_RXFIFOADD_OFFSET 0x66

#define MUSB_TXFUNCADDR0_OFFSET 0x80
#define MUSB_TXHUBADDR0_OFFSET  0x82
#define MUSB_TXHUBPORT0_OFFSET  0x83
#define MUSB_TXFUNCADDRx_OFFSET 0x88
#define MUSB_TXHUBADDRx_OFFSET  0x8A
#define MUSB_TXHUBPORTx_OFFSET  0x8B
#define MUSB_RXFUNCADDRx_OFFSET 0x8C
#define MUSB_RXHUBADDRx_OFFSET  0x8E
#define MUSB_RXHUBPORTx_OFFSET  0x8F

#if CONFIG_BK_USB_CHERRYUSB_V1_6
/* BK7259 MHDRC uses EPIDX + indexed registers. The generic direct endpoint
 * window at 0x100 causes EP0 setup transfers to fail for FS storage devices. */
#define USB_TXMAP_BASE(ep_idx)      (USB_BASE + MUSB_IND_TXMAP_OFFSET)
#define USB_TXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXCSRL_OFFSET)
#define USB_TXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXCSRH_OFFSET)
#define USB_RXMAP_BASE(ep_idx)      (USB_BASE + MUSB_IND_RXMAP_OFFSET)
#define USB_RXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXCSRL_OFFSET)
#define USB_RXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXCSRH_OFFSET)
#define USB_RXCOUNT_BASE(ep_idx)    (USB_BASE + MUSB_IND_RXCOUNT_OFFSET)
#define USB_TXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_IND_TXTYPE_OFFSET)
#define USB_TXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_IND_TXINTERVAL_OFFSET)
#define USB_RXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_IND_RXTYPE_OFFSET)
#define USB_RXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_IND_RXINTERVAL_OFFSET)
#else
#define MUSB_TXMAP0_OFFSET          0x100

// do not use EPIDX
#define USB_TXMAP_BASE(ep_idx)      (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx)
#define USB_TXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 2)
#define USB_TXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 3)
#define USB_RXMAP_BASE(ep_idx)      (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 4)
#define USB_RXCSRL_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 6)
#define USB_RXCSRH_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 7)
#define USB_RXCOUNT_BASE(ep_idx)    (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 8)
#define USB_TXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 0x0A)
#define USB_TXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 0x0B)
#define USB_RXTYPE_BASE(ep_idx)     (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 0x0C)
#define USB_RXINTERVAL_BASE(ep_idx) (USB_BASE + MUSB_TXMAP0_OFFSET + 0x10 * ep_idx + 0x0D)
#endif

#define USB_TXADDR_BASE(ep_idx)    (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx)
#define USB_TXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 2)
#define USB_TXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 3)
#define USB_RXADDR_BASE(ep_idx)    (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 4)
#define USB_RXHUBADDR_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 6)
#define USB_RXHUBPORT_BASE(ep_idx) (USB_BASE + MUSB_TXFUNCADDR0_OFFSET + 0x8 * ep_idx + 7)
#endif

#define USB_FIFO_BASE(ep_idx) (USB_BASE + MUSB_FIFO_OFFSET + 0x4 * ep_idx)

#if CONFIG_BK_USB_CHERRYUSB_V1_6
#define BK_USB_PHY_BASE(bus)        ((bus)->hcd.reg_base + 0x400)
#define BK_NANENG_PHY_FC_REG0C     (0x0C * 4)
#endif

typedef enum {
    USB_EP0_STATE_SETUP = 0x0, /**< SETUP DATA */
    USB_EP0_STATE_IN_DATA,     /**< IN DATA */
    USB_EP0_STATE_IN_STATUS,   /**< IN status*/
    USB_EP0_STATE_OUT_DATA,    /**< OUT DATA */
    USB_EP0_STATE_OUT_STATUS,  /**< OUT status */
} ep0_state_t;

struct musb_pipe {
    uint8_t chidx;
    bool inuse;
    uint32_t xfrd;
    volatile uint8_t ep0_state;
    usb_osal_sem_t waitsem;
    struct usbh_urb *urb;
};

struct musb_hcd {
    volatile bool port_csc;
    volatile bool port_pec;
    volatile bool port_pe;
    struct musb_pipe pipe_pool[CONFIG_USB_MUSB_PIPE_NUM];
} g_musb_hcd[CONFIG_USBHOST_MAX_BUS];

/* get current active ep */
static uint8_t musb_get_active_ep(struct usbh_bus *bus)
{
    return HWREGB(USB_BASE + MUSB_EPIDX_OFFSET);
}

/* set the active ep */
static void musb_set_active_ep(struct usbh_bus *bus, uint8_t ep_index)
{
    HWREGB(USB_BASE + MUSB_EPIDX_OFFSET) = ep_index;
}

static void musb_fifo_flush(struct usbh_bus *bus, uint8_t ep)
{
    uint8_t ep_idx = ep & 0x7f;
    if (ep_idx == 0) {
        if ((HWREGB(USB_TXCSRL_BASE(ep_idx)) & (USB_CSRL0_RXRDY | USB_CSRL0_TXRDY)) != 0)
            HWREGB(USB_RXCSRL_BASE(ep_idx)) |= USB_CSRH0_FLUSH;
    } else {
        if (ep & 0x80) {
            if (HWREGB(USB_TXCSRL_BASE(ep_idx)) & USB_TXCSRL1_TXRDY)
                HWREGB(USB_TXCSRL_BASE(ep_idx)) |= USB_TXCSRL1_FLUSH;
        } else {
            if (HWREGB(USB_RXCSRL_BASE(ep_idx)) & USB_RXCSRL1_RXRDY)
                HWREGB(USB_RXCSRL_BASE(ep_idx)) |= USB_RXCSRL1_FLUSH;
        }
    }
}

static void musb_write_packet(struct usbh_bus *bus, uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;

    if ((uint32_t)buffer & 0x03) {
        buf8 = buffer;
        for (i = 0; i < len; i++) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    } else {
        count32 = len >> 2;
        count8 = len & 0x03;

        buf32 = (uint32_t *)buffer;

        while (count32--) {
            HWREG(USB_FIFO_BASE(ep_idx)) = *buf32++;
        }

        buf8 = (uint8_t *)buf32;

        while (count8--) {
            HWREGB(USB_FIFO_BASE(ep_idx)) = *buf8++;
        }
    }
}

static void musb_read_packet(struct usbh_bus *bus, uint8_t ep_idx, uint8_t *buffer, uint16_t len)
{
    uint32_t *buf32;
    uint8_t *buf8;
    uint32_t count32;
    uint32_t count8;
    int i;

    if ((uint32_t)buffer & 0x03) {
        buf8 = buffer;
        for (i = 0; i < len; i++) {
            *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
        }
    } else {
        count32 = len >> 2;
        count8 = len & 0x03;

        buf32 = (uint32_t *)buffer;

        while (count32--) {
            *buf32++ = HWREG(USB_FIFO_BASE(ep_idx));
        }

        buf8 = (uint8_t *)buf32;

        while (count8--) {
            *buf8++ = HWREGB(USB_FIFO_BASE(ep_idx));
        }
    }
}

static uint32_t musb_get_fifo_size(uint16_t mps, uint16_t *used)
{
    uint32_t size;

    for (uint8_t i = USB_TXFIFOSZ_SIZE_8; i <= USB_TXFIFOSZ_SIZE_2048; i++) {
        size = (8 << i);
        if (mps <= size) {
            *used = size;
            return i;
        }
    }

    *used = 0;
    return USB_TXFIFOSZ_SIZE_8;
}

static uint32_t usbh_musb_fifo_config(struct usbh_bus *bus, struct musb_fifo_cfg *cfg, uint32_t offset)
{
    uint16_t fifo_used;
    uint8_t c_size;
    uint16_t c_off;

    c_off = offset >> 3;
    c_size = musb_get_fifo_size(cfg->maxpacket, &fifo_used);

    musb_set_active_ep(bus, cfg->ep_num);

    switch (cfg->style) {
        case FIFO_TX:
            HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = c_size & 0x0f;
            HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = c_off;
            break;
        case FIFO_RX:
            HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = c_size & 0x0f;
            HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = c_off;
            break;
        case FIFO_TXRX:
            HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = c_size & 0x0f;
            HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = c_off;
            HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = c_size & 0x0f;
            HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = c_off;
            break;

        default:
            break;
    }

    return (offset + fifo_used);
}

void musb_control_urb_init(struct usbh_bus *bus, uint8_t chidx, struct usbh_urb *urb, struct usb_setup_packet *setup, uint8_t *buffer, uint32_t buflen)
{
    uint8_t old_ep_index;
    uint8_t speed = USB_TXTYPE1_SPEED_FULL;
#if CONFIG_USB_RISCV_BRIDGE
    volatile riscv_usb_probe_t *epidx_ctx = bk_v16_bridge_active() ? get_riscv_usb_probe() : NULL;

    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_lock();
    }
#endif

    old_ep_index = musb_get_active_ep(bus);
    musb_set_active_ep(bus, chidx);

    if (urb->hport->speed == USB_SPEED_HIGH) {
        speed = USB_TYPE0_SPEED_HIGH;
    } else if (urb->hport->speed == USB_SPEED_FULL) {
        speed = USB_TYPE0_SPEED_FULL;
    } else if (urb->hport->speed == USB_SPEED_LOW) {
        speed = USB_TYPE0_SPEED_LOW;
    }

#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
    /* Without multipoint, use FADDR for host target addressing and do not access Hub/FuncAddr regs */
    HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
    HWREGB(USB_TXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
    HWREGB(USB_TXTYPE_BASE(chidx)) = speed;
#else
    HWREGB(USB_TXADDR_BASE(chidx)) = urb->hport->dev_addr;
    HWREGB(USB_TXTYPE_BASE(chidx)) = speed;
    HWREGB(USB_TXHUBADDR_BASE(chidx)) = 0;
    HWREGB(USB_TXHUBPORT_BASE(chidx)) = 0;
#endif

    musb_write_packet(bus, chidx, (uint8_t *)setup, 8);
    HWREGB(USB_TXCSRL_BASE(chidx)) = USB_CSRL0_TXRDY | USB_CSRL0_SETUP;
    musb_set_active_ep(bus, old_ep_index);
#if CONFIG_USB_RISCV_BRIDGE
    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_unlock();
    }
#endif
}

int musb_bulk_urb_init(struct usbh_bus *bus, uint8_t chidx, struct usbh_urb *urb, uint8_t *buffer, uint32_t buflen)
{
    uint8_t old_ep_index;
    uint8_t speed = USB_TXTYPE1_SPEED_FULL;
#if CONFIG_USB_RISCV_BRIDGE
    volatile riscv_usb_probe_t *epidx_ctx = bk_v16_bridge_active() ? get_riscv_usb_probe() : NULL;

    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_lock();
    }
#define BK_EPIDX_BULK_UNLOCK() do { if (epidx_ctx != NULL) { bk_v16_epidx_hspl_unlock(); } } while (0)
#else
#define BK_EPIDX_BULK_UNLOCK() do { } while (0)
#endif

    old_ep_index = musb_get_active_ep(bus);
    musb_set_active_ep(bus, chidx);

    if (urb->hport->speed == USB_SPEED_HIGH) {
        speed = USB_TXTYPE1_SPEED_HIGH;
    } else if (urb->hport->speed == USB_SPEED_FULL) {
        speed = USB_TXTYPE1_SPEED_FULL;
    } else if (urb->hport->speed == USB_SPEED_LOW) {
        speed = USB_TXTYPE1_SPEED_LOW;
    }

    if (urb->ep->bEndpointAddress & 0x80) {
        if ((8 << HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET)) < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", urb->ep->bEndpointAddress);
            BK_EPIDX_BULK_UNLOCK();
            return -USB_ERR_RANGE;
        }

#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_BULK;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = 0;
#else
        HWREGB(USB_RXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_BULK;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = 0;
        HWREGB(USB_RXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_RXHUBPORT_BASE(chidx)) = 0;
#endif
        HWREGB(USB_TXCSRH_BASE(chidx)) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_RXCSRL_BASE(chidx)) = USB_RXCSRL1_REQPKT;

        HWREGH(USB_BASE + MUSB_RXIE_OFFSET) |= (1 << chidx);
    } else {
        if ((8 << HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET)) < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", urb->ep->bEndpointAddress);
            BK_EPIDX_BULK_UNLOCK();
            return -USB_ERR_RANGE;
        }

#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_BULK;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = 0;
#else
        HWREGB(USB_TXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_BULK;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = 0;
        HWREGB(USB_TXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(chidx)) = 0;
#endif

        if (buflen > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            buflen = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        }

        musb_write_packet(bus, chidx, buffer, buflen);
        HWREGB(USB_TXCSRH_BASE(chidx)) |= USB_TXCSRH1_MODE;
        HWREGB(USB_TXCSRL_BASE(chidx)) = USB_TXCSRL1_TXRDY;

        HWREGH(USB_BASE + MUSB_TXIE_OFFSET) |= (1 << chidx);
    }
    musb_set_active_ep(bus, old_ep_index);
    BK_EPIDX_BULK_UNLOCK();
#undef BK_EPIDX_BULK_UNLOCK
    return 0;
}

int musb_intr_urb_init(struct usbh_bus *bus, uint8_t chidx, struct usbh_urb *urb, uint8_t *buffer, uint32_t buflen)
{
    uint8_t old_ep_index;
    uint8_t speed = USB_TXTYPE1_SPEED_FULL;
#if CONFIG_USB_RISCV_BRIDGE
    volatile riscv_usb_probe_t *epidx_ctx = bk_v16_bridge_active() ? get_riscv_usb_probe() : NULL;

    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_lock();
    }
#define BK_EPIDX_INTR_UNLOCK() do { if (epidx_ctx != NULL) { bk_v16_epidx_hspl_unlock(); } } while (0)
#else
#define BK_EPIDX_INTR_UNLOCK() do { } while (0)
#endif

    old_ep_index = musb_get_active_ep(bus);
    musb_set_active_ep(bus, chidx);

    if (urb->hport->speed == USB_SPEED_HIGH) {
        speed = USB_TXTYPE1_SPEED_HIGH;
    } else if (urb->hport->speed == USB_SPEED_FULL) {
        speed = USB_TXTYPE1_SPEED_FULL;
    } else if (urb->hport->speed == USB_SPEED_LOW) {
        speed = USB_TXTYPE1_SPEED_LOW;
    }

    if (urb->ep->bEndpointAddress & 0x80) {
        if ((8 << HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET)) < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", urb->ep->bEndpointAddress);
            BK_EPIDX_INTR_UNLOCK();
            return -USB_ERR_RANGE;
        }

#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_INT;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
#else
        HWREGB(USB_RXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_INT;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
        HWREGB(USB_RXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_RXHUBPORT_BASE(chidx)) = 0;
#endif
        HWREGB(USB_TXCSRH_BASE(chidx)) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_RXCSRL_BASE(chidx)) = USB_RXCSRL1_REQPKT;

        HWREGH(USB_BASE + MUSB_RXIE_OFFSET) |= (1 << chidx);
    } else {
        if ((8 << HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET)) < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            USB_LOG_ERR("Ep %02x fifo is overflow\r\n", urb->ep->bEndpointAddress);
            BK_EPIDX_INTR_UNLOCK();
            return -USB_ERR_RANGE;
        }

#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_INT;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
#else
        HWREGB(USB_TXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_INT;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
        HWREGB(USB_TXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(chidx)) = 0;
#endif

        if (buflen > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            buflen = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        }

        musb_write_packet(bus, chidx, buffer, buflen);
        HWREGB(USB_TXCSRH_BASE(chidx)) |= USB_TXCSRH1_MODE;
        HWREGB(USB_TXCSRL_BASE(chidx)) = USB_TXCSRL1_TXRDY;

        HWREGH(USB_BASE + MUSB_TXIE_OFFSET) |= (1 << chidx);
    }
    musb_set_active_ep(bus, old_ep_index);
    BK_EPIDX_INTR_UNLOCK();
#undef BK_EPIDX_INTR_UNLOCK
    return 0;
}

int musb_isoc_urb_init(struct usbh_bus *bus, uint8_t chidx, struct usbh_urb *urb, uint8_t *buffer, uint32_t buflen)
{
    uint8_t old_ep_index;
    uint8_t speed = USB_TXTYPE1_SPEED_FULL;
#if CONFIG_USB_RISCV_BRIDGE
    volatile riscv_usb_probe_t *epidx_ctx = bk_v16_bridge_active() ? get_riscv_usb_probe() : NULL;

    /* Serialise the EPIDX indexed-register window against the CP USB ISR. The
     * ISO (re)arm below runs ~1kHz per active endpoint and, when a second ISO
     * IN endpoint streams concurrently, races the CP's per-channel EPIDX use. */
    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_lock();
    }
#endif

    old_ep_index = musb_get_active_ep(bus);
    musb_set_active_ep(bus, chidx);

    if (urb->hport->speed == USB_SPEED_HIGH) {
        speed = USB_TXTYPE1_SPEED_HIGH;
    } else if (urb->hport->speed == USB_SPEED_FULL) {
        speed = USB_TXTYPE1_SPEED_FULL;
    } else if (urb->hport->speed == USB_SPEED_LOW) {
        speed = USB_TXTYPE1_SPEED_LOW;
    }

    if (urb->ep->bEndpointAddress & 0x80) {
#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_RXTYPE1_PROTO_ISOC;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
#else
        HWREGB(USB_RXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_RXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_RXTYPE1_PROTO_ISOC;
        HWREGH(USB_RXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_RXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
        HWREGB(USB_RXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_RXHUBPORT_BASE(chidx)) = 0;
#endif
        /* ISO RX: set ISO mode in RXCSRH, then arm the first packet request.
         * In bridge mode the CP firmware drives the remaining microframes. */
        HWREGB(USB_TXCSRH_BASE(chidx)) &= ~USB_TXCSRH1_MODE;
        HWREGB(USB_RXCSRH_BASE(chidx)) |= USB_RXCSRH1_ISO;
        HWREGB(USB_RXCSRL_BASE(chidx)) = USB_RXCSRL1_REQPKT;

        HWREGH(USB_BASE + MUSB_RXIE_OFFSET) |= (1 << chidx);
    } else {
#ifdef CONFIG_USB_MUSB_WITHOUT_MULTIPOINT
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXADDR_BASE(chidx)) = (urb->hport->dev_addr & 0x7F);
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_ISOC;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
#else
        HWREGB(USB_TXADDR_BASE(chidx)) = urb->hport->dev_addr;
        HWREGB(USB_TXTYPE_BASE(chidx)) = (urb->ep->bEndpointAddress & 0x0f) | speed | USB_TXTYPE1_PROTO_ISOC;
        HWREGH(USB_TXMAP_BASE(chidx)) = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        HWREGB(USB_TXINTERVAL_BASE(chidx)) = urb->ep->bInterval;
        HWREGB(USB_TXHUBADDR_BASE(chidx)) = 0;
        HWREGB(USB_TXHUBPORT_BASE(chidx)) = 0;
#endif
        if (buflen > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
            buflen = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
        }

        HWREGB(USB_TXCSRH_BASE(chidx)) |= (USB_TXCSRH1_MODE | USB_TXCSRH1_ISO);
        musb_write_packet(bus, chidx, buffer, buflen);
        HWREGB(USB_TXCSRL_BASE(chidx)) = USB_TXCSRL1_TXRDY;

        HWREGH(USB_BASE + MUSB_TXIE_OFFSET) |= (1 << chidx);
    }
    musb_set_active_ep(bus, old_ep_index);
#if CONFIG_USB_RISCV_BRIDGE
    if (epidx_ctx != NULL) {
        bk_v16_epidx_hspl_unlock();
    }
#endif
    return 0;
}

static int usbh_reset_port(struct usbh_bus *bus, const uint8_t port)
{
    g_musb_hcd[bus->hcd.hcd_id].port_pe = 0;

#if CONFIG_BK_USB_CHERRYUSB_V1_6
    /* Clear any residual device-mode soft-connect that survives the OTG switch. */
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_SOFTCONN;

    /* High-speed storage devices on this MUSB/PHY are frequently reported as
     * full-speed after the *first* bus reset: the HS chirp handshake loses the
     * race and the device falls back to FS, where this controller then gets no
     * response to the first GET_DESCRIPTOR SETUP (csr0=ERROR). CherryUSB only
     * retries the control transfer, never the port reset, so a single failed
     * chirp permanently strands the device at FS.
     *
     * Drive the chirp ourselves: keep HSENAB armed and re-assert bus reset a
     * few times, breaking out as soon as POWER.HSMODE shows the device
     * negotiated high speed. A genuinely full-speed device simply stays FS for
     * all attempts (no regression for FS-only peripherals). */
    for (int attempt = 0; attempt < 3; attempt++) {
        HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
        HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESET;
        usb_osal_msleep(20);
        HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~(USB_POWER_RESET);
        usb_osal_msleep(20);
        if (HWREGB(USB_BASE + MUSB_POWER_OFFSET) & USB_POWER_HSMODE) {
            break;
        }
    }
#else
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESET;

#ifdef CONFIG_USB_MUSB_SIFLI
    extern void musb_reset_prev(void);
    musb_reset_prev();
#endif
    usb_osal_msleep(20);
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~(USB_POWER_RESET);
    usb_osal_msleep(20);
#ifdef CONFIG_USB_MUSB_SIFLI
    extern void musb_reset_post(void);
    musb_reset_post();
#endif
#endif
    g_musb_hcd[bus->hcd.hcd_id].port_pe = 1;
    return 0;
}

static uint8_t usbh_get_port_speed(struct usbh_bus *bus, const uint8_t port)
{
    uint8_t speed = USB_SPEED_UNKNOWN;

    if (HWREGB(USB_BASE + MUSB_POWER_OFFSET) & USB_POWER_HSMODE)
        speed = USB_SPEED_HIGH;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_FSDEV)
        speed = USB_SPEED_FULL;
    else if (HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_LSDEV)
        speed = USB_SPEED_LOW;

    return speed;
}

static int musb_pipe_alloc(struct usbh_bus *bus)
{
    int chidx;
    uintptr_t flags;

    flags = usb_osal_enter_critical_section();
    for (chidx = 1; chidx < CONFIG_USB_MUSB_PIPE_NUM; chidx++) {
        if (!g_musb_hcd[bus->hcd.hcd_id].pipe_pool[chidx].inuse) {
            g_musb_hcd[bus->hcd.hcd_id].pipe_pool[chidx].inuse = true;
            usb_osal_leave_critical_section(flags);
            return chidx;
        }
    }
    usb_osal_leave_critical_section(flags);

    return -1;
}

static void musb_pipe_free(struct musb_pipe *pipe)
{
    uintptr_t flags;

    flags = usb_osal_enter_critical_section();
    if (pipe->urb) {
        pipe->urb->hcpriv = NULL;
        pipe->urb = NULL;
    }

    pipe->inuse = false;
    usb_osal_leave_critical_section(flags);
}

__WEAK void usb_hc_low_level_init(struct usbh_bus *bus)
{
    (void)bus;
}

__WEAK void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
    (void)bus;
}

static void musb_host_controller_arm(struct usbh_bus *bus)
{
    uint8_t regval;
    uint16_t offset = 0;
    uint8_t cfg_num;
    struct musb_fifo_cfg *cfg;

    cfg_num = usbh_get_musb_fifo_cfg(&cfg);

    for (uint8_t i = 0; i < cfg_num; i++) {
        offset = usbh_musb_fifo_config(bus, &cfg[i], offset);
    }

    USB_ASSERT_MSG(offset <= usb_get_musb_ram_size(), "Your fifo config is overflow, please check");

    /* Enable USB interrupts */
    regval = USB_IE_RESET | USB_IE_CONN | USB_IE_DISCON |
             USB_IE_RESUME | USB_IE_SUSPND |
             USB_IE_BABBLE | USB_IE_SESREQ | USB_IE_VBUSERR;

    HWREGB(USB_BASE + MUSB_IE_OFFSET) = regval;
    HWREGH(USB_BASE + MUSB_TXIE_OFFSET) = USB_TXIE_EP0;
    HWREGH(USB_BASE + MUSB_RXIE_OFFSET) = 0;

#if CONFIG_BK_USB_CHERRYUSB_V1_6
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_SOFTCONN;
#endif
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

#ifdef CONFIG_USB_MUSB_SUNXI
    musb_set_active_ep(bus, 0);
    HWREGB(USB_TXCSRL_BASE(0)) = USB_CSRL0_TXRDY;
#endif
}

int usb_hc_init(struct usbh_bus *bus)
{
    usb_hc_low_level_init(bus);

    memset(&g_musb_hcd[bus->hcd.hcd_id], 0, sizeof(struct musb_hcd));

    for (uint8_t i = 0; i < CONFIG_USB_MUSB_PIPE_NUM; i++) {
        g_musb_hcd[bus->hcd.hcd_id].pipe_pool[i].waitsem = usb_osal_sem_create(0);
    }

    musb_host_controller_arm(bus);
    return 0;
}

#if CONFIG_BK_USB_CHERRYUSB_V1_6
/* Roothub enumeration-failure recovery for BK7259.
 *
 * High-speed storage devices on this MUSB/PHY can lose the very first HS chirp
 * after reset and fall back to full speed, where the controller then gets no
 * SETUP response. The upstream hub simply gives up after one failed
 * enumeration. The legacy Beken hub instead power-cycles the USB analog PHY and
 * re-arms the controller, which forces the still-attached device to physically
 * re-attach and run a fresh chirp -- retried until high speed is negotiated.
 * Replicate that here so the v1.6 host stack enumerates the same devices.
 *
 * bk_analog_layer_usb_sys_related_ops(USB_HOST_MODE, false/true) drops and
 * re-applies the USB clock + analog PHY + OTG host routing (the heavy part of
 * the legacy bk_usb_phy_register_refresh()); musb_host_controller_arm() then
 * re-enables interrupts, HS chirp and the VBUS session. The resulting CONNECT
 * interrupt wakes the hub thread to enumerate again. */
void bk_cherryusb_v16_host_recover(struct usbh_bus *bus)
{
    extern void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops);

    bk_analog_layer_usb_sys_related_ops(0 /* USB_HOST_MODE */, false);
    bk_analog_layer_usb_sys_related_ops(0 /* USB_HOST_MODE */, true);
    musb_host_controller_arm(bus);
}
#endif

int usb_hc_deinit(struct usbh_bus *bus)
{
    HWREGB(USB_BASE + MUSB_IE_OFFSET) = 0;
    HWREGH(USB_BASE + MUSB_TXIE_OFFSET) = 0;
    HWREGH(USB_BASE + MUSB_RXIE_OFFSET) = 0;

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_HSENAB;
    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) &= ~USB_DEVCTL_SESSION;

    for (uint8_t i = 0; i < CONFIG_USB_MUSB_PIPE_NUM; i++) {
        usb_osal_sem_delete(g_musb_hcd[bus->hcd.hcd_id].pipe_pool[i].waitsem);
    }

    usb_hc_low_level_deinit(bus);
    return 0;
}

int usbh_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf)
{
    uint8_t nports;
    uint8_t port;
    uint32_t status;

    nports = CONFIG_USBHOST_MAX_RHPORTS;
    port = setup->wIndex;
    if (setup->bmRequestType & USB_REQUEST_RECIPIENT_DEVICE) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -USB_ERR_INVAL;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -USB_ERR_INVAL;
                }
                break;
            case HUB_REQUEST_GET_DESCRIPTOR:
                break;
            case HUB_REQUEST_GET_STATUS:
                memset(buf, 0, 4);
                break;
            default:
                break;
        }
    } else if (setup->bmRequestType & USB_REQUEST_RECIPIENT_OTHER) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_ENABLE:
                        break;
                    case HUB_PORT_FEATURE_SUSPEND:
                    case HUB_PORT_FEATURE_C_SUSPEND:
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        break;
                    case HUB_PORT_FEATURE_C_CONNECTION:
                        g_musb_hcd[bus->hcd.hcd_id].port_csc = 0;
                        break;
                    case HUB_PORT_FEATURE_C_ENABLE:
                        g_musb_hcd[bus->hcd.hcd_id].port_pec = 0;
                        break;
                    case HUB_PORT_FEATURE_C_OVER_CURREN:
                        break;
                    case HUB_PORT_FEATURE_C_RESET:
                        break;
                    default:
                        return -USB_ERR_INVAL;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_SUSPEND:
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        break;
                    case HUB_PORT_FEATURE_RESET:
                        usbh_reset_port(bus, port);
                        break;

                    default:
                        return -USB_ERR_INVAL;
                }
                break;
            case HUB_REQUEST_GET_STATUS:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }

                status = 0;
                if (g_musb_hcd[bus->hcd.hcd_id].port_csc) {
                    status |= (1 << HUB_PORT_FEATURE_C_CONNECTION);
                }
                if (g_musb_hcd[bus->hcd.hcd_id].port_pec) {
                    status |= (1 << HUB_PORT_FEATURE_C_ENABLE);
                }

                if (g_musb_hcd[bus->hcd.hcd_id].port_pe) {
                    status |= (1 << HUB_PORT_FEATURE_CONNECTION);
                    status |= (1 << HUB_PORT_FEATURE_ENABLE);
                    if (usbh_get_port_speed(bus, port) == USB_SPEED_LOW) {
                        status |= (1 << HUB_PORT_FEATURE_LOWSPEED);
                    } else if (usbh_get_port_speed(bus, port) == USB_SPEED_HIGH) {
                        status |= (1 << HUB_PORT_FEATURE_HIGHSPEED);
                    }
                }

                status |= (1 << HUB_PORT_FEATURE_POWER);
                memcpy(buf, &status, 4);
                break;
            default:
                break;
        }
    }
    return 0;
}

int usbh_submit_urb(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;
    struct usbh_bus *bus;
    int chidx;
    size_t flags;
    int ret = 0;

    if (!urb || !urb->hport || !urb->ep || !urb->hport->bus) {
        return -USB_ERR_INVAL;
    }

    if (!urb->hport->connected) {
        return -USB_ERR_NOTCONN;
    }

    if (urb->errorcode == -USB_ERR_BUSY) {
        return -USB_ERR_BUSY;
    }

    bus = urb->hport->bus;

    if (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) == USB_ENDPOINT_TYPE_CONTROL) {
        chidx = 0;
    } else {
        chidx = musb_pipe_alloc(bus);
        if (chidx == -1) {
            return -USB_ERR_NOMEM;
        }
    }

    flags = usb_osal_enter_critical_section();

    pipe = &g_musb_hcd[bus->hcd.hcd_id].pipe_pool[chidx];
    pipe->chidx = chidx;
    pipe->urb = urb;

    urb->hcpriv = pipe;
    urb->errorcode = -USB_ERR_BUSY;
    urb->actual_length = 0;

#if CONFIG_USB_RISCV_BRIDGE
    bool bridge = bk_v16_bridge_active();
    uint8_t bdir = (urb->ep->bEndpointAddress & 0x80) ? 1 : 0;
#endif

    switch (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes)) {
        case USB_ENDPOINT_TYPE_CONTROL:
            pipe->ep0_state = USB_EP0_STATE_SETUP;
#if CONFIG_USB_RISCV_BRIDGE
            if (bridge) {
                usb_ep0_state = USB_EP0_STATE_SETUP;
                ret = bk_v16_bridge_arm_xfer(0, 0, urb);
                if (ret < 0) {
                    goto errout_submit_init;
                }
            }
#endif
            musb_control_urb_init(bus, 0, urb, urb->setup, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        case USB_ENDPOINT_TYPE_BULK:
#if CONFIG_USB_RISCV_BRIDGE
            if (bridge) {
                ret = bk_v16_bridge_arm_xfer(chidx, bdir, urb);
                if (ret < 0) {
                    goto errout_submit_init;
                }
            }
#endif
            ret = musb_bulk_urb_init(bus, chidx, urb, urb->transfer_buffer, urb->transfer_buffer_length);
            if (ret < 0) {
                goto errout_submit_init;
            }
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
#if CONFIG_USB_RISCV_BRIDGE
            if (bridge) {
                ret = bk_v16_bridge_arm_xfer(chidx, bdir, urb);
                if (ret < 0) {
                    goto errout_submit_init;
                }
            }
#endif
            ret = musb_intr_urb_init(bus, chidx, urb, urb->transfer_buffer, urb->transfer_buffer_length);
            if (ret < 0) {
                goto errout_submit_init;
            }
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
#if CONFIG_USB_RISCV_BRIDGE
            if (bridge) {
                ret = bk_v16_bridge_arm_xfer(chidx, bdir, urb);
                if (ret < 0) {
                    goto errout_submit_init;
                }
            }
#endif
            ret = musb_isoc_urb_init(bus, chidx, urb, urb->transfer_buffer, urb->transfer_buffer_length);
            if (ret < 0) {
                goto errout_submit_init;
            }
            break;
        default:
            break;
    }
    usb_osal_leave_critical_section(flags);

    if (urb->timeout > 0) {
        /* wait until timeout or sem give */
        ret = usb_osal_sem_take(pipe->waitsem, urb->timeout);
        if (ret < 0) {
            goto errout_timeout;
        }
        urb->timeout = 0;
        ret = urb->errorcode;
        /* we can free pipe when waitsem is done */
        musb_pipe_free(pipe);
    }
    return ret;
errout_timeout:
    urb->timeout = 0;
    usbh_kill_urb(urb);
    return ret;
errout_submit_init:
    if (pipe->urb == urb) {
        pipe->urb = NULL;
    }
    pipe->inuse = false;
    urb->hcpriv = NULL;
    urb->errorcode = 0;
    usb_osal_leave_critical_section(flags);
    return ret;
}

int usbh_kill_urb(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;
    struct usbh_bus *bus;
    usbh_complete_callback_t complete;
    void *complete_arg;
    int complete_status;
    size_t flags;

    if (!urb || !urb->hcpriv || !urb->hport->bus) {
        return -USB_ERR_INVAL;
    }

    bus = urb->hport->bus;

    ARG_UNUSED(bus);

    flags = usb_osal_enter_critical_section();

    pipe = (struct musb_pipe *)urb->hcpriv;
    urb->errorcode = -USB_ERR_SHUTDOWN;

#if CONFIG_USB_RISCV_BRIDGE
    if (bk_v16_bridge_active()) {
        bk_v16_bridge_kill_xfer(urb);
    }
#endif

    if (urb->ep->bEndpointAddress & 0x80) {
        HWREGH(USB_BASE + MUSB_RXIE_OFFSET) &= ~(1 << (urb->ep->bEndpointAddress & 0x0f));
        HWREGH(USB_BASE + MUSB_RXIS_OFFSET) = (1 << (urb->ep->bEndpointAddress & 0x0f));
    } else {
        HWREGH(USB_BASE + MUSB_TXIE_OFFSET) &= ~(1 << (urb->ep->bEndpointAddress & 0x0f));
        HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = (1 << (urb->ep->bEndpointAddress & 0x0f));
    }

    musb_fifo_flush(bus, urb->ep->bEndpointAddress);

    if (urb->timeout) {
        usb_osal_sem_give(pipe->waitsem);
    } else {
        if (pipe->urb) {
            pipe->urb->hcpriv = NULL;
            pipe->urb = NULL;
        }
        pipe->inuse = false;
    }

    complete = urb->complete;
    complete_arg = urb->arg;
    complete_status = urb->errorcode;

    usb_osal_leave_critical_section(flags);
    if (complete) {
        complete(complete_arg, complete_status);
    }
    return 0;
}

static void musb_urb_waitup(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;

    pipe = (struct musb_pipe *)urb->hcpriv;

    /* The URB may have been torn down (hcpriv cleared, complete already invoked)
     * between the CP queuing this completion and the AP draining it. Drop it
     * safely rather than dereferencing a NULL pipe. */
    if (pipe == NULL) {
        return;
    }

    if (urb->timeout) {
        usb_osal_sem_give(pipe->waitsem);
    } else {
        musb_pipe_free(pipe);
    }

    if (urb->complete) {
        if (urb->errorcode < 0) {
            urb->complete(urb->arg, urb->errorcode);
        } else {
            urb->complete(urb->arg, urb->actual_length);
        }
    }
}

#if CONFIG_USB_RISCV_BRIDGE
/* ===================================================================
 * RISC-V CP USB-host bridge for CherryUSB v1.6
 *
 * The low-level MUSB host engine (HS chirp / reset timing, EP0 state
 * machine, FIFO read/write) runs on the RISC-V CP firmware -- exactly the
 * proven legacy path. The AP keeps running the v1.6 host stack and only:
 *   - publishes a bridge-layout shadow HCD + EP0 state the CP drives in place
 *   - kicks the first packet of each transfer (same register writes as the
 *     AP-direct port: musb_*_urb_init)
 *   - receives CP completions over IPI and wakes the v1.6 urb waiter.
 *
 * The CP firmware reads a FIXED struct layout (riscv_src/fw/common/
 * riscv_usb_bridge.h). CherryUSB v1.6's struct usbh_urb is NOT that layout,
 * so each in-flight transfer carries a byte-compatible *shadow* urb whose
 * setup/transfer_buffer point at the real v1.6 buffers; on completion the
 * CP-written actual_length/errorcode are copied back to the v1.6 urb.
 * =================================================================== */

#define BK_V16_EVT_NONE       0U
#define BK_V16_EVT_CONNECT    1U
#define BK_V16_EVT_DISCONNECT 2U
#define BK_V16_EVT_EP0_DONE   3U
#define BK_V16_EVT_PIPE_TX    4U
#define BK_V16_EVT_PIPE_RX    5U
#define BK_V16_EVT_ISR_DRAIN  6U

/* --- byte-for-byte mirrors of the CP firmware bridge structs --- */
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} bk_v16_b_setup_t;

typedef struct {
    uint8_t *transfer_buffer;
    uint32_t transfer_buffer_length;
    uint32_t actual_length;
    int errorcode;
} bk_v16_b_iso_t;

typedef struct {
    uint32_t pipe;
    bk_v16_b_setup_t *setup;
    uint8_t *transfer_buffer;
    uint32_t transfer_buffer_length;
    int transfer_flags;
    uint32_t actual_length;
    uint32_t timeout;
    int errorcode;
    uint32_t num_of_iso_packets;
    void *complete;
    void *arg;
    bk_v16_b_iso_t iso_packet[0];
} bk_v16_b_urb_t;

typedef struct {
    uint8_t dev_addr;
    uint8_t ep_addr;
    uint8_t ep_type;
    uint8_t ep_interval;
    uint8_t speed;
    uint16_t ep_mps;
    uint16_t ep_local_index;
    uint8_t inuse;
    uint32_t xfrd;
    uint8_t waiter;
    void *waitsem;
    void *hport;
    bk_v16_b_urb_t *urb;
    uint32_t iso_frame_idx;
} bk_v16_b_pipe_t;

typedef struct {
    uint8_t port_csc;
    uint8_t port_pec;
    uint8_t port_pe;
    uint8_t ep_local_index_record;
    uint32_t fifo_size_offset;
    bk_v16_b_pipe_t pipe_pool[CONFIG_USB_MUSB_PIPE_NUM][2];
} bk_v16_b_hcd_t;

/* Shared with the CP via probe ctx (g_musb_hcd_addr / usb_ep0_state_addr). */
static bk_v16_b_hcd_t s_bridge_hcd;
volatile uint8_t usb_ep0_state = USB_EP0_STATE_SETUP;

/* Max ISO microframe packets per URB the bridge can forward to the RISC-V CP.
 * Matches the UVC URB pool layout (UVC_NUM_PACKET_PER_URB, default 8). The CP
 * firmware walks shadow_urb.urb.iso_packet[0..num_of_iso_packets-1]; the trailing
 * iso[] array below is what backs that flexible-array member. */
#ifndef BK_V16_BRIDGE_MAX_ISO_PACKETS
#define BK_V16_BRIDGE_MAX_ISO_PACKETS 8
#endif

/* The CP firmware reads bk_v16_b_urb_t.iso_packet[i] (a flexible array). A bare
 * bk_v16_b_urb_t has no storage for it, so back it with a contiguous iso[] that
 * immediately follows the header -- iso_packet[i] then aliases iso[i]. */
typedef struct {
    bk_v16_b_urb_t urb;
    bk_v16_b_iso_t iso[BK_V16_BRIDGE_MAX_ISO_PACKETS];
} bk_v16_shadow_urb_t;

/* AP-private side tables (never read by the CP). */
static bk_v16_shadow_urb_t s_bridge_urb[CONFIG_USB_MUSB_PIPE_NUM][2];
static struct usbh_urb *s_v16_urb_map[CONFIG_USB_MUSB_PIPE_NUM][2];
static struct usbh_bus *s_bridge_bus;

/* RISC-V->M55 completions are delivered through the SPSC event ring in the
 * shared probe (evt_ring/evt_wr/evt_rd). An edge-triggered IPI has no handshake,
 * so two completions raised close together can coalesce into a single AP ISR;
 * because every completion is its own ring entry the AP simply drains
 * evt_rd..evt_wr and never loses one. s_bridge_poll_timer is a low-rate,
 * IPI-independent reconciliation that re-runs poll_events to drain any backlog
 * left by a coalesced/dropped IPI within one poll interval. s_bridge_poll_busy
 * serialises the IPI path and the timer so the ring has a single consumer. */
static volatile uint32_t s_bridge_poll_busy;
static volatile uint32_t s_bridge_poll_resched;
#define BK_V16_BRIDGE_POLL_MS 10
static beken_timer_t s_bridge_poll_timer;
static volatile uint8_t s_bridge_poll_timer_on;

static bool bk_v16_bridge_active(void)
{
    volatile riscv_usb_probe_t *ctx = get_riscv_usb_probe();

    return (ctx->magic == RISCV_USB_PROBE_MAGIC) &&
           (ctx->owner == RISCV_USB_PROBE_OWNER_RISCV);
}

/* Populate the shadow pipe + urb the CP firmware drives. Called by the AP just
 * before it kicks the first packet, while bridge mode is active. */
static int bk_v16_bridge_arm_xfer(uint8_t chidx, uint8_t dir, struct usbh_urb *urb)
{
    bk_v16_b_pipe_t *bp = &s_bridge_hcd.pipe_pool[chidx][dir];
    bk_v16_b_urb_t *bu = &s_bridge_urb[chidx][dir].urb;

    bk_v16_epidx_hspl_lock();

    bu->setup = (bk_v16_b_setup_t *)urb->setup;
    bu->transfer_buffer = urb->transfer_buffer;
    bu->transfer_buffer_length = urb->transfer_buffer_length;
    bu->actual_length = 0;
    bu->errorcode = -USB_ERR_BUSY;
    bu->num_of_iso_packets = 0;
    bu->timeout = urb->timeout;

    /* ISO: mirror each microframe packet descriptor into the shadow urb so the
     * CP firmware can scatter received data straight into the app buffers. The
     * transfer_buffer pointers are shared (no copy) -- the CP writes into them
     * directly; only actual_length/errorcode are copied back on completion. */
    if (urb->num_of_iso_packets > 0) {
        uint32_t npk = urb->num_of_iso_packets;

        if (npk > BK_V16_BRIDGE_MAX_ISO_PACKETS) {
            npk = BK_V16_BRIDGE_MAX_ISO_PACKETS;
        }
        bu->num_of_iso_packets = npk;
        for (uint32_t i = 0; i < npk; i++) {
            bu->iso_packet[i].transfer_buffer = urb->iso_packet[i].transfer_buffer;
            bu->iso_packet[i].transfer_buffer_length = urb->iso_packet[i].transfer_buffer_length;
            bu->iso_packet[i].actual_length = 0;
            bu->iso_packet[i].errorcode = 0;
        }
    }

    bp->dev_addr = urb->hport->dev_addr;
    bp->ep_addr = urb->ep->bEndpointAddress;
    bp->ep_type = USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes);
    bp->ep_interval = urb->ep->bInterval;
    bp->speed = urb->hport->speed;
    bp->ep_mps = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
    bp->ep_local_index = chidx;
    bp->inuse = 1;
    bp->xfrd = 0;
    bp->iso_frame_idx = 0;
    bp->hport = urb->hport;
    bp->urb = bu;

    s_v16_urb_map[chidx][dir] = urb;
    /* The shadow stores above must be globally visible before the caller kicks
     * the first packet (musb_*_urb_init), or the CP ISR could read a stale
     * shadow. That ordering is provided by bk_v16_epidx_hspl_unlock() ->
     * bk_hspl_res_unlock(), which issues a dsb before releasing the channel; keep
     * a barrier here if that lock path ever changes. */
    bk_v16_epidx_hspl_unlock();

    return 0;
}

static void bk_v16_bridge_kill_xfer(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;
    bk_v16_b_pipe_t *bp;
    uint8_t chidx;
    uint8_t dir;

    if (urb == NULL || urb->hcpriv == NULL || urb->ep == NULL) {
        return;
    }

    pipe = (struct musb_pipe *)urb->hcpriv;
    chidx = pipe->chidx;
    dir = (urb->ep->bEndpointAddress & 0x80) ? 1 : 0;
    if (chidx >= CONFIG_USB_MUSB_PIPE_NUM) {
        return;
    }

    bk_v16_epidx_hspl_lock();
    bp = &s_bridge_hcd.pipe_pool[chidx][dir];
    if (s_v16_urb_map[chidx][dir] == urb) {
        s_v16_urb_map[chidx][dir] = NULL;
    }
    if (bp->urb == &s_bridge_urb[chidx][dir].urb) {
        bp->urb->errorcode = -USB_ERR_SHUTDOWN;
        bp->urb = NULL;
    }
    bp->inuse = 0;
    bp->xfrd = 0;
    bp->iso_frame_idx = 0;
    bk_v16_epidx_hspl_unlock();
}

int bk_usbh_soft_abort_urb(struct usbh_urb *urb)
{
    struct musb_pipe *pipe;
    size_t flags;

    if (!urb || !urb->hcpriv) {
        return -USB_ERR_INVAL;
    }

    flags = usb_osal_enter_critical_section();
    pipe = (struct musb_pipe *)urb->hcpriv;
    urb->errorcode = -USB_ERR_SHUTDOWN;

#if CONFIG_USB_RISCV_BRIDGE
    if (bk_v16_bridge_active()) {
        bk_v16_bridge_kill_xfer(urb);
    }
#endif

    if (pipe->urb == urb) {
        pipe->urb = NULL;
    }
    pipe->inuse = false;
    urb->hcpriv = NULL;
    usb_osal_leave_critical_section(flags);

    return 0;
}

static void bk_v16_bridge_complete_one(uint32_t event, uint32_t ep)
{
    uint32_t dir;
    bk_v16_b_pipe_t *bp;
    bk_v16_b_urb_t *bu;
    struct usbh_urb *v16;

    if (event == BK_V16_EVT_EP0_DONE) {
        ep = 0;
        dir = 0;
    } else if (event == BK_V16_EVT_PIPE_TX) {
        dir = 0;
    } else {
        dir = 1;
    }

    if (ep >= CONFIG_USB_MUSB_PIPE_NUM) {
        return;
    }

    /* Serialise the shadow-map read+clear against arm_xfer/kill_xfer and any
     * concurrent drain. The completion drain runs from BOTH the IPI handler and
     * the poll timer, which can execute on different AP cores; the USB OSAL
     * critical section on this port is a no-op, so without this lock two drains
     * can each observe the same urb and double re-submit it -- leaking a pipe
     * and corrupting urb/shadow state until the ISO stream wedges. The
     * completion callback (which re-arms and itself takes the HSPL) is invoked
     * AFTER releasing the lock, so the HSPL is never re-acquired re-entrantly. */
    bk_v16_epidx_hspl_lock();
    v16 = s_v16_urb_map[ep][dir];
    if (v16 == NULL) {
        bk_v16_epidx_hspl_unlock();
        return;
    }

    bp = &s_bridge_hcd.pipe_pool[ep][dir];
    bu = bp->urb;
    if (bu != NULL) {
        v16->actual_length = bu->actual_length;
        v16->errorcode = bu->errorcode;

        /* ISO: copy per-packet results back so the app sees each microframe's
         * actual_length (data already landed in the shared buffers). */
        if (bu->num_of_iso_packets > 0 && v16->num_of_iso_packets > 0) {
            uint32_t npk = bu->num_of_iso_packets;

            if (npk > v16->num_of_iso_packets) {
                npk = v16->num_of_iso_packets;
            }
            for (uint32_t i = 0; i < npk; i++) {
                v16->iso_packet[i].actual_length = bu->iso_packet[i].actual_length;
                v16->iso_packet[i].errorcode = bu->iso_packet[i].errorcode;
            }
        }
    }

    s_v16_urb_map[ep][dir] = NULL;
    bp->urb = NULL;
    bp->inuse = 0;
    bk_v16_epidx_hspl_unlock();

    musb_urb_waitup(v16);
}

/* Re-assert the USB-HS interrupt route to the CP (bit8 = 1). Plain RMW: this
 * runs in the IPI/poll recovery path and must NOT block on the EPIDX HSPL, or a
 * held lock would stall the very mechanism that heals a stranded completion. */
static void bk_v16_bridge_route_enable_to_cp(void)
{
    /* Publish all prior AP writes to the shared probe (owner, evt_rd, shadow
     * clears) before the CP is (re)routed the USB IRQ: the probe is Normal
     * non-cacheable and the route enable is a Device write, so without this the
     * Device write could be observed first and the CP, once routed, could act on
     * a stale owner/evt_rd. */
    //__sync_synchronize();
    __asm volatile ( "dsb" ::: "memory" );

    uint32_t cfg = sys_drv_get_ints_config_riscv_0_31();
    cfg |= (1U << 8);
    sys_drv_set_ints_config_riscv_0_31(cfg);
}

static void bk_v16_bridge_poll_events(void)
{
    volatile riscv_usb_probe_t *ctx = get_riscv_usb_probe();
    struct usbh_bus *bus = s_bridge_bus;
    uint32_t rd;
    uint32_t wr;

    if (!bk_v16_bridge_active() || bus == NULL) {
        return;
    }

    /* Single-consumer guard for the SPSC ring: the IPI callback and the low-rate
     * poll timer can both call this (and from different AP cores), but only one
     * context may advance evt_rd at a time. A contending caller sets resched and
     * the active drainer loops again before returning. */
    {
        uint32_t f = rtos_disable_int();
        if (s_bridge_poll_busy) {
            s_bridge_poll_resched = 1;
            rtos_enable_int(f);
            return;
        }
        s_bridge_poll_busy = 1;
        rtos_enable_int(f);
    }

    /* Hand the IRQ route back to the CP up front so it keeps enqueuing while we
     * drain; the ring fully decouples CP production from AP consumption. */
    bk_v16_bridge_route_enable_to_cp();

    /* Drain queued completions evt_rd..evt_wr. Each completion is its own ring
     * entry, so a coalesced or dropped RISC-V->M55 IPI cannot lose one: the next
     * poll (IPI or the low-rate timer) still sees evt_rd != evt_wr and finishes
     * the backlog.
     *
     * Snapshot evt_wr ONCE and only drain up to it. This runs in IPI IRQ context;
     * chasing a live evt_wr lets a high ISO completion rate (UVC multi-packet +
     * UAC mic/spk) enqueue faster than complete_one (which contends the EPIDX
     * HSPL with the RISC-V) can drain, so the loop would never exit -> the IPI
     * ISR never returns -> the AP core hangs -> CP heartbeat timeout (the
     * observed doorbell regression). Entries produced during this pass are taken
     * by the next IPI (the producer raises one per ISR) or the poll timer; the
     * ring still loses nothing. */
    wr = ctx->evt_wr;
    rd = ctx->evt_rd;
    while (rd != wr) {
        uint32_t e;
        uint32_t type;
        uint32_t ep;

        e = ctx->evt_ring[rd & RISCV_USB_EVT_RING_MASK];
        rd = rd + 1U;
        ctx->evt_rd = rd;       /* release the consumed slot back to the producer */

        type = RISCV_USB_EVT_TYPE(e);
        ep = RISCV_USB_EVT_EP(e);

        switch (type) {
            case RISCV_USB_EVT_TYPE_CONN:
                HWREGB(BK_USB_PHY_BASE(bus) + BK_NANENG_PHY_FC_REG0C) = 0xE0;
                HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
                g_musb_hcd[bus->hcd.hcd_id].port_csc = 1;
                g_musb_hcd[bus->hcd.hcd_id].port_pec = 1;
                g_musb_hcd[bus->hcd.hcd_id].port_pe = 1;
                bus->hcd.roothub.int_buffer[0] = (1 << 1);
                usbh_hub_thread_wakeup(&bus->hcd.roothub);
                break;
            case RISCV_USB_EVT_TYPE_DISC:
                g_musb_hcd[bus->hcd.hcd_id].port_csc = 1;
                g_musb_hcd[bus->hcd.hcd_id].port_pec = 1;
                g_musb_hcd[bus->hcd.hcd_id].port_pe = 0;
                bus->hcd.roothub.int_buffer[0] = (1 << 1);
                usbh_hub_thread_wakeup(&bus->hcd.roothub);
                break;
            case RISCV_USB_EVT_TYPE_EP0:
                bk_v16_bridge_complete_one(BK_V16_EVT_EP0_DONE, 0);
                break;
            case RISCV_USB_EVT_TYPE_TX:
                bk_v16_bridge_complete_one(BK_V16_EVT_PIPE_TX, ep);
                break;
            case RISCV_USB_EVT_TYPE_RX:
                bk_v16_bridge_complete_one(BK_V16_EVT_PIPE_RX, ep);
                break;
            default:
                break;
        }
    }

    /* hand the USB HS IRQ back to the RISC-V CP (bit8 = 1). */
    bk_v16_bridge_route_enable_to_cp();

    s_bridge_poll_busy = 0;
    if (s_bridge_poll_resched) {
        s_bridge_poll_resched = 0;
        bk_v16_bridge_poll_events();
    }
}

/* Low-rate, IPI-independent reconciliation. poll_events is a cheap seq compare
 * when IPIs flow normally; if the CP dropped a completion-IPI edge this drains
 * the stranded event and re-enables the CP route within BK_V16_BRIDGE_POLL_MS. */
static void bk_v16_bridge_poll_timer_cb(void *arg)
{
    (void)arg;
    bk_v16_bridge_poll_events();

    /* Surface EPIDX-HSPL steals (peer AP core / RISC-V CP wedged while holding the
     * lock). Zero in healthy operation; a rising count means the bounded-spin steal
     * in bk_v16_epidx_hspl_lock() saved the AP from a hard hang, so the real wedge
     * is upstream. Printed at the low poll rate, only on change. */
    {
        static uint32_t s_last_steal;
        uint32_t now = s_v16_epidx_hspl_steal_cnt;

        if (now != s_last_steal) {
            s_last_steal = now;
            USB_LOG_WRN("[bk_v1_6] EPIDX hspl steal cnt=%u (peer/CP wedged)\r\n", now);
        }
    }
}

static void bk_v16_bridge_poll_timer_start(void)
{
    if (s_bridge_poll_timer_on) {
        return;
    }
    if (rtos_init_timer(&s_bridge_poll_timer, BK_V16_BRIDGE_POLL_MS,
                        bk_v16_bridge_poll_timer_cb, NULL) != BK_OK) {
        USB_LOG_ERR("[bk_v1_6] bridge poll timer init failed\r\n");
        return;
    }
    rtos_start_timer(&s_bridge_poll_timer);
    s_bridge_poll_timer_on = 1;
}

static void bk_v16_bridge_poll_timer_stop(void)
{
    if (!s_bridge_poll_timer_on) {
        return;
    }
    rtos_stop_timer(&s_bridge_poll_timer);
    rtos_deinit_timer(&s_bridge_poll_timer);
    s_bridge_poll_timer_on = 0;
}

#if CONFIG_IPI
#if CONFIG_USB_DEVICE
/* Implemented in the v1.6 device port (usb_dc_musb.c): drains the device-role
 * RISCV_USBD_EVT_* events. The shared IPI_DOMAIN_USB callback below dispatches
 * to it by probe->role so host and device reuse one IPI registration -- same
 * design as the legacy stack. */
extern void usb_dc_riscv_poll_events(void);
#endif

static void bk_v16_bridge_ipi_cb(ipi_core_id_t core_id, uint32_t value,
                                 uint8_t src_cpu, uint8_t event, uint16_t payload,
                                 void *param)
{
    (void)core_id;
    (void)value;
    (void)src_cpu;
    (void)event;
    (void)payload;
    (void)param;

#if CONFIG_USB_DEVICE
    if (get_riscv_usb_probe()->role == RISCV_USB_ROLE_DEVICE) {
        usb_dc_riscv_poll_events();
        return;
    }
#endif
    bk_v16_bridge_poll_events();
}

bk_err_t bk_v16_bridge_ipi_enable(void)
{
    bk_err_t ret;

    ret = bk_ipi_driver_init();
    if (ret != BK_OK) {
        return ret;
    }
    ret = bk_ipi_register_domain_callback(IPI_DOMAIN_USB, bk_v16_bridge_ipi_cb, NULL);
    if (ret != BK_OK) {
        return ret;
    }
    ret = bk_ipi_enable(IPI_AP_CORE0);
    if (ret != BK_OK) {
        return ret;
    }
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_IPI, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_IPI, 1);
#endif
    return BK_OK;
}
#endif /* CONFIG_IPI */

static void bk_v16_bridge_probe_init(uint32_t role)
{
    volatile riscv_usb_probe_t *ctx = get_riscv_usb_probe();

    memset(&s_bridge_hcd, 0, sizeof(s_bridge_hcd));
    memset(s_bridge_urb, 0, sizeof(s_bridge_urb));
    memset(s_v16_urb_map, 0, sizeof(s_v16_urb_map));
    s_bridge_poll_resched = 0;
    usb_ep0_state = USB_EP0_STATE_SETUP;

    ctx->magic = RISCV_USB_PROBE_MAGIC;
    ctx->owner = RISCV_USB_PROBE_OWNER_AP;
    ctx->irq_seq = 0;
    ctx->event = BK_V16_EVT_NONE;
    ctx->event_data = 0;
    /* SPSC event ring starts empty (producer cursor == consumer cursor). */
    ctx->evt_wr = 0;
    ctx->evt_rd = 0;
    ctx->evt_drop = 0;
    ctx->g_musb_hcd_addr = SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&s_bridge_hcd);
    ctx->usb_ep0_state_addr = SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&usb_ep0_state);
    ctx->pending_ep0 = 0;
    ctx->role = role;
    for (uint32_t i = 0U; i < (uint32_t)RISCV_USB_PROBE_PIPE_NUM; i++) {
        ctx->pending_pipe_tx[i] = 0;
        ctx->pending_pipe_rx[i] = 0;
    }

    /* The EPIDX window is now guarded by the HSPL hardware spin lock
     * (BK_HSPL_RES_USB -> HSPL_ID_1 channel RISCV_USB_HSPL_CHANNEL). Make sure
     * the HSPL block is clocked/initialised and the USB channel is released
     * before the CP (RISC-V) starts servicing IRQs, otherwise the CP could spin
     * forever on an un-clocked channel. bk_hspl_driver_init() is idempotent. */
    (void)bk_hspl_driver_init();
    (void)bk_hspl_unlock(BK_HSPL_ID_1, RISCV_USB_HSPL_CHANNEL);
}

/* Returns true when the RISC-V CP firmware took ownership of the USB IRQ and
 * the AP must NOT register its own USBH_IRQHandler. Returns false to fall back
 * to the AP-direct host path. */
bool bk_v16_bridge_bringup(struct usbh_bus *bus)
{
    s_bridge_bus = bus;
    bk_v16_bridge_probe_init(RISCV_USB_ROLE_HOST);

    if (usb_hc_riscv_host_prepare() != 0) {
        USB_LOG_WRN("[bk_v1_6] riscv host_prepare failed, fall back to AP-direct\r\n");
        return false;
    }

#if CONFIG_IPI
    if (bk_v16_bridge_ipi_enable() != BK_OK) {
        USB_LOG_ERR("[bk_v1_6] riscv IPI enable failed\r\n");
    }
#endif

    get_riscv_usb_probe()->owner = RISCV_USB_PROBE_OWNER_RISCV;

    /* route USB HS IRQ to the CP (bit8 = 1). */
    bk_v16_bridge_route_enable_to_cp();

    bk_v16_bridge_poll_timer_start();

    USB_LOG_INFO("[bk_v1_6] usb host use riscv CP bridge path (evt-ring bounded-drain)\r\n");
    return true;
}

void bk_v16_bridge_teardown(void)
{
    bk_v16_bridge_poll_timer_stop();
    if (bk_v16_bridge_active()) {
        get_riscv_usb_probe()->owner = RISCV_USB_PROBE_OWNER_NONE;
    }
    usb_hc_riscv_stop_firmware();
    s_bridge_bus = NULL;
}
#endif /* CONFIG_USB_RISCV_BRIDGE */

void handle_ep0(struct usbh_bus *bus)
{
    uint8_t ep_idx = 0;
    uint8_t ep0_status;
    uint8_t old_ep_idx;
    struct musb_pipe *pipe;
    struct usbh_urb *urb;
    uint32_t size;

    pipe = (struct musb_pipe *)&g_musb_hcd[bus->hcd.hcd_id].pipe_pool[0];
    urb = pipe->urb;
    if (urb == NULL) {
        return;
    }

    (void)ep_idx;
    old_ep_idx = musb_get_active_ep(bus);
    musb_set_active_ep(bus, 0);
    ep0_status = HWREGB(USB_TXCSRL_BASE(ep_idx));
    if (ep0_status & USB_CSRL0_STALLED) {
        HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_CSRL0_STALLED;
        pipe->ep0_state = USB_EP0_STATE_SETUP;
        urb->errorcode = -USB_ERR_STALL;
        musb_urb_waitup(urb);
        musb_set_active_ep(bus, old_ep_idx);
        return;
    }
    if (ep0_status & USB_CSRL0_ERROR) {
        HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_CSRL0_ERROR;
        musb_fifo_flush(bus, 0);
        pipe->ep0_state = USB_EP0_STATE_SETUP;
        urb->errorcode = -USB_ERR_IO;
        musb_urb_waitup(urb);
        musb_set_active_ep(bus, old_ep_idx);
        return;
    }

    switch (pipe->ep0_state) {
        case USB_EP0_STATE_SETUP:
            urb->actual_length += 8;
            if (urb->transfer_buffer_length) {
                if (urb->setup->bmRequestType & 0x80) {
                    pipe->ep0_state = USB_EP0_STATE_IN_DATA;
                    HWREGB(USB_TXCSRL_BASE(ep_idx)) = USB_CSRL0_REQPKT;
                } else {
                    pipe->ep0_state = USB_EP0_STATE_OUT_DATA;
                    size = urb->transfer_buffer_length;
                    if (size > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
                        size = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
                    }

                    musb_write_packet(bus, 0, urb->transfer_buffer, size);
                    HWREGB(USB_TXCSRL_BASE(ep_idx)) = USB_CSRL0_TXRDY;

                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;
                }
            } else {
                pipe->ep0_state = USB_EP0_STATE_IN_STATUS;
                HWREGB(USB_TXCSRL_BASE(ep_idx)) = (USB_CSRL0_REQPKT | USB_CSRL0_STATUS);
            }
            break;
        case USB_EP0_STATE_IN_DATA:
            if (ep0_status & USB_CSRL0_RXRDY) {
                size = urb->transfer_buffer_length;
                if (size > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
                    size = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
                }
                if (size > HWREGH(USB_RXCOUNT_BASE(ep_idx))) {
                    size = HWREGH(USB_RXCOUNT_BASE(ep_idx));
                }
                musb_read_packet(bus, 0, urb->transfer_buffer, size);
                HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_CSRL0_RXRDY;
                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;

                if ((size < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) || (urb->transfer_buffer_length == 0)) {
                    pipe->ep0_state = USB_EP0_STATE_OUT_STATUS;
                    HWREGB(USB_TXCSRL_BASE(ep_idx)) = (USB_CSRL0_TXRDY | USB_CSRL0_STATUS);
                } else {
                    HWREGB(USB_TXCSRL_BASE(ep_idx)) = USB_CSRL0_REQPKT;
                }
            }
            break;
        case USB_EP0_STATE_OUT_DATA:
            if (urb->transfer_buffer_length > 0) {
                size = urb->transfer_buffer_length;
                if (size > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
                    size = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
                }

                musb_write_packet(bus, 0, urb->transfer_buffer, size);
                HWREGB(USB_TXCSRL_BASE(ep_idx)) = USB_CSRL0_TXRDY;

                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;
            } else {
                pipe->ep0_state = USB_EP0_STATE_IN_STATUS;
                HWREGB(USB_TXCSRL_BASE(ep_idx)) = (USB_CSRL0_REQPKT | USB_CSRL0_STATUS);
            }
            break;
        case USB_EP0_STATE_OUT_STATUS:
            urb->errorcode = 0;
            musb_urb_waitup(urb);
            break;
        case USB_EP0_STATE_IN_STATUS:
            if (ep0_status & (USB_CSRL0_RXRDY | USB_CSRL0_STATUS)) {
                HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~(USB_CSRL0_RXRDY | USB_CSRL0_STATUS);
                urb->errorcode = 0;
                musb_urb_waitup(urb);
            }
            break;
    }
    musb_set_active_ep(bus, old_ep_idx);
}

void USBH_IRQHandler(uint8_t busid)
{
    uint32_t is;
    uint32_t txis;
    uint32_t rxis;
    uint8_t ep_csrl_status;
    // uint8_t ep_csrh_status;
    struct musb_pipe *pipe;
    struct usbh_urb *urb;
    uint8_t ep_idx;
    uint8_t old_ep_idx;
    struct usbh_bus *bus;
    uint32_t size;

    bus = &g_usbhost_bus[busid];

#if 0
    if (!(HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) & USB_DEVCTL_HOST)) {
        return;
    }
#endif

    is = HWREGB(USB_BASE + MUSB_IS_OFFSET);
    txis = HWREGH(USB_BASE + MUSB_TXIS_OFFSET);
    rxis = HWREGH(USB_BASE + MUSB_RXIS_OFFSET);

    HWREGB(USB_BASE + MUSB_IS_OFFSET) = is;

    old_ep_idx = musb_get_active_ep(bus);

    if (is & USB_IS_CONN) {
#if CONFIG_BK_USB_CHERRYUSB_V1_6
        HWREGB(BK_USB_PHY_BASE(bus) + BK_NANENG_PHY_FC_REG0C) = 0xE0;
        HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
#endif
        g_musb_hcd[bus->hcd.hcd_id].port_csc = 1;
        g_musb_hcd[bus->hcd.hcd_id].port_pec = 1;
        g_musb_hcd[bus->hcd.hcd_id].port_pe = 1;
        bus->hcd.roothub.int_buffer[0] = (1 << 1);
        usbh_hub_thread_wakeup(&bus->hcd.roothub);
    }

    if (is & USB_IS_DISCON) {
        g_musb_hcd[bus->hcd.hcd_id].port_csc = 1;
        g_musb_hcd[bus->hcd.hcd_id].port_pec = 1;
        g_musb_hcd[bus->hcd.hcd_id].port_pe = 0;
        bus->hcd.roothub.int_buffer[0] = (1 << 1);
        usbh_hub_thread_wakeup(&bus->hcd.roothub);
    }

    if (is & USB_IS_SOF) {
    }

    if (is & USB_IS_RESUME) {
    }

    if (is & USB_IS_SUSPEND) {
    }

    if (is & USB_IS_VBUSERR) {
    }

    if (is & USB_IS_SESREQ) {
    }

    if (is & USB_IS_BABBLE) {
    }

    txis &= HWREGH(USB_BASE + MUSB_TXIE_OFFSET);
    /* Handle EP0 interrupt */
    if (txis & USB_TXIE_EP0) {
        txis &= ~USB_TXIE_EP0;
        HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = USB_TXIE_EP0;
        handle_ep0(bus);
    }

    for (ep_idx = 1; ep_idx < CONFIG_USB_MUSB_PIPE_NUM; ep_idx++) {
        if (txis & (1 << ep_idx)) {
            HWREGH(USB_BASE + MUSB_TXIS_OFFSET) = (1 << ep_idx);

            pipe = &g_musb_hcd[bus->hcd.hcd_id].pipe_pool[ep_idx];
            urb = pipe->urb;
            musb_set_active_ep(bus, ep_idx);

            ep_csrl_status = HWREGB(USB_TXCSRL_BASE(ep_idx));

            if (ep_csrl_status & USB_TXCSRL1_ERROR) {
                HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_TXCSRL1_ERROR;
                urb->errorcode = -USB_ERR_IO;
                musb_urb_waitup(urb);
            } else if (ep_csrl_status & USB_TXCSRL1_NAKTO) {
                HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_TXCSRL1_NAKTO;
                urb->errorcode = -USB_ERR_NAK;
                musb_urb_waitup(urb);
            } else if (ep_csrl_status & USB_TXCSRL1_STALLED) {
                HWREGB(USB_TXCSRL_BASE(ep_idx)) &= ~USB_TXCSRL1_STALLED;
                urb->errorcode = -USB_ERR_STALL;
                musb_urb_waitup(urb);
            } else {
                if (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) != USB_ENDPOINT_TYPE_ISOCHRONOUS) {
                    uint32_t size = urb->transfer_buffer_length;

                    if (size > USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) {
                        size = USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize);
                    }

                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;

                    if (urb->transfer_buffer_length == 0) {
                        //HWREGH(USB_BASE + MUSB_TXIE_OFFSET) &= ~(1 << ep_idx);
                        urb->errorcode = 0;
                        musb_urb_waitup(urb);
                    } else {
                        musb_write_packet(bus, ep_idx, urb->transfer_buffer, MIN(urb->transfer_buffer_length, USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)));
                        HWREGB(USB_TXCSRL_BASE(ep_idx)) = USB_TXCSRL1_TXRDY;
                    }
                }
            }
        }
    }

    rxis &= HWREGH(USB_BASE + MUSB_RXIE_OFFSET);
    for (ep_idx = 1; ep_idx < CONFIG_USB_MUSB_PIPE_NUM; ep_idx++) {
        if (rxis & (1 << ep_idx)) {
            HWREGH(USB_BASE + MUSB_RXIS_OFFSET) = (1 << ep_idx); // clear isr flag

            pipe = &g_musb_hcd[bus->hcd.hcd_id].pipe_pool[ep_idx];
            urb = pipe->urb;
            musb_set_active_ep(bus, ep_idx);

            ep_csrl_status = HWREGB(USB_RXCSRL_BASE(ep_idx));
            //ep_csrh_status = HWREGB(USB_BASE + USB_RXCSRH_BASE(ep_idx)); // todo:for iso transfer

            if (ep_csrl_status & USB_RXCSRL1_ERROR) {
                HWREGB(USB_RXCSRL_BASE(ep_idx)) &= ~USB_RXCSRL1_ERROR;
                urb->errorcode = -USB_ERR_IO;
                musb_urb_waitup(urb);
            } else if (ep_csrl_status & USB_RXCSRL1_NAKTO) {
                HWREGB(USB_RXCSRL_BASE(ep_idx)) &= ~USB_RXCSRL1_NAKTO;
                urb->errorcode = -USB_ERR_NAK;
                musb_urb_waitup(urb);
            } else if (ep_csrl_status & USB_RXCSRL1_STALLED) {
                HWREGB(USB_RXCSRL_BASE(ep_idx)) &= ~USB_RXCSRL1_STALLED;
                urb->errorcode = -USB_ERR_STALL;
                musb_urb_waitup(urb);
            } else if (ep_csrl_status & USB_RXCSRL1_RXRDY) {
                if (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) != USB_ENDPOINT_TYPE_ISOCHRONOUS) {
                    size = HWREGH(USB_RXCOUNT_BASE(ep_idx));

                    musb_read_packet(bus, ep_idx, urb->transfer_buffer, size);

                    HWREGB(USB_RXCSRL_BASE(ep_idx)) &= ~USB_RXCSRL1_RXRDY;

                    urb->transfer_buffer += size;
                    urb->transfer_buffer_length -= size;
                    urb->actual_length += size;

                    if ((size < USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize)) || (urb->transfer_buffer_length == 0)) {
                        //HWREGH(USB_BASE + MUSB_RXIE_OFFSET) &= ~(1 << ep_idx);
                        urb->errorcode = 0;
                        musb_urb_waitup(urb);
                    } else {
                        HWREGB(USB_RXCSRL_BASE(ep_idx)) = USB_RXCSRL1_REQPKT;
                    }
                }
            }
        }
    }
    musb_set_active_ep(bus, old_ep_idx);
}
