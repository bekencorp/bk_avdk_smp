#include <stdint.h>
#include "partitions.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PARTITION_PRIMARY_ALL,
	PARTITION_SECONDARY_ALL,
	PARTITION_PARTITION,
	PARTITION_PRIMARY_MANIFEST,	
	PARTITION_BL2,
	PARTITION_SYS_PS,
	PARTITION_SYS_ITS,
	PARTITION_PRIMARY_TFM_S,
	PARTITION_PRIMARY_CPU0_APP,	
	PARTITION_BOOT_PARAM,
#if CONFIG_OTA_OVERWRITE
	/* Compressed-overwrite OTA: staging area for the received compressed
	 * image (ota) and the resume/confirm journal (ota_control). Only present
	 * for the secureboot_overwrite project (ota.csv strategy=OVERWRITE). */
	PARTITION_OTA,
	PARTITION_OTA_CONTROL,
#endif
	PARTITION_CNT,
} partition_id_e;


#define PARTITION_AMOUNT 				(50)
#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
#define PARTITION_PPC_OFFSET             (0x400)
#define PARTITION_NAME_LEN               (20)
#define PARTITION_ENTRY_LEN              (32)
#define PARTITION_OFFSET_OFFSET          (22)
#define PARTITION_SIZE_OFFSET            (26)
#define PARTITION_FLAGS_OFFSET           (30)

/* BK7259: the generated security.h (from crc_en=FALSE) provides the correct
 * identity phy<->virtual mapping. Only fall back to the CRC interleave formula
 * here if security.h was not included first. (The old hardcoded CRC mapping and
 * the bk7234 flash base 0x02000000 are wrong for bk7259, whose flash CRC is
 * disabled and whose XIP base is 0x04000000.) */
#ifndef FLASH_PHY2VIRTUAL
#define FLASH_PHY2VIRTUAL(phy_addr)      ((((phy_addr) / 34) << 5) + ((phy_addr) % 34))
#endif
#ifndef SOC_FLASH_BASE_ADDR
#define SOC_FLASH_BASE_ADDR 0x04000000
#endif

int partition_init(void);
uint32_t partition_get_phy_offset(uint32_t id);
uint32_t partition_get_phy_size(uint32_t id);
void dump_partition(void);
