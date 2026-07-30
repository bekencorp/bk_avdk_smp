/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdlib.h>
#include <time.h>
#include "tfm_hal_device_header.h"
#include "tfm_fih_rng.h"

fih_int tfm_fih_random_init(void)
{
    // SysTick->LOAD  = 0x000000FE;                  /* Set reload register */
    // SysTick->VAL   = 0UL;                         /* Load the SysTick Counter Value */
    // SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
    //                  SysTick_CTRL_ENABLE_Msk;     /* Enable SysTick Timer */
    srand(time(NULL));
    return FIH_SUCCESS;
}

void tfm_fih_random_generate(uint8_t *rng)
{
    /* Repeat reading SysTick value to mitigate instruction skip */
    // *rand = (uint8_t)SysTick->VAL;
    // *rand = (uint8_t)SysTick->VAL;
    // *rand = (uint8_t)SysTick->VAL;
    // *rand = (uint8_t)SysTick->VAL;
    // *rand = (uint8_t)SysTick->VAL;
    *rng = rand() % 256;
}
