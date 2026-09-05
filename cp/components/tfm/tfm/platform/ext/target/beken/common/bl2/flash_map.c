/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
/* Include generated security.h first so BK7259 uses the configured direct-XIP
 * flash mapping instead of the fallback CRC-interleave mapping. */
#include "security.h"
#include "flash_map/flash_map.h"
#include "bl2_flash_map.h"
#include "target.h"
#include "Driver_Flash.h"
#include "tfm_flash_partition.h"
#include "hal_flash.h"
typedef enum
{
	FLASH_MAP_IMAGE_PRIMARY_ALL = 0,
	FLASH_MAP_IMAGE_SECONDARY_ALL,
	FLASH_MAP_IMAGE_PRIMARY_PARTITION,
	FLASH_MAP_IMAGE_PRIMARY_MANIFEST,
	FLASH_MAP_IMAGE_PRIMARY_BL2,
	FLASH_MAP_IMAGE_SYS_PS,
	FLASH_MAP_IMAGE_SYS_ITS,
	FLASH_MAP_IMAGE_PRIMARY_TFM_S,
	FLASH_MAP_IMAGE_PRIMARY_CPU0_APP,
	FLASH_MAP_IMAGE_BOOT_PARAM,
	FLASH_MAP_IMAGE_MAX	
}flash_map_e;

#if CONFIG_DIRECT_XIP
extern void flash_set_xip_offset(uint32_t primary_start, uint32_t secondary_start,uint32_t code_size);
extern void flash_set_ota_enable(bool enable);
extern void flash_set_excute_enable(int enable);
#endif /* CONFIG_DIRECT_XIP */

/* When undefined FLASH_DEV_NAME_0 or FLASH_DEVICE_ID_0 , default */
#if !defined(FLASH_DEV_NAME_0) || !defined(FLASH_DEVICE_ID_0)
#define FLASH_DEV_NAME_0  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_0 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_1 or FLASH_DEVICE_ID_1 , default */
#if !defined(FLASH_DEV_NAME_1) || !defined(FLASH_DEVICE_ID_1)
#define FLASH_DEV_NAME_1  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_1 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_2 or FLASH_DEVICE_ID_2 , default */
#if !defined(FLASH_DEV_NAME_2) || !defined(FLASH_DEVICE_ID_2)
#define FLASH_DEV_NAME_2  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_2 FLASH_DEVICE_ID
#endif

/* When undefined FLASH_DEV_NAME_3 or FLASH_DEVICE_ID_3 , default */
#if !defined(FLASH_DEV_NAME_3) || !defined(FLASH_DEVICE_ID_3)
#define FLASH_DEV_NAME_3  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_3 FLASH_DEVICE_ID
#endif

#if defined(MCUBOOT_SWAP_USING_SCRATCH)
/* When undefined FLASH_DEV_NAME_SCRATCH or FLASH_DEVICE_ID_SCRATCH , default */
#if !defined(FLASH_DEV_NAME_SCRATCH) || !defined(FLASH_DEVICE_ID_SCRATCH)
#define FLASH_DEV_NAME_SCRATCH  FLASH_DEV_NAME
#define FLASH_DEVICE_ID_SCRATCH FLASH_DEVICE_ID
#endif
#endif  /* defined(MCUBOOT_SWAP_USING_SCRATCH) */

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define FLASH_AREA_PRIVATE_ID(id) ((uint8_t)(0x80u + (id)))

/* Flash device names must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_1;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_3;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_SCRATCH;

struct flash_area flash_map[];

typedef struct {
	flash_map_e flash_map_id;
	uint32_t partition_id;
} flash_partition_map_t;

static const flash_partition_map_t s_partition_map[] = {
	{ FLASH_MAP_IMAGE_PRIMARY_ALL, PARTITION_PRIMARY_ALL },
#if CONFIG_OTA_OVERWRITE
	/* Compressed-overwrite: MCUboot's "secondary slot" is the ota staging
	 * partition. BL2 reads the received compressed image from here and, when
	 * ota_control holds OVERWRITE_CONFIRM, decompresses it into primary_all.
	 * decompress_bl2.c uses get_flash_map_offset/size(FLASH_MAP_IMAGE_SECONDARY_ALL). */
	{ FLASH_MAP_IMAGE_SECONDARY_ALL, PARTITION_OTA },
#else
	{ FLASH_MAP_IMAGE_SECONDARY_ALL, PARTITION_SECONDARY_ALL },
#endif
	{ FLASH_MAP_IMAGE_PRIMARY_MANIFEST, PARTITION_PRIMARY_MANIFEST },
	{ FLASH_MAP_IMAGE_PRIMARY_PARTITION, PARTITION_PARTITION },
	{ FLASH_MAP_IMAGE_PRIMARY_BL2, PARTITION_BL2 },
	{ FLASH_MAP_IMAGE_SYS_PS, PARTITION_SYS_PS },
	{ FLASH_MAP_IMAGE_SYS_ITS, PARTITION_SYS_ITS },
	{ FLASH_MAP_IMAGE_PRIMARY_TFM_S, PARTITION_PRIMARY_TFM_S },
	{ FLASH_MAP_IMAGE_PRIMARY_CPU0_APP, PARTITION_PRIMARY_CPU0_APP },
	{ FLASH_MAP_IMAGE_BOOT_PARAM, PARTITION_BOOT_PARAM },
};

static uint32_t flash_area_align_down(uint32_t size)
{
	return size & ~(FLASH_AREA_IMAGE_SECTOR_SIZE - 1);
}

static void flash_area_set_partition(flash_map_e flash_map_id, uint32_t partition_id)
{
	uint32_t size = partition_get_phy_size(partition_id);

	flash_map[flash_map_id].fa_off = partition_get_phy_offset(partition_id);
	flash_map[flash_map_id].fa_size = size;
	flash_map[flash_map_id].fa_phy_size = size;
}

#if CONFIG_DIRECT_XIP
/* Direct-XIP A/B slot remap programming. Guarded by CONFIG_DIRECT_XIP because it
 * calls the flash_set_*() XIP remap APIs (declared above under the same guard).
 * The compressed-overwrite project (CONFIG_OTA_OVERWRITE, DIRECT_XIP=0) has a
 * single execute slot (primary_all) and never programs A/B remap. */
static void flash_area_config_direct_xip(void)
{
	uint32_t primary_start = flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_off;
	uint32_t primary_size = flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_size;
	/* Use partition table size, not flash_map[] — single-slot init may
	 * synthesize a non-zero secondary fa_size for MCUboot sector checks. */
	uint32_t secondary_size = partition_get_phy_size(PARTITION_SECONDARY_ALL);

	/* Clear stale slot-B remap before BL2 reads A/B images. */
	flash_set_excute_enable(0);

	/* Single-slot layouts (e.g. secureboot_ai) have no secondary_all.
	 * Do not program A/B XIP remap with secondary_start=0 — that corrupts
	 * remap state. Keep primary window only and disable OTA remap. */
	if (secondary_size == 0u) {
		flash_set_xip_offset(primary_start, primary_start, primary_size);
		flash_set_ota_enable(false);
		return;
	}

	flash_set_xip_offset(primary_start,
			     flash_map[FLASH_MAP_IMAGE_SECONDARY_ALL].fa_off,
			     primary_size);
	flash_set_ota_enable(true);
}
#endif /* CONFIG_DIRECT_XIP */

uint32_t get_flash_map_offset(uint32_t index)
{
	return flash_map[index].fa_off;
}

uint32_t get_flash_map_size(uint32_t index)
{
	return flash_map[index].fa_size;
}

uint32_t get_flash_map_phy_size(uint32_t index)
{
	return flash_map[index].fa_phy_size;
}

int flash_map_init(void)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(s_partition_map); i++) {
		flash_area_set_partition(s_partition_map[i].flash_map_id,
					 s_partition_map[i].partition_id);
	}

	/* MCUboot always opens both slots and compares sector layouts. When
	 * secondary_all is absent, give the secondary flash_area the same
	 * geometry as primary so sector checks pass; flash_area_read() returns
	 * erased content for that slot so it never looks like a valid image.
	 *
	 * Skip this for CONFIG_OTA_OVERWRITE: there the secondary flash_area is
	 * deliberately mapped onto the (smaller) ota staging partition, which is
	 * the real source of the compressed image; overwriting it with primary
	 * geometry would point BL2 at the wrong offset. */
#if !CONFIG_OTA_OVERWRITE
	if ((!CONFIG_DIRECT_XIP ||
	     partition_get_phy_size(PARTITION_SECONDARY_ALL) == 0u) &&
	    flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_size != 0u) {
		flash_map[FLASH_MAP_IMAGE_SECONDARY_ALL].fa_off =
			flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_off;
		flash_map[FLASH_MAP_IMAGE_SECONDARY_ALL].fa_size =
			flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_size;
		flash_map[FLASH_MAP_IMAGE_SECONDARY_ALL].fa_phy_size =
			flash_map[FLASH_MAP_IMAGE_PRIMARY_ALL].fa_phy_size;
	}
#endif /* !CONFIG_OTA_OVERWRITE */

#if CONFIG_DIRECT_XIP
	flash_area_config_direct_xip();
#endif /* CONFIG_DIRECT_XIP */
	return 0;
}

struct flash_area flash_map[] = {
	[FLASH_MAP_IMAGE_PRIMARY_ALL] = {
		.fa_id = FLASH_AREA_PRIMARY_ALL_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_SECONDARY_ALL] = {
		.fa_id = FLASH_AREA_SECONDARY_ALL_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_PRIMARY_PARTITION] = {
		.fa_id = FLASH_AREA_PRIMARY_PARTITION_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_PRIMARY_MANIFEST] = {
		.fa_id = FLASH_AREA_PRIMARY_MANIFEST_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_PRIMARY_BL2] = {
		.fa_id = FLASH_AREA_PRIMARY_BL2_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_SYS_PS] = {
		.fa_id = FLASH_AREA_SYS_PS_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_SYS_ITS] = {
		.fa_id = FLASH_AREA_SYS_ITS_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_PRIMARY_TFM_S] = {
		.fa_id = FLASH_AREA_PRIMARY_TFM_S_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_PRIMARY_CPU0_APP] = {
		.fa_id = FLASH_AREA_PRIMARY_CPU0_APP_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
	[FLASH_MAP_IMAGE_BOOT_PARAM] = {
		.fa_id = FLASH_AREA_BOOT_PARAM_ID,
		.fa_device_id = FLASH_DEVICE_ID,
		.fa_driver = &FLASH_DEV_NAME,
	},
};

const int flash_map_entry_num = ARRAY_SIZE(flash_map);
