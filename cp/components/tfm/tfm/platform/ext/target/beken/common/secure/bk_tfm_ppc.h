#pragma once

#include <stdint.h>

#define BK_PPC_CONFIG_WORD_COUNT  12
#define BK_PPHS_CONFIG_WORD_COUNT 3

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
uint32_t bk_ppc_lock_flash(void);
void bk_ppc_unlock_flash(uint32_t flash_secure_flag);
void bk_ppc_lock_sys(void);
void bk_ppc_unlock_sys(void);
