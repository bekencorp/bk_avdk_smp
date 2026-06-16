// Copyright 2026 Beken
//
// dbg_probe module_id namespace (CP side).
// First-level classification (frame byte1). Second-level detail goes in value.
// Segment plan (base aligned to 16, gaps reserved for in-place expansion):
//   0x00        reserved sentinel (unknown/invalid)
//   0x01..0x18  Platform (24)
//   0x20..0x2F  WIFI     (16)
//   0x30..0x37  BT       (8)
//   0xF0..0xFF  framework meta (sync/drop/...)
// CP/AP are independent namespaces (each owns full 256); shared segments keep
// the same base; chip identity is carried by stream/ringbuf-header/sync-frame,
// not by per-frame bits. (See design doc section 4.2 / decision D10.)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DBG_MOD_NONE            0x00u   /* reserved: unknown / placeholder */

/* ---- Platform 0x01..0x18 (24) ---- */
#define DBG_MOD_PLATFORM_BASE   0x01u
#define DBG_MOD_BOOT            0x01u
#define DBG_MOD_KERNEL          0x02u
#define DBG_MOD_SCHED           0x03u
#define DBG_MOD_IPC             0x04u
#define DBG_MOD_PM              0x05u
#define DBG_MOD_MULTICORE       0x06u
#define DBG_MOD_IRQ             0x07u
#define DBG_MOD_MEM             0x08u
#define DBG_MOD_GPIO            0x09u
#define DBG_MOD_DMA             0x0Au
#define DBG_MOD_UART            0x0Bu
#define DBG_MOD_FLASH           0x0Cu
#define DBG_MOD_TIMER           0x0Du
#define DBG_MOD_CLK             0x0Eu
#define DBG_MOD_WDT             0x0Fu
#define DBG_MOD_AON             0x10u
#define DBG_MOD_PSRAM           0x11u
#define DBG_MOD_SPI             0x12u
#define DBG_MOD_I2C             0x13u
#define DBG_MOD_ADC             0x14u
#define DBG_MOD_TRNG            0x15u
#define DBG_MOD_SECURITY        0x16u
#define DBG_MOD_OTA             0x17u
#define DBG_MOD_PLATFORM_MISC   0x18u
/* 0x19..0x1F reserved (Platform overflow guard) */

/* ---- WIFI 0x20..0x2F (16) ---- */
#define DBG_MOD_WIFI_BASE       0x20u
#define DBG_MOD_WIFI_MAC        0x20u
#define DBG_MOD_WIFI_RX         0x21u
#define DBG_MOD_WIFI_TX         0x22u
#define DBG_MOD_WIFI_SCAN       0x23u
#define DBG_MOD_WIFI_CONN       0x24u
#define DBG_MOD_WIFI_PS         0x25u   /* power save */
#define DBG_MOD_WIFI_CAL        0x26u   /* RF calibration */
#define DBG_MOD_WIFI_UMAC       0x27u
#define DBG_MOD_WIFI_LMAC       0x28u
#define DBG_MOD_WIFI_MISC       0x2Fu
/* 0x29..0x2E reserved for further WIFI members */

/* ---- BT 0x30..0x37 (8) ---- */
#define DBG_MOD_BT_BASE         0x30u
#define DBG_MOD_BT_HCI          0x30u
#define DBG_MOD_BT_LL           0x31u   /* link layer */
#define DBG_MOD_BT_GAP          0x32u
#define DBG_MOD_BT_GATT         0x33u
#define DBG_MOD_BT_SMP          0x34u   /* security manager */
#define DBG_MOD_BT_L2CAP        0x35u
#define DBG_MOD_BT_MISC         0x37u
/* 0x36 reserved for further BT members */

/* ---- framework meta 0xF0..0xFF ---- */
#define DBG_MOD_SYNC            0xF0u   /* periodic magic / stream-start marker */
#define DBG_MOD_DROP            0xF1u   /* dropped-frame counter */
#define DBG_MOD_META            0xF2u

#ifdef __cplusplus
}
#endif
