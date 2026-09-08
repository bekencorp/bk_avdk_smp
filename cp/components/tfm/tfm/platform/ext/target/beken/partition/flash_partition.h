#include <stdint.h>
#include "partitions.h"
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BL2/TF-M partition IDs. Keep index-aligned with s_partition_name[] /
 * s_partition_expected[] in flash_partition.c (BK7259-style layout).
 */
typedef enum {
	PARTITION_PRIMARY_ALL = 0,
	PARTITION_SECONDARY_ALL,
	PARTITION_PARTITION,
	PARTITION_PRIMARY_MANIFEST,
	PARTITION_BL2,
	PARTITION_SYS_PS,
	PARTITION_SYS_ITS,
	PARTITION_PRIMARY_TFM_S,
	PARTITION_PRIMARY_CPU0_APP,
	PARTITION_BOOT_PARAM,
	PARTITION_OTA,
	PARTITION_OTA_CONTROL,
	PARTITION_CNT,
} partition_id_e;

/* Compatibility aliases for older call sites. */
#define PARTITION_PRIMARY_BL2          PARTITION_BL2
#define PARTITION_SECONDARY_BL2        PARTITION_BL2
#define PARTITION_SECONDARY_MANIFEST   PARTITION_PRIMARY_MANIFEST
#define PARTITION_PRIMARY_PARTITION    PARTITION_PARTITION
#define PARTITION_SECONDARY_PARTITION  PARTITION_PARTITION
#define PARTITION_PS                   PARTITION_SYS_PS
#define PARTITION_ITS                  PARTITION_SYS_ITS
#define PARTITION_SPE                  PARTITION_PRIMARY_TFM_S
#define PARTITION_NSPE                 PARTITION_PRIMARY_CPU0_APP
#define PARTITION_TFM_NS               PARTITION_PRIMARY_CPU0_APP
#define PARTITION_OTP_NV               PARTITION_SYS_ITS

#define PARTITION_PARTITION_PHY_OFFSET   CONFIG_PARTITION_PHY_PARTITION_OFFSET
#define PARTITION_PPC_OFFSET             0x400
#define PARTITION_NAME_LEN               20
#define PARTITION_ENTRY_LEN              32
#define PARTITION_OFFSET_OFFSET          22
#define PARTITION_SIZE_OFFSET            26
#define PARTITION_FLAGS_OFFSET           30

#define FLASH_PHY2VIRTUAL(phy_addr)      ((((phy_addr) / 34) << 5) + ((phy_addr) % 34))
#define SOC_FLASH_BASE_ADDR 0x02000000

int partition_init(void);
uint32_t partition_get_phy_offset(uint32_t id);
uint32_t partition_get_phy_size(uint32_t id);
void dump_partition(void);

#ifdef __cplusplus
}
#endif
