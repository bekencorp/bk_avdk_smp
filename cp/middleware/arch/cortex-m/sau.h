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

#pragma once

/*
//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
*/

/*
// <e>Initialize Security Attribution Unit (SAU) CTRL register
*/
#define SAU_INIT_CTRL          1

/*
//   <q> Enable SAU
//   <i> Value for SAU->CTRL register bit ENABLE
*/
#define SAU_INIT_CTRL_ENABLE   1

/*
//   <o> When SAU is disabled
//     <0=> All Memory is Secure
//     <1=> All Memory is Non-Secure
//   <i> Value for SAU->CTRL register bit ALLNS
//   <i> When all Memory is Non-Secure (ALLNS is 1), IDAU can override memory map configuration.
*/
#define SAU_INIT_CTRL_ALLNS  0

/*
// </e>
*/

/*
// <h>Initialize Security Attribution Unit (SAU) Address Regions
// <i>SAU configuration specifies regions to be one of:
// <i> - Secure and Non-Secure Callable
// <i> - Non-Secure
// <i>Note: All memory regions not configured by SAU are Secure
*/
#define SAU_REGIONS_MAX   8                 /* Max. number of SAU regions */

/*
//   <e>Initialize SAU Region 0
//   <i> Setup SAU Region 0 memory attributes
*/
#define SAU_INIT_REGION0    1

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START0     0x00000000      /* start address of SAU region 0 */

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END0       0x0FFFFFFF      /* end address of SAU region 0 */

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC0       1
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 1
//   <i> Setup SAU Region 1 memory attributes
*/
#define SAU_INIT_REGION1    1

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START1     0x10000000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END1       0xEFFFFFFF

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC1       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 2
//   <i> Setup SAU Region 2 memory attributes
*/
#define SAU_INIT_REGION2    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START2     0x20200000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END2       0x203FFFFF

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC2       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 3
//   <i> Setup SAU Region 3 memory attributes
*/
#define SAU_INIT_REGION3    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START3     0x40000000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END3       0x40040000

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC3       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 4
//   <i> Setup SAU Region 4 memory attributes
*/
#define SAU_INIT_REGION4    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START4     0x00000000      /* start address of SAU region 4 */

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END4       0x00000000      /* end address of SAU region 4 */

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC4       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 5
//   <i> Setup SAU Region 5 memory attributes
*/
#define SAU_INIT_REGION5    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START5     0x00000000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END5       0x00000000

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC5       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 6
//   <i> Setup SAU Region 6 memory attributes
*/
#define SAU_INIT_REGION6    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START6     0x00000000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END6       0x00000000

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC6       0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 7
//   <i> Setup SAU Region 7 memory attributes
*/
#define SAU_INIT_REGION7    0

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START7     0x00000000

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END7       0x00000000

/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC7       0
/*
//   </e>
*/

/*
// </h>
*/

/*
// <e>Setup behaviour of Sleep and Exception Handling
*/
#define SCB_CSR_AIRCR_INIT  1

/*
//   <o> Deep Sleep can be enabled by
//     <0=>Secure and Non-Secure state
//     <1=>Secure state only
//   <i> Value for SCB->CSR register bit DEEPSLEEPS
*/
#define SCB_CSR_DEEPSLEEPS_VAL  1

/*
//   <o>System reset request accessible from
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for SCB->AIRCR register bit SYSRESETREQS
*/
#define SCB_AIRCR_SYSRESETREQS_VAL  1

/*
//   <o>Priority of Non-Secure exceptions is
//     <0=> Not altered
//     <1=> Lowered to 0x80-0xFF
//   <i> Value for SCB->AIRCR register bit PRIS
*/
#define SCB_AIRCR_PRIS_VAL      1

/*
//   <o>BusFault, HardFault, and NMI target
//     <0=> Secure state
//     <1=> Non-Secure state
//   <i> Value for SCB->AIRCR register bit BFHFNMINS
*/
#define SCB_AIRCR_BFHFNMINS_VAL 0

/*
// </e>
*/

/*
// <e>Setup behaviour of Floating Point Unit
*/
#define TZ_FPU_NS_USAGE 1

/*
// <o>Floating Point Unit usage
//     <0=> Secure state only
//     <3=> Secure and Non-Secure state
//   <i> Value for SCB->NSACR register bits CP10, CP11
*/
#define SCB_NSACR_CP10_11_VAL       3

/*
// <o>Treat floating-point registers as Secure
//     <0=> Disabled
//     <1=> Enabled
//   <i> Value for FPU->FPCCR register bit TS
*/
#define FPU_FPCCR_TS_VAL            0

/*
// <o>Clear on return (CLRONRET) accessibility
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for FPU->FPCCR register bit CLRONRETS
*/
#define FPU_FPCCR_CLRONRETS_VAL     0

/*
// <o>Clear floating-point caller saved registers on exception return
//     <0=> Disabled
//     <1=> Enabled
//   <i> Value for FPU->FPCCR register bit CLRONRET
*/
#define FPU_FPCCR_CLRONRET_VAL      1

/*
    max 128 SAU regions.
    SAU regions are defined in partition.h
 */

#define SAU_INIT_REGION(n) \
    SAU->RNR  =  (n                                     & SAU_RNR_REGION_Msk); \
    SAU->RBAR =  (SAU_INIT_START##n                     & SAU_RBAR_BADDR_Msk); \
    SAU->RLAR =  (SAU_INIT_END##n                       & SAU_RLAR_LADDR_Msk) | \
                ((SAU_INIT_NSC##n << SAU_RLAR_NSC_Pos)  & SAU_RLAR_NSC_Msk)   | 1U

/**
  \brief   Setup a SAU Region
  \details Writes the region information contained in SAU_Region to the
           registers SAU_RNR, SAU_RBAR, and SAU_RLAR
 */
__STATIC_INLINE void TZ_SAU_Setup (void)
{
  #if defined (SAU_INIT_REGION0) && (SAU_INIT_REGION0 == 1U)
    SAU_INIT_REGION(0);
  #endif

  #if defined (SAU_INIT_REGION1) && (SAU_INIT_REGION1 == 1U)
    SAU_INIT_REGION(1);
  #endif

  #if defined (SAU_INIT_REGION2) && (SAU_INIT_REGION2 == 1U)
    SAU_INIT_REGION(2);
  #endif

  #if defined (SAU_INIT_REGION3) && (SAU_INIT_REGION3 == 1U)
    SAU_INIT_REGION(3);
  #endif

  #if defined (SAU_INIT_REGION4) && (SAU_INIT_REGION4 == 1U)
    SAU_INIT_REGION(4);
  #endif

  #if defined (SAU_INIT_REGION5) && (SAU_INIT_REGION5 == 1U)
    SAU_INIT_REGION(5);
  #endif

  #if defined (SAU_INIT_REGION6) && (SAU_INIT_REGION6 == 1U)
    SAU_INIT_REGION(6);
  #endif

  #if defined (SAU_INIT_REGION7) && (SAU_INIT_REGION7 == 1U)
    SAU_INIT_REGION(7);
  #endif

  #if defined (SAU_INIT_CTRL) && (SAU_INIT_CTRL == 1U)
    SAU->CTRL = ((SAU_INIT_CTRL_ENABLE << SAU_CTRL_ENABLE_Pos) & SAU_CTRL_ENABLE_Msk) |
                ((SAU_INIT_CTRL_ALLNS  << SAU_CTRL_ALLNS_Pos)  & SAU_CTRL_ALLNS_Msk)   ;
  #endif

  #if defined (SCB_CSR_AIRCR_INIT) && (SCB_CSR_AIRCR_INIT == 1U)
    SCB->SCR   = (SCB->SCR   & ~(SCB_SCR_SLEEPDEEPS_Msk    )) |
                   ((SCB_CSR_DEEPSLEEPS_VAL     << SCB_SCR_SLEEPDEEPS_Pos)     & SCB_SCR_SLEEPDEEPS_Msk);

    SCB->AIRCR = (SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk   | SCB_AIRCR_SYSRESETREQS_Msk |
                                 SCB_AIRCR_BFHFNMINS_Msk | SCB_AIRCR_PRIS_Msk          ))                    |
                   ((0x05FAU                    << SCB_AIRCR_VECTKEY_Pos)      & SCB_AIRCR_VECTKEY_Msk)      |
                   ((SCB_AIRCR_SYSRESETREQS_VAL << SCB_AIRCR_SYSRESETREQS_Pos) & SCB_AIRCR_SYSRESETREQS_Msk) |
                   ((SCB_AIRCR_PRIS_VAL         << SCB_AIRCR_PRIS_Pos)         & SCB_AIRCR_PRIS_Msk)         |
                   ((SCB_AIRCR_BFHFNMINS_VAL    << SCB_AIRCR_BFHFNMINS_Pos)    & SCB_AIRCR_BFHFNMINS_Msk);
  #endif /* defined (SCB_CSR_AIRCR_INIT) && (SCB_CSR_AIRCR_INIT == 1U) */
}
// eof
