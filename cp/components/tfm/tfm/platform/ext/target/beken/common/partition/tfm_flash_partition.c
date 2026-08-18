#include "Driver_Flash.h"
#include "tfm_flash_partition.h"
#include <components/log.h>
#include <stdlib.h>
#include <string.h>

#define TAG "partition"

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size);
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

/* MUST stay index-aligned with the partition_id_e enum in tfm_flash_partition.h
 * (one name per PARTITION_* value, PARTITION_CNT entries total) and with
 * s_partition_expected[] below. */
const char *s_partition_name[PARTITION_CNT] = {
	"primary_all",       /* PARTITION_PRIMARY_ALL      */
	"secondary_all",     /* PARTITION_SECONDARY_ALL    */
	"partition",         /* PARTITION_PARTITION        */
	"primary_manifest",  /* PARTITION_PRIMARY_MANIFEST */
	"bl2",               /* PARTITION_BL2              */
	"sys_ps",            /* PARTITION_SYS_PS           */
	"sys_its",           /* PARTITION_SYS_ITS          */
	"primary_tfm_s",     /* PARTITION_PRIMARY_TFM_S    */
	"primary_cpu0_app",  /* PARTITION_PRIMARY_CPU0_APP */
	"boot_param",        /* PARTITION_BOOT_PARAM       */
#if CONFIG_OTA_OVERWRITE
	"ota",               /* PARTITION_OTA              */
	"ota_control",       /* PARTITION_OTA_CONTROL      */
#endif
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
#if CONFIG_OTA_OVERWRITE
	[PARTITION_OTA] = {
		CONFIG_OTA_PHY_PARTITION_OFFSET,
		CONFIG_OTA_PHY_PARTITION_SIZE,
	},
	[PARTITION_OTA_CONTROL] = {
		CONFIG_OTA_CONTROL_PHY_PARTITION_OFFSET,
		CONFIG_OTA_CONTROL_PHY_PARTITION_SIZE,
	},
#endif
};

static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

uint32_t piece_address(uint8_t *array,uint32_t index)
{
    return ((uint32_t)(array[index]) << 24 | (uint32_t)(array[index+1])  << 16 | (uint32_t)(array[index+2])  << 8 | (uint32_t)((array[index+3])));
}

uint16_t short_address(uint8_t *array,uint16_t index)
{
    return ((uint16_t)(array[index]) << 8 | (uint16_t)(array[index+1]));
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
int partition_check_partition_address_valid(partition_id_e partition_id)
{
	if (partition_id >= PARTITION_CNT) {
		return -1;
	}

	const partition_config_t *config = &s_partition_config[partition_id];
	const partition_expected_t *expected = &s_partition_expected[partition_id];
	uint32_t partition_bit = (1u << partition_id);

	if (config->phy_offset == expected->phy_offset && config->phy_size == expected->phy_size) {
		s_partition_valid |= partition_bit;
		return 0;
	}

	s_partition_valid &= ~partition_bit;
	BK_LOGE(TAG, "%s offset=%x size=%x expected_offset=%x expected_size=%x\r\n",
		s_partition_name[partition_id], config->phy_offset, config->phy_size,
		expected->phy_offset, expected->phy_size);

	return -1;
}

int get_partion_valid(void)
{
	return s_partition_valid;
}

int partition_init(void)
{
	uint8_t buf[PARTITION_ENTRY_LEN];
	char name[PARTITION_NAME_LEN + 1];
	uint32_t partition_start = PARTITION_PARTITION_PHY_OFFSET + PARTITION_PPC_OFFSET;

	memset(s_partition_config, 0, sizeof(s_partition_config));

	for (uint32_t i = 0; i < PARTITION_AMOUNT; i++) {
		uint32_t entry_addr = partition_start + PARTITION_ENTRY_LEN * i;

		if (bk_flash_read_bytes(entry_addr, buf, sizeof(buf)) != BK_OK) {
			BK_LOGE(TAG, "read partition entry %u fails.\r\n", i);
			return -1;
		}

		if (is_alpha(buf[0]) == 0) {
			break;
		}

		uint32_t name_len = 0;
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

		for (int k = 0; k < PARTITION_CNT; k++) {
			if (strcmp(s_partition_name[k], name) == 0) {
				s_partition_config[k].phy_offset = piece_address(buf, PARTITION_OFFSET_OFFSET);
				s_partition_config[k].phy_size = piece_address(buf, PARTITION_SIZE_OFFSET);
				s_partition_config[k].phy_flags = short_address(buf, PARTITION_FLAGS_OFFSET);
				if (partition_check_partition_address_valid((partition_id_e)k) != 0) {
					return -1;
				}
				break;
			}
		}
	}

	return 0;
}

void dump_partition(void)
{
	for (int k = 0; k < PARTITION_CNT; k++) {
		BK_LOGI(TAG, "%s offset=%x size=%x flags=%x\r\n", s_partition_name[k], \
            s_partition_config[k].phy_offset, s_partition_config[k].phy_size, s_partition_config[k].phy_flags);
	}
}
