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

#include <stdint.h>
#include <stdbool.h>
#include "flash_hal.h"
#include "flash_core_config.h"
#include "bk_flash_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable flash core: the single implementation of the bk7259 flash algorithm,
 * shared physically by CP/AP middleware, TF-M and the aboot bootloader.
 *
 * The operational surface every consumer needs day to day is just three verbs:
 *
 *     flash_core_read()  / flash_core_read_word()
 *     flash_core_write()
 *     flash_core_erase()
 *
 * These take the outer op lock and internally run the whole atomic sequence
 * (switch two-line -> [per-op protect] -> op -> [per-op re-protect] -> restore
 * line mode). Consumers no longer hand-assemble raw ops + protection + line
 * mode; they only:
 *
 *   1. inject concurrency / notify via flash_core_set_port(),
 *   2. (optionally) register a per-range permission gate via
 *      flash_core_set_perm_cb(),
 *   3. call read / write / erase.
 *
 * The status-register WRSR / QE / raw-op / lock primitives are internal to the
 * core. A small "session control" and "advanced glue" surface remains for the
 * secure-boot / serial-download / low-power paths that must bracket protection
 * and line mode explicitly, or poke SoC-specific flash controller bits.
 */

/* --- algorithm constants (guarded so a consumer header may pre-define them) --- */
#ifndef FLASH_STATUS_REG_PROTECT_MASK
#define FLASH_STATUS_REG_PROTECT_MASK    0xff
#endif
#ifndef FLASH_STATUS_REG_PROTECT_OFFSET
#define FLASH_STATUS_REG_PROTECT_OFFSET  8
#endif
#ifndef FLASH_CMP_MASK
#define FLASH_CMP_MASK                   0x1
#endif
#ifndef FLASH_BUFFER_LEN
#define FLASH_BUFFER_LEN                 8
#endif
#ifndef FLASH_BYTES_CNT
#define FLASH_BYTES_CNT                  32
#endif
#ifndef FLASH_ADDRESS_MASK
#define FLASH_ADDRESS_MASK               0x1f
#endif
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE                0x1000
#endif
#ifndef FLASH_BLOCK32_SIZE
#define FLASH_BLOCK32_SIZE               (0x8000)
#endif
#ifndef FLASH_BLOCK_SIZE
#define FLASH_BLOCK_SIZE                 (0x10000)
#endif

/*
 * Section attribute for the erase/write hot path: these routines must not be
 * fetched from flash while the flash controller is mid-operation. CP/AP/TF-M
 * all provide a ".iram" output section; a consumer may override this before
 * including the header if its linker uses a different name.
 */
#ifndef FLASH_CORE_IRAM
#define FLASH_CORE_IRAM __attribute__((section(".iram")))
#endif

/**
 * Optional per-range write/erase permission callback.
 * Return true to allow the operation on [addr, addr+size), false to reject.
 * Middleware registers its partition permission check; TF-M / bootloader leave
 * it NULL.
 */
typedef bool (*flash_core_perm_cb_t)(uint32_t addr, uint32_t size);

/* ===========================================================================
 * Port / permission (install once at init)
 * ========================================================================= */

/** Install the environment concurrency port. Copies the struct; pass NULL fields for nop. */
void flash_core_set_port(const flash_core_port_t *port);

/** Register the per-range write/erase permission gate (NULL to allow all). */
void flash_core_set_perm_cb(flash_core_perm_cb_t cb);

/** Protection type re-asserted by PER_OP re-protect and by flash_core_protect(). */
void flash_core_set_runtime_protect_type(flash_protect_type_t type);

/* ===========================================================================
 * Setup (driver init)
 * ========================================================================= */

/** Bind the HAL to the flash controller base and soft-reset it (flash_ll_init). */
void flash_core_hal_init(void);

/** Access the core HAL handle (for consumer-specific SoC pokes: XIP/DBUS/OTA...). */
flash_hal_t *flash_core_hal(void);

/**
 * Read the JEDEC id (nested critical) and match it against the table, setting
 * the active config. Returns true if a table entry matched, false if the
 * catch-all default was selected. Caller may bracket this in its own critical
 * section (flash_core_enter_critical) if it needs the whole init atomic.
 */
bool flash_core_identify(void);

/** Inject the active config (for consumers with their own device-selection policy). */
void flash_core_set_cfg(const flash_config_t *cfg);

/** Init-time critical bracket (mirrors flash_enter_critical(); re-entrant). */
uint32_t flash_core_enter_critical(void);
void flash_core_exit_critical(uint32_t int_level);

/** Drain any residual 8-word read data left in the controller FIFO. */
bk_err_t flash_core_clear_rd_residual(void);

/* ===========================================================================
 * Accessors
 * ========================================================================= */

uint32_t flash_core_get_id(void);
const flash_config_t *flash_core_get_cfg(void);
uint32_t flash_core_get_total_size(void);
flash_line_mode_t flash_core_get_line_mode(void);
uint8_t flash_core_get_continuous_read_mode(void);

/* ===========================================================================
 * Primary: read / write / erase  (take the outer op lock)
 * ========================================================================= */

/**
 * Read raw bytes / words under the outer op lock. No protection or line-mode
 * change (op_sw READ is valid in QUAD continuous-read). Inner per-chunk
 * critical still nests inside the outer lock.
 */
bk_err_t flash_core_read(uint8_t *buffer, uint32_t address, uint32_t len);
bk_err_t flash_core_read_word(uint32_t *buffer, uint32_t address, uint32_t len);

/**
 * Write / erase under the outer op lock. Line mode is bracketed to two-line and
 * restored; protection is handled per the active policy. flash_core_erase()
 * accepts an arbitrary size and erases the covering sectors/blocks, choosing the
 * SE/BE1/BE2 opcode (exact aligned single-unit sizes keep their historical
 * opcode; larger ranges loop sectors + 64K blocks with op_progress() between).
 */
bk_err_t flash_core_write(uint32_t address, const uint8_t *user_buf, uint32_t size);
bk_err_t flash_core_erase(uint32_t address, uint32_t size);

/* ===========================================================================
 * Session control (secure-boot / serial-download / low-power brackets)
 * ========================================================================= */

/** Volatile unprotect (whole device) / re-protect (runtime type), line-bracketed. */
void flash_core_unprotect(void);
void flash_core_protect(void);

/** Persist the runtime protection type across power cycles (non-volatile WRSR). */
void flash_core_protect_nvol(void);

/** Switch/restore continuous-read line mode; returns the previous *runtime*
 * mode (may be 0 / unset before the first successful sync). Config capability
 * is flash_core_get_line_mode(), not this return value. */
FLASH_CORE_IRAM flash_line_mode_t flash_core_set_line_mode(flash_line_mode_t line_mode);

/** Mark runtime line-mode unsynced so the next set_line_mode() re-applies HW
 * (post deep-sleep / handoff). */
void flash_core_reset_line_mode(void);

/**
 * Atomic RDSR (resolved config's status-register width): outer op lock + switch
 * to two-line + RDSR + restore line mode, all in one critical section. This is
 * the only public RDSR entry; the raw primitive is file-local to the core. Use
 * this from any path (the device ignores RDSR while in QUAD continuous-read, so
 * the two-line switch must be atomic with the read).
 */
FLASH_CORE_IRAM uint32_t flash_core_read_status_reg(void);

/* ===========================================================================
 * Advanced glue: thin wrappers over the shared HAL for the CPU-write window
 * (memory-mapped/cbus flash programming).
 * ========================================================================= */

void     flash_core_cpu_wr_enable(void);
void     flash_core_cpu_wr_disable(void);

#ifdef __cplusplus
}
#endif
