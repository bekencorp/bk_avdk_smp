/*
 * Copyright (c) 2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7259 FS/LS USB IP register definitions (Mentor MUSB compact FDRC core).
 *
 * Provenance:
 *  - Core register offsets/bit fields: transcribed from the Mentor stack
 *    headers (mu_fdrdf.h / mu_drcdf.h) which match this IP generation
 *    (BK3253-era compact FDRC, MUSB_FDRC_AHB variant: FIFO regs are
 *    word-indexed, 0x20 + 4*ep).
 *  - Wrapper register block (datasheet word indices 0x8A0-0x8Aa) and the
 *    bring-up sequence: hardware-validated by the Mentor CDC host bring-up
 *    (bk_mentor_usb_drv.c, 2026-10-29 "MUSB_UcdcConnect" on silicon).
 *
 * Layout difference vs the HS IP (beken_musb, HDRC): the FDRC core has no
 * per-endpoint TxFuncAddr/HubAddr register page and no 0x60 DEVCTL --
 * target address is multiplexed through FADDR after INDEX selection, and
 * DEVCTL sits at 0x0F.
 */
#ifndef USB_BEKEN_FS_MUSB_REG_H
#define USB_BEKEN_FS_MUSB_REG_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Base addresses                                                      */
/* ------------------------------------------------------------------ */
#include <soc/soc.h> /* SOC_USB_FS_BASE / SOC_SYS_REG_BASE */

#define MUSBFS_BASE                 (SOC_USB_FS_BASE)          /* 0x48090000 */
#define MUSBFS_WRAPPER_OFF_PRIMARY  (0x2280) /* datasheet word idx 0x8A0 * 4 */
#define MUSBFS_WRAPPER_OFF_FALLBACK (0x8A0)  /* literal byte offset family */

/* ------------------------------------------------------------------ */
/* Register access helpers (8-bit core register file)                 */
/* ------------------------------------------------------------------ */
#define MUSBFS_R8(off)      (*(volatile uint8_t *)(MUSBFS_BASE + (off)))
#define MUSBFS_W8(off, v)   ((*(volatile uint8_t *)(MUSBFS_BASE + (off))) = (uint8_t)(v))
#define MUSBFS_R16(off)      (*(volatile uint16_t *)(MUSBFS_BASE + (off)))
#define MUSBFS_W16(off, v)   ((*(volatile uint16_t *)(MUSBFS_BASE + (off))) = (uint16_t)(v))

/* FIFO window: FDRC AHB variant -> word indexed (MUSB_FDRC_AHB) */
#define MUSBFS_FIFO_OFFSET   (0x20)
#define MUSBFS_FIFO(ep_idx)  (MUSBFS_BASE + MUSBFS_FIFO_OFFSET + 0x4 * (ep_idx))

/* ------------------------------------------------------------------ */
/* FDRC core register offsets (from mu_fdrdf.h)                       */
/* ------------------------------------------------------------------ */
#define MGC_O_FS_FADDR        0x00  /* 8 bit: target function address (host) */
#define MGC_O_FS_POWER        0x01
#define MGC_O_FS_INTRTX1      0x02
#define MGC_O_FS_INTRTX2      0x03
#define MGC_O_FS_INTRRX1      0x04
#define MGC_O_FS_INTRRX2      0x05
#define MGC_O_FS_INTRUSB      0x06
#define MGC_O_FS_INTRTX1E     0x07
#define MGC_O_FS_INTRTX2E     0x08
#define MGC_O_FS_INTRRX1E     0x09
#define MGC_O_FS_INTRRX2E     0x0A
#define MGC_O_FS_INTRUSBE     0x0B
#define MGC_O_FS_FRAME1       0x0C
#define MGC_O_FS_FRAME2        0x0D
#define MGC_O_FS_INDEX         0x0E
#define MGC_O_FS_DEVCTL        0x0F  /* NOT the HDRC +0x60 layout */

#define MGC_O_FS_TXMAXP        0x10
#define MGC_O_FS_CSR0          0x11  /* alias TXCSR1 */
#define MGC_O_FS_TXCSR1        0x11
#define MGC_O_FS_CSR02         0x12  /* alias TXCSR2 */
#define MGC_O_FS_TXCSR2         0x12
#define MGC_O_FS_RXMAXP        0x13
#define MGC_O_FS_RXCSR1        0x14
#define MGC_O_FS_RXCSR2        0x15
#define MGC_O_FS_COUNT0        0x16  /* alias RXCOUNT1 */
#define MGC_O_FS_RXCOUNT1      0x16
#define MGC_O_FS_RXCOUNT2      0x17
#define MGC_O_FS_TXTYPE        0x18
#define MGC_O_FS_TXINTERVAL    0x19  /* alias NAKLIMIT0 */
#define MGC_O_FS_NAKLIMIT0     0x19
#define MGC_O_FS_RXTYPE        0x1A
#define MGC_O_FS_RXINTERVAL    0x1B
#define MGC_O_FS_TXFIFO1       0x1C  /* lower 8 bits of 8-byte-aligned address */
#define MGC_O_FS_TXFIFO2       0x1D  /* [4] dpb [6:5] size [11:8] addr hi */
#define MGC_O_FS_RXFIFO1       0x1E
#define MGC_O_FS_RXFIFO2       0x1F
#define MGC_O_FS_FIFOSIZE      0x1F  /* no dynamic FIFO sizing variant */
#define MGC_O_FS_CONFIGDATA    0x1F  /* dynamic FIFO sizing variant */

/* ------------------------------------------------------------------ */
/* POWER register (FDRC layout, differs from HDRC!)                    */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_POWER_ENSUSPEND   0x01
#define MGC_M_FS_POWER_SUSPENDM    0x02
#define MGC_M_FS_POWER_RESUME      0x04
#define MGC_M_FS_POWER_RESET       0x08  /* host: generate bus reset */
#define MGC_M_FS_POWER_VBUSLO      0x10
#define MGC_M_FS_POWER_VBUSSESS    0x20
#define MGC_M_FS_POWER_VBUSVAL     0x40
#define MGC_M_FS_POWER_ISOUP       0x80

/* ------------------------------------------------------------------ */
/* INTRUSB bits (mu_drcdf.h)                                          */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_INTR_SUSPEND    0x01
#define MGC_M_FS_INTR_RESUME     0x02
#define MGC_M_FS_INTR_RESET      0x04  /* reset / babble */
#define MGC_M_FS_INTR_SOF        0x08
#define MGC_M_FS_INTR_CONNECT    0x10
#define MGC_M_FS_INTR_DISCONNECT 0x20
#define MGC_M_FS_INTR_SESSREQ    0x40
#define MGC_M_FS_INTR_VBUSERROR  0x80

/* ------------------------------------------------------------------ */
/* DEVCTL bits (mu_drcdf.h)                                           */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_DEVCTL_BDEVICE  0x80
#define MGC_M_FS_DEVCTL_FSDEV    0x40
#define MGC_M_FS_DEVCTL_LSDEV    0x20
#define MGC_M_FS_DEVCTL_VBUS_M   0x18
#define MGC_M_FS_DEVCTL_VBUS_NONE    0x00
#define MGC_M_FS_DEVCTL_VBUS_SEND    0x08
#define MGC_M_FS_DEVCTL_VBUS_AVALID  0x10
#define MGC_M_FS_DEVCTL_VBUS_VALID   0x18
#define MGC_M_FS_DEVCTL_HM       0x04
#define MGC_M_FS_DEVCTL_HOSTREQ  0x02
#define MGC_M_FS_DEVCTL_SESSION  0x01

/* ------------------------------------------------------------------ */
/* CSR0 (ep0) bits (mu_drcdf.h, host mode relevant)                    */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_CSR0_RXPKTRDY      0x0001
#define MGC_M_FS_CSR0_TXPKTRDY      0x0002
#define MGC_M_FS_CSR0_SENTSTALL      0x0004
#define MGC_M_FS_CSR0_H_RXSTALL      0x0004
#define MGC_M_FS_CSR0_DATAEND        0x0008
#define MGC_M_FS_CSR0_H_SETUPPKT     0x0008
#define MGC_M_FS_CSR0_SETUPEND        0x0010
#define MGC_M_FS_CSR0_H_ERROR        0x0010
#define MGC_M_FS_CSR0_P_SENDSTALL    0x0020
#define MGC_M_FS_CSR0_H_REQPKT       0x0020
#define MGC_M_FS_CSR0_P_SVDRXPKTRDY  0x0040
#define MGC_M_FS_CSR0_H_STATUSPKT    0x0040
#define MGC_M_FS_CSR0_P_SVDSETUPEND  0x0080
#define MGC_M_FS_CSR0_H_NAKTIMEOUT   0x0080
#define MGC_M_FS_CSR02_FLUSHFIFO     0x01

/* ------------------------------------------------------------------ */
/* TXCSR1 (mode-agnostic / host)                                       */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_TXCSR1_TXPKTRDY     0x01
#define MGC_M_FS_TXCSR1_FIFONOTEMPTY 0x02
#define MGC_M_FS_TXCSR1_H_ERROR      0x04
#define MGC_M_FS_TXCSR1_FLUSHFIFO    0x08
#define MGC_M_FS_TXCSR1_H_SENDSTALL  0x10  /* slave only */
#define MGC_M_FS_TXCSR1_H_RXSTALL    0x20
#define MGC_M_FS_TXCSR1_CLRDATATOG   0x40

/* TXCSR2 */
#define MGC_M_FS_TXCSR2_DMAMODE   0x04
#define MGC_M_FS_TXCSR2_FRCDT     0x08
#define MGC_M_FS_TXCSR2_DMAENAB   0x10
#define MGC_M_FS_TXCSR2_MODE      0x20  /* 1 = TX (host OUT) */
#define MGC_M_FS_TXCSR2_ISO       0x40
#define MGC_M_FS_TXCSR2_AUTOSET   0x80

/* ------------------------------------------------------------------ */
/* RXCSR1 (mode-agnostic / host)                                       */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_RXCSR1_RXPKTRDY    0x01
#define MGC_M_FS_RXCSR1_FIFOFULL    0x02
#define MGC_M_FS_RXCSR1_H_ERROR     0x04
#define MGC_M_FS_RXCSR1_DATAERR     0x08  /* iso: no data / nak timeout */
#define MGC_M_FS_RXCSR1_FLUSHFIFO   0x10
#define MGC_M_FS_RXCSR1_P_SENDSTALL 0x20  /* slave only */
#define MGC_M_FS_RXCSR1_H_RXSTALL   0x40
#define MGC_M_FS_RXCSR1_CLRDATATOG  0x80
#define MGC_M_FS_RXCSR1_H_REQPKT    0x20  /* host: IN token */

/* RXCSR2 */
#define MGC_M_FS_RXCSR2_DMAMODE     0x10
#define MGC_M_FS_RXCSR2_DMAENAB     0x20
#define MGC_M_FS_RXCSR2_P_ISO        0x40  /* peripheral */
#define MGC_M_FS_RXCSR2_H_AUTOREQ   0x40  /* host */
#define MGC_M_FS_RXCSR2_AUTOCLEAR   0x80

/* ------------------------------------------------------------------ */
/* TxType / RxType encoding                                           */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_TYPE_PROTO      0x30
#define MGC_S_FS_TYPE_PROTO      4
#define MGC_M_FS_TYPE_REMOTE_END 0x0F
#define MGC_M_FS_TYPE_SPEED      0xC0
#define MGC_S_FS_TYPE_SPEED      6
#define MGC_V_FS_SPEED_FULL      0x00  /* 00: full */
#define MGC_V_FS_SPEED_LOW       0x40  /* 01: low */
#define MGC_V_FS_SPEED_HIGH      0x80  /* 10: high (never on this IP) */

#define MGC_V_FS_PROTO_CONTROL   0x00
#define MGC_V_FS_PROTO_ISO       0x10
#define MGC_V_FS_PROTO_BULK      0x20
#define MGC_V_FS_PROTO_INT       0x30

/* ------------------------------------------------------------------ */
/* Dynamic FIFO sizing (TXFIFO2/RXFIFO2)                               */
/* ------------------------------------------------------------------ */
#define MGC_M_FS_FIFO_DPB        0x10  /* double packet buffer */
#define MGC_M_FS_FIFO_SZ         0xE0
#define MGC_S_FS_FIFO_SZ         5
/* size field n -> 2^(n+3) bytes: 3=64B ... 8=2048B (2KB total FIFO) */

/* ------------------------------------------------------------------ */
/* BK7259 FS IP wrapper block (datasheet word indices 0x8A0-0x8Aa,     */
/* 32-bit registers). Byte offset probed at runtime between the two    */
/* candidates below; the wrapper does not decode until GlobalCtrl is   */
/* written 0x3 (soft_resetn released + bps_clkgate open).              */
/* ------------------------------------------------------------------ */
#define MUSBFS_WRAPPER_DEVID     0x55534246u /* "USBF" */

#define FSWRAP_REG(off)  (*(volatile uint32_t *)(MUSBFS_BASE + g_fs_wrapper_off + (off)))

#define FSWRAP_O_DEVICEID     0   /* 0x8A0: "USBF" */
#define FSWRAP_O_VERSIONID    4   /* 0x8A1 */
#define FSWRAP_O_GLOBALCTRL   8   /* 0x8A2: [1] bps_clkgate, [0] soft_resetn */
#define FSWRAP_O_GLOBALSTATUS 12  /* 0x8A3 */
#define FSWRAP_O_OTG_CFG      16  /* 0x8A4: [6] dn_pd [5] dn_pu [4] dp_pd [3] dp_pu [0] usb_mode_sw (0=host) */
#define FSWRAP_O_DMA_ENDP     20  /* 0x8A5 (unused: PIO-only port) */
#define FSWRAP_O_VTH          24  /* 0x8A6: [5] INT_HOST_CONN_EN */
#define FSWRAP_O_GEN         32  /* 0x8A8: [2] vbus_vld [1] vbus_low [0] sess_vld */
#define FSWRAP_O_RESET       40  /* 0x8Aa: [0] usb_rstn_sw (write-1 self-clearing pulse) */

#define FSWRAP_GLBCTRL_RELEASE  0x3  /* soft_resetn=1 + bps_clkgate=1 */
#define FSWRAP_GLBCTRL_HOLD     0x0  /* hold core in reset, clock gated */
#define FSWRAP_OTG_HOST_PD      ((1u << 6) | (1u << 4)) /* dn_pd | dp_pd */
#define FSWRAP_VTH_INT_HOST_CONN_EN (1u << 5)

/* ------------------------------------------------------------------ */
/* SYS-block USB analog registers (BK7259 AP hal sys_drv_usb_analog_* */
/* are empty stubs; program directly via the latch protocol proven on  */
/* the HS path, sys_ana_usb_phy_op()).                                 */
/* ------------------------------------------------------------------ */
#define MUSBFS_ANA_REG14       (SOC_SYS_REG_BASE + (0x4E << 2)) /* VCC enables */
#define MUSBFS_ANA_REG42       (SOC_SYS_REG_BASE + (0x6A << 2)) /* PWD_USB etc. */
#define MUSBFS_ANA_LATCH_REG   (SOC_SYS_REG_BASE + (0x4A << 2))
#define MUSBFS_SYS_OP_STATUS   (SOC_SYS_REG_BASE + (0x3A << 2))

#define MUSBFS_ANA14_VCC_MASK  ((1u << 10) | (1u << 11)) /* EN_USBVCC18 | EN_USBVCC3V */
#define MUSBFS_ANA42_PWD_USB   (1u << 31)
#define MUSBFS_ANA_LATCH_USB   (0x01u << 9)

#endif /* USB_BEKEN_FS_MUSB_REG_H */
