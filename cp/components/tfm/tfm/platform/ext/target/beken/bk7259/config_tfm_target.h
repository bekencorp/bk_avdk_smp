/*
 * Copyright (c) 2024 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __CONFIG_TFM_TARGET_H__
#define __CONFIG_TFM_TARGET_H__

/*
 * BK7259 bring-up: back the Internal Trusted Storage filesystem with a RAM
 * buffer instead of real flash.
 *
 * Rationale: ITS partition init (tfm_its_init -> its_flash_fs_prepare ->
 * its_flash_fs_wipe_all) drives the CMSIS flash driver (Driver_FLASH0), which
 * in turn calls the full non-secure SDK flash stack (bk_flash_driver_init /
 * bk_flash_erase_sector / bk_flash_write_bytes). Running that erase/program
 * path inside the TF-M secure context stalls the SPM scheduler, so ns_agent
 * (lowest priority) never runs and we never reach the NS world.
 *
 * The immediate bring-up goal is to see the NS app log, and ITS does not need
 * to be persistent for that. RAM FS keeps all ITS data in its_block_data[]
 * (sized by ITS_RAM_FS_SIZE from flash_layout.h) and never touches the flash
 * controller, so partition init completes and the scheduler can advance.
 *
 * TODO(secure-storage): switch back to flash-backed ITS once the secure-world
 * thin-shim flash driver is verified to be safe under the TF-M scheduler.
 */
#define ITS_RAM_FS 1

#endif /* __CONFIG_TFM_TARGET_H__ */
