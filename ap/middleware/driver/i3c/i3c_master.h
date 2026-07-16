/**
 * @file i3c_master.h
 * @brief I3C Master: config API + data transfer API (poll/interrupt)
 */
#ifndef I3C_MASTER_H
#define I3C_MASTER_H

#include <stdint.h>
#include <stddef.h>
#include "i3c_common.h"
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * During SDR bus init (RSTDAA, ENTDAA, CCCs), use at most this SCL if target_hz is higher.
 * ENTDAA uses open-drain phases; very high values (e.g. 10 MHz) can fail enumeration.
 * Default 4M lets target_hz up to 4M run full init at one SCL (no prescaler bump + soft reset after CCC).
 * If ENTDAA is unreliable, override to 2000000u before including this header.
 */
#ifndef I3C_MST_SDR_ENUM_MAX_HZ
#define I3C_MST_SDR_ENUM_MAX_HZ  4000000u
#endif

/**
 * Exact SCL target (Hz) for f_core = 98.304 MHz with integer prescaler product = 4, PP_LOW = 0:
 * f_SCL = f_core / 8 = 12.288 MHz. Same hardware setting as CLI "10M" with current truncate rule
 * (10 MHz request also selects product = 4). Use this symbol when you want "12.3 M" in lab terms.
 */
#ifndef I3C_MST_SDR_TARGET_HZ_12M288
#define I3C_MST_SDR_TARGET_HZ_12M288  12288000u
#endif

/* ---------- CCC opcodes, CMD0/CMD1 FIFO patterns, masks (controller UG / I3C spec; used by i3c_master.c) ---------- */
#define I3C_CCC_RSTDAA    0x06u
#define I3C_CCC_ENTDAA    0x07u
#define I3C_CCC_SETDASA    0x87u
#define I3C_CCC_SETNEWDA   0x88u
#define I3C_CCC_SETMWL     0x89u
#define I3C_CCC_SETMRL     0x8Au
#define I3C_CCC_GETMWL     0x8Bu
#define I3C_CCC_GETMRL     0x8Cu
#define I3C_CCC_GETPID     0x8Du
#define I3C_CCC_GETBCR     0x8Eu
#define I3C_CCC_GETDCR     0x8Fu
#define I3C_CCC_ENEC_BC    0xC0u
#define I3C_CCC_ENEC_DIR   0xC1u

#define I3C_MST_CMD1_CCC(ccc)           ((uint32_t)(ccc))
#define I3C_MST_CMD1_CCC_DUP24(ccc)     ((uint32_t)(ccc) | ((uint32_t)(ccc) << 24))
#define I3C_MST_CMD1_HDR_DDR_PREAMBLE   0x00000020u
#define I3C_MST_CMD1_ZERO               0x00000000u
#define I3C_MST_CMD1_ENTDAA             I3C_MST_CMD1_CCC_DUP24(I3C_CCC_ENTDAA)

#define I3C_MST_CMD0_BROADCAST_CCC_PL0       0x60000000u
#define I3C_MST_CMD0_BROADCAST_CCC_PL1       0x60001000u
#define I3C_MST_CMD0_DIRECT_CCC_W_PL1(da)      (0x40001000u | ((uint32_t)(da) << 1))
#define I3C_MST_CMD0_DIRECT_CCC_W_PL2(da)    (0x40002000u | ((uint32_t)(da) << 1))
#define I3C_MST_CMD0_DIRECT_CCC_R_PL1(da)    (0x40001001u | ((uint32_t)(da) << 1))
#define I3C_MST_CMD0_DIRECT_CCC_R_PL2(da)    (0x40002001u | ((uint32_t)(da) << 1))
#define I3C_MST_CMD0_DIRECT_CCC_R_PL6(da)    (0x40006001u | ((uint32_t)(da) << 1))

#define I3C_MST_SDR_CMD0_PRIVATE_BASE   0x1a000000u

#define I3C_MST_CMD0_HDR_DAA            0x60000000u
#define I3C_MST_CMD0_HDR_DDR_TX_FIRST   0x80005000u
#define I3C_MST_CMD0_HDR_DDR_RX_FIRST   0x80001000u
#define I3C_MST_CMD0_HDR_DDR_CHUNK(pl_words)  (0x80000000u | ((uint32_t)(pl_words) << 12))

#define I3C_MST_CMD0_I2C_TX             0x180080a8u
#define I3C_MST_CMD0_I2C_RX             0x180080a9u

#define I3C_MST_ICR_CLEAR_MASK          0x0007FFFFu
#define I3C_MST_TX_RX_THR_DEFAULT       0x00020001u
#define I3C_MST_IER_RX_THR            (1u << 7)

#define I3C_MST_CMDR_ERR_SHIFT          24u
#define I3C_MST_CMDR_ERR_MASK           0xFu
#define I3C_MST_CMDR_ERR_GET(cmdr)      (((uint32_t)(cmdr) >> I3C_MST_CMDR_ERR_SHIFT) & I3C_MST_CMDR_ERR_MASK)

#define I3C_MST_PP_HIGH_LEGACY          0x19u

typedef enum {
	I3C_MST_MODE_SDR = 0,
	I3C_MST_MODE_HDR = 1,
} i3c_master_mode_t;

typedef struct {
	const i3c_platform_config_t *platform;  /**< NULL = default GPIO 32/33/34 */
	i3c_master_mode_t mode;                 /**< SDR or HDR */
	uint8_t clk_div;                        /**< 0 = use target_hz; else prescaler value */
	uint32_t target_hz;                     /**< Target SCL Hz, e.g. 500000/1000000 */
	uint16_t mwl_bytes;                     /**< Device max write length, 0 = default 256 */
	uint16_t mrl_bytes;                     /**< Device max read length, 0 = default 256 */
	i3c_core_clk_src_t core_clk_src;        /**< IP clock: default APLL (reference); XTAL_26M for 26 MHz crystal */
	uint32_t core_hz;                       /**< 0 = default Hz for core_clk_src; else override (e.g. actual APLL) */
} i3c_master_config_t;

/* ---------- Config API ---------- */
bk_err_t bk_i3c_master_init(const i3c_master_config_t *cfg);
bk_err_t bk_i3c_master_deinit(void);
void i3c_master_int_unregister(void);  /* Internal use by platform_deinit */

/** Print enumerated devices from controller device table (DA, PID, BCR, DCR). Call after successful init. */
void bk_i3c_master_devices_print(void);

/* ---------- Data transfer API (poll/interrupt) ---------- */
/** Write data. dev_addr: 7-bit dynamic address; timeout_ms: 0 = default */
bk_err_t bk_i3c_master_write(uint8_t dev_addr, const uint8_t *data, uint32_t len, uint32_t timeout_ms);

/** Read data. len: requested bytes; *recv_len: actual received; xfer_mode: poll or interrupt */
bk_err_t bk_i3c_master_read(uint8_t dev_addr, uint8_t *buf, uint32_t len, uint32_t *recv_len,
                            uint32_t timeout_ms, i3c_xfer_mode_t xfer_mode);

#ifdef __cplusplus
}
#endif

#endif /* I3C_MASTER_H */
