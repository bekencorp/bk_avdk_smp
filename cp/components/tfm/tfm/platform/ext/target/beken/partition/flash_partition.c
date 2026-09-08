#include "flash_map/flash_map.h"
#include "target.h"
#include "Driver_Flash.h"
#include "flash_partition.h"
#include <components/log.h>
#include <ctype.h>
#include <string.h>

#define TAG "partition"

#if CONFIG_BL2_UPDATE_WITH_PC
#include "partitions_gen.h"
#endif

/*
 * BK7258 secureboot_xip layout may not publish every TF-M named partition
 * (sys_ps / sys_its / primary_tfm_s). Provide compile-time defaults so BL2
 * still builds; unmatched names simply keep offset/size 0 at runtime.
 */
#ifndef CONFIG_SYS_PS_PHY_PARTITION_OFFSET
#define CONFIG_SYS_PS_PHY_PARTITION_OFFSET            0
#define CONFIG_SYS_PS_PHY_PARTITION_SIZE              0
#endif
#ifndef CONFIG_SYS_ITS_PHY_PARTITION_OFFSET
#define CONFIG_SYS_ITS_PHY_PARTITION_OFFSET           0
#define CONFIG_SYS_ITS_PHY_PARTITION_SIZE             0
#endif
#ifndef CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET
#define CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET     0
#define CONFIG_PRIMARY_TFM_S_PHY_PARTITION_SIZE       0
#endif
#ifndef CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET
#ifdef CONFIG_PRIMARY_CP_APP_PHY_PARTITION_OFFSET
#define CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET  CONFIG_PRIMARY_CP_APP_PHY_PARTITION_OFFSET
#define CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE    CONFIG_PRIMARY_CP_APP_PHY_PARTITION_SIZE
#else
#define CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET  0
#define CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE    0
#endif
#endif
#ifndef CONFIG_BOOT_PARAM_PHY_PARTITION_OFFSET
#define CONFIG_BOOT_PARAM_PHY_PARTITION_OFFSET        0
#define CONFIG_BOOT_PARAM_PHY_PARTITION_SIZE          0
#endif

typedef struct {
	uint32_t phy_offset;
	uint32_t phy_size;
	uint32_t phy_flags;
} partition_config_t;

typedef struct {
	uint32_t phy_offset;
	uint32_t phy_size;
} partition_expected_t;

static partition_config_t s_partition_config[PARTITION_CNT] = {0};

/* MUST stay index-aligned with partition_id_e in flash_partition.h. */
const char *s_partition_name[PARTITION_CNT] = {
	"primary_all",       /* PARTITION_PRIMARY_ALL      */
	"secondary_all",     /* PARTITION_SECONDARY_ALL    */
	"partition",         /* PARTITION_PARTITION        */
	"primary_manifest",  /* PARTITION_PRIMARY_MANIFEST */
	"bl2",               /* PARTITION_BL2              */
	"sys_ps",            /* PARTITION_SYS_PS           */
	"sys_its",           /* PARTITION_SYS_ITS          */
	"primary_tfm_s",     /* PARTITION_PRIMARY_TFM_S    */
	/* Match BK7258 csv name (primary_cp_app), not BK7259 primary_cpu0_app. */
	"primary_cp_app",    /* PARTITION_PRIMARY_CPU0_APP */
	"boot_param",        /* PARTITION_BOOT_PARAM       */
	"ota",               /* PARTITION_OTA              */
	"ota_control",       /* PARTITION_OTA_CONTROL      */
};

static const partition_expected_t s_partition_expected[PARTITION_CNT] = {
	[PARTITION_PRIMARY_ALL] = {
		CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET,
		CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE,
	},
	[PARTITION_SECONDARY_ALL] = {
		CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET,
		CONFIG_SECONDARY_ALL_PHY_PARTITION_SIZE,
	},
	[PARTITION_PARTITION] = {
		CONFIG_PARTITION_PHY_PARTITION_OFFSET,
		CONFIG_PARTITION_PHY_PARTITION_SIZE,
	},
	[PARTITION_PRIMARY_MANIFEST] = {
		CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET,
		CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_SIZE,
	},
	[PARTITION_BL2] = {
		CONFIG_BL2_PHY_PARTITION_OFFSET,
		CONFIG_BL2_PHY_PARTITION_SIZE,
	},
	[PARTITION_SYS_PS] = {
		CONFIG_SYS_PS_PHY_PARTITION_OFFSET,
		CONFIG_SYS_PS_PHY_PARTITION_SIZE,
	},
	[PARTITION_SYS_ITS] = {
		CONFIG_SYS_ITS_PHY_PARTITION_OFFSET,
		CONFIG_SYS_ITS_PHY_PARTITION_SIZE,
	},
	[PARTITION_PRIMARY_TFM_S] = {
		CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET,
		CONFIG_PRIMARY_TFM_S_PHY_PARTITION_SIZE,
	},
	[PARTITION_PRIMARY_CPU0_APP] = {
		CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_OFFSET,
		CONFIG_PRIMARY_CPU0_APP_PHY_PARTITION_SIZE,
	},
	[PARTITION_BOOT_PARAM] = {
		CONFIG_BOOT_PARAM_PHY_PARTITION_OFFSET,
		CONFIG_BOOT_PARAM_PHY_PARTITION_SIZE,
	},
	[PARTITION_OTA] = {
#ifdef CONFIG_OTA_PHY_PARTITION_OFFSET
		CONFIG_OTA_PHY_PARTITION_OFFSET,
		CONFIG_OTA_PHY_PARTITION_SIZE,
#else
		0, 0,
#endif
	},
	[PARTITION_OTA_CONTROL] = {
#ifdef CONFIG_OTA_CONTROL_PHY_PARTITION_OFFSET
		CONFIG_OTA_CONTROL_PHY_PARTITION_OFFSET,
		CONFIG_OTA_CONTROL_PHY_PARTITION_SIZE,
#else
		0, 0,
#endif
	},
};

static int is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static uint32_t piece_address(uint8_t *array, uint32_t index)
{
	return ((uint32_t)(array[index]) << 24) |
	       ((uint32_t)(array[index + 1]) << 16) |
	       ((uint32_t)(array[index + 2]) << 8) |
	       ((uint32_t)(array[index + 3]));
}

static uint16_t short_address(uint8_t *array, uint16_t index)
{
	return ((uint16_t)(array[index]) << 8) | ((uint16_t)(array[index + 1]));
}

uint32_t partition_get_phy_offset(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_offset;
}

uint32_t partition_get_phy_size(uint32_t id)
{
	if (id >= PARTITION_CNT) {
		return 0;
	}
	return s_partition_config[id].phy_size;
}

static uint32_t s_partition_valid = 0;

static int partition_check_partition_address_valid(partition_id_e partition_id)
{
	uint32_t partition_bit;

	if (partition_id >= PARTITION_CNT) {
		return -1;
	}

	/* Optional / absent in this layout: expected size 0 => skip. */
	if (s_partition_expected[partition_id].phy_size == 0) {
		return 0;
	}

	partition_bit = (1u << partition_id);

	if (s_partition_config[partition_id].phy_offset ==
		    s_partition_expected[partition_id].phy_offset &&
	    s_partition_config[partition_id].phy_size ==
		    s_partition_expected[partition_id].phy_size) {
		s_partition_valid |= partition_bit;
		return 0;
	}

	s_partition_valid &= ~partition_bit;
	BK_LOGE(TAG, "%s offset=%x size=%x expected_offset=%x expected_size=%x\r\n",
		s_partition_name[partition_id],
		s_partition_config[partition_id].phy_offset,
		s_partition_config[partition_id].phy_size,
		s_partition_expected[partition_id].phy_offset,
		s_partition_expected[partition_id].phy_size);
	return -1;
}

int get_partion_valid(void)
{
	return (int)s_partition_valid;
}

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size);

int partition_init(void)
{
	uint8_t buf[PARTITION_ENTRY_LEN];
	char name[PARTITION_NAME_LEN + 1];
	uint32_t partition_start = PARTITION_PARTITION_PHY_OFFSET + PARTITION_PPC_OFFSET;
	uint32_t i;

	memset(s_partition_config, 0, sizeof(s_partition_config));
	s_partition_valid = 0;

	for (i = 0; i < 50; i++) {
		uint32_t entry_addr = partition_start + PARTITION_ENTRY_LEN * i;
		uint32_t name_len = 0;
		int k;

		if (bk_flash_read_bytes(entry_addr, buf, sizeof(buf)) != BK_OK) {
			BK_LOGE(TAG, "read partition entry %u fails.\r\n", i);
			return -1;
		}

		if (is_alpha((char)buf[0]) == 0) {
			break;
		}

		while (name_len < PARTITION_NAME_LEN) {
			if (buf[name_len] == 0xFF || buf[name_len] == '\0') {
				break;
			}
			name[name_len] = (char)buf[name_len];
			name_len++;
		}

		if (name_len == PARTITION_NAME_LEN) {
			BK_LOGE(TAG, "invalid partition name at entry %u.\r\n", i);
			return -1;
		}

		name[name_len] = '\0';

		for (k = 0; k < PARTITION_CNT; k++) {
			if (strcmp(s_partition_name[k], name) == 0) {
				s_partition_config[k].phy_offset =
					piece_address(buf, PARTITION_OFFSET_OFFSET);
				s_partition_config[k].phy_size =
					piece_address(buf, PARTITION_SIZE_OFFSET);
				s_partition_config[k].phy_flags =
					short_address(buf, PARTITION_FLAGS_OFFSET);
				if (partition_check_partition_address_valid((partition_id_e)k) != 0) {
					return -1;
				}
				break;
			}
		}
	}

	/* Required partitions (expected size != 0) must be present and matched. */
	for (i = 0; i < PARTITION_CNT; i++) {
		if (s_partition_expected[i].phy_size == 0) {
			continue;
		}
		if ((s_partition_valid & (1u << i)) == 0) {
			BK_LOGE(TAG, "required partition %s missing or invalid\r\n",
				s_partition_name[i]);
			return -1;
		}
	}

	return 0;
}

void dump_partition(void)
{
	int k;

	for (k = 0; k < PARTITION_CNT; k++) {
		BK_LOGE(TAG, "%s offset=%x size=%x flags=%x\r\n",
			s_partition_name[k],
			s_partition_config[k].phy_offset,
			s_partition_config[k].phy_size,
			s_partition_config[k].phy_flags);
	}
}
