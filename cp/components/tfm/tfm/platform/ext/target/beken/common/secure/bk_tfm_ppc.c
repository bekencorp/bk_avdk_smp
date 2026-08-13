// Copyright     2023-2028 Beken
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

#include <stdbool.h>
#include <stdint.h>

#include "driver/efuse.h"
#include "sys_driver.h"
#include "bk_tfm_ppc.h"
#include "cmsis.h"
#include "common/bk_err.h"
#include "partitions_gen.h"

#define PPC_CONFIG_BIN_SIZE         0x200u
#if defined(CONFIG_PARTITION_PHY_PARTITION_OFFSET)
#define PPC_CP_CONFIG_FLASH_OFFSET  CONFIG_PARTITION_PHY_PARTITION_OFFSET
#else
#define PPC_CP_CONFIG_FLASH_OFFSET  CONFIG_PARTITION_PARTITION_OFFSET
#endif
#define PPC_AP_CONFIG_FLASH_OFFSET  (PPC_CP_CONFIG_FLASH_OFFSET + PPC_CONFIG_BIN_SIZE)

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf,
                                    uint32_t size);

static sys_lock_ctx_t s_ppc_lock;
static uint32_t s_sys_ns_flag;

/* Cold-boot cache of the plaintext CP/AP protection configs. Loaded once while
 * flash is still Secure; every later apply reads from RAM so the secure world
 * never touches the flash controller after CP marks the flash region Non-secure
 * (and so the config survives for AP re-prepare / register restore). */
static uint32_t s_cp_config[BK_PPC_CONFIG_WORD_COUNT];
static uint32_t s_ap_config[BK_PPC_CONFIG_WORD_COUNT];
static bool s_ppc_cache_valid;

static uint32_t ppc_get_bit(uint32_t reg, uint32_t bit)
{   
    uint32_t v = REG_READ((SOC_PPRO_REG_BASE + (reg << 2)));
    return (v & BIT(bit));
}

static void ppc_set_bit(uint32_t reg, uint32_t bit)
{   
    uint32_t v = REG_READ((SOC_PPRO_REG_BASE + (reg << 2)));
    v |= BIT(bit);
    REG_WRITE((SOC_PPRO_REG_BASE + (reg << 2)), v);
}   

static void ppc_clear_bit(uint32_t reg, uint32_t bit)
{
    uint32_t v = REG_READ((SOC_PPRO_REG_BASE + (reg << 2)));
    v &= ~BIT(bit);
    REG_WRITE((SOC_PPRO_REG_BASE + (reg << 2)), v);
}       

void bk_ppc_set_ap_master_nsec(void)
{
#if CONFIG_AP_BOOT_NSC
    /* AP master access security attribute, PPRO reg0xF[3:2]. PPRO is a Secure
     * register, so the secure world sets the AP master Non-secure here; the CP
     * Non-secure world then reaches the AP SYS/AHBP registers through the NS
     * alias. Applied after the CP config so the config words cannot clear it. */
    ppc_set_bit(0xF, 2);
    ppc_set_bit(0xF, 3);
    __DSB();
    __ISB();
#endif
}

int bk_ppc_apply_config(const uint32_t config[BK_PPC_CONFIG_WORD_COUNT])
{
    if (config == NULL) {
        return BK_ERR_PARAM;
    }

    for (uint32_t i = 0; i < BK_PPC_CONFIG_WORD_COUNT; i++) {
        REG_WRITE(SOC_PPRO_REG_BASE + ((0x4u + i) << 2), config[i]);
    }

    return BK_OK;
}

int bk_pphs_apply_config(const uint32_t config[BK_PPHS_CONFIG_WORD_COUNT])
{
    if (config == NULL) {
        return BK_ERR_PARAM;
    }

    /* Apply PPHS reg4..7 as masks, including the master security/privilege
     * bank used by VIDP_M and the other AP bus masters. Flash config marks SYS
     * Non-Secure so CP NS can program AP SysCfg; Secure AHBP access after this
     * must use the NS alias. */
    for (uint32_t i = 0; i < BK_PPHS_CONFIG_WORD_COUNT; i++) {
        volatile uint32_t *reg =
            (volatile uint32_t *)(SOC_PPHS_REG_BASE + ((0x4u + i) << 2));
        *reg |= config[i];
    }

    return BK_OK;
}

static uint32_t ppc_read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

static int ppc_load_config(uint32_t flash_offset,
                           uint32_t config[BK_PPC_CONFIG_WORD_COUNT])
{
    uint8_t data[BK_PPC_CONFIG_WORD_COUNT * sizeof(uint32_t)];
    bool all_erased = true;

    if (bk_flash_read_bytes(flash_offset, data, sizeof(data)) != BK_OK) {
        return BK_FAIL;
    }

    for (uint32_t i = 0; i < sizeof(data); i++) {
        if (data[i] != 0xFFu) {
            all_erased = false;
            break;
        }
    }
    if (all_erased) {
        return BK_FAIL;
    }

    for (uint32_t i = 0; i < BK_PPC_CONFIG_WORD_COUNT; i++) {
        config[i] = ppc_read_be32(&data[i * sizeof(uint32_t)]);
    }

    return BK_OK;
}

int bk_ppc_cache_load_from_flash(void)
{
    if (s_ppc_cache_valid) {
        return BK_OK;
    }

    /* Read and validate both plaintext images before caching either block. */
    if (ppc_load_config(PPC_CP_CONFIG_FLASH_OFFSET, s_cp_config) != BK_OK ||
        ppc_load_config(PPC_AP_CONFIG_FLASH_OFFSET, s_ap_config) != BK_OK) {
        return BK_FAIL;
    }

    s_ppc_cache_valid = true;
    return BK_OK;
}

int bk_ppc_apply_config_from_flash(void)
{
    if (bk_ppc_cache_load_from_flash() != BK_OK) {
        return BK_FAIL;
    }

    if (bk_ppc_apply_config(s_cp_config) != BK_OK ||
        bk_pphs_apply_config(s_ap_config) != BK_OK) {
        return BK_FAIL;
    }

    return BK_OK;
}

int bk_ppc_apply_cp_config_from_flash(void)
{
    if (bk_ppc_cache_load_from_flash() != BK_OK) {
        return BK_FAIL;
    }

    return bk_ppc_apply_config(s_cp_config);
}

int bk_pphs_apply_ap_config_from_flash(void)
{
    if (bk_ppc_cache_load_from_flash() != BK_OK) {
        return BK_FAIL;
    }

    return bk_pphs_apply_config(s_ap_config);
}

int bk_ppc_init(void)
{
    *((volatile uint32_t *)(SOC_PPRO_REG_BASE + 2 * 4)) = 1;
    *((volatile uint32_t *)(SOC_PPRO_REG_BASE + 8 * 4)) = BIT(16);
    *((volatile uint32_t *)(SOC_PPRO_REG_BASE + 5 * 4)) = 0;
    *((volatile uint32_t *)(SOC_PPRO_REG_BASE + 11 * 4)) = 0;
    return 0;
}

uint32_t bk_ppc_lock_flash(void)
{
        s_sys_ns_flag = ppc_get_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
        uint32_t flash_ns_flag = ppc_get_bit(PPC_FLASH_REG, PPC_FLASH_NS_BIT);

	//TODO disable CPU interrupt first

	ppc_clear_bit(PPC_SYS_REG, PPC_SYS_NS_BIT); //SYS to secure
	sys_drv_disable_int(&s_ppc_lock);
	sys_drv_set_base_addr(SOC_SYS_REG_BASE);
	ppc_clear_bit(PPC_FLASH_REG, PPC_FLASH_NS_BIT); //Flash to secure
	return flash_ns_flag;
}

void bk_ppc_unlock_flash(uint32_t flash_ns_flag)
{
	if (flash_ns_flag) {
		ppc_set_bit(PPC_FLASH_REG, PPC_FLASH_NS_BIT);
	} else {
		ppc_clear_bit(PPC_FLASH_REG, PPC_FLASH_NS_BIT);
	}

        sys_drv_enable_int(&s_ppc_lock);
	if (s_sys_ns_flag) {
		ppc_set_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
	} else {
		ppc_clear_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
	}
	//TODO enable CPU int
}

void bk_ppc_lock_sys(void)
{
        s_sys_ns_flag = ppc_get_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
	ppc_clear_bit(PPC_SYS_REG, PPC_SYS_NS_BIT); //SYS to secure
	//sys_drv_disable_int(&s_ppc_lock);
}

void bk_ppc_unlock_sys(void)
{
        //sys_drv_enable_int(&s_ppc_lock);
	if (s_sys_ns_flag) {
		ppc_set_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
	} else {
		ppc_clear_bit(PPC_SYS_REG, PPC_SYS_NS_BIT);
	}
}

