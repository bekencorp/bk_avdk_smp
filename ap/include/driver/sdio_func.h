// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/bk_include.h>
#include <driver/sdio_host.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SDIO peripheral (I/O card) core.
 *
 * Implements the SDIO device side (CMD5 / CMD52 / CMD53, CCCR / FBR / CIS) on
 * top of the generic SDIO host controller interface (<driver/sdio_host.h>).
 *
 * The public API mirrors the Linux SDIO function API one-to-one, differing
 * only by a "bk_" prefix, so existing Linux SDIO peripheral drivers can be
 * ported with near mechanical changes:
 *
 *   Linux                     this driver
 *   ------------------------  ---------------------------
 *   sdio_claim_host()         bk_sdio_claim_host()
 *   sdio_release_host()       bk_sdio_release_host()
 *   sdio_enable_func()        bk_sdio_enable_func()
 *   sdio_disable_func()       bk_sdio_disable_func()
 *   sdio_set_block_size()     bk_sdio_set_block_size()
 *   sdio_claim_irq()          bk_sdio_claim_irq()
 *   sdio_release_irq()        bk_sdio_release_irq()
 *   sdio_readb()/writeb()     bk_sdio_readb()/bk_sdio_writeb()
 *   sdio_writeb_readb()       bk_sdio_writeb_readb()
 *   sdio_readw()/writew()     bk_sdio_readw()/bk_sdio_writew()
 *   sdio_readl()/writel()     bk_sdio_readl()/bk_sdio_writel()
 *   sdio_memcpy_fromio()      bk_sdio_memcpy_fromio()
 *   sdio_memcpy_toio()        bk_sdio_memcpy_toio()
 *   sdio_readsb()/writesb()   bk_sdio_readsb()/bk_sdio_writesb()
 *   sdio_f0_readb()/writeb()  bk_sdio_f0_readb()/bk_sdio_f0_writeb()
 *   sdio_align_size()         bk_sdio_align_size()
 *
 * Adaptation notes vs. Linux:
 *  - Linux passes a "struct sdio_func *"; this port has no card/func object
 *    model, so functions take the I/O function number (@p func, 1..7) directly.
 *    Function-0 / host-context helpers ignore @p func (single bound host).
 *  - The bk_sdio_func_init()/deinit()/count()/get_cis() helpers have no direct
 *    Linux equivalent (in Linux the MMC core enumerates the card); they drive
 *    CMD5/CMD3/CMD7 bring-up that the core would otherwise do.
 *
 * @note Requires a real SDIO peripheral on the bus and async-interrupt wiring
 *       (DAT1) for bring-up validation.
 */

/**
 * @brief Per-function async-interrupt handler.
 *
 * Linux's sdio_irq_handler_t is "void(*)(struct sdio_func *)"; here it gets the
 * function number plus an optional user @p arg captured at bk_sdio_claim_irq().
 */
typedef void (*bk_sdio_irq_handler_t)(uint8_t func, void *arg);

/* Backward-compatible alias for the previous type name. */
typedef bk_sdio_irq_handler_t sdio_func_irq_handler_t;

/** @brief Common CIS information parsed from function 0. */
typedef struct {
	uint16_t manf_id;      /**< CISTPL_MANFID manufacturer id */
	uint16_t card_id;      /**< CISTPL_MANFID card/device id */
	uint16_t func0_blksz;  /**< function 0 max block size */
} sdio_func_cis_t;

/* ----------------- bring-up helpers (no direct Linux equivalent) --------- */

/**
 * @brief Initialize the SDIO peripheral on @p host_id.
 *
 * Runs CMD5 (IO_SEND_OP_COND) / CMD3 / CMD7, reads CCCR and parses the common
 * CIS. Must be called before any other API.
 */
bk_err_t bk_sdio_func_init(sdio_host_id_t host_id);

/** @brief Deinitialize and release the host controller. */
bk_err_t bk_sdio_func_deinit(void);

/** @brief Number of I/O functions reported by the card (excluding function 0). */
uint8_t bk_sdio_func_count(void);

/** @brief Get parsed common CIS info. */
bk_err_t bk_sdio_func_get_cis(sdio_func_cis_t *cis);

/* ----------------- bus claim (Linux: sdio_claim_host/release_host) ------- */

/**
 * @brief Serialize access to the bound SDIO host across threads.
 *
 * Mirrors Linux sdio_claim_host()/sdio_release_host(). Claims are recursive on
 * the calling thread. @p func is accepted for call-site parity with Linux and
 * is otherwise unused (the host is shared by all functions of the card).
 */
void bk_sdio_claim_host(uint8_t func);
void bk_sdio_release_host(uint8_t func);

/* ----------------- function enable / block size ------------------------- */

/** @brief Enable an I/O function (CCCR IOEx) and wait until ready (IORx). */
bk_err_t bk_sdio_enable_func(uint8_t func);

/** @brief Disable an I/O function (CCCR IOEx). */
bk_err_t bk_sdio_disable_func(uint8_t func);

/** @brief Set the block size of an I/O function (FBR), used by CMD53 block mode. */
bk_err_t bk_sdio_set_block_size(uint8_t func, uint16_t blksz);

/**
 * @brief Round @p sz up to the function's block size (Linux sdio_align_size()).
 */
uint32_t bk_sdio_align_size(uint8_t func, uint32_t sz);

/* ----------------- async interrupt -------------------------------------- */

/** @brief Claim the async SDIO interrupt for a function (Linux sdio_claim_irq()). */
bk_err_t bk_sdio_claim_irq(uint8_t func, bk_sdio_irq_handler_t handler, void *arg);

/** @brief Release the async SDIO interrupt for a function (Linux sdio_release_irq()). */
bk_err_t bk_sdio_release_irq(uint8_t func);

/* ----------------- CMD52 byte / word / long IO -------------------------- */

/**
 * @brief CMD52 single-byte read (Linux sdio_readb()).
 * @param err_ret optional, receives BK_OK / error code; may be NULL.
 * @return the byte read (0 on error).
 */
uint8_t bk_sdio_readb(uint8_t func, uint32_t addr, int *err_ret);

/** @brief CMD52 single-byte write (Linux sdio_writeb()). */
void bk_sdio_writeb(uint8_t func, uint8_t b, uint32_t addr, int *err_ret);

/**
 * @brief CMD52 write-then-read the same address (RAW flag, Linux sdio_writeb_readb()).
 * @return the byte read back after the write.
 */
uint8_t bk_sdio_writeb_readb(uint8_t func, uint8_t write_byte, uint32_t addr, int *err_ret);

/** @brief 16-bit little-endian read (Linux sdio_readw()). */
uint16_t bk_sdio_readw(uint8_t func, uint32_t addr, int *err_ret);

/** @brief 16-bit little-endian write (Linux sdio_writew()). */
void bk_sdio_writew(uint8_t func, uint16_t b, uint32_t addr, int *err_ret);

/** @brief 32-bit little-endian read (Linux sdio_readl()). */
uint32_t bk_sdio_readl(uint8_t func, uint32_t addr, int *err_ret);

/** @brief 32-bit little-endian write (Linux sdio_writel()). */
void bk_sdio_writel(uint8_t func, uint32_t b, uint32_t addr, int *err_ret);

/* ----------------- function-0 (CCCR/FBR) byte IO ------------------------ */

/** @brief CMD52 read from function 0 / CCCR (Linux sdio_f0_readb()). */
uint8_t bk_sdio_f0_readb(uint8_t func, uint32_t addr, int *err_ret);

/** @brief CMD52 write to function 0 / CCCR (Linux sdio_f0_writeb(), vendor range only). */
void bk_sdio_f0_writeb(uint8_t func, uint8_t b, uint32_t addr, int *err_ret);

/* ----------------- CMD53 buffer IO -------------------------------------- */

/**
 * @brief CMD53 read with auto-incrementing address (Linux sdio_memcpy_fromio()).
 *        Uses block mode when @p count is a multiple of the function block size.
 */
bk_err_t bk_sdio_memcpy_fromio(uint8_t func, void *dst, uint32_t addr, uint32_t count);

/** @brief CMD53 write with auto-incrementing address (Linux sdio_memcpy_toio()). */
bk_err_t bk_sdio_memcpy_toio(uint8_t func, uint32_t addr, const void *src, uint32_t count);

/**
 * @brief CMD53 read from a fixed address / FIFO (Linux sdio_readsb()).
 */
bk_err_t bk_sdio_readsb(uint8_t func, void *dst, uint32_t addr, uint32_t count);

/** @brief CMD53 write to a fixed address / FIFO (Linux sdio_writesb()). */
bk_err_t bk_sdio_writesb(uint8_t func, uint32_t addr, const void *src, uint32_t count);

#ifdef __cplusplus
}
#endif
