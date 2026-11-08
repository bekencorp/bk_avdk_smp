/*
 * Copyright (c) 2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7259 FS/LS USB IP CherryUSB host controller port (compact FDRC core).
 *
 * Hardware bring-up sequence below is transcribed 1:1 from the
 * hardware-validated Mentor bring-up glue (bk_mentor_usb_drv.c, silicon
 * proven 2026-10-29): VBUS LDO (driver layer) -> FS power sub-domain +
 * usb_fs_cken -> USB analog latch writes -> shared USB_1 clock -> wrapper
 * probe + GlobalCtrl release -> OTG host termination -> rstn pulse ->
 * core interrupt unmask -> ISR registration -> DEVCTL SESSION kick for
 * pre-attached devices.
 *
 * Skeleton status (see the FS port task plan):
 *  - [DONE]    low-level init/deinit, ISR plumbing, roothub control,
 *              pipe pool, port connect status
 *  - [DONE]    ep0 control transfer state machine (stage 3)
 *  - [DONE]    bulk/int TX/RX CSR state machines + dynamic FIFO
 *              allocation (stage 4), usbh_beken_fs_submit_urb/kill_urb
 *              (isochronous remains unsupported)
 */
#include "usbh_core.h"
#include "usbh_hub.h"
#include "usb_beken_fs_musb_reg.h"
#include "usb_hc_beken_fs.h"
#include "sys_driver.h"
#include "components/log.h"
#include <os/os.h>
#include <soc/soc.h>
#include <driver/int_types.h>
#include <driver/hal/hal_int_types.h>
#include <modules/pm.h>
#include "usb_errno.h"

#if CONFIG_USB_FS_HOST

#include <stdint.h>
#include <string.h>
#include <driver/int.h>

/* ------------------------------------------------------------------ */
/* Logging                                                             */
/* ------------------------------------------------------------------ */
#define FS_PORT_TAG "usb_fs"
#define FS_LOGI(...) BK_LOGI(FS_PORT_TAG, ##__VA_ARGS__)
#define FS_LOGW(...) BK_LOGW(FS_PORT_TAG, ##__VA_ARGS__)
#define FS_LOGE(...) BK_LOGE(FS_PORT_TAG, ##__VA_ARGS__)

/* Legacy errno mapping (same as beken_musb port) */
#ifndef EPERM
#define EPERM USB_ERR_STALL
#endif
#ifndef EIO
#define EIO USB_ERR_IO
#endif
#ifndef ENOMEM
#define ENOMEM USB_ERR_NOMEM
#endif
#ifndef EBUSY
#define EBUSY USB_ERR_BUSY
#endif
#ifndef EINVAL
#define EINVAL USB_ERR_INVAL
#endif
#ifndef ENODEV
#define ENODEV USB_ERR_NODEV
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN USB_ERR_NODEV
#endif
#ifndef EPIPE
#define EPIPE USB_ERR_INVAL
#endif

#ifndef FS_MIN
#define FS_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

extern void delay(INT32 num);
/* sys_drv_usb_clock_ctrl(bool, void*) is declared in sys_driver.h */

/* Wrapper byte offset, probed at init between the two datasheet
 * interpretations (word-indexed 0x8A0*4 vs literal). */
static uint32_t g_fs_wrapper_off;

/* ------------------------------------------------------------------ */
/* Port state (mirrors beken_musb single-instance layout, static)     */
/* ------------------------------------------------------------------ */
typedef enum {
    FS_EP0_STATE_SETUP = 0x0,
    FS_EP0_STATE_IN_DATA,
    FS_EP0_STATE_IN_STATUS,
    FS_EP0_STATE_OUT_DATA,
    FS_EP0_STATE_OUT_STATUS,
} fs_ep0_state_t;

struct fs_pipe {
    uint8_t dev_addr;
    uint8_t ep_addr;
    uint8_t ep_type;
    uint8_t ep_interval;
    uint8_t speed;
    uint16_t ep_mps;
    uint16_t ep_local_index;
    bool inuse;
    uint32_t xfrd;
    volatile bool waiter;
    usb_osal_sem_t waitsem;
    struct usbh_hubport *hport;
    struct usbh_urb *urb;
    uint32_t iso_frame_idx;
};

struct fs_hcd {
    volatile bool port_csc;      /* connect status change */
    volatile bool port_pec;      /* enable status change */
    volatile bool port_crc;      /* reset status change (one-shot) */
    volatile bool port_pe;       /* port enabled */
    volatile uint8_t ep_local_index_record;
    struct fs_pipe pipe_pool[CONFIG_USBHOST_PIPE_NUM][2]; /* [i][0]=TX(host OUT), [i][1]=RX(host IN) */
    uint16_t fifo_used;          /* next free dynamic-FIFO RAM offset */
};

static struct fs_hcd g_fs_hcd;

/* Dynamic FIFO RAM geometry: MUSB_C_RAM_BITS=12 (brd_cnf.h) -> 4KB total.
 * ep0 owns [0,64) (single 64B buffer programmed at init, matching the
 * MGC_AllocateDynamicFifo reservation in the Mentor stack); bulk/int pipes
 * allocate pow2-sized buffers above it, each base aligned to its size. */
#define FS_FIFO_RAM_SIZE   4096
#define FS_FIFO_EP0_SIZE   64
#define FS_FIFO_EP0_CODE   3   /* size code: 2^(3+3) = 64 bytes */

/* ------------------------------------------------------------------ */
/* USB analog latch write (protocol copied from sys_ana_usb_phy_op(),  */
/* the proven HS analog path; AP hal stubs are empty on BK7259).       */
/* ------------------------------------------------------------------ */
static void fs_ana_write(uint32_t addr, uint32_t val)
{
    /* sys_ana_usb_phy_op() (HS analog path) RMWs the same 0x4E register
     * through this latch protocol under rtos_enter_critical; mirror that
     * so a concurrent HS open/close cannot interleave latch cycles. */
    uint32_t lvl = rtos_enter_critical();
    uint32_t reg_latch = REG_READ(MUSBFS_ANA_LATCH_REG);

    REG_WRITE(MUSBFS_ANA_LATCH_REG, reg_latch | MUSBFS_ANA_LATCH_USB);
    REG_WRITE(addr, val);
    while (REG_READ(MUSBFS_SYS_OP_STATUS) >> 14)
        ;
    REG_WRITE(MUSBFS_ANA_LATCH_REG, reg_latch);
    rtos_exit_critical(lvl);
}

/* ------------------------------------------------------------------ */
/* ISR routing: IRQ 0..31 default to the CP (RISC-V) core on SMP; clear */
/* the FS USB bit so the AP core takes it (mirror of the HS path).    */
/* ------------------------------------------------------------------ */
static void fs_irq_route_to_ap(void)
{
    uint32_t ints_config = sys_drv_get_ints_config_riscv_0_31();

    ints_config &= ~(1U << INT_SRC_USB_FS);
    sys_drv_set_ints_config_riscv_0_31(ints_config);
}

static void fs_irq_enable(bool enable)
{
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_USB_FS, enable ? 1 : 0);
}

/* ------------------------------------------------------------------ */
/* Low-level hardware bring-up (hardware-validated sequence)          */
/*                                                                     */
/* NOTE: the shared VBUS LDO (GPIO32, CONFIG_USB_VBAT_CONTROL_GPIO_ID) */
/* is owned by the driver layer (bk_usb_fs_open/close, stage 2.5) with */
/* an HS interlock; it must already be ON when this is called.         */
/* ------------------------------------------------------------------ */
static int fs_low_level_init(void)
{
    uint32_t i;

    /* step 1: FS/LS IP dedicated power sub-domain + clock gate
     * (usb_fs_cken, CLK_PWR_ID_USB_FS). Without these the IP does not
     * decode reads at SOC_USB_FS_BASE at all (reads return 0x00). */
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_USB_FS, PM_POWER_MODULE_STATE_ON);
    bk_pm_clock_ctrl(CLK_PWR_ID_USB_FS, CLK_PWR_CTRL_PWR_UP);

    /* step 2: FS USB analog power-up via latch protocol:
     *   ana_reg14: EN_USBVCC18 | EN_USBVCC3V
     *   ana_reg42: PWD_USB -> 0 powers the FS PHY island
     * Without PWD_USB=0 the FS IP register file reads back 0x00. */
    fs_ana_write(MUSBFS_ANA_REG14, REG_READ(MUSBFS_ANA_REG14) | MUSBFS_ANA14_VCC_MASK);
    fs_ana_write(MUSBFS_ANA_REG42, REG_READ(MUSBFS_ANA_REG42) & ~MUSBFS_ANA42_PWD_USB);

    /* step 3: shared USB subsystem clock (USB_1 gate). Never powered down
     * by the FS port: the CherryUSB HS IP shares it. */
    sys_drv_usb_clock_ctrl(true, NULL);
    delay(500);

    /* step 4: locate the wrapper (two candidate offsets) and release the
     * core: GlobalCtrl defaults to 0 = soft_resetn LOW + bps_clkgate
     * gated; nothing else makes the core respond while these are 0. */
    {
        static const uint32_t candidates[] = { MUSBFS_WRAPPER_OFF_PRIMARY, MUSBFS_WRAPPER_OFF_FALLBACK };
        uint32_t devid = 0;

        for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
            g_fs_wrapper_off = candidates[i];
            devid = FSWRAP_REG(FSWRAP_O_DEVICEID);
            if (devid == MUSBFS_WRAPPER_DEVID)
                break;
        }
        if (devid != MUSBFS_WRAPPER_DEVID) {
            FS_LOGE("fs wrapper NOT found (last devid 0x%08x)\r\n", devid);
            return -ENODEV;
        }
        FS_LOGI("fs wrapper @+0x%x ver:0x%08x\r\n", g_fs_wrapper_off, FSWRAP_REG(FSWRAP_O_VERSIONID));
    }

    FSWRAP_REG(FSWRAP_O_GLOBALCTRL) = FSWRAP_GLBCTRL_RELEASE;
    delay(100);

    /* host mode: DP/DN pulldowns (host-side idle termination) */
    FSWRAP_REG(FSWRAP_O_OTG_CFG) = FSWRAP_OTG_HOST_PD;
    /* wrapper interrupt: host connect event routed into INT_SRC_USB_FS */
    FSWRAP_REG(FSWRAP_O_VTH) = FSWRAP_VTH_INT_HOST_CONN_EN;
    /* pulse the IP-wide soft reset (self-clearing) */
    FSWRAP_REG(FSWRAP_O_RESET) = 1;
    delay(100);

    /* step 5: prove the core is alive: read back CONFIGDATA/FIFO depth. */
    {
        uint8_t configdata = MUSBFS_R8(MGC_O_FS_CONFIGDATA);
        uint8_t devctl = MUSBFS_R8(MGC_O_FS_DEVCTL);

        FS_LOGI("fs core up: configdata:0x%02x devctl:0x%02x gen:0x%08x\r\n",
                configdata, devctl, FSWRAP_REG(FSWRAP_O_GEN));
    }

    return 0;
}

static void fs_low_level_deinit(void)
{
    FSWRAP_REG(FSWRAP_O_OTG_CFG) = 0;
    FSWRAP_REG(FSWRAP_O_RESET) = 1;
    /* hold the core in reset and re-gate its functional clock */
    FSWRAP_REG(FSWRAP_O_GLOBALCTRL) = FSWRAP_GLBCTRL_HOLD;

    /* FS-dedicated clock + power sub-domain off; shared USB_1 stays on
     * for the HS IP. */
    bk_pm_clock_ctrl(CLK_PWR_ID_USB_FS, CLK_PWR_CTRL_PWR_DOWN);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_USB_FS, PM_POWER_MODULE_STATE_OFF);
}

/* ------------------------------------------------------------------ */
/* Port speed / connect status                                         */
/* ------------------------------------------------------------------ */
static uint8_t fs_get_port_speed(void)
{
    uint8_t devctl = MUSBFS_R8(MGC_O_FS_DEVCTL);

    if (devctl & MGC_M_FS_DEVCTL_FSDEV)
        return USB_SPEED_FULL;
    if (devctl & MGC_M_FS_DEVCTL_LSDEV)
        return USB_SPEED_LOW;
    return USB_SPEED_FULL; /* no HS on this IP */
}

bool usb_hc_beken_fs_port_connect_status(void)
{
    return g_fs_hcd.port_pe;
}

/* ------------------------------------------------------------------ */
/* ISR skeleton                                                        */
/* ------------------------------------------------------------------ */
/* forward: defined in the ep0 engine block below */
static void fs_abort_all_pipes(void);

/* Wake the hub thread the way the HS port does (usbh_roothub_thread_wakeup):
 * usbh_hub_events() decides which port to service from the roothub
 * interrupt bitmap, so the port-change bit MUST be set before the mq
 * wakeup or the hub thread wakes up, sees an empty bitmap and skips
 * every port (enumeration never starts). */
static void fs_roothub_wakeup(uint8_t port)
{
    struct usbh_bus *bus = &g_usbhost_bus[BK_FS_USB_BUS_ID];

    if (bus->hcd.roothub.int_buffer) {
        bus->hcd.roothub.int_buffer[0] |= (1 << port);
    }
    usbh_hub_thread_wakeup(&bus->hcd.roothub);
}

static void fs_handle_connect(void)
{
    g_fs_hcd.port_csc = 1;
    g_fs_hcd.port_pec = 1;
    g_fs_hcd.port_pe = 1;
    fs_roothub_wakeup(1); /* single port -> bitmap bit 1 */
    FS_LOGI("device connected, speed:%d\r\n", fs_get_port_speed());
}

static void fs_handle_disconnect(void)
{
    g_fs_hcd.port_csc = 1;
    g_fs_hcd.port_pec = 1;
    g_fs_hcd.port_pe = 0;
    fs_abort_all_pipes();
    fs_roothub_wakeup(1);
    FS_LOGI("device disconnected\r\n");
}

/* ================================================================== */
/* ep0 control transfer engine (TODO-3)                                */
/*                                                                     */
/* Contract (mirrors the beken_musb HS port, verified against core):   */
/*  - usbh_control_transfer() calls usbh_submit_urb() and immediately   */
/*    reads urb->actual_length -> submit must BLOCK until completion    */
/*    when urb->timeout > 0 (sem on pipe->waitsem).                    */
/*  - urb->hcpriv caches the pipe pointer across retries.              */
/*  - FDRC host CSR0 bits are value-identical to HDRC host CSRL0       */
/*    (RXRDY/TXRDY/STALL/SETUP/ERROR/REQPKT/STATUS), so the HS state  */
/*    machine transfers 1:1; only register offsets and the ep address  */
/*    path differ (single FADDR register behind INDEX, no hub page).  */
/* ================================================================== */
static volatile uint8_t g_fs_ep0_state = FS_EP0_STATE_SETUP;

/* ep address -> pipe map, per hport (port-internal, like the HS port) */
struct fs_ep_pipe_map {
    struct usbh_hubport *hport;
    uint8_t ep_addr;
    uint8_t ep_type;
    struct fs_pipe *pipe;
};
static struct fs_ep_pipe_map s_fs_ep_pipe_map[CONFIG_USBHOST_PIPE_NUM * 2];

static struct fs_pipe *fs_ep_pipe_map_find(struct usbh_hubport *hport, uint8_t ep_addr, uint8_t ep_type)
{
    for (uint32_t i = 0; i < (sizeof(s_fs_ep_pipe_map) / sizeof(s_fs_ep_pipe_map[0])); i++) {
        if (s_fs_ep_pipe_map[i].pipe
            && s_fs_ep_pipe_map[i].hport == hport
            && s_fs_ep_pipe_map[i].ep_addr == ep_addr
            && s_fs_ep_pipe_map[i].ep_type == ep_type) {
            return s_fs_ep_pipe_map[i].pipe;
        }
    }
    return NULL;
}

static void fs_ep_pipe_map_add(struct usbh_hubport *hport, uint8_t ep_addr, uint8_t ep_type, struct fs_pipe *pipe)
{
    if (!pipe) {
        return;
    }
    for (uint32_t i = 0; i < (sizeof(s_fs_ep_pipe_map) / sizeof(s_fs_ep_pipe_map[0])); i++) {
        if (s_fs_ep_pipe_map[i].pipe == NULL) {
            s_fs_ep_pipe_map[i].hport = hport;
            s_fs_ep_pipe_map[i].ep_addr = ep_addr;
            s_fs_ep_pipe_map[i].ep_type = ep_type;
            s_fs_ep_pipe_map[i].pipe = pipe;
            return;
        }
    }
}

static void fs_ep_pipe_map_clear(void)
{
    memset(s_fs_ep_pipe_map, 0, sizeof(s_fs_ep_pipe_map));
}

/* complete the urb bound to the pipe: errorcode/actual_length must be
 * final before calling. Mirrors the HS port musb_pipe_waitup(): a sync
 * waiter gets the semaphore AND the complete callback runs (consumers
 * like the ch34x tx path rely on complete firing even for timeout>0
 * urbs; control urbs simply have complete==NULL). */
static void fs_pipe_waitup(struct fs_pipe *pipe)
{
    struct usbh_urb *urb = pipe->urb;

    if (!urb) {
        return;
    }
    pipe->urb = NULL;
    if (pipe->waiter) {
        pipe->waiter = false;
        usb_osal_sem_give(pipe->waitsem);
    }
    if (urb->complete) {
        if (urb->errorcode < 0) {
            urb->complete(urb->arg, urb->errorcode);
        } else {
            urb->complete(urb->arg, urb->actual_length);
        }
    }
}

/* disconnect: abort every pipe with an in-flight urb so no submitter
 * stays blocked forever on a gone device, then drop the map. The dynamic
 * FIFO allocator restarts from the ep0 baseline: the next device's pipes
 * are re-allocated (and re-programmed with fresh flush/datatog) on its
 * own enumeration. */
static void fs_abort_all_pipes(void)
{
    fs_ep_pipe_map_clear();
    for (uint8_t i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        for (uint8_t dir = 0; dir < 2; dir++) {
            struct fs_pipe *pipe = &g_fs_hcd.pipe_pool[i][dir];

            pipe->inuse = false;
            if (pipe->urb) {
                pipe->urb->errorcode = -ENODEV;
                fs_pipe_waitup(pipe);
            }
        }
    }
    /* mask all endpoint interrupts except ep0; stray TX/RX sources with
     * no pipe are drained defensively by the ISR handlers */
    MUSBFS_W8(MGC_O_FS_INTRTX1E, 0x01);
    MUSBFS_W8(MGC_O_FS_INTRTX2E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRRX1E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRRX2E, 0x00);
    g_fs_hcd.fifo_used = FS_FIFO_EP0_SIZE;
    g_fs_ep0_state = FS_EP0_STATE_SETUP;
}

static void fs_set_active_ep(uint8_t idx)
{
    MUSBFS_W8(MGC_O_FS_INDEX, idx);
}

/* ISR-safe critical section (same pattern as the beken_musb HS port) */
#include <spinlock.h>
static SPINLOCK_SECTION volatile spinlock_t fs_usb_spin_lock = SPIN_LOCK_INIT;

static inline uint32_t usb_enter_critical(void)
{
    uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
    spin_lock(&fs_usb_spin_lock);
#endif
    return flags;
}

static inline void usb_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
    spin_unlock(&fs_usb_spin_lock);
#endif
    rtos_enable_int(flags);
}

/* FIFO access: FDRC AHB variant exposes a 32-bit window per endpoint,
 * byte offset (0x20 + ep) * 4 (see usb_dc_beken_musb_fdrc.c, same core).
 * Access WIDTH = bytes pushed/popped per access: a 32-bit store pushes
 * 4 FIFO bytes, an 8-bit store pushes exactly 1. Non-multiple-of-4
 * tails therefore MUST use 8-bit accesses (Mentor MGC_FdrcLoadFifo does
 * word loop + byte tail; a word store on the tail corrupts the packet
 * with extra bytes). */
static void fs_write_packet(uint8_t ep, const uint8_t *src, uint16_t len)
{
    volatile uint32_t *fifo32 = (volatile uint32_t *)MUSBFS_FIFO(ep);
    volatile uint8_t *fifo8 = (volatile uint8_t *)MUSBFS_FIFO(ep);
    uint32_t word;

    while (len >= 4) {
        word = (uint32_t)src[0] | ((uint32_t)src[1] << 8)
             | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
        *fifo32 = word;
        src += 4;
        len -= 4;
    }
    while (len--) {
        *fifo8 = *src++;
    }
}

static void fs_read_packet(uint8_t ep, uint8_t *dst, uint16_t len)
{
    volatile uint32_t *fifo32 = (volatile uint32_t *)MUSBFS_FIFO(ep);
    volatile uint8_t *fifo8 = (volatile uint8_t *)MUSBFS_FIFO(ep);
    uint32_t word;

    while (len >= 4) {
        word = *fifo32;
        dst[0] = (uint8_t)(word >> 0);
        dst[1] = (uint8_t)(word >> 8);
        dst[2] = (uint8_t)(word >> 16);
        dst[3] = (uint8_t)(word >> 24);
        dst += 4;
        len -= 4;
    }
    while (len--) {
        *dst++ = *fifo8;
    }
}

/* discard a resident packet without a destination buffer (spurious RX,
 * overflow beyond the requested length) */
static void fs_drain_packet(uint8_t ep, uint16_t len)
{
    volatile uint32_t *fifo32 = (volatile uint32_t *)MUSBFS_FIFO(ep);
    volatile uint8_t *fifo8 = (volatile uint8_t *)MUSBFS_FIFO(ep);

    while (len >= 4) {
        (void)*fifo32;
        len -= 4;
    }
    while (len--) {
        (void)*fifo8;
    }
}

/* ================================================================== */
/* bulk/int pipe engine (TODO-4)                                       */
/*                                                                     */
/* Register contract transcribed from the Mentor FDRC driver           */
/* (mu_fdr.c MGC_FdrcBindEndpoint/StartRx/StartTx), which is the       */
/* silicon-proven reference for this exact IP:                         */
/*  - TXMAXP/RXMAXP are in 8-byte units (mps >> 3), NOT raw like HDRC  */
/*  - TX/RXINTERVAL = 0 for bulk AND int: upper bits set make the core */
/*    hammer RxError interrupts (Mentor TODO note); 0 = retry NAKs    */
/*    forever, which matches CherryUSB's async URB model              */
/*  - host OUT (TX): TXCSR2.MODE=1; host IN (RX): MODE=0              */
/*  - dynamic FIFO: FIFO1 = base>>3, FIFO2 = size_code<<5 | (base>>11)*/
/*  - FADDR is a single shared register (no per-ep func-addr page on  */
/*    FDRC); single-device bus means every pipe writes the same value */
/* ================================================================== */

/* smallest pow2 >= mps, as FDRC size code (2^(code+3) bytes) */
static uint8_t fs_fifo_size_code(uint16_t mps)
{
    uint8_t code = 0;
    uint16_t size = 8;

    while ((size < mps) && (code < 7)) {
        size <<= 1;
        code++;
    }
    return code;
}

static uint8_t fs_type_reg(struct fs_pipe *pipe)
{
    /* speed [7:6] | proto [5:4] | remote ep [3:0]; FS devices encode
     * speed as 0x00, identical to the Mentor-programmed value */
    return (uint8_t)(pipe->speed | (pipe->ep_type << MGC_S_FS_TYPE_PROTO)
                     | (pipe->ep_addr & MGC_M_FS_TYPE_REMOTE_END));
}

/* allocate a pool slot + dynamic FIFO for a new bulk/int pipe and
 * program the hardware endpoint (Mentor bind sequence) */
static struct fs_pipe *fs_bulk_pipe_alloc(struct usbh_urb *urb)
{
    uint8_t ep_type = (uint8_t)(urb->ep->bmAttributes & USB_ENDPOINT_TYPE_MASK);
    uint8_t ep_addr = urb->ep->bEndpointAddress;
    uint8_t dir = (ep_addr & 0x80) ? 1 : 0; /* 1=RX(host IN), 0=TX(host OUT) */
    uint16_t mps = (uint16_t)(urb->ep->wMaxPacketSize & USB_MAXPACKETSIZE_MASK);
    uint8_t size_code = fs_fifo_size_code(mps);
    uint16_t size = (uint16_t)(8u << size_code);
    uint16_t base = (g_fs_hcd.fifo_used + size - 1) & (uint16_t)~(size - 1);
    struct fs_pipe *pipe = NULL;
    uint8_t old_idx;
    uint8_t idx;

    if ((uint32_t)base + size > FS_FIFO_RAM_SIZE) {
        return NULL;
    }
    for (idx = 1; idx < CONFIG_USBHOST_PIPE_NUM; idx++) {
        if (!g_fs_hcd.pipe_pool[idx][dir].inuse) {
            pipe = &g_fs_hcd.pipe_pool[idx][dir];
            break;
        }
    }
    if (!pipe) {
        return NULL;
    }

    pipe->inuse = true;
    pipe->ep_local_index = idx;
    pipe->dev_addr = urb->hport->dev_addr;
    pipe->ep_addr = ep_addr;
    pipe->ep_type = ep_type;
    pipe->ep_interval = urb->ep->bInterval;
    pipe->speed = (urb->hport->speed == USB_SPEED_LOW)
                  ? MGC_V_FS_SPEED_LOW : MGC_V_FS_SPEED_FULL;
    pipe->ep_mps = mps;
    pipe->hport = urb->hport;

    old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
    fs_set_active_ep(idx);
    MUSBFS_W8(MGC_O_FS_FADDR, pipe->dev_addr);

    if (dir) {
        /* host IN: program the RX side of hw ep idx */
        MUSBFS_W8(MGC_O_FS_RXMAXP, mps >> 3);
        MUSBFS_W8(MGC_O_FS_RXTYPE, fs_type_reg(pipe));
        MUSBFS_W8(MGC_O_FS_RXINTERVAL, 0);
        MUSBFS_W8(MGC_O_FS_RXFIFO1, base >> 3);
        MUSBFS_W8(MGC_O_FS_RXFIFO2, size_code << MGC_S_FS_FIFO_SZ);
        MUSBFS_W8(MGC_O_FS_TXCSR2,
                  MUSBFS_R8(MGC_O_FS_TXCSR2) & ~MGC_M_FS_TXCSR2_MODE);
        MUSBFS_W8(MGC_O_FS_RXCSR2, 0);
        MUSBFS_W8(MGC_O_FS_RXCSR1, MGC_M_FS_RXCSR1_CLRDATATOG);
        MUSBFS_W8(MGC_O_FS_RXCSR1, MGC_M_FS_RXCSR1_CLRDATATOG);
        MUSBFS_W8(MGC_O_FS_INTRRX1E,
                  MUSBFS_R8(MGC_O_FS_INTRRX1E) | (uint8_t)(1u << idx));
    } else {
        /* host OUT: program the TX side of hw ep idx */
        MUSBFS_W8(MGC_O_FS_TXMAXP, mps >> 3);
        MUSBFS_W8(MGC_O_FS_TXTYPE, fs_type_reg(pipe));
        MUSBFS_W8(MGC_O_FS_TXINTERVAL, 0);
        MUSBFS_W8(MGC_O_FS_TXFIFO1, base >> 3);
        MUSBFS_W8(MGC_O_FS_TXFIFO2, size_code << MGC_S_FS_FIFO_SZ);
        MUSBFS_W8(MGC_O_FS_TXCSR2, MGC_M_FS_TXCSR2_MODE);
        /* flush twice in case of double packet buffering (Mentor habit) */
        MUSBFS_W8(MGC_O_FS_TXCSR1,
                  MGC_M_FS_TXCSR1_FLUSHFIFO | MGC_M_FS_TXCSR1_CLRDATATOG);
        MUSBFS_W8(MGC_O_FS_TXCSR1,
                  MGC_M_FS_TXCSR1_FLUSHFIFO | MGC_M_FS_TXCSR1_CLRDATATOG);
        MUSBFS_W8(MGC_O_FS_INTRTX1E,
                  MUSBFS_R8(MGC_O_FS_INTRTX1E) | (uint8_t)(1u << idx));
    }
    fs_set_active_ep(old_idx);

    g_fs_hcd.fifo_used = (uint16_t)(base + size);
    FS_LOGI("fs pipe alloc ep%02x type:%d -> hw ep%d %s fifo:%d/%d\r\n",
            ep_addr, ep_type, idx, dir ? "IN" : "OUT", base, size);
    return pipe;
}

/* start a bulk/int transfer: IN arms REQPKT (core retries NAKs while
 * the URB pends), OUT loads the first MPS-sized chunk (Mentor StartTx /
 * StartRx sequences) */
static void fs_bulk_pipe_start(struct fs_pipe *pipe, struct usbh_urb *urb)
{
    uint8_t old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
    uint8_t idx = pipe->ep_local_index;
    uint8_t csr1;

    fs_set_active_ep(idx);
    MUSBFS_W8(MGC_O_FS_FADDR, pipe->dev_addr);

    if (pipe->ep_addr & 0x80) {
        /* host IN: rewrite the token type, then arm the IN token */
        MUSBFS_W8(MGC_O_FS_RXTYPE, fs_type_reg(pipe));
        csr1 = MUSBFS_R8(MGC_O_FS_RXCSR1);
        csr1 |= MGC_M_FS_RXCSR1_H_REQPKT;
        csr1 &= (uint8_t)~MGC_M_FS_RXCSR1_RXPKTRDY;
        MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
    } else {
        uint16_t size = FS_MIN(urb->transfer_buffer_length, pipe->ep_mps);

        MUSBFS_W8(MGC_O_FS_TXTYPE, fs_type_reg(pipe));
        if (size) {
            fs_write_packet(idx, urb->transfer_buffer, size);
        }
        urb->transfer_buffer += size;
        urb->transfer_buffer_length -= size;
        urb->actual_length += size;
        MUSBFS_W8(MGC_O_FS_TXCSR1, MGC_M_FS_TXCSR1_TXPKTRDY);
        MUSBFS_W8(MGC_O_FS_TXCSR2,
                  MUSBFS_R8(MGC_O_FS_TXCSR2) | MGC_M_FS_TXCSR2_MODE);
    }
    fs_set_active_ep(old_idx);
}

/* start a control transfer: address the target (single FADDR register,
 * selected via INDEX 0), set the token speed, push the 8-byte setup and
 * mark it SETUPPKT|TXPKTRDY. No hub address programming: an FS-only
 * topology has no split transactions and the FDRC has no hub page. */
static void fs_control_pipe_init(struct fs_pipe *pipe, struct usb_setup_packet *setup)
{
    fs_set_active_ep(0);
    MUSBFS_W8(MGC_O_FS_FADDR, pipe->dev_addr);
    /* Repro bring-up: FDRC needs a short settle after FADDR changes before
     * the next token is issued; without this the post-SET_ADDRESS transfer
     * times out (enumeration fails with -3). */
    delay(50);
    MUSBFS_W8(MGC_O_FS_TXTYPE, pipe->speed); /* proto=control, remote ep 0 */

    FS_LOGI("ep0 setup: addr:%02x type:%02x req:%02x wVal:%04x wIdx:%04x wLen:%d\r\n",
            pipe->dev_addr, setup->bmRequestType, setup->bRequest,
            setup->wValue, setup->wIndex, setup->wLength);
    fs_write_packet(0, (const uint8_t *)setup, 8);
    MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_TXPKTRDY | MGC_M_FS_CSR0_H_SETUPPKT);
}

/* ep0 state machine, driven from the TX1 interrupt source (bit0).
 * Mirrors the HDRC host handler: SETUP -> (IN|OUT)_DATA -> STATUS. */
static void fs_ep0_isr_handler(uint8_t ep0_intr)
{
    struct fs_pipe *pipe = &g_fs_hcd.pipe_pool[0][0];
    struct usbh_urb *urb = pipe->urb;
    uint16_t size;
    uint8_t old_idx;
    uint8_t csr0;

    (void)ep0_intr;
    if (!urb) {
        return; /* spurious (e.g. leftover from an aborted transfer) */
    }

    old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
    fs_set_active_ep(0);
    csr0 = MUSBFS_R8(MGC_O_FS_CSR0);

    FS_LOGI("ep0 isr: csr0:%02x state:%d\r\n", csr0, g_fs_ep0_state);

    if (csr0 & MGC_M_FS_CSR0_H_RXSTALL) {
        MUSBFS_W8(MGC_O_FS_CSR0, csr0 & ~MGC_M_FS_CSR0_H_RXSTALL);
        FS_LOGW("ep0 stall\r\n");
        urb->errorcode = -EPERM;
        goto out_err;
    }
    if (csr0 & MGC_M_FS_CSR0_H_ERROR) {
        MUSBFS_W8(MGC_O_FS_CSR0, csr0 & ~MGC_M_FS_CSR0_H_ERROR);
        MUSBFS_W8(MGC_O_FS_CSR02, MGC_M_FS_CSR02_FLUSHFIFO);
        FS_LOGW("ep0 error, flush\r\n");
        urb->errorcode = -EIO;
        goto out_err;
    }
    if (csr0 & MGC_M_FS_CSR0_H_NAKTIMEOUT) {
        MUSBFS_W8(MGC_O_FS_CSR0, csr0 & ~MGC_M_FS_CSR0_H_NAKTIMEOUT);
        FS_LOGW("ep0 nak timeout\r\n");
        urb->errorcode = -EBUSY; /* core's control retry loop takes over */
        goto out_err;
    }

    switch (g_fs_ep0_state) {
    case FS_EP0_STATE_SETUP:
        urb->actual_length += 8;
        if (urb->transfer_buffer_length) {
            if (urb->setup->bmRequestType & 0x80) {
                g_fs_ep0_state = FS_EP0_STATE_IN_DATA;
                MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_H_REQPKT);
            } else {
                g_fs_ep0_state = FS_EP0_STATE_OUT_DATA;
                size = urb->transfer_buffer_length;
                if (size > pipe->ep_mps) {
                    size = pipe->ep_mps;
                }
                fs_write_packet(0, urb->transfer_buffer, size);
                MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_TXPKTRDY);

                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;
            }
        } else {
            g_fs_ep0_state = FS_EP0_STATE_IN_STATUS;
            MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_H_REQPKT | MGC_M_FS_CSR0_H_STATUSPKT);
        }
        break;
    case FS_EP0_STATE_IN_DATA:
        if (csr0 & MGC_M_FS_CSR0_RXPKTRDY) {
            size = urb->transfer_buffer_length;
            if (size > pipe->ep_mps) {
                size = pipe->ep_mps;
            }
            size = FS_MIN(size, MUSBFS_R16(MGC_O_FS_COUNT0));
            fs_read_packet(0, urb->transfer_buffer, size);
            MUSBFS_W8(MGC_O_FS_CSR0, csr0 & ~MGC_M_FS_CSR0_RXPKTRDY);

            urb->transfer_buffer += size;
            urb->transfer_buffer_length -= size;
            urb->actual_length += size;

            if ((size < pipe->ep_mps) || (urb->transfer_buffer_length == 0)) {
                g_fs_ep0_state = FS_EP0_STATE_OUT_STATUS;
                MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_TXPKTRDY | MGC_M_FS_CSR0_H_STATUSPKT);
            } else {
                MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_H_REQPKT);
            }
        }
        break;
    case FS_EP0_STATE_OUT_DATA:
        if (urb->transfer_buffer_length > 0) {
            size = urb->transfer_buffer_length;
            if (size > pipe->ep_mps) {
                size = pipe->ep_mps;
            }
            fs_write_packet(0, urb->transfer_buffer, size);
            MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_TXPKTRDY);

            urb->transfer_buffer += size;
            urb->transfer_buffer_length -= size;
            urb->actual_length += size;
        } else {
            g_fs_ep0_state = FS_EP0_STATE_IN_STATUS;
            MUSBFS_W8(MGC_O_FS_CSR0, MGC_M_FS_CSR0_H_REQPKT | MGC_M_FS_CSR0_H_STATUSPKT);
        }
        break;
    case FS_EP0_STATE_OUT_STATUS:
        urb->errorcode = 0;
        FS_LOGI("ep0 done(out-status): len:%d\r\n", urb->actual_length);
        fs_pipe_waitup(pipe);
        break;
    case FS_EP0_STATE_IN_STATUS:
        if (csr0 & (MGC_M_FS_CSR0_RXPKTRDY | MGC_M_FS_CSR0_H_STATUSPKT)) {
            MUSBFS_W8(MGC_O_FS_CSR0, csr0 & ~(MGC_M_FS_CSR0_RXPKTRDY | MGC_M_FS_CSR0_H_STATUSPKT));
            urb->errorcode = 0;
            FS_LOGI("ep0 done(in-status): len:%d\r\n", urb->actual_length);
            fs_pipe_waitup(pipe);
        }
        break;
    default:
        break;
    }

    fs_set_active_ep(old_idx);
    return;

out_err:
    g_fs_ep0_state = FS_EP0_STATE_SETUP;
    fs_pipe_waitup(pipe);
    fs_set_active_ep(old_idx);
}

/* bulk/int TX (host OUT) completion state machine. FDRC TX side has no
 * NAK-timeout bit: a NAK'd OUT is retried by the core (TXINTERVAL=0) and
 * only STALL / bus ERROR terminate here. */
static void fs_tx_isr_handler(uint16_t tx_intr)
{
    for (uint8_t i = 1; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        struct fs_pipe *pipe = &g_fs_hcd.pipe_pool[i][0];
        struct usbh_urb *urb;
        uint8_t old_idx;
        uint8_t csr1;

        if (!(tx_intr & (1u << i))) {
            continue;
        }
        old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
        fs_set_active_ep(i);
        csr1 = MUSBFS_R8(MGC_O_FS_TXCSR1);

        urb = pipe->urb;
        if (!urb) {
            /* spurious (aborted/late): just flush the FIFO */
            MUSBFS_W8(MGC_O_FS_TXCSR1, MGC_M_FS_TXCSR1_FLUSHFIFO);
            fs_set_active_ep(old_idx);
            continue;
        }

        if (csr1 & MGC_M_FS_TXCSR1_H_RXSTALL) {
            MUSBFS_W8(MGC_O_FS_TXCSR1, csr1 & (uint8_t)~MGC_M_FS_TXCSR1_H_RXSTALL);
            FS_LOGW("fs tx ep%d stall\r\n", i);
            urb->errorcode = -EPERM;
            fs_pipe_waitup(pipe);
        } else if (csr1 & MGC_M_FS_TXCSR1_H_ERROR) {
            MUSBFS_W8(MGC_O_FS_TXCSR1, csr1 & (uint8_t)~MGC_M_FS_TXCSR1_H_ERROR);
            MUSBFS_W8(MGC_O_FS_TXCSR1, MGC_M_FS_TXCSR1_FLUSHFIFO);
            FS_LOGW("fs tx ep%d error\r\n", i);
            urb->errorcode = -EIO;
            fs_pipe_waitup(pipe);
        } else {
            /* previous packet left the FIFO; feed the next chunk or finish */
            if (urb->transfer_buffer_length > 0) {
                uint16_t size = FS_MIN(urb->transfer_buffer_length, pipe->ep_mps);

                fs_write_packet(i, urb->transfer_buffer, size);
                urb->transfer_buffer += size;
                urb->transfer_buffer_length -= size;
                urb->actual_length += size;
                MUSBFS_W8(MGC_O_FS_TXCSR1, MGC_M_FS_TXCSR1_TXPKTRDY);
            } else {
                urb->errorcode = 0;
                fs_pipe_waitup(pipe);
            }
        }
        fs_set_active_ep(old_idx);
    }
}

/* bulk/int RX (host IN) completion state machine. Completion follows the
 * CherryUSB contract: short packet (< mps) or requested length reached;
 * an incomplete transfer re-arms REQPKT together with the RXPKTRDY clear
 * (single write, Mentor StartRx style). DATAERR doubles as the nak
 * timeout on the RX side; with RXINTERVAL=0 it should never fire. */
static void fs_rx_isr_handler(uint16_t rx_intr)
{
    for (uint8_t i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        struct fs_pipe *pipe = &g_fs_hcd.pipe_pool[i][1];
        struct usbh_urb *urb;
        uint8_t old_idx;
        uint8_t csr1;
        uint16_t count;
        uint16_t size;

        if (!(rx_intr & (1u << i))) {
            continue;
        }
        old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
        fs_set_active_ep(i);
        csr1 = MUSBFS_R8(MGC_O_FS_RXCSR1);

        urb = pipe->urb;
        if (!urb) {
            /* spurious: drain any resident packet and clear RXPKTRDY */
            if (csr1 & MGC_M_FS_RXCSR1_RXPKTRDY) {
                fs_drain_packet(i, MUSBFS_R16(MGC_O_FS_RXCOUNT1));
                MUSBFS_W8(MGC_O_FS_RXCSR1, csr1 & (uint8_t)~MGC_M_FS_RXCSR1_RXPKTRDY);
            }
            fs_set_active_ep(old_idx);
            continue;
        }

        if (csr1 & (MGC_M_FS_RXCSR1_H_ERROR | MGC_M_FS_RXCSR1_DATAERR)) {
            /* abort the pending token first, then reset the error bits */
            csr1 &= (uint8_t)~MGC_M_FS_RXCSR1_H_REQPKT;
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
            csr1 &= (uint8_t)~(MGC_M_FS_RXCSR1_H_ERROR | MGC_M_FS_RXCSR1_DATAERR);
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
            FS_LOGW("fs rx ep%d error/dataerr\r\n", i);
            urb->errorcode = -EIO;
            fs_pipe_waitup(pipe);
            fs_set_active_ep(old_idx);
            continue;
        }
        if (csr1 & MGC_M_FS_RXCSR1_H_RXSTALL) {
            csr1 &= (uint8_t)~MGC_M_FS_RXCSR1_H_REQPKT;
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
            csr1 &= (uint8_t)~MGC_M_FS_RXCSR1_H_RXSTALL;
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
            FS_LOGW("fs rx ep%d stall\r\n", i);
            urb->errorcode = -EPERM;
            fs_pipe_waitup(pipe);
            fs_set_active_ep(old_idx);
            continue;
        }

        if (!(csr1 & MGC_M_FS_RXCSR1_RXPKTRDY)) {
            fs_set_active_ep(old_idx);
            continue;
        }

        count = MUSBFS_R16(MGC_O_FS_RXCOUNT1);
        size = FS_MIN(count, urb->transfer_buffer_length);
        fs_read_packet(i, urb->transfer_buffer, size);
        urb->transfer_buffer += size;
        urb->transfer_buffer_length -= size;
        urb->actual_length += size;
        if (count > size) {
            /* device overran the request: discard the tail */
            fs_drain_packet(i, count - size);
        }

        if ((size < pipe->ep_mps) || (urb->transfer_buffer_length == 0)) {
            /* short packet or requested length reached */
            csr1 = MUSBFS_R8(MGC_O_FS_RXCSR1);
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1 & (uint8_t)~MGC_M_FS_RXCSR1_RXPKTRDY);
            urb->errorcode = 0;
            fs_pipe_waitup(pipe);
        } else {
            /* more to go: clear RXPKTRDY and re-arm the IN token at once */
            csr1 = MUSBFS_R8(MGC_O_FS_RXCSR1);
            csr1 |= MGC_M_FS_RXCSR1_H_REQPKT;
            csr1 &= (uint8_t)~MGC_M_FS_RXCSR1_RXPKTRDY;
            MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
        }
        fs_set_active_ep(old_idx);
    }
}

static void fs_usb_isr(void)
{
    uint8_t intrusb;
    uint16_t tx_intr, rx_intr;

    /* order: INTRUSB first (connect/disconnect edges), then TX, then RX */
    intrusb = MUSBFS_R8(MGC_O_FS_INTRUSB);
    tx_intr = MUSBFS_R16(MGC_O_FS_INTRTX1);
    rx_intr = MUSBFS_R16(MGC_O_FS_INTRRX1);

    if (intrusb & MGC_M_FS_INTR_CONNECT)
        fs_handle_connect();
    if (intrusb & MGC_M_FS_INTR_DISCONNECT)
        fs_handle_disconnect();
    if (intrusb & MGC_M_FS_INTR_RESET) {
        /* host-mode babble/reset: clear and re-kick the session */
        FS_LOGW("fs bus reset/babble\r\n");
    }
    if (intrusb & MGC_M_FS_INTR_VBUSERROR) {
        FS_LOGW("fs vbus error\r\n");
    }
    /* suspend/resume/SOF: not tracked by the skeleton */

    if (tx_intr & 0x01)
        fs_ep0_isr_handler(tx_intr);
    if (tx_intr & 0xFFFE)
        fs_tx_isr_handler(tx_intr);
    if (rx_intr)
        fs_rx_isr_handler(rx_intr);

    /* reading INTRUSB/TX/RX clears the latched sources (data book style) */
}

/* ------------------------------------------------------------------ */
/* Contract: usb_hc_init / usb_hc_deinit (busid == BK_FS_USB_BUS_ID)   */
/* ------------------------------------------------------------------ */
int usb_hc_beken_fs_init(struct usbh_bus *bus)
{
    int ret;
    uint8_t devctl;
    uint32_t i;

    if (bus->busid != BK_FS_USB_BUS_ID) {
        return -EINVAL;
    }

    memset(&g_fs_hcd, 0, sizeof(g_fs_hcd));

    for (i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        g_fs_hcd.pipe_pool[i][0].waitsem = usb_osal_sem_create(0);
        if (g_fs_hcd.pipe_pool[i][0].waitsem == NULL)
            return -ENOMEM;
        g_fs_hcd.pipe_pool[i][1].waitsem = usb_osal_sem_create(0);
        if (g_fs_hcd.pipe_pool[i][1].waitsem == NULL)
            return -ENOMEM;
    }

    ret = fs_low_level_init();
    if (ret != 0)
        return ret;

    /* ep0 dynamic FIFO: single 64-byte buffer at offset 0 (both the TX
     * and RX views of the shared ep0 FIFO); bulk/int pipes allocate
     * above it starting at FS_FIFO_EP0_SIZE. */
    {
        uint8_t old_idx = MUSBFS_R8(MGC_O_FS_INDEX);

        fs_set_active_ep(0);
        MUSBFS_W8(MGC_O_FS_TXFIFO1, 0);
        MUSBFS_W8(MGC_O_FS_TXFIFO2, FS_FIFO_EP0_CODE << MGC_S_FS_FIFO_SZ);
        MUSBFS_W8(MGC_O_FS_RXFIFO1, 0);
        MUSBFS_W8(MGC_O_FS_RXFIFO2, FS_FIFO_EP0_CODE << MGC_S_FS_FIFO_SZ);
        fs_set_active_ep(old_idx);
    }
    g_fs_hcd.fifo_used = FS_FIFO_EP0_SIZE;

    /* core: no HSENAB/chirp on FDRC; enable ep0 + bus interrupts */
    MUSBFS_W8(MGC_O_FS_INTRTX1E, 0x01); /* ep0 only; per-ep on pipe alloc */
    MUSBFS_W8(MGC_O_FS_INTRTX2E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRRX1E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRRX2E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRUSBE,
              MGC_M_FS_INTR_SUSPEND | MGC_M_FS_INTR_RESUME | MGC_M_FS_INTR_RESET |
              MGC_M_FS_INTR_CONNECT | MGC_M_FS_INTR_DISCONNECT);

    /* ISR: route to AP + register + enable */
    fs_irq_route_to_ap();
    bk_int_isr_register(INT_SRC_USB_FS, fs_usb_isr, NULL);
    bk_int_set_priority(INT_SRC_USB_FS, 2);
    fs_irq_enable(true);

    /* pre-attached device kick: the attach edge may have happened while
     * the controller was off. Assert SESSION so connect detection
     * restarts (verified fix for the ret=-6 open timeout class of bug). */
    devctl = MUSBFS_R8(MGC_O_FS_DEVCTL);
    if ((devctl & MGC_M_FS_DEVCTL_VBUS_M) >= MGC_M_FS_DEVCTL_VBUS_SEND) {
        MUSBFS_W8(MGC_O_FS_DEVCTL, devctl | MGC_M_FS_DEVCTL_SESSION);
        FS_LOGI("session kicked for pre-attached device (devctl 0x%02x)\r\n", devctl);
    }

    FS_LOGI("fs host up on bus %d\r\n", bus->busid);
    return 0;
}

int usb_hc_beken_fs_deinit(struct usbh_bus *bus)
{
    uint32_t i;

    if (bus->busid != BK_FS_USB_BUS_ID) {
        return -EINVAL;
    }

    fs_irq_enable(false);
    bk_int_isr_unregister(INT_SRC_USB_FS);

    MUSBFS_W8(MGC_O_FS_INTRTX1E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRRX1E, 0x00);
    MUSBFS_W8(MGC_O_FS_INTRUSBE, 0x00);

    fs_low_level_deinit();

    for (i = 0; i < CONFIG_USBHOST_PIPE_NUM; i++) {
        if (g_fs_hcd.pipe_pool[i][0].waitsem)
            usb_osal_sem_delete(g_fs_hcd.pipe_pool[i][0].waitsem);
        if (g_fs_hcd.pipe_pool[i][1].waitsem)
            usb_osal_sem_delete(g_fs_hcd.pipe_pool[i][1].waitsem);
    }
    memset(&g_fs_hcd, 0, sizeof(g_fs_hcd));
    /* Repro fix: without this, a close->open cycle leaves stale hport->pipe
     * mappings; on the next enumeration the map hits a cleared pipe with the
     * old dev_addr, and control transfers time out (-3). */
    fs_ep_pipe_map_clear();
    return 0;
}

/* ------------------------------------------------------------------ */
/* Contract: roothub control (single port)                             */
/*                                                                     */
/* Dispatch mirrors the HS port: recipient==device -> hub-level        */
/* requests, recipient==other -> port requests. Feature selector       */
/* numbers double as status/change bit positions (see usb_hub.h).     */
/* ------------------------------------------------------------------ */
int usbh_beken_fs_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf)
{
    uint8_t nports = 1;
    uint8_t port = setup->wIndex;
    uint32_t status;

    if (setup->bmRequestType & USB_REQUEST_RECIPIENT_DEVICE) {
        switch (setup->bRequest) {
        case HUB_REQUEST_CLEAR_FEATURE:
        case HUB_REQUEST_SET_FEATURE:
            /* hub-level local power / over-current: nothing to do */
            return 0;
        case HUB_REQUEST_GET_STATUS:
            memset(buf, 0, 4); /* hub status: no changes, no power issues */
            return 0;
        default:
            return -EPIPE;
        }
    }

    /* port requests (recipient == other) */
    if (port == 0 || port > nports) {
        return -EPIPE;
    }

    if (setup->bmRequestType & USB_REQUEST_DIR_IN) {
        switch (setup->bRequest) {
        case HUB_REQUEST_GET_STATUS:
            status = 0;
            /* host mode powers the port unconditionally (VBUS LDO on),
             * so PORT_POWER must always be reported -- usbh_hub_events()
             * derives the speed only after seeing POWER set, else it
             * bails out with "does not enable power" after reset. */
            status |= (1 << HUB_PORT_FEATURE_POWER);
            if (g_fs_hcd.port_pe) {
                status |= (1 << HUB_PORT_FEATURE_CONNECTION) | (1 << HUB_PORT_FEATURE_ENABLE);
                if (fs_get_port_speed() == USB_SPEED_LOW)
                    status |= (1 << HUB_PORT_FEATURE_LOWSPEED);
            }
            if (g_fs_hcd.port_csc)
                status |= (1 << HUB_PORT_FEATURE_C_CONNECTION);
            if (g_fs_hcd.port_pec)
                status |= (1 << HUB_PORT_FEATURE_C_ENABLE);
            if (g_fs_hcd.port_crc) /* set by port reset, cleared below */
                status |= (1 << HUB_PORT_FEATURE_C_RESET);
            FS_LOGI("roothub get_status port%d: %08x\r\n", port, status);
            memcpy(buf, &status, 4);
            return 0;
        case HUB_REQUEST_GET_DESCRIPTOR:
            if (setup->wValue == (uint16_t)((HUB_DESCRIPTOR_TYPE_HUB << 8) | 0)) {
                /* FS roothub: 1 port, ganged power, no TT descriptors */
                static const uint8_t hub_desc[] = {
                    0x09,       /* bDescLength */
                    0x29,       /* bDescriptorType: hub */
                    0x01,       /* bNrPorts */
                    0x09, 0x00, /* wHubCharacteristics: ganged power, no OC */
                    0x32,       /* bPwrOn2PwrGood: 100ms (units of 2ms) */
                    0x10,       /* bHubContrCurrent: 16mA */
                    0x00,       /* DeviceRemovable */
                    0x00,       /* PortPwrCtrlMask */
                };
                memcpy(buf, hub_desc, sizeof(hub_desc));
                return 0;
            }
            return -EPIPE;
        default:
            return -EPIPE;
        }
    } else {
        switch (setup->bRequest) {
        case HUB_REQUEST_CLEAR_FEATURE:
            switch (setup->wValue) {
            case HUB_PORT_FEATURE_C_CONNECTION:
                g_fs_hcd.port_csc = 0;
                break;
            case HUB_PORT_FEATURE_C_ENABLE:
                g_fs_hcd.port_pec = 0;
                break;
            case HUB_PORT_FEATURE_C_RESET:
                g_fs_hcd.port_crc = 0;
                break;
            case HUB_PORT_FEATURE_ENABLE:
            case HUB_PORT_FEATURE_SUSPEND:
            case HUB_PORT_FEATURE_POWER:
                break;
            default:
                return -EPIPE;
            }
            return 0;
        case HUB_REQUEST_SET_FEATURE:
            switch (setup->wValue) {
            case HUB_PORT_FEATURE_RESET:
                /* FDRC host bus reset: POWER.RESET held >=10ms.
                 * delay() calibration per the verified mentor sequence. */
                FS_LOGI("roothub port reset\r\n");
                MUSBFS_W8(MGC_O_FS_POWER, MUSBFS_R8(MGC_O_FS_POWER) | MGC_M_FS_POWER_RESET);
                delay(500);
                MUSBFS_W8(MGC_O_FS_POWER, MUSBFS_R8(MGC_O_FS_POWER) & ~MGC_M_FS_POWER_RESET);
                g_fs_hcd.port_pe = 1;
                g_fs_hcd.port_pec = 1;
                g_fs_hcd.port_crc = 1; /* report C_RESET until cleared */
                return 0;
            case HUB_PORT_FEATURE_POWER:
                /* VBUS LDO is driver-layer owned; nothing to do here */
                return 0;
            case HUB_PORT_FEATURE_SUSPEND:
                /* TODO-3: FDRC has no dedicated host-suspend command bit
                 * (POWER.SUSPENDM is a status bit); bus simply idles when
                 * no tokens are issued. Kept as a no-op for the skeleton. */
                return 0;
            default:
                return -EPIPE;
            }
        default:
            return -EPIPE;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Contract: submit/kill urb                                           */
/*  - control: ep0 state machine (TODO-3, silicon verified)            */
/*  - bulk/int: dynamic-FIFO pipes + TX/RX CSR state machines (TODO-4) */
/* ------------------------------------------------------------------ */
int usbh_beken_fs_submit_urb(struct usbh_urb *urb)
{
    struct fs_pipe *pipe;
    int ret = 0;

    if (!urb) {
        return -EINVAL;
    }
    /* no device / no port: gone device raises no completion event, so a
     * submit would burn the full URB timeout (same guard as HS port) */
    if (!g_fs_hcd.port_pe) {
        return -ENODEV;
    }

    /* FS port is single-device only: reject transfers to devices hanging
     * off an external hub (their hport->parent is the hub, not the roothub).
     * The roothub itself has parent == NULL. */
    if (urb->hport && urb->hport->parent && urb->hport->parent->parent) {
        FS_LOGW("fs: external-hub downstream device not supported (hport %p)\r\n",
                urb->hport);
        return -ENODEV;
    }

    pipe = (struct fs_pipe *)urb->hcpriv;
    if (!pipe && urb->hport && urb->ep) {
        uint8_t ep_type = (uint8_t)(urb->ep->bmAttributes & USB_ENDPOINT_TYPE_MASK);

        pipe = fs_ep_pipe_map_find(urb->hport, urb->ep->bEndpointAddress, ep_type);
        if (pipe) {
            urb->hcpriv = pipe;
        }
    }
    if (!pipe && urb->hport && urb->ep) {
        uint8_t ep_type = (uint8_t)(urb->ep->bmAttributes & USB_ENDPOINT_TYPE_MASK);

        if (ep_type == USB_ENDPOINT_TYPE_CONTROL) {
            pipe = &g_fs_hcd.pipe_pool[0][0]; /* single ep0 slot */
            if (pipe->inuse) {
                return -EBUSY;
            }
            pipe->inuse = true;
            pipe->hport = urb->hport;
        } else if ((ep_type == USB_ENDPOINT_TYPE_BULK)
                   || (ep_type == USB_ENDPOINT_TYPE_INTERRUPT)) {
            pipe = fs_bulk_pipe_alloc(urb);
            if (!pipe) {
                FS_LOGW("fs pipe alloc failed ep%02x (pool/FIFO exhausted)\r\n",
                        urb->ep->bEndpointAddress);
                return -EBUSY;
            }
        } else {
            FS_LOGW("fs submit ep%02x type:%d unsupported (iso)\r\n",
                    urb->ep->bEndpointAddress, ep_type);
            return -ENODEV;
        }
        urb->hcpriv = pipe;
        fs_ep_pipe_map_add(urb->hport, urb->ep->bEndpointAddress, ep_type, pipe);
    }
    if (!pipe) {
        return -EINVAL;
    }

    /* refresh per-transfer target state (address/speed may have changed
     * via SET_ADDRESS or a reset-enumerate cycle since the last submit) */
    if (urb->hport && urb->ep) {
        pipe->dev_addr = urb->hport->dev_addr;
        pipe->ep_mps = (uint16_t)(urb->ep->wMaxPacketSize & USB_MAXPACKETSIZE_MASK);
        pipe->ep_type = (uint8_t)(urb->ep->bmAttributes & USB_ENDPOINT_TYPE_MASK);
        pipe->speed = (urb->hport->speed == USB_SPEED_LOW)
                      ? MGC_V_FS_SPEED_LOW : MGC_V_FS_SPEED_FULL;
    }

    if (!pipe->hport || !pipe->hport->connected) {
        return -ENODEV;
    }
    if (pipe->urb) {
        return -EBUSY;
    }

    uint32_t flags = usb_enter_critical();

    pipe->waiter = false;
    pipe->urb = urb;
    urb->errorcode = -EBUSY;
    urb->actual_length = 0;
    if (urb->timeout > 0) {
        pipe->waiter = true;
    }

    if (pipe->ep_type == USB_ENDPOINT_TYPE_CONTROL) {
        g_fs_ep0_state = FS_EP0_STATE_SETUP;
        fs_control_pipe_init(pipe, urb->setup);
    } else {
        fs_bulk_pipe_start(pipe, urb);
    }

    usb_exit_critical(flags);

    if (urb->timeout > 0) {
        ret = usb_osal_sem_take(pipe->waitsem, urb->timeout);
        if (ret < 0) {
            pipe->waiter = false;
            usbh_beken_fs_kill_urb(urb);
            return ret;
        }
        ret = urb->errorcode;
    }
    return ret;
}

int usbh_beken_fs_kill_urb(struct usbh_urb *urb)
{
    struct fs_pipe *pipe;
    uint8_t old_idx;

    if (!urb) {
        return -EINVAL;
    }
    pipe = (struct fs_pipe *)urb->hcpriv;
    if (!pipe) {
        return -EINVAL;
    }

    if (pipe->ep_type == USB_ENDPOINT_TYPE_CONTROL) {
        old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
        fs_set_active_ep(0);
        /* drop any pending token and flush the ep0 FIFO */
        MUSBFS_W8(MGC_O_FS_CSR0, 0);
        MUSBFS_W8(MGC_O_FS_CSR02, MGC_M_FS_CSR02_FLUSHFIFO);
        fs_set_active_ep(old_idx);

        if (pipe->urb) {
            pipe->urb->errorcode = -ESHUTDOWN;
            fs_pipe_waitup(pipe);
        }
        g_fs_ep0_state = FS_EP0_STATE_SETUP;
        return 0;
    }

    /* bulk/int: disarm the pending token and flush the endpoint. Do NOT
     * touch CLRDATATOG here: a NAK-timeout kill means no packet was ACKed,
     * so neither side advanced its data toggle -- resetting only the host
     * toggle desyncs the pair, and the next transfer is dropped by the core
     * as a duplicate (observed: 37B CDC echo ACKed on the wire, device saw
     * TXEMPTY, host URB timed out with 0 bytes). Toggle is reset at pipe
     * alloc (fresh endpoint = DATA0) which is the only place that needs it. */
    old_idx = MUSBFS_R8(MGC_O_FS_INDEX);
    fs_set_active_ep(pipe->ep_local_index);
    if (pipe->ep_addr & 0x80) {
        uint8_t csr1 = MUSBFS_R8(MGC_O_FS_RXCSR1);

        csr1 &= (uint8_t)~(MGC_M_FS_RXCSR1_H_REQPKT
                           | MGC_M_FS_RXCSR1_H_ERROR
                           | MGC_M_FS_RXCSR1_DATAERR);
        MUSBFS_W8(MGC_O_FS_RXCSR1, csr1);
        MUSBFS_W8(MGC_O_FS_RXCSR1, MGC_M_FS_RXCSR1_FLUSHFIFO);
    } else {
        MUSBFS_W8(MGC_O_FS_TXCSR1, MGC_M_FS_TXCSR1_FLUSHFIFO);
    }
    fs_set_active_ep(old_idx);

    if (pipe->urb) {
        pipe->urb->errorcode = -ESHUTDOWN;
        fs_pipe_waitup(pipe);
    }
    return 0;
}

#endif /* CONFIG_USB_FS_HOST */
