/* 
 * Copyright     2023-2028 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http:*www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "tfm_hal_defs.h"
// #include "psa/crypto.h"

#if CONFIG_SCA_DEFENSE

void bk_sca_secure_delay(void);

uint8_t bk_sca_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t len);

uint8_t bk_sca_memcmp_nontrivial(const uint8_t *a, const uint8_t *b, size_t len);

void *bk_sca_secure_memcpy(uint8_t *dst, const uint8_t *src, size_t len);

void bk_sca_read_flash(uint32_t off_vir_addr);

void bk_sca_power_switch(void);

void bk_sca_random_freq(void);

void bk_sca_random_freq_init(void);

void bk_sca_buck_switch(void);

#define SECURE_MEMCMP(a, b, len) bk_sca_memcmp_nontrivial((const uint8_t *)(a), (const uint8_t *)(b), (len))

#else
#define bk_sca_secure_delay()
#define bk_sca_ct_memcmp(dst, src, len)    memcmp(dst, src, len)
#define bk_sca_memcmp_nontrivial(dst, src, len)    memcmp(dst, src, len)
#define bk_sca_secure_memcpy(dst, src, len)            memcpy(dst, src, len)
#define bk_sca_read_flash(off_vir_addr)
#define bk_sca_power_switch()
#define bk_sca_random_freq()
#define bk_sca_random_freq_init()   
#define bk_sca_buck_switch()
#define SECURE_MEMCMP(a, b, len) memcmp((a), (b), (len))
#endif
