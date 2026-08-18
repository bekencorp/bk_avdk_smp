/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * BK7259 MUSB-MHDRC device controller port, native CherryUSB v1.6 (busid) API.
 *
 * The controller entry points (usb_dc_init / usbd_ep_open / ...) and the core
 * upcalls (usbd_event_*_handler) all take a leading busid. This SoC is a single
 * device bus, so bus 0 is used throughout. The SoC interrupt line
 * (INT_SRC_USB_HS) drives a void() ISR, so it is bridged to USBD_IRQHandler(0)
 * via USBD_IRQHandler_Compat() below -- mirroring the host-side
 * USBH_IRQHandler_Compat in usb_hc_beken_musb.c.
 */
#include "usbd_core.h"
#include "usb_beken_musb_reg.h"
#include "sys_driver.h"
#include <driver/int.h>
#include "bk_misc.h"

/* usb_mode selector for bk_analog_layer_usb_sys_related_ops(): 0 = host,
 * 1 = device. Defined locally instead of pulling <components/usb.h>, which
 * drags in the public cherryusb host chain (usb_types.h -> usbh_core.h ->
 * usb_mem.h) and collides with the tree usb_util.h USB_MEM_ALIGNX definition.
 * bk_mtp/bk_usbd_mtp.c uses the same local-constant approach. */
#ifndef USB_DEVICE_MODE
#define USB_DEVICE_MODE 1
#endif

#include "riscv_bridge/riscv_usb_bridge.h"
#include "riscv_bridge/riscv_usb_probe_defs.h"

#define HWREG(x) \
    (*((volatile uint32_t *)(x)))
#define HWREGH(x) \
    (*((volatile uint16_t *)(x)))
#define HWREGB(x) \
    (*((volatile uint8_t *)(x)))

#ifndef USB_BASE
#define USB_BASE (SOC_USB_HS_BASE)
#endif

#define REG_USB_BASE_ADDR              USB_BASE

#define REG_USB_USR_700                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x700)))
#define REG_USB_USR_704                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x704)))
#define REG_USB_USR_708                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x708)))
#define REG_USB_USR_70C                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x70C)))
#define REG_USB_USR_710                (*((volatile unsigned long *)   (REG_USB_BASE_ADDR + 0x710)))

/* 00h-0Fh Common USB registers */
#define MUSB_FADDR_OFFSET    0x00
#define MUSB_POWER_OFFSET    0x01
#define MUSB_INTRTX_OFFSET   0x02
#define MUSB_INTRRX_OFFSET   0x04
#define MUSB_INTRTXE_OFFSET  0x06
#define MUSB_INTRRXE_OFFSET  0x08
#define MUSB_INTRUSB_OFFSET  0x0A
#define MUSB_INTRUSBE_OFFSET 0x0B
#define MUSB_FRAME_OFFSET    0x0C
#define MUSB_INDEX_OFFSET    0x0E
#define MUSB_TESTMODE_OFFSET 0x0F

/* 10h-1Fh Host/Peripheral mode */
#define MUSB_IND_TXMAXP_OFFSET   0x10
#define MUSB_IND_TXCSRL_OFFSET   0x12
#define MUSB_IND_TXCSRH_OFFSET   0x13
#define MUSB_IND_RXMAXP_OFFSET   0x14
#define MUSB_IND_RXCSRL_OFFSET   0x16
#define MUSB_IND_RXCSRH_OFFSET   0x17
#define MUSB_IND_RXCOUNT_OFFSET  0x18

/* 20h-5Fh EP0-15 FIFOs */
#define MUSB_FIFO_OFFSET 0x20
#define USB_FIFO_BASE(ep_idx) (USB_BASE + MUSB_FIFO_OFFSET + 0x4 * ep_idx)

/*
 * Report the negotiated device port speed.
 *
 * Every other CherryUSB v1.6 device port implements usbd_get_port_speed(), but
 * the beken MUSB-MHDRC port was missing it, leaving usbd_core.c
 * (usbd_setup_request_handler, USB_DESCRIPTOR_TYPE_DEVICE case) with an
 * undefined reference. Read the MUSB POWER HSMODE bit: after reset/HS chirp the
 * controller latches HSMODE when it enumerated at high speed (BK7259 forces
 * USB_POWER_HSENAB via CONFIG_USB_HS); otherwise it is running full speed.
 */
uint8_t usbd_get_port_speed(uint8_t busid)
{
    (void)busid;

    if (HWREGB(USB_BASE + MUSB_POWER_OFFSET) & USB_POWER_HSMODE)
    {
        return USB_SPEED_HIGH;
    }

    return USB_SPEED_FULL;
}

/* 60h-7Fh Additional Control & Configuration Registers */
#define MUSB_DEVCTL_OFFSET     0x60
#define MUSB_MISC_OFFSET       0x61
#define MUSB_TXFIFOSZ_OFFSET   0x62
#define MUSB_RXFIFOSZ_OFFSET   0x63
#define MUSB_TXFIFOADD_OFFSET  0x64
#define MUSB_RXFIFOADD_OFFSET  0x66

#define MUSB_LPM_ATTR_OFFSET   0x360
#define MUSB_LPM_CNTRL_OFFSET  0x362
#define MUSB_LPM_INTREN_OFFSET 0x363
#define MUSB_LPM_INTR_OFFSET   0x364
#define MUSB_LPM_FADDR_OFFSET  0x365

#ifndef USB_NUM_BIDIR_ENDPOINTS
#define USB_NUM_BIDIR_ENDPOINTS 16
#endif

typedef enum {
    USB_EP0_STATE_SETUP = 0x0,      /**< SETUP DATA */
    USB_EP0_STATE_IN_DATA = 0x1,    /**< IN DATA */
    USB_EP0_STATE_OUT_DATA = 0x3,   /**< OUT DATA */
    USB_EP0_STATE_IN_STATUS = 0x4,  /**< IN status */
    USB_EP0_STATE_OUT_STATUS = 0x5, /**< OUT status */
    USB_EP0_STATE_IN_ZLP = 0x6,     /**< OUT status */
    USB_EP0_STATE_STALL = 0x7,      /**< STALL status */
} ep0_state_t;

/* Endpoint state */
struct musb_ep_state {
    uint16_t ep_mps;    /* Endpoint max packet size */
    uint8_t ep_type;    /* Endpoint type */
    uint8_t ep_stalled; /* Endpoint stall flag */
    uint8_t ep_enable;  /* Endpoint enable */
    uint8_t *xfer_buf;
    uint32_t xfer_len;
    uint32_t actual_xfer_len;
};

/* Driver state */
struct musb_udc {
    volatile uint8_t dev_addr;
    volatile uint32_t fifo_size_offset;
    __attribute__((aligned(32))) struct usb_setup_packet setup;
    struct musb_ep_state in_ep[USB_NUM_BIDIR_ENDPOINTS];  /*!< IN endpoint parameters*/
    struct musb_ep_state out_ep[USB_NUM_BIDIR_ENDPOINTS]; /*!< OUT endpoint parameters */
} g_musb_udc;

void USBD_IRQHandler(uint8_t busid);
/* SoC INT_SRC_USB_HS ISR entry (void signature) -> USBD_IRQHandler(0). */
void USBD_IRQHandler_Compat(void);

static volatile uint8_t usb_ep0_state = USB_EP0_STATE_SETUP;
volatile bool zlp_flag = 0;

/* get current active ep */
static uint8_t musb_get_active_ep(void)
{
    return HWREGB(USB_BASE + MUSB_INDEX_OFFSET);
}

/* set the active ep */
static void musb_set_active_ep(uint8_t ep_index)
{
    HWREGB(USB_BASE + MUSB_INDEX_OFFSET) = ep_index;
}

static void musb_write_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
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

static void musb_read_packet(uint8_t ep_idx, uint8_t *buffer, uint16_t len)
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

#if CONFIG_USB_RISCV_BRIDGE
/* Device-role drain protocol relayed by the RISC-V firmware via the shared
 * probe. MUST mirror
 * ap/properties/modules/bk_riscv/riscv_src/fw/common/sys_sw_regs_shared.h. The
 * firmware accumulates per-item flags (pending_usbd_evt / pending_setup /
 * pending_ep_in[] / pending_ep_out[]) and raises a single ISR_DRAIN; we replay
 * every set flag so coalesced IPIs never drop EP0 transactions. */
#define RISCV_USBD_EVT_ISR_DRAIN  0x16U
#define RISCV_USBD_PEND_RESET     0x1U
#define RISCV_USBD_PEND_SUSPEND   0x2U
#define RISCV_USBD_PEND_RESUME    0x4U

/* Registered in usb_hc_beken_musb.c; shared IPI_DOMAIN_USB callback. */
extern bk_err_t usb_hc_riscv_ipi_enable(void);

static uint32_t s_dev_last_irq_seq;

/* Fill the shared probe so the RISC-V firmware dispatches to its device ISR.
 * Runs BEFORE usb_dc_riscv_device_prepare() starts the core: g_musb_udc /
 * usb_ep0_state are device-driver symbols the firmware reads in place via
 * these pointers (mirroring how the host path publishes g_musb_hcd). owner is
 * left at AP until the IPI is armed, then flipped to RISCV (see below). */
static void usb_dc_riscv_probe_init_device(void)
{
    volatile riscv_usb_probe_t *ctx = get_riscv_usb_probe();

    s_dev_last_irq_seq = 0;
    ctx->magic = RISCV_USB_PROBE_MAGIC;
    ctx->owner = RISCV_USB_PROBE_OWNER_AP;
    ctx->irq_seq = 0;
    ctx->event = 0;
    ctx->event_data = 0;
    ctx->g_musb_hcd_addr = 0;
    ctx->g_musb_udc_addr = SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&g_musb_udc);
    ctx->usb_ep0_state_addr = SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&usb_ep0_state);
    ctx->pending_ep0 = 0;
    ctx->role = RISCV_USB_ROLE_DEVICE;
    ctx->pending_usbd_evt = 0;
    ctx->pending_setup = 0;
    for (uint32_t i = 0U; i < (uint32_t)RISCV_USB_PROBE_PIPE_NUM; i++) {
        ctx->pending_pipe_tx[i] = 0;
        ctx->pending_pipe_rx[i] = 0;
        ctx->pending_ep_in[i] = 0;
        ctx->pending_ep_out[i] = 0;
    }
}

/* AP-side consumer of the device drain protocol. Called from the IPI callback
 * when probe->role is DEVICE. The RISC-V firmware owns all MUSB register/FIFO
 * access and updates g_musb_udc in place; here we only replay the cherryusb
 * upcalls, mirroring the M55-resident USBD_IRQHandler order
 * (RESET -> resume/suspend -> SETUP -> OUT -> IN).
 *
 * Snapshot+clear the pending flags before processing so a flag set by a new
 * RISC-V ISR mid-drain is preserved (its finalize bumps irq_seq -> next poll).
 * This mirrors usb_hc_riscv_poll_events()'s host drain. */
void usb_dc_riscv_poll_events(void)
{
    volatile riscv_usb_probe_t *ctx = get_riscv_usb_probe();
    uint32_t seq;

    seq = ctx->irq_seq;
    if (seq == s_dev_last_irq_seq) {
        return;
    }
    s_dev_last_irq_seq = seq;

    if (ctx->event == RISCV_USBD_EVT_ISR_DRAIN) {
        uint32_t usbd_evt = ctx->pending_usbd_evt;
        uint32_t setup_pending = ctx->pending_setup;
        uint32_t in_pending[RISCV_USB_PROBE_PIPE_NUM];
        uint32_t out_pending[RISCV_USB_PROBE_PIPE_NUM];
        uint32_t ep;

        ctx->pending_usbd_evt = 0;
        ctx->pending_setup = 0;
        for (ep = 0U; ep < (uint32_t)RISCV_USB_PROBE_PIPE_NUM; ep++) {
            in_pending[ep] = ctx->pending_ep_in[ep];
            ctx->pending_ep_in[ep] = 0;
            out_pending[ep] = ctx->pending_ep_out[ep];
            ctx->pending_ep_out[ep] = 0;
        }

        if (usbd_evt & RISCV_USBD_PEND_RESET) {
            memset(&g_musb_udc, 0, sizeof(struct musb_udc));
            g_musb_udc.fifo_size_offset = USB_CTRL_EP_MPS;
            usbd_event_reset_handler(0);
            usb_ep0_state = USB_EP0_STATE_SETUP;
        }
        if (usbd_evt & RISCV_USBD_PEND_RESUME) {
            usbd_event_resume_handler(0);
        }
        if (usbd_evt & RISCV_USBD_PEND_SUSPEND) {
            usbd_event_suspend_handler(0);
        }
        if (setup_pending) {
            usbd_event_ep0_setup_complete_handler(0, (uint8_t *)&g_musb_udc.setup);
        }
        for (ep = 0U; ep < (uint32_t)RISCV_USB_PROBE_PIPE_NUM; ep++) {
            if (out_pending[ep]) {
                usbd_event_ep_out_complete_handler(0, (uint8_t)ep,
                                                   g_musb_udc.out_ep[ep].actual_xfer_len);
            }
        }
        for (ep = 0U; ep < (uint32_t)RISCV_USB_PROBE_PIPE_NUM; ep++) {
            if (in_pending[ep]) {
                usbd_event_ep_in_complete_handler(0, (uint8_t)(ep | 0x80U),
                                                  g_musb_udc.in_ep[ep].actual_xfer_len);
            }
        }
    }

    /* route USB HS IRQ back to RISC-V (bit8=1); the firmware's finalize handed
     * it to AP for the duration of this drain. */
    {
        uint32_t ints_config = sys_drv_get_ints_config_riscv_0_31();

        ints_config |= (1U << 8);
        sys_drv_set_ints_config_riscv_0_31(ints_config);
    }
}
#endif /* CONFIG_USB_RISCV_BRIDGE */

__WEAK void usb_dc_low_level_init(void)
{
    USB_LOG_INFO("[usb_dc_ll] enter; vote CPU freq + enable analog phy + USB clock\r\n");

    extern void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops);
    bk_analog_layer_usb_sys_related_ops(USB_DEVICE_MODE, true);

#if CONFIG_SOC_BK7259
    /* BK7259 has a different system-control / interrupt model than
     * BK7258. The legacy macro USB_INTERRUPT_CTRL_BIT (defined in
     * middleware/soc/bk7259_ap/hal/sys_types.h:44) tries to expand
     * SYS_CPU0_INT_0_31_EN_CPU0_USB_INT_EN_POS, which is a BK7258
     * register-bit symbol that does NOT exist on the BK7259 system
     * controller -- instead it ships a pair of explicit USB_FS / USB_HS
     * interrupt sources (INT_SRC_USB_FS=7, INT_SRC_USB_HS=8 in
     * include/soc/bk7259/int_types_impl.h). Likewise INT_SRC_USB itself
     * is not defined on BK7259.
     *
     * The USB device controller this file drives (MUSB-MHDRC) sits on
     * the high-speed USB phy on BK7259, so we route its interrupt the
     * same way the host driver (CherryUSB/port/beken_musb/
     * usb_hc_beken_musb.c L856-861) does: register the ISR against
     * INT_SRC_USB_HS and enable that interrupt line on the core that
     * actually services it (CPU2 on the SMP build, current core
     * otherwise).
     *
     * cmds/input.txt section 11 has the full incident write-up; this
     * patch is the first half ("BK7258 -> BK7259 interrupt model
     * adaptation"). The second half lives in usbd_msc.c
     * (MSC_SD_BACKEND_AVAILABLE) and sd_card_driver.c (clock-gate
     * ordering). All three must coexist for U-disk over MSC to work
     * on BK7259. */
    /* Sanity-check the ISR address before we register it. If the
     * link picked up a __WEAK NULL stub for USBD_IRQHandler (which
     * could happen if the device port file got compiled without a
     * concrete usbd_irq path) the very first USB interrupt would
     * jump to PC=0 -- exactly the MemFault pattern we are debugging
     * (see cmds/input.txt). Defensive: we still register so the
     * crash, if any, is reproducible, but we LOUDLY warn first. */
    /* MILESTONE A: try the RISC-V USB bridge first. The stub currently
     * returns -1 (see riscv_usb_bridge.c::usb_dc_riscv_device_prepare()),
     * so we always fall through to the legacy M55 path below. The probe
     * is wired here, so the follow-up milestone only needs to flip the
     * stub return value -- the call site is already in place and the
     * regression behaviour (M55 fallback on bridge failure) is the safe
     * default. */
    int riscv_bridge_rc = -1;
#if CONFIG_USB_RISCV_BRIDGE
    /* Publish the device-role handshake region BEFORE starting the core (the
     * firmware reads g_musb_udc / usb_ep0_state in place via these pointers).
     * owner stays AP so the firmware ISR is dormant until we arm the IPI and
     * flip owner to RISCV below -- same handshake the host path uses. */
    usb_dc_riscv_probe_init_device();
    riscv_bridge_rc = usb_dc_riscv_device_prepare();
    if (riscv_bridge_rc == 0) {
#if CONFIG_IPI
        if (usb_hc_riscv_ipi_enable() != BK_OK) {
            USB_LOG_ERR("[usb_dc_ll] arm riscv device IPI failed\r\n");
        }
#endif
        get_riscv_usb_probe()->owner = RISCV_USB_PROBE_OWNER_RISCV;
        USB_LOG_INFO("[usb_dc_ll] USBD IRQ now owned by RISC-V bridge; skip M55 ISR registration\r\n");
    } else {
        USB_LOG_INFO("[usb_dc_ll] RISC-V device bridge unavailable (rc=%d); using M55 USBD_IRQHandler path\r\n",
                     riscv_bridge_rc);
    }
#endif

    if (riscv_bridge_rc != 0) {
        USB_LOG_INFO("[usb_dc_ll] register INT_SRC_USB_HS isr=%p\r\n", (void*)USBD_IRQHandler_Compat);
        if (USBD_IRQHandler_Compat == NULL) {
            USB_LOG_ERR("[usb_dc_ll] USBD_IRQHandler is NULL -- next USB IRQ will MemFault\r\n");
        }
        bk_int_isr_register(INT_SRC_USB_HS, USBD_IRQHandler_Compat, NULL);
        bk_int_set_priority(INT_SRC_USB_HS, 2);
#if CONFIG_SOC_SMP
        USB_LOG_INFO("[usb_dc_ll] enable INT_SRC_USB_HS on CPU2 (SMP)\r\n");
        sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 1);
#else
        USB_LOG_INFO("[usb_dc_ll] enable INT_SRC_USB_HS on current core\r\n");
        sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 1);
#endif
    }
#else  /* CONFIG_SOC_BK7259 */
    sys_drv_int_enable(USB_INTERRUPT_CTRL_BIT);

    bk_int_isr_register(INT_SRC_USB, USBD_IRQHandler_Compat, NULL);
    bk_int_set_priority(INT_SRC_USB, 2);
#endif /* CONFIG_SOC_BK7259 */

    /* Re-attach fix: the wrapper register at 0x710 (PHY digital control: pll_en
     * bit6, reset bit7, cfg_rstn bit1) keeps its previous value across an MTP
     * stop/start because the deinit path only powers the analog PHY LDO down, it
     * does not reset this digital register. A bare |= therefore produces NO
     * 0->1 edge on the second bring-up, and Naneng PHY spec 8.2 requires a rising
     * edge of PLL_EN (and a reset pulse) for the delay/clock cell to lock. Drive
     * the edge-sensitive controls low first so the sequence below re-creates the
     * required rising edges; the first-ever attach is unaffected. */
    REG_USB_USR_710 &= ~((0x1u<<6) | (0x1u<<7) | (0x1u<<1));
    {
        extern void delay(int num);
        delay(100);
    }

    REG_USB_USR_710 |= (0x1<<15);
    REG_USB_USR_710 |= (0x1<<14);
    REG_USB_USR_710 |= (0x1<<16);
    REG_USB_USR_710 |= (0x1<<17);
    REG_USB_USR_710 |= (0x1<<18);
    REG_USB_USR_710 |= (0x1<<19);
    REG_USB_USR_710 &=~(0x1<<20);
    REG_USB_USR_710 |= (0x1<<21);
    REG_USB_USR_710 |= (0x0<< 0);
    REG_USB_USR_710 |= (0x1<< 5);
    REG_USB_USR_710 |= (0x1<< 6);
    REG_USB_USR_710 |= (0x1<< 9);
    REG_USB_USR_710 |= (0x1<<10);
    REG_USB_USR_710 |= (0x1<< 7);

    REG_USB_USR_708 = 0x1;
    USB_LOG_INFO("[usb_dc_ll] leave; USB device controller phy/IRQ live\r\n");
}

__WEAK void usb_dc_low_level_deinit(void)
{
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);

#if CONFIG_SOC_BK7259
    /* Mirror image of low_level_init above: tear the per-core IRQ
     * enable down BEFORE unregistering the ISR, otherwise a tail
     * interrupt could fire into the now-unregistered slot. */
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_USB_HS, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_HS, 0);
#endif
    bk_int_isr_unregister(INT_SRC_USB_HS);

    sys_hal_usb_analog_phy_en(false);
#else  /* CONFIG_SOC_BK7259 */
    bk_int_isr_unregister(INT_SRC_USB);
    sys_hal_usb_analog_phy_en(false);
    sys_drv_int_disable(USB_INTERRUPT_CTRL_BIT);
#endif /* CONFIG_SOC_BK7259 */

    sys_drv_usb_clock_ctrl(false, NULL);
}

#ifdef CONFIG_USBDEV_TEST_MODE
/* USB 2.0 spec 7.1.20 fixed 53-byte test packet payload. The MUSB core appends
 * the DATA0 PID + CRC16 and re-sends it continuously once TxPktRdy is set, so
 * it only has to be loaded into the EP0 FIFO once. */
static const uint8_t g_musb_test_packet[53] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF,
    0xEF, 0xF7, 0xFB, 0xFD, 0x7E,
};

/*
 * Enter a USB 2.0 high-speed test mode in response to a host
 * SET_FEATURE(TEST_MODE) request. The device core (usbd_core.c) calls this
 * AFTER the control-transfer status stage has completed. test_mode is the test
 * selector = HI_BYTE(wIndex) per USB 2.0 spec 9.4.9 Table 9-7:
 *   1 = Test_J, 2 = Test_K, 3 = Test_SE0_NAK, 4 = Test_Packet
 *   (5 = Test_Force_Enable is hub-only and not applicable to a device)
 * These map onto the MUSB TESTMODE register (offset 0x0F):
 *   D0 Test_SE0_NAK (0x01), D1 Test_J (0x02), D2 Test_K (0x04), D3 Test_Packet (0x08).
 */
void usbd_execute_test_mode(uint8_t busid, uint8_t test_mode)
{
    (void)busid;

    switch (test_mode) {
        case 1: /* Test_J */
            HWREGB(USB_BASE + MUSB_TESTMODE_OFFSET) = 0x02;
            break;
        case 2: /* Test_K */
            HWREGB(USB_BASE + MUSB_TESTMODE_OFFSET) = 0x04;
            break;
        case 3: /* Test_SE0_NAK */
            HWREGB(USB_BASE + MUSB_TESTMODE_OFFSET) = 0x01;
            break;
        case 4: /* Test_Packet: load the standard 53-byte packet into the EP0
                 * FIFO, enter the mode, then set TxPktRdy so the core starts
                 * (and keeps) transmitting it. */
            musb_set_active_ep(0);
            musb_write_packet(0, (uint8_t *)g_musb_test_packet, sizeof(g_musb_test_packet));
            HWREGB(USB_BASE + MUSB_TESTMODE_OFFSET) = 0x08;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;
            break;
        default:
            break;
    }
}
#endif /* CONFIG_USBDEV_TEST_MODE */

int usb_dc_init(uint8_t busid)
{
    (void)busid;
    usb_dc_low_level_init();

#ifdef CONFIG_USB_HS
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
#else
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_HSENAB;
#endif

    musb_set_active_ep(0);
    HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;

    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

    /* Enable USB interrupts */
    HWREGB(USB_BASE + MUSB_INTRUSBE_OFFSET) = (USB_IE_RESET | USB_IE_SUSPND | USB_IE_RESUME);
    HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) = USB_TXIE_EP0;
    HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) = 0;

    /* Enable and support extended LPM transactions */
    HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) = (USB_LPMCNTRL_EN_M | USB_LPMCNTRL_TXLPM | USB_LPMCNTRL_NAK);
    HWREGB(USB_BASE + MUSB_LPM_INTREN_OFFSET) = (USB_LPMIM_ACK | USB_LPMIM_RES);

    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_SOFTCONN;

    return 0;
}

int usb_dc_deinit(uint8_t busid)
{
    (void)busid;

    /* Present a clean disconnect to the host and quiesce the controller BEFORE
     * powering the PHY/clock down (the PHY is still live at this point). Dropping
     * SOFTCONN removes the D+ pull-up so the host sees a real detach; clearing
     * the interrupt-enables and the OTG SESSION bit leaves the MUSB core in the
     * same state a fresh usb_dc_init() expects. Without this, a stop->start cycle
     * left stale SESSION/IE state and the host would not re-enumerate the gadget
     * on the second start (no RESET/CONFIGURED). Mirrors usb_hc_deinit(). */
    HWREGB(USB_BASE + MUSB_POWER_OFFSET)   &= ~USB_POWER_SOFTCONN;
    HWREGB(USB_BASE + MUSB_INTRUSBE_OFFSET) = 0;
    HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET)  = 0;
    HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET)  = 0;
    HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET)  &= ~USB_DEVCTL_SESSION;

    usb_dc_low_level_deinit();
    return 0;
}

int usbd_set_address(uint8_t busid, const uint8_t addr)
{
    (void)busid;
    if (addr == 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = 0;
    }

    g_musb_udc.dev_addr = addr;
    return 0;
}

int usbd_ep_open(uint8_t busid, const struct usb_endpoint_descriptor *ep)
{
    /* Translate the v1.6 endpoint descriptor into a local ep_cfg this port body
     * was written against, so the register logic below stays unchanged. A local
     * struct is used so the port no longer depends on the (v0.7-only)
     * struct usbd_endpoint_cfg from the removed public shim header. */
    struct { uint8_t ep_addr; uint8_t ep_type; uint16_t ep_mps; } ep_cfg_local = {
        .ep_addr = ep->bEndpointAddress,
        .ep_type = (uint8_t)(ep->bmAttributes & 0x03U),
        .ep_mps  = (uint16_t)(ep->wMaxPacketSize & 0x07FFU),
    };
    const typeof(ep_cfg_local) *ep_cfg = &ep_cfg_local;
    (void)busid;
    uint16_t used = 0;
    uint16_t fifo_size = 0;
    uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->ep_addr);
    uint8_t old_ep_idx;
    uint32_t ui32Flags = 0;
    uint16_t ui32Register = 0;

    if (ep_idx == 0) {
        g_musb_udc.out_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.out_ep[0].ep_type = 0x00;
        g_musb_udc.out_ep[0].ep_enable = true;
        g_musb_udc.in_ep[0].ep_mps = USB_CTRL_EP_MPS;
        g_musb_udc.in_ep[0].ep_type = 0x00;
        g_musb_udc.in_ep[0].ep_enable = true;
        return 0;
    }

    if (ep_idx > (USB_NUM_BIDIR_ENDPOINTS - 1)) {
        USB_LOG_ERR("Ep addr %d overflow\r\n", ep_cfg->ep_addr);
        return -1;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep_cfg->ep_addr)) {
        g_musb_udc.out_ep[ep_idx].ep_mps = ep_cfg->ep_mps;
        g_musb_udc.out_ep[ep_idx].ep_type = ep_cfg->ep_type;
        g_musb_udc.out_ep[ep_idx].ep_enable = true;

        HWREGH(USB_BASE + MUSB_IND_RXMAXP_OFFSET) = ep_cfg->ep_mps;

        //
        // Allow auto clearing of RxPktRdy when packet of size max packet
        // has been unloaded from the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_CLEAR) {
            ui32Register = USB_RXCSRH1_AUTOCL;
        }
        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_RXCSRH1_DMAEN | USB_RXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_RXCSRH1_DMAEN;
        }
        //
        // If requested, disable NYET responses for high-speed bulk and
        // interrupt endpoints.
        //
        if (ui32Flags & USB_EP_DIS_NYET) {
            ui32Register |= USB_RXCSRH1_DISNYET;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (ep_cfg->ep_type == 0x01) {
            ui32Register |= USB_RXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_RXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY)
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = (USB_RXCSRL1_CLRDT | USB_RXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) = USB_RXCSRL1_CLRDT;

        fifo_size = musb_get_fifo_size(ep_cfg->ep_mps, &used);

        HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = fifo_size & 0x0f;
        HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = (g_musb_udc.fifo_size_offset >> 3);

        g_musb_udc.fifo_size_offset += used;
    } else {
        g_musb_udc.in_ep[ep_idx].ep_mps = ep_cfg->ep_mps;
        g_musb_udc.in_ep[ep_idx].ep_type = ep_cfg->ep_type;
        g_musb_udc.in_ep[ep_idx].ep_enable = true;

        HWREGH(USB_BASE + MUSB_IND_TXMAXP_OFFSET) = ep_cfg->ep_mps;

        //
        // Allow auto setting of TxPktRdy when max packet size has been loaded
        // into the FIFO.
        //
        if (ui32Flags & USB_EP_AUTO_SET) {
            ui32Register |= USB_TXCSRH1_AUTOSET;
        }

        //
        // Configure the DMA mode.
        //
        if (ui32Flags & USB_EP_DMA_MODE_1) {
            ui32Register |= USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD;
        } else if (ui32Flags & USB_EP_DMA_MODE_0) {
            ui32Register |= USB_TXCSRH1_DMAEN;
        }

        //
        // Enable isochronous mode if requested.
        //
        if (ep_cfg->ep_type == 0x01) {
            ui32Register |= USB_TXCSRH1_ISO;
        }

        HWREGB(USB_BASE + MUSB_IND_TXCSRH_OFFSET) = ui32Register;

        // Reset the Data toggle to zero.
        if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY)
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_TXCSRL1_CLRDT | USB_TXCSRL1_FLUSH);
        else
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_CLRDT;

        fifo_size = musb_get_fifo_size(ep_cfg->ep_mps, &used);

        HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = fifo_size & 0x0f;
        HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = (g_musb_udc.fifo_size_offset >> 3);

        g_musb_udc.fifo_size_offset += used;
    }

    musb_set_active_ep(old_ep_idx);

    return 0;
}

int usbd_ep_close(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    return 0;
}

int usbd_ep_set_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_STALL;
        }
    } else {
        if (ep_idx == 0x00) {
            usb_ep0_state = USB_EP0_STATE_STALL;
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= (USB_CSRL0_STALL | USB_CSRL0_RXRDYC);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_STALL;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_clear_stall(uint8_t busid, const uint8_t ep)
{
    (void)busid;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (USB_EP_DIR_IS_OUT(ep)) {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an OUT endpoint.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_STALL | USB_RXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) |= USB_RXCSRL1_CLRDT;
        }
    } else {
        if (ep_idx == 0x00) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        } else {
            // Clear the stall on an IN endpoint.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~(USB_TXCSRL1_STALL | USB_TXCSRL1_STALLED);
            // Reset the data toggle.
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) |= USB_TXCSRL1_CLRDT;
        }
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_is_stalled(uint8_t busid, const uint8_t ep, uint8_t *stalled)
{
    (void)busid;
    return 0;
}

int usbd_ep_start_write(uint8_t busid, const uint8_t ep, const uint8_t *data, uint32_t data_len)
{
    (void)busid;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.in_ep[ep_idx].ep_enable) {
        return -2;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_TXRDY) {
        musb_set_active_ep(old_ep_idx);
        return -3;
    }

    g_musb_udc.in_ep[ep_idx].xfer_buf = (uint8_t *)data;
    g_musb_udc.in_ep[ep_idx].xfer_len = data_len;
    g_musb_udc.in_ep[ep_idx].actual_xfer_len = 0;

    if (data_len == 0) {
        if (ep_idx == 0x00) {
            if (g_musb_udc.setup.wLength == 0) {
                usb_ep0_state = USB_EP0_STATE_IN_STATUS;
            } else {
                usb_ep0_state = USB_EP0_STATE_IN_ZLP;
            }
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
        }
        musb_set_active_ep(old_ep_idx);
        return 0;
    }
    data_len = MIN(data_len, g_musb_udc.in_ep[ep_idx].ep_mps);

    musb_write_packet(ep_idx, (uint8_t *)data, data_len);
    HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) |= (1 << ep_idx);

    if (ep_idx == 0x00) {
        usb_ep0_state = USB_EP0_STATE_IN_DATA;
        if (data_len < g_musb_udc.in_ep[ep_idx].ep_mps) {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_TXRDY | USB_CSRL0_DATAEND);
        } else {
            HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_TXRDY;
        }
    } else {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
    }

    musb_set_active_ep(old_ep_idx);
    return 0;
}

int usbd_ep_start_read(uint8_t busid, const uint8_t ep, uint8_t *data, uint32_t data_len)
{
    (void)busid;
    uint8_t ep_idx = USB_EP_GET_IDX(ep);
    uint8_t old_ep_idx;

    if (!data && data_len) {
        return -1;
    }
    if (!g_musb_udc.out_ep[ep_idx].ep_enable) {
        return -2;
    }

    old_ep_idx = musb_get_active_ep();
    musb_set_active_ep(ep_idx);

    g_musb_udc.out_ep[ep_idx].xfer_buf = data;
    g_musb_udc.out_ep[ep_idx].xfer_len = data_len;
    g_musb_udc.out_ep[ep_idx].actual_xfer_len = 0;

    if (data_len == 0) {
        if (ep_idx == 0) {
            usb_ep0_state = USB_EP0_STATE_SETUP;
        }
        musb_set_active_ep(old_ep_idx);
        return 0;
    }
    if (ep_idx == 0) {
        usb_ep0_state = USB_EP0_STATE_OUT_DATA;
    } else {
        HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) |= (1 << ep_idx);
    }
    musb_set_active_ep(old_ep_idx);
    return 0;
}

void usbd_remote_wakeup_from_L2_state(void)
{
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_RESUME;
    delay_ms(10);
    HWREGB(USB_BASE + MUSB_POWER_OFFSET) &= ~USB_POWER_RESUME;
}

void usbd_remote_wakeup_from_L1_state(void)
{
    HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) |= USB_LPMCNTRL_RES;
}

static void handle_ep0(void)
{
    uint8_t ep0_status = HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET);
    uint16_t read_count;

    /* SentStall */
    if (ep0_status & USB_CSRL0_STALLED) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_CSRL0_STALLED;
        usb_ep0_state = USB_EP0_STATE_SETUP;
        return;
    }

    /* SetupEnd */
    if (ep0_status & USB_CSRL0_SETEND) {
        HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_SETENDC;
    }

    /* Function Address */
    if (g_musb_udc.dev_addr > 0) {
        HWREGB(USB_BASE + MUSB_FADDR_OFFSET) = g_musb_udc.dev_addr;
        g_musb_udc.dev_addr = 0;
    }

    switch (usb_ep0_state) {
        case USB_EP0_STATE_SETUP:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);
                if (read_count != 8) {
                    return;
                }

                musb_read_packet(0, (uint8_t *)&g_musb_udc.setup, 8);
                if (g_musb_udc.setup.wLength) {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                }

                usbd_event_ep0_setup_complete_handler(0, (uint8_t *)&g_musb_udc.setup);
            }
            break;

        case USB_EP0_STATE_IN_DATA:
            if (g_musb_udc.in_ep[0].xfer_len > g_musb_udc.in_ep[0].ep_mps) {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].ep_mps;
                g_musb_udc.in_ep[0].xfer_len -= g_musb_udc.in_ep[0].ep_mps;
            } else {
                g_musb_udc.in_ep[0].actual_xfer_len += g_musb_udc.in_ep[0].xfer_len;
                g_musb_udc.in_ep[0].xfer_len = 0;
            }

            usbd_event_ep_in_complete_handler(0, 0x80, g_musb_udc.in_ep[0].actual_xfer_len);

            break;
        case USB_EP0_STATE_OUT_DATA:
            if (ep0_status & USB_CSRL0_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                musb_read_packet(0, g_musb_udc.out_ep[0].xfer_buf, read_count);
                g_musb_udc.out_ep[0].xfer_buf += read_count;
                g_musb_udc.out_ep[0].actual_xfer_len += read_count;

                if (read_count < g_musb_udc.out_ep[0].ep_mps) {
                    usbd_event_ep_out_complete_handler(0, 0x00, g_musb_udc.out_ep[0].actual_xfer_len);
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = (USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND);
                    usb_ep0_state = USB_EP0_STATE_IN_STATUS;
                } else {
                    HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_CSRL0_RXRDYC;
                }
            }
            break;
        case USB_EP0_STATE_IN_STATUS:
        case USB_EP0_STATE_IN_ZLP:
            usb_ep0_state = USB_EP0_STATE_SETUP;
            usbd_event_ep_in_complete_handler(0, 0x80, 0);
            break;
    }
}

/* ------------------------------------------------------------------
 * BK7259 USB bring-up diagnostics (2026-05-19 U-disk debugging).
 *
 * usb_storage.c on the app side externs these as weak symbols. They
 * give an easy way to tell from the UART log whether the USB IRQ is
 * actually being delivered to AP CPU2 after the manual PHY power-up.
 * The two variables here are intentionally non-static so the linker
 * makes them visible to the app's extern declaration.
 * ------------------------------------------------------------------ */
volatile uint32_t g_usbd_irq_count = 0;
volatile uint8_t  g_usbd_last_intrusb = 0;

#ifndef USBD_IRQ_DEBUG_PRINT_COUNT
/* How many of the first IRQs to print verbose state for. After this
 * many, only the periodic watchdog dump in usb_storage.c keeps the
 * log alive. 8 is enough to see RESET + SETADDR + GET_DESCRIPTOR +
 * SET_CONFIG transitions but not enough to drown out other logs once
 * the SCSI traffic starts. */
#define USBD_IRQ_DEBUG_PRINT_COUNT 8
#endif

void USBD_IRQHandler(uint8_t busid)
{
    (void)busid;
    uint32_t is;
    uint32_t txis;
    uint32_t rxis;
    uint32_t lpmris;
    uint8_t old_ep_idx;
    uint8_t ep_idx;
    uint16_t write_count, read_count;

    is = HWREGB(USB_BASE + MUSB_INTRUSB_OFFSET);
    txis = HWREGH(USB_BASE + MUSB_INTRTX_OFFSET);
    rxis = HWREGH(USB_BASE + MUSB_INTRRX_OFFSET);
    lpmris = HWREGB(USB_BASE + MUSB_LPM_INTR_OFFSET);
    HWREGB(USB_BASE + MUSB_INTRUSB_OFFSET) = is;
    HWREGB(USB_BASE + MUSB_LPM_INTR_OFFSET) = lpmris;
    old_ep_idx = musb_get_active_ep();

    /* BK7259 bring-up: counter + first-N-times verbose dump. The
     * sub-cost is one increment + one byte store, no allocation. The
     * counters themselves are ALWAYS compiled in -- they're snapshotted
     * by the user-space usb-dbg watchdog thread (see g_usbd_irq_count /
     * g_usbd_last_intrusb externs in usb_storage.c) and we want that
     * path available in production builds too. */
    g_usbd_irq_count++;
    g_usbd_last_intrusb = (uint8_t)is;
#if CONFIG_USBD_IRQ_DEBUG_LOG
    /* The verbose per-IRQ print itself is gated by Kconfig (default n).
     *
     * IRQ-context printf was empirically observed to wedge the bring-up:
     * once SCSI traffic starts, the per-IRQ INFO line backs up on the
     * UART TX FIFO inside the ISR, the ISR returns later and later, and
     * eventually the AP cluster stops servicing the CP heartbeat ->
     * mb_ipc_task asserts ~8 s later. We further bound the damage by
     * only printing the first USBD_IRQ_DEBUG_PRINT_COUNT interrupts so
     * the trace still survives enumeration (RESET / SETADDR / GET_DESC
     * / SET_CONFIG) without bleeding into the SCSI data phase. */
    if (g_usbd_irq_count <= USBD_IRQ_DEBUG_PRINT_COUNT) {
        USB_LOG_DBG("[usbd_irq #%u] IS=0x%02x TX=0x%04x RX=0x%04x "
                     "LPM=0x%02x POWER=0x%02x DEVCTL=0x%02x FADDR=%u%s%s%s%s\r\n",
                     (unsigned)g_usbd_irq_count, (unsigned)is,
                     (unsigned)txis, (unsigned)rxis, (unsigned)lpmris,
                     HWREGB(USB_BASE + MUSB_POWER_OFFSET),
                     HWREGB(USB_BASE + MUSB_DEVCTL_OFFSET),
                     HWREGB(USB_BASE + MUSB_FADDR_OFFSET),
                     (is & USB_IS_RESET)   ? " RESET"   : "",
                     (is & USB_IS_SOF)     ? " SOF"     : "",
                     (is & USB_IS_RESUME)  ? " RESUME"  : "",
                     (is & USB_IS_SUSPEND) ? " SUSPEND" : "");
    }
#endif

    /* Receive a reset signal from the USB bus */
    if (is & USB_IS_RESET) {
        memset(&g_musb_udc, 0, sizeof(struct musb_udc));
        g_musb_udc.fifo_size_offset = USB_CTRL_EP_MPS;
        usbd_event_reset_handler(0);
        HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) = USB_TXIE_EP0;
        HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) = 0;

        for (uint8_t i = 1; i < USB_NUM_BIDIR_ENDPOINTS; i++) {
            musb_set_active_ep(i);
            HWREGB(USB_BASE + MUSB_TXFIFOSZ_OFFSET) = 0;
            HWREGH(USB_BASE + MUSB_TXFIFOADD_OFFSET) = 0;
            HWREGB(USB_BASE + MUSB_RXFIFOSZ_OFFSET) = 0;
            HWREGH(USB_BASE + MUSB_RXFIFOADD_OFFSET) = 0;
        }
        usb_ep0_state = USB_EP0_STATE_SETUP;
    }

    if (is & USB_IS_SOF) {
    }

    if (is & USB_IS_RESUME) {
        USB_LOG_DBG("usbd resume int triggered\r\n");
        usbd_event_resume_handler(0);
    }

    if (is & USB_IS_SUSPEND) {
        USB_LOG_DBG("usbd suspend int triggered\r\n");
        usbd_event_suspend_handler(0);
    }

    if (lpmris & USB_LPMRIS_ACK) {
        USB_LOG_DBG("usbd enter L1 state\r\n");
        HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) |= USB_LPMCNTRL_NAK;
    }

    if (lpmris & USB_LPMRIS_RES) {
        USB_LOG_DBG("usbd LPM resume int\r\n");
        HWREGB(USB_BASE + MUSB_LPM_CNTRL_OFFSET) &= ~USB_LPMCNTRL_NAK;
    }

    txis &= HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET);
    /* Handle EP0 interrupt */
    if (txis & USB_TXIE_EP0) {
        HWREGH(USB_BASE + MUSB_INTRTX_OFFSET) = USB_TXIE_EP0;
        musb_set_active_ep(0);
        handle_ep0();
        txis &= ~USB_TXIE_EP0;
    }

    ep_idx = 1;
    while (txis) {
        if (txis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_INTRTX_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) & USB_TXCSRL1_UNDRN) {
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) &= ~USB_TXCSRL1_UNDRN;
            }

            if (g_musb_udc.in_ep[ep_idx].xfer_len > g_musb_udc.in_ep[ep_idx].ep_mps) {
                g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].ep_mps;
                g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].ep_mps;
                g_musb_udc.in_ep[ep_idx].xfer_len -= g_musb_udc.in_ep[ep_idx].ep_mps;
            } else {
                g_musb_udc.in_ep[ep_idx].xfer_buf += g_musb_udc.in_ep[ep_idx].xfer_len;
                g_musb_udc.in_ep[ep_idx].actual_xfer_len += g_musb_udc.in_ep[ep_idx].xfer_len;
                g_musb_udc.in_ep[ep_idx].xfer_len = 0;
            }

            if (g_musb_udc.in_ep[ep_idx].xfer_len == 0) {
                HWREGH(USB_BASE + MUSB_INTRTXE_OFFSET) &= ~(1 << ep_idx);
                usbd_event_ep_in_complete_handler(0, ep_idx | 0x80, g_musb_udc.in_ep[ep_idx].actual_xfer_len);
            } else {
                write_count = MIN(g_musb_udc.in_ep[ep_idx].xfer_len, g_musb_udc.in_ep[ep_idx].ep_mps);

                musb_write_packet(ep_idx, g_musb_udc.in_ep[ep_idx].xfer_buf, write_count);
                HWREGB(USB_BASE + MUSB_IND_TXCSRL_OFFSET) = USB_TXCSRL1_TXRDY;
            }

            txis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    rxis &= HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET);
    ep_idx = 1;
    while (rxis) {
        if (rxis & (1 << ep_idx)) {
            musb_set_active_ep(ep_idx);
            HWREGH(USB_BASE + MUSB_INTRRX_OFFSET) = (1 << ep_idx);
            if (HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) & USB_RXCSRL1_RXRDY) {
                read_count = HWREGH(USB_BASE + MUSB_IND_RXCOUNT_OFFSET);

                musb_read_packet(ep_idx, g_musb_udc.out_ep[ep_idx].xfer_buf, read_count);
                HWREGB(USB_BASE + MUSB_IND_RXCSRL_OFFSET) &= ~(USB_RXCSRL1_RXRDY);

                g_musb_udc.out_ep[ep_idx].xfer_buf += read_count;
                g_musb_udc.out_ep[ep_idx].actual_xfer_len += read_count;
                g_musb_udc.out_ep[ep_idx].xfer_len -= read_count;

                if ((read_count < g_musb_udc.out_ep[ep_idx].ep_mps) || (g_musb_udc.out_ep[ep_idx].xfer_len == 0)) {
                    HWREGH(USB_BASE + MUSB_INTRRXE_OFFSET) &= ~(1 << ep_idx);
                    usbd_event_ep_out_complete_handler(0, ep_idx, g_musb_udc.out_ep[ep_idx].actual_xfer_len);
                } else {
                }
            }

            rxis &= ~(1 << ep_idx);
        }
        ep_idx++;
    }

    musb_set_active_ep(old_ep_idx);
}

/* SoC interrupt entry. The BK7259 INT_SRC_USB_HS ISR slot expects a void()
 * function; bridge it to the v1.6 busid device ISR (single device bus 0).
 * Mirrors the host side USBH_IRQHandler_Compat in usb_hc_beken_musb.c. */
void USBD_IRQHandler_Compat(void)
{
    USBD_IRQHandler(0);
}
