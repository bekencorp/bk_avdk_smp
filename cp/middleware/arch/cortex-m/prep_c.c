// Copyright 2020-2025 Beken
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

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_ARMV8_M_MAINLINE
#include "armv8m_reg.h"
#endif

#include "cmsis_gcc.h"
#include "bk_arch.h"

#include "os/os.h"
#include "cache.h"


void _soc_start(void);
void soc_prep_data_relocation(void);

#define sys_write_word(addr,val)                 *((volatile uint32_t *)(addr)) = val    /**< write value by word size */
#define sys_read_word(addr,val)                  val = *((volatile uint32_t *)(addr))    /**< read value by word size */
#define sys_get_word(addr)                       *((volatile uint32_t *)(addr))          /**< get value by word size */

/* Unrolled copy: runs from IRAM to avoid Flash code/data read contention (faster .data copy). */
#define COPY_UNROLL 8
__attribute__((optimize("-O3"))) __attribute__((section(".iram"))) __attribute__((noinline))
void data_copy_block_ram(uint32_t *dest, const uint32_t *src, uint32_t word_cnt)
{
    uint32_t n = word_cnt / COPY_UNROLL;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t *d = dest + (i * COPY_UNROLL);
        const uint32_t *s = src + (i * COPY_UNROLL);
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
    }
    uint32_t r = word_cnt & (COPY_UNROLL - 1);
    dest += n * COPY_UNROLL;
    src += n * COPY_UNROLL;
    for (uint32_t i = 0; i < r; i++)
        dest[i] = src[i];
}

__attribute__((optimize("-O3"))) \
__attribute__((section(".flash_boot_code"))) \
__STATIC_FORCEINLINE void sys_memcpy_word(uint32_t *out, const uint32_t *in, uint32_t word_cnt)
{
    for(int i = 0; i < word_cnt; i++)
    {
        sys_write_word((out + i), sys_get_word(in + i));
    }

}

__attribute__((optimize("-O3"))) \
__attribute__((section(".flash_boot_code"))) \
__STATIC_FORCEINLINE void sys_memset_word(uint32_t *b, uint32_t word_cnt)
{
    // Note:
    // the word count == sizeof(buf)/sizeof(uint32_t)
    for(int i = 0; i < word_cnt; i++)
    {
        sys_write_word((b + i), 0);
    }
}

/**
 * @file
 * @brief Full C support initialization
 *
 */
void b_bss_zero(void)
{
  void * memset(void *, int, unsigned int);

  typedef struct {
    uint32_t* dest;
    uint32_t  wlen;
  } __zero_table_t;

  extern const __zero_table_t __zero_table_start__;
  extern const __zero_table_t __zero_table_end__;

  for (__zero_table_t const* pTable = &__zero_table_start__; pTable < &__zero_table_end__; ++pTable) {
    memset(pTable->dest, 0, pTable->wlen*sizeof(uint32_t));
  }
}

void b_data_copy(void)
{
  void * memcpy(void *, const void *, unsigned int);

  typedef struct {
    uint32_t const* src;
    uint32_t* dest;
    uint32_t  wlen;
  } __copy_table_t;

  extern const __copy_table_t __copy_table_start__;
  extern const __copy_table_t __copy_table_end__;

  __copy_table_t const* pTable = &__copy_table_start__;
  /* First entry: copy IRAM (so data_copy_block_ram is in IRAM). Use memcpy. */
  if (pTable < &__copy_table_end__) {
    memcpy(pTable->dest, pTable->src, pTable->wlen * sizeof(uint32_t));
    ++pTable;
  }
  /* Remaining entries (e.g. .data): run copy from IRAM to reduce Flash contention. */
  for (; pTable < &__copy_table_end__; ++pTable) {
    data_copy_block_ram(pTable->dest, pTable->src, pTable->wlen);
  }
}

#if (CONFIG_SUPPORT_FPU)
static inline void b_arm_floating_point_init(void)
{
	/*
	 * Upon reset, the Co-Processor Access Control Register is, normally,
	 * 0x00000000. However, it might be left un-cleared by firmware running
	 * before Zephyr boot.
	 */
	SYS_CTRL_BLK->CPACR &= (~(CPACR_CP10_MSK | CPACR_CP11_MSK));

	/* Full access */
	SYS_CTRL_BLK->CPACR |= CPACR_CP10_FULL_ACCESS | CPACR_CP11_FULL_ACCESS;
	
	/* Make the side-effects of modifying the FPCCR be realized
	 * immediately.
	 * barrier_dsync_fence_full();
	 * barrier_isync_fence_full();
	 */

	/* Initialize the Floating Point Status and Control Register. */
#if defined(CONFIG_ARMV8_1_M_MAINLINE)
	/*
	 * For ARMv8.1-M with FPU, the FPSCR[18:16] LTPSIZE field must be set
	 * to 0b100 for "Tail predication not applied" as it's reset value
	 */
	__set_FPSCR(4 << FPU_FPDSCR_LTPSIZE_Pos);
#else
	__set_FPSCR(0);
#endif

	/*
	 * Note:
	 * The use of the FP register bank is enabled, however the FP context
	 * will be activated (FPCA bit on the CONTROL register) in the presence
	 * of floating point instructions.
	 * Upon reset, the CONTROL.FPCA bit is, normally, cleared. However,
	 * it might be left un-cleared by firmware running before Zephyr boot.
	 * We must clear this bit to prevent errors in exception unstacking.
	 *
	 * Note:
	 * In Sharing FP Registers mode CONTROL.FPCA is cleared before switching
	 * to main, so it may be skipped here (saving few boot cycles).
	 *
	 * If CONFIG_INIT_ARCH_HW_AT_BOOT is set, CONTROL is cleared at reset.
	 */
}
#endif /* CONFIG_SUPPORT_FPU */

/**
 *
 * @brief Prepare to and run Entry Main
 *
 * This routine prepares for the execution of and runs Entry code.
 *
 */
void b_prep_entry_main(void)
{
#if (CONFIG_SOC_PREP_HOOK)
	void soc_prep_hook(void);

	soc_prep_hook();
#endif

#if (CONFIG_SUPPORT_FPU)
	b_arm_floating_point_init();
#endif

	soc_prep_data_relocation();

#if CONFIG_SOC_SMP
    uint32_t core_id = portGET_CORE_ID();
    if (core_id == 0) {
#endif
		b_bss_zero();
		b_data_copy();
#if CONFIG_SOC_SMP
	}
#endif

#if CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT
	z_arm_debug_enable_null_pointer_detection();
#endif
	arch_icache_invd_all();
}

void b_program_start(void)
{
	_soc_start();
}
