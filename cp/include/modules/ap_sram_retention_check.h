#pragma once

#include <stdint.h>
#include "ram_regions.h"

#define AP_SRAM_CHECK_MAGIC                 (0x41504352u) /* "APCR" */
#define AP_SRAM_CHECK_REGION_COUNT          (4u)
#define AP_SRAM_CHECK_SKIP_RANGE_COUNT      (3u)
#define AP_SRAM_CHECK_MAX_CHANGE_LOGS       (64u)
#define AP_SRAM_CHECK_CONTROL_SIZE          (0x1000u)
#define AP_SRAM_CHECK_SNAPSHOT_SIZE         (0x0e0000u)
#define AP_SRAM_CHECK_TOTAL_SIZE            \
	(AP_SRAM_CHECK_CONTROL_SIZE + AP_SRAM_CHECK_SNAPSHOT_SIZE)
#define AP_SRAM_CHECK_PSRAM_BASE            \
	((uintptr_t)CONFIG_AP_PSRAM_DATA_SECTION_ADDR)
#define AP_SRAM_CHECK_SNAPSHOT_BASE         \
	(AP_SRAM_CHECK_PSRAM_BASE + AP_SRAM_CHECK_CONTROL_SIZE)

typedef struct {
	uint32_t magic;
	uint32_t magic_inv;
	uint32_t region_count;
	uint32_t generation;
	uint32_t skip_start[AP_SRAM_CHECK_SKIP_RANGE_COUNT];
	uint32_t skip_end[AP_SRAM_CHECK_SKIP_RANGE_COUNT];
	uint32_t crc_before[AP_SRAM_CHECK_REGION_COUNT];
	uint32_t crc_before_inv[AP_SRAM_CHECK_REGION_COUNT];
} ap_sram_check_shared_t;

