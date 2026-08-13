#pragma once

#include <stdint.h>

#define BK_PPC_CONFIG_WORD_COUNT  12
#define BK_PPHS_CONFIG_WORD_COUNT 4

/* PPRO aonp_sap sits at reg7: aon_gpio_nsec2 occupies reg6, which shifts the
 * peripheral nsec/ap bits to reg7. */
#define PPC_FLASH_REG    7
#define PPC_FLASH_NS_BIT 6
#define PPC_SYS_REG      7
#define PPC_SYS_NS_BIT   5

int bk_ppc_init(void);
int bk_ppc_apply_config(const uint32_t config[BK_PPC_CONFIG_WORD_COUNT]);
int bk_pphs_apply_config(const uint32_t config[BK_PPHS_CONFIG_WORD_COUNT]);
int bk_ppc_apply_config_from_flash(void);

/* Cold-boot: read the plaintext CP/AP protection configs from flash into RAM
 * while flash is still Secure. All later apply_*_from_flash calls read from the
 * RAM cache, so the secure world never accesses the flash controller after CP
 * marks the flash region Non-secure. Idempotent. */
int bk_ppc_cache_load_from_flash(void);

/* Apply only the CP-side PPRO attributes from the signed flash config. Used
 * before entering CP NS; the AP-side PPHS is deferred until the AP domain is
 * powered (bk_pphs_apply_ap_config_from_flash from the secure-prepare NSC). */
int bk_ppc_apply_cp_config_from_flash(void);

/* Apply only the AP-side PPHS attributes from the signed flash config. The AP
 * power domain must already be on. Applied from psa_ap_secure_prepare(). */
int bk_pphs_apply_ap_config_from_flash(void);

/* Secure-world only: mark the AP master accesses Non-secure (PPRO reg0xF[3:2]).
 * PPRO is a Secure register, so this is set from the secure world before CP
 * enters NS, letting CP NS reach the AP SYS/AHBP registers via the NS alias.
 * No-op unless CONFIG_AP_BOOT_NSC is enabled. */
void bk_ppc_set_ap_master_nsec(void);
uint32_t bk_ppc_lock_flash(void);
void bk_ppc_unlock_flash(uint32_t flash_secure_flag);
void bk_ppc_lock_sys(void);
void bk_ppc_unlock_sys(void);
