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

#include "cmsis.h"
#include "bk_sca_defense.h"
#include "sys_driver.h"
#include "sys_hal.h"
#include <modules/pm.h>
#include "otp.h"
#include "flash.h"
#include <driver/otp.h>
#include <stdbool.h>
#include <stdint.h>
#include "cache.h"

extern int sys_drv_video_power_en(uint32_t value);
extern void sys_hal_buck_switch(uint32_t flag);

#if CONFIG_SCA_DEFENSE
#define DECOY_PROBABILITY 0.5
#define CONST1 0xC0A0B000
#define CONST2 0x03050400

#if defined(__GNUC__)
#define BK_SCA_NO_OPT __attribute__((optimize("O0")))
#else
#define BK_SCA_NO_OPT
#endif

/* Random number cache for performance optimization */
static uint32_t random_cache[4] = {0};
static uint32_t random_cache_index = 0;
static bool random_cache_initialized = false;

/**
 * @brief Get a random number from OTP hardware TRNG
 * @return Random 32-bit value, or 0 if OTP read fails (should not happen in production)
 * @note In case of OTP failure, the function returns 0 which should trigger
 *       error handling in the calling code. This is safer than returning a fixed value.
 */
static uint32_t otp_get_random(void)
{
    uint32_t random_value;
    
    /* Use cached random numbers for better performance */
    if (random_cache_initialized && random_cache_index < 4) {
        random_value = random_cache[random_cache_index++];
        if (random_cache_index >= 4) {
            random_cache_index = 0;
            random_cache_initialized = false; /* Refill cache on next call */
        }
        return random_value;
    }
    
    /* Read random number from OTP hardware */
    if (bk_otp_read_random_number(&random_value, 1) == BK_OK) {
        return random_value;
    }
    
    /* OTP read failed - this should not happen in production
     * Return 0 to indicate failure, caller should handle this case
     */
    return 0;
}

/**
 * @brief Pre-fill random number cache for better performance
 * This reduces the number of OTP hardware accesses
 */
static void otp_refill_random_cache(void)
{
    if (bk_otp_read_random_number(random_cache, 4) == BK_OK) {
    random_cache_index = 0;
    random_cache_initialized = true;
}
}

/**
 * @brief Get a random number in the range [min, max]
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random number in the specified range, or min if OTP fails
 * @note Returns min if max < min or if OTP read fails
 */
static uint32_t otp_get_random_range(uint32_t min, uint32_t max)
{
    if (max < min) {
        return min; /* Invalid range, return min */
    }
    
    uint32_t range = max - min + 1;
    uint32_t random = otp_get_random();
    
    /* If OTP read failed (returned 0), use a simple counter-based fallback
     * This is not cryptographically secure but prevents complete failure
     */
    if (random == 0 && range > 1) {
        static uint32_t fallback_counter = 0;
        fallback_counter++;
        random = fallback_counter;
    }
    
    return min + (random % range);
}

// Use the hardware TRNG via OTP interface (True Random Number Generator)
uint32_t get_random(void)
{
    /* Pre-fill cache if needed */
    if (!random_cache_initialized) {
        otp_refill_random_cache();
    }
    
    /* Get random number in range [10, 200] for delay
     * Expanded range to increase difficulty of fault injection attacks
     */
    return otp_get_random_range(10, 200);
}

// Insert a delay before a sensitive operation
void bk_sca_secure_delay(void)
{
    uint32_t delay = get_random();
    // Busy - wait for 'delay' clock cycles
    for (volatile uint32_t i = 0; i < delay; i++); 
}

// Constant - time comparison (prevent timing leakage)
BK_SCA_NO_OPT
uint8_t bk_sca_ct_memcmp(const uint8_t *a, const uint8_t *b, size_t len)
{
    volatile uint8_t result = 0;
    // Calculate the bitwise XOR of corresponding elements in 'a' and 'b'
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    // Return whether the two arrays are equal in a constant - time manner
    uint8_t ret = result;
    return ret; 
}

// with decoy functions and non-trivial functions
BK_SCA_NO_OPT
uint8_t bk_sca_memcmp_nontrivial(const uint8_t *a, const uint8_t *b, size_t len)
{
    size_t i;
    volatile uint32_t diff_accum = 0;
    uint32_t rnd;
    uint32_t random_val;

    if (len == 0) {
        return 0;
    }

    /* Use OTP TRNG to get random starting index */
    rnd = otp_get_random_range(0, len - 1);

    for (i = 0; i < len; ) {
        size_t idx = (i + rnd) % len;
        /* Use OTP TRNG to determine if decoy operation */
        random_val = otp_get_random();
        int do_decoy = ((random_val & 0xFFFF) < (0xFFFF * DECOY_PROBABILITY));

        uint32_t tmpdiff;
        uint32_t decoy;

        if (do_decoy) {
            // decoy operation
            decoy = (CONST1 | 0) ^ (CONST2 | 0);
            tmpdiff = CONST1 | CONST2;
        } else {
            //read opteration
            tmpdiff = (CONST1 | a[idx]) ^ (CONST2 | b[idx]);
            decoy = CONST1 | CONST2;
            i++;
        }

        diff_accum |= (tmpdiff | decoy);
    }

    return (uint8_t)(diff_accum & 0xFF);
}

void shuffle_indices(uint32_t *indices, size_t len)
{
    if (!indices || len == 0) {
        return;
    }
    
    /* Fisher-Yates shuffle using OTP TRNG */
    for (size_t i = len - 1; i > 0; i--) {
        size_t j = otp_get_random_range(0, (uint32_t)i);
        /* XOR swap to avoid temporary variable */
        indices[i] = indices[i] ^ indices[j];
        indices[j] = indices[i] ^ indices[j];
        indices[i] = indices[i] ^ indices[j];
    }
}

/* Maximum size for secure memcpy to avoid stack overflow
 * For larger sizes, fall back to standard memcpy
 */
#define SCA_SECURE_MEMCPY_MAX_SIZE 256

// Randomize the memory access order
void *bk_sca_secure_memcpy(uint8_t *dst, const uint8_t *src, size_t len)
{
    if (!dst || !src || len == 0) {
        return dst;
    }

    /* For large buffers, use standard memcpy to avoid stack overflow
     * The security benefit of randomized access is less critical for large buffers
     */
    if (len > SCA_SECURE_MEMCPY_MAX_SIZE) {
        /* Fall back to standard memcpy for large buffers */
        for (size_t i = 0; i < len; i++) {
            dst[i] = src[i];
        }
        return dst;
    }

    /* Use static buffer to avoid VLA stack overflow issues */
    static uint32_t indices[SCA_SECURE_MEMCPY_MAX_SIZE];
    
    /* Initialize the indices array with sequential values */
    for (size_t i = 0; i < len; i++) {
        indices[i] = (uint32_t)i;
    }
    
    /* Shuffle the indices array randomly */
    shuffle_indices(indices, len); 

    /* Copy data in the shuffled order */
    for (size_t i = 0; i < len; i++) {
        dst[indices[i]] = src[indices[i]];
    }

    return dst;
}

void bk_sca_read_flash(uint32_t off_vir_addr)
{
    uint8_t src[32];
    uint32_t i = otp_get_random_range(0, 31);
    uint32_t flash_addr = off_vir_addr + (i * 32);
    
    /* Validate flash address to prevent out-of-bounds access */
    if (flash_addr < off_vir_addr) {
        /* Overflow detected, use base address */
        flash_addr = off_vir_addr;
    }
    
    flush_dcache((void *)(0x02000000 + flash_addr), 32);
    bk_flash_read_cbus(flash_addr, src, 32);
}

void bk_sca_power_switch(void)
{
    bk_sca_secure_delay();
    sys_drv_video_power_en(1);
    bk_sca_secure_delay();
    sys_drv_video_power_en(0);
}

static uint32_t g_bandmanual = 0;
static uint32_t g_cur_value = 0;
static bool g_freq_initialized = false;

void bk_sca_random_freq(void)
{
    /* Ensure initialization before use */
    if (!g_freq_initialized) {
        bk_sca_random_freq_init();
    }
    
    uint32_t random_val = otp_get_random();
    
    /* If OTP read failed, skip this update to avoid using invalid random value */
    if (random_val == 0) {
        return;
    }
    
    uint32_t offset = otp_get_random_range(0, 0x30);
    /* 50% probability to increase or decrease frequency */
    if ((random_val & 0x1) == 0) {
        if (g_cur_value + offset > g_bandmanual) {
            g_cur_value = g_bandmanual;
        } else {
            g_cur_value = g_cur_value + offset;
        }
    } else {
        if (offset > g_cur_value) {
            g_cur_value = 0;
        } else {
            g_cur_value = g_cur_value - offset;
        }
    }
    sys_hal_set_ana_reg1_bandmanual(g_cur_value);
}

void bk_sca_random_freq_init(void)
{
    g_bandmanual = sys_hal_get_ana_reg1_bandmanual();
    g_cur_value = g_bandmanual;
    
    if (sys_hal_get_ana_reg0_bp_caldone() == 0) {
        sys_hal_set_ana_reg0_bp_caldone(1);
    }
    if (sys_hal_get_ana_reg1_manual() == 0) {
        sys_hal_set_ana_reg1_manual(1);
    }
    if (sys_hal_get_ana_reg1_closeloop_en() == 1) {
        sys_hal_set_ana_reg1_closeloop_en(0);
    }
    
    g_freq_initialized = true;
}

void bk_sca_buck_switch(void)
{
    uint32_t switch_val = otp_get_random() & 0x1;
    sys_hal_buck_switch(switch_val);
    bk_sca_secure_delay();
}
#endif