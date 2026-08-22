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
#include "sys_driver.h"
#include "driver/wdt.h"

#define PRRO_REG02                      (*((volatile unsigned int *)(SOC_PPRO_REG_BASE + 0x2*4)))
#define PRRO_REG02_BIT_SOFT_RESET       (1)
#define PRRO_SOFT_RESET                 PRRO_REG02 |= PRRO_REG02_BIT_SOFT_RESET

#define PRRO_REG17_CMP0_ADDR_START      (SOC_PPRO_REG_BASE + 0x17*4)
#define PRRO_REG18_CMP0_ADDR_END        (SOC_PPRO_REG_BASE + 0x18*4)
#define PRRO_REG19_CMP0_DATA_SRC        (SOC_PPRO_REG_BASE + 0x19*4)
#define PRRO_REG1A_CMP0_DATA_DST        (SOC_PPRO_REG_BASE + 0x1a*4)

#define PRRO_REG1B_CMP1_ADDR_START      (SOC_PPRO_REG_BASE + 0x1B*4)
#define PRRO_REG1C_CMP1_ADDR_END        (SOC_PPRO_REG_BASE + 0x1C*4)
#define PRRO_REG1D_CMP1_DATA_SRC        (SOC_PPRO_REG_BASE + 0x1D*4)
#define PRRO_REG1E_CMP1_DATA_DST        (SOC_PPRO_REG_BASE + 0x1E*4)

#define PRRO_REG1F_CMP2_ADDR_START      (SOC_PPRO_REG_BASE + 0x1F*4)
#define PRRO_REG20_CMP2_ADDR_END        (SOC_PPRO_REG_BASE + 0x20*4)
#define PRRO_REG21_CMP2_DATA_SRC        (SOC_PPRO_REG_BASE + 0x21*4)
#define PRRO_REG22_CMP2_DATA_DST        (SOC_PPRO_REG_BASE + 0x22*4)

#define PRRO_REG23_CMP_INT_STATUS       (SOC_PPRO_REG_BASE + 0x23*4)

enum {
	FIH_DATA_EFUSE,
	FIH_DATA_MPC = FIH_DATA_EFUSE,
	FIH_DATA_BOOT_TYPE,
	FIH_DATA_DBUS = FIH_DATA_BOOT_TYPE,
	FIH_DATA_BOOT_FLAG,
	FIH_DATA_SAU = FIH_DATA_BOOT_FLAG,
	FIH_DATA_LCS,
	FIH_DATA_PUBLIC_KEY_HASH,
	FIH_DATA_MPU = FIH_DATA_PUBLIC_KEY_HASH,
	FIH_DATA_SIG,
	FIH_DATA_RESERVE3 = FIH_DATA_SIG,
	FIH_DATA_IMG_HASH,
	FIH_DATA_RESERVE4 = FIH_DATA_IMG_HASH,
	FIH_DATA_MSP_PC,
	FIH_DATA_INVALID,
};

#define FIH_DATA_EFUSE_V             0xF
#define FIH_DATA_EFUSE_S             0

#define FIH_DATA_BOOT_TYPE_V         0xF
#define FIH_DATA_BOOT_TYPE_S         4

#define FIH_DATA_BOOT_FLAG_V         0xF
#define FIH_DATA_BOOT_FLAG_S         8

#define FIH_DATA_LCS_V               0xF
#define FIH_DATA_LCS_S               12

#define FIH_DATA_PUBLIC_KEY_HASH_V   0xF
#define FIH_DATA_PUBLIC_KEY_HASH_S   16

#define FIH_DATA_SIG_V               0xF
#define FIH_DATA_SIG_S               20

#define FIH_DATA_IMG_HASH_V          0xF
#define FIH_DATA_IMG_HASH_S          24

#define FIH_DATA_MSP_PC_V            0xF
#define FIH_DATA_MSP_PC_S            28

#if CONFIG_HW_FIH
int bk_fih_init(void);
void bk_fih_set_src(uint32_t id, uint32_t data);
void bk_fih_set_dst(uint32_t id, uint32_t data);
int bk_fih_set_addr_range(uint32_t start, uint32_t end);
void bk_fih_start(void);
void bk_fih_stop(void);
void bk_fih_disable(void);
void bk_fih_validate(void);
#else
#define UNUSED_VAR(var) (void)(var)
/* Returns 0 (success) so call sites like FIH_ASSERT128(!bk_fih_init()) remain
 * valid expressions when hardware FIH is disabled (the empty expansion would
 * otherwise produce "expected expression" inside the negation). */
#define bk_fih_init(...) (0)
#define bk_fih_set_src(id, data) UNUSED_VAR(id); UNUSED_VAR(data)
#define bk_fih_set_dst(id, data) UNUSED_VAR(id); UNUSED_VAR(data)
#define bk_fih_set_addr_range(start, end) UNUSED_VAR(start); UNUSED_VAR(end)
#define bk_fih_start()
#define bk_fih_stop()
#define bk_fih_disable()
#define bk_fih_validate()
#endif

void dump_prro_regs(void);

#define FIH_LOOP1(op) op;
#define FIH_LOOP2(op) FIH_LOOP1(op); FIH_LOOP1(op);
#define FIH_LOOP4(op) FIH_LOOP2(op); FIH_LOOP2(op);
#define FIH_LOOP8(op) FIH_LOOP4(op); FIH_LOOP4(op);
#define FIH_LOOP16(op) FIH_LOOP8(op); FIH_LOOP8(op);
#define FIH_LOOP32(op) FIH_LOOP16(op); FIH_LOOP16(op);
#define FIH_LOOP64(op) FIH_LOOP32(op); FIH_LOOP32(op);
#define FIH_LOOP128(op) FIH_LOOP64(op); FIH_LOOP64(op);
#define FIH_LOOP256(op) FIH_LOOP128(op); FIH_LOOP128(op);

#define FIH_ASSERT1(condition) if (!(condition)) {update_wdt(0xa); update_aon_wdt(0xa);}
#define FIH_ASSERT2(condition) FIH_ASSERT1(condition); FIH_ASSERT1(condition);
#define FIH_ASSERT4(condition) FIH_ASSERT2(condition); FIH_ASSERT2(condition);
#define FIH_ASSERT8(condition) FIH_ASSERT4(condition); FIH_ASSERT4(condition);
#define FIH_ASSERT16(condition) FIH_ASSERT8(condition); FIH_ASSERT8(condition);
#define FIH_ASSERT32(condition) FIH_ASSERT16(condition); FIH_ASSERT16(condition);
#define FIH_ASSERT64(condition) FIH_ASSERT32(condition); FIH_ASSERT32(condition);
#define FIH_ASSERT128(condition) FIH_ASSERT64(condition); FIH_ASSERT64(condition);
#define FIH_ASSERT256(condition) FIH_ASSERT128(condition); FIH_ASSERT128(condition);