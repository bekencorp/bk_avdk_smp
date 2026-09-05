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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BL2 flash-map bring-up (definition in common/bl2/flash_map.c). */
int flash_map_init(void);

/* flash_map index -> virtual/physical geometry (common/bl2/flash_map.c). */
uint32_t get_flash_map_offset(uint32_t index);
uint32_t get_flash_map_size(uint32_t index);
uint32_t get_flash_map_phy_size(uint32_t index);

/* Block-granular fast erase used by the OTA install (cmsis_drivers/Driver_Flash.c). */
int flash_area_erase_fast(uint32_t erase_off, uint32_t len);

#if CONFIG_OTA_OVERWRITE
/* CBUS encrypt-on-write (0x04 + HW XTS-AES). Overwrite only; see flash_min.c. */
void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size);
#endif

/* CBUS read: XTS-decrypts on read (overwrite verify + SCA). */
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size);

#ifdef __cplusplus
}
#endif
