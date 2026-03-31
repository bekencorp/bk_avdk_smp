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

/* IO definitions (access restrictions to peripheral registers) */
/**
    \defgroup CMSIS_glob_defs CMSIS Global Defines

    <strong>IO Type Qualifiers</strong> are used
    \li to specify the access to peripheral variables.
    \li for automatic generation of peripheral register debug information.
*/
#define     _O_     volatile             /*!< Defines 'write only' permissions */
#define     _IO_    volatile             /*!< Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     _IM_     volatile const      /*! Defines 'read only' structure member permissions */
#define     _OM_     volatile            /*! Defines 'write only' structure member permissions */
#define     _IOM_    volatile            /*! Defines 'read / write' structure member permissions */


/*******************************************************************************
 *                 Register Abstraction
  Core Register contain:
  - Core Register
  - Core SCB Register
 ******************************************************************************/
/**
  \defgroup CMSIS_core_register Defines and Type Definitions
  \brief Type definitions and defines for Cortex-M processor based devices.
*/

/**
  \ingroup    CMSIS_core_register
  \defgroup   CMSIS_CORE  Status and Control Registers
  \brief      Core Register type definitions.
  @{
 */
/**
  \ingroup  CMSIS_core_register
  \defgroup CMSIS_SCB     System Control Block (SCB)
  \brief    Type definitions for the System Control Block Registers
  @{
 */

/**
  \brief  Structure type to access the System Control Block (SCB).
 */
typedef struct
{
  _IM_  uint32_t CPUID;                  /*!< Offset: 0x000 (R/ )  CPUID Base Register */
  _IOM_ uint32_t ICSR;                   /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
  _IOM_ uint32_t VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
  _IOM_ uint32_t AIRCR;                  /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
  _IOM_ uint32_t SCR;                    /*!< Offset: 0x010 (R/W)  System Control Register */
  _IOM_ uint32_t CCR;                    /*!< Offset: 0x014 (R/W)  Configuration Control Register */
  _IOM_ uint8_t  SHPR[12U];              /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
  _IOM_ uint32_t SHCSR;                  /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
  _IOM_ uint32_t CFSR;                   /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register */
  _IOM_ uint32_t HFSR;                   /*!< Offset: 0x02C (R/W)  HardFault Status Register */
  _IOM_ uint32_t DFSR;                   /*!< Offset: 0x030 (R/W)  Debug Fault Status Register */
  _IOM_ uint32_t MMFAR;                  /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register */
  _IOM_ uint32_t BFAR;                   /*!< Offset: 0x038 (R/W)  BusFault Address Register */
  _IOM_ uint32_t AFSR;                   /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
  _IM_  uint32_t ID_PFR[2U];             /*!< Offset: 0x040 (R/ )  Processor Feature Register */
  _IM_  uint32_t ID_DFR;                 /*!< Offset: 0x048 (R/ )  Debug Feature Register */
  _IM_  uint32_t ID_AFR;                 /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register */
  _IM_  uint32_t ID_MMFR[4U];            /*!< Offset: 0x050 (R/ )  Memory Model Feature Register */
  _IM_  uint32_t ID_ISAR[6U];            /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
  _IM_  uint32_t CLIDR;                  /*!< Offset: 0x078 (R/ )  Cache Level ID register */
  _IM_  uint32_t CTR;                    /*!< Offset: 0x07C (R/ )  Cache Type register */
  _IM_  uint32_t CCSIDR;                 /*!< Offset: 0x080 (R/ )  Cache Size ID Register */
  _IOM_ uint32_t CSSELR;                 /*!< Offset: 0x084 (R/W)  Cache Size Selection Register */
  _IOM_ uint32_t CPACR;                  /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
  _IOM_ uint32_t NSACR;                  /*!< Offset: 0x08C (R/W)  Non-Secure Access Control Register */
        uint32_t RESERVED0[21U];
  _IOM_ uint32_t SFSR;                   /*!< Offset: 0x0E4 (R/W)  Secure Fault Status Register */
  _IOM_ uint32_t SFAR;                   /*!< Offset: 0x0E8 (R/W)  Secure Fault Address Register */
        uint32_t RESERVED1[69U];
  _OM_  uint32_t STIR;                   /*!< Offset: 0x200 ( /W)  Software Triggered Interrupt Register */
  _IOM_ uint32_t RFSR;                   /*!< Offset: 0x204 (R/W)  RAS Fault Status Register */
        uint32_t RESERVED2[14U];
  _IM_  uint32_t MVFR0;                  /*!< Offset: 0x240 (R/ )  Media and VFP Feature Register 0 */
  _IM_  uint32_t MVFR1;                  /*!< Offset: 0x244 (R/ )  Media and VFP Feature Register 1 */
  _IM_  uint32_t MVFR2;                  /*!< Offset: 0x248 (R/ )  Media and VFP Feature Register 2 */
        uint32_t RESERVED3[1U];
  _OM_  uint32_t ICIALLU;                /*!< Offset: 0x250 ( /W)  I-Cache Invalidate All to PoU */
        uint32_t RESERVED4[1U];
  _OM_  uint32_t ICIMVAU;                /*!< Offset: 0x258 ( /W)  I-Cache Invalidate by MVA to PoU */
  _OM_  uint32_t DCIMVAC;                /*!< Offset: 0x25C ( /W)  D-Cache Invalidate by MVA to PoC */
  _OM_  uint32_t DCISW;                  /*!< Offset: 0x260 ( /W)  D-Cache Invalidate by Set-way */
  _OM_  uint32_t DCCMVAU;                /*!< Offset: 0x264 ( /W)  D-Cache Clean by MVA to PoU */
  _OM_  uint32_t DCCMVAC;                /*!< Offset: 0x268 ( /W)  D-Cache Clean by MVA to PoC */
  _OM_  uint32_t DCCSW;                  /*!< Offset: 0x26C ( /W)  D-Cache Clean by Set-way */
  _OM_  uint32_t DCCIMVAC;               /*!< Offset: 0x270 ( /W)  D-Cache Clean and Invalidate by MVA to PoC */
  _OM_  uint32_t DCCISW;                 /*!< Offset: 0x274 ( /W)  D-Cache Clean and Invalidate by Set-way */
  _OM_  uint32_t BPIALL;                 /*!< Offset: 0x278 ( /W)  Branch Predictor Invalidate All */
} SCB_TYPE;

/** \brief SCB CPUID Register Definitions */
#define SCB_CPUID_IMPLEMENTER_POS          24U                                            /*!< SCB CPUID: IMPLEMENTER Position */
#define SCB_CPUID_IMPLEMENTER_MSK          (0xFFUL << SCB_CPUID_IMPLEMENTER_POS)          /*!< SCB CPUID: IMPLEMENTER Mask */

#define SCB_CPUID_VARIANT_POS              20U                                            /*!< SCB CPUID: VARIANT Position */
#define SCB_CPUID_VARIANT_MSK              (0xFUL << SCB_CPUID_VARIANT_POS)               /*!< SCB CPUID: VARIANT Mask */

#define SCB_CPUID_ARCHITECTURE_POS         16U                                            /*!< SCB CPUID: ARCHITECTURE Position */
#define SCB_CPUID_ARCHITECTURE_MSK         (0xFUL << SCB_CPUID_ARCHITECTURE_POS)          /*!< SCB CPUID: ARCHITECTURE Mask */

#define SCB_CPUID_PARTNO_POS                4U                                            /*!< SCB CPUID: PARTNO Position */
#define SCB_CPUID_PARTNO_MSK               (0xFFFUL << SCB_CPUID_PARTNO_POS)              /*!< SCB CPUID: PARTNO Mask */

#define SCB_CPUID_REVISION_POS              0U                                            /*!< SCB CPUID: REVISION Position */
#define SCB_CPUID_REVISION_MSK             (0xFUL /*<< SCB_CPUID_REVISION_POS*/)          /*!< SCB CPUID: REVISION Mask */

/** \brief SCB Interrupt Control State Register Definitions */
#define SCB_ICSR_PENDNMISET_POS            31U                                            /*!< SCB ICSR: PENDNMISET Position */
#define SCB_ICSR_PENDNMISET_MSK            (1UL << SCB_ICSR_PENDNMISET_POS)               /*!< SCB ICSR: PENDNMISET Mask */

#define SCB_ICSR_PENDNMICLR_POS            30U                                            /*!< SCB ICSR: PENDNMICLR Position */
#define SCB_ICSR_PENDNMICLR_MSK            (1UL << SCB_ICSR_PENDNMICLR_POS)               /*!< SCB ICSR: PENDNMICLR Mask */

#define SCB_ICSR_PENDSVSET_POS             28U                                            /*!< SCB ICSR: PENDSVSET Position */
#define SCB_ICSR_PENDSVSET_MSK             (1UL << SCB_ICSR_PENDSVSET_POS)                /*!< SCB ICSR: PENDSVSET Mask */

#define SCB_ICSR_PENDSVCLR_POS             27U                                            /*!< SCB ICSR: PENDSVCLR Position */
#define SCB_ICSR_PENDSVCLR_MSK             (1UL << SCB_ICSR_PENDSVCLR_POS)                /*!< SCB ICSR: PENDSVCLR Mask */

#define SCB_ICSR_PENDSTSET_POS             26U                                            /*!< SCB ICSR: PENDSTSET Position */
#define SCB_ICSR_PENDSTSET_MSK             (1UL << SCB_ICSR_PENDSTSET_POS)                /*!< SCB ICSR: PENDSTSET Mask */

#define SCB_ICSR_PENDSTCLR_POS             25U                                            /*!< SCB ICSR: PENDSTCLR Position */
#define SCB_ICSR_PENDSTCLR_MSK             (1UL << SCB_ICSR_PENDSTCLR_POS)                /*!< SCB ICSR: PENDSTCLR Mask */

#define SCB_ICSR_STTNS_POS                 24U                                            /*!< SCB ICSR: STTNS Position (Security Extension) */
#define SCB_ICSR_STTNS_MSK                 (1UL << SCB_ICSR_STTNS_POS)                    /*!< SCB ICSR: STTNS Mask (Security Extension) */

#define SCB_ICSR_ISRPREEMPT_POS            23U                                            /*!< SCB ICSR: ISRPREEMPT Position */
#define SCB_ICSR_ISRPREEMPT_MSK            (1UL << SCB_ICSR_ISRPREEMPT_POS)               /*!< SCB ICSR: ISRPREEMPT Mask */

#define SCB_ICSR_ISRPENDING_POS            22U                                            /*!< SCB ICSR: ISRPENDING Position */
#define SCB_ICSR_ISRPENDING_MSK            (1UL << SCB_ICSR_ISRPENDING_POS)               /*!< SCB ICSR: ISRPENDING Mask */

#define SCB_ICSR_VECTPENDING_POS           12U                                            /*!< SCB ICSR: VECTPENDING Position */
#define SCB_ICSR_VECTPENDING_MSK           (0x1FFUL << SCB_ICSR_VECTPENDING_POS)          /*!< SCB ICSR: VECTPENDING Mask */

#define SCB_ICSR_RETTOBASE_POS             11U                                            /*!< SCB ICSR: RETTOBASE Position */
#define SCB_ICSR_RETTOBASE_MSK             (1UL << SCB_ICSR_RETTOBASE_POS)                /*!< SCB ICSR: RETTOBASE Mask */

#define SCB_ICSR_VECTACTIVE_POS             0U                                            /*!< SCB ICSR: VECTACTIVE Position */
#define SCB_ICSR_VECTACTIVE_MSK            (0x1FFUL /*<< SCB_ICSR_VECTACTIVE_POS*/)       /*!< SCB ICSR: VECTACTIVE Mask */

/** \brief SCB Vector Table Offset Register Definitions */
#define SCB_VTOR_TBLOFF_POS                 7U                                            /*!< SCB VTOR: TBLOFF Position */
#define SCB_VTOR_TBLOFF_MSK                (0x1FFFFFFUL << SCB_VTOR_TBLOFF_POS)           /*!< SCB VTOR: TBLOFF Mask */

/** \brief SCB Application Interrupt and Reset Control Register Definitions */
#define SCB_AIRCR_VECTKEY_POS              16U                                            /*!< SCB AIRCR: VECTKEY Position */
#define SCB_AIRCR_VECTKEY_MSK              (0xFFFFUL << SCB_AIRCR_VECTKEY_POS)            /*!< SCB AIRCR: VECTKEY Mask */

#define SCB_AIRCR_VECTKEYSTAT_POS          16U                                            /*!< SCB AIRCR: VECTKEYSTAT Position */
#define SCB_AIRCR_VECTKEYSTAT_MSK          (0xFFFFUL << SCB_AIRCR_VECTKEYSTAT_POS)        /*!< SCB AIRCR: VECTKEYSTAT Mask */

#define SCB_AIRCR_ENDIANNESS_POS           15U                                            /*!< SCB AIRCR: ENDIANNESS Position */
#define SCB_AIRCR_ENDIANNESS_MSK           (1UL << SCB_AIRCR_ENDIANNESS_POS)              /*!< SCB AIRCR: ENDIANNESS Mask */

#define SCB_AIRCR_PRIS_POS                 14U                                            /*!< SCB AIRCR: PRIS Position */
#define SCB_AIRCR_PRIS_MSK                 (1UL << SCB_AIRCR_PRIS_POS)                    /*!< SCB AIRCR: PRIS Mask */

#define SCB_AIRCR_BFHFNMINS_POS            13U                                            /*!< SCB AIRCR: BFHFNMINS Position */
#define SCB_AIRCR_BFHFNMINS_MSK            (1UL << SCB_AIRCR_BFHFNMINS_POS)               /*!< SCB AIRCR: BFHFNMINS Mask */

#define SCB_AIRCR_PRIGROUP_POS              8U                                            /*!< SCB AIRCR: PRIGROUP Position */
#define SCB_AIRCR_PRIGROUP_MSK             (7UL << SCB_AIRCR_PRIGROUP_POS)                /*!< SCB AIRCR: PRIGROUP Mask */

#define SCB_AIRCR_IESB_POS                  5U                                            /*!< SCB AIRCR: Implicit ESB Enable Position */
#define SCB_AIRCR_IESB_MSK                 (1UL << SCB_AIRCR_IESB_POS)                    /*!< SCB AIRCR: Implicit ESB Enable Mask */

#define SCB_AIRCR_DIT_POS                   4U                                            /*!< SCB AIRCR: Data Independent Timing Position */
#define SCB_AIRCR_DIT_MSK                  (1UL << SCB_AIRCR_DIT_POS)                     /*!< SCB AIRCR: Data Independent Timing Mask */

#define SCB_AIRCR_SYSRESETREQS_POS          3U                                            /*!< SCB AIRCR: SYSRESETREQS Position */
#define SCB_AIRCR_SYSRESETREQS_MSK         (1UL << SCB_AIRCR_SYSRESETREQS_POS)            /*!< SCB AIRCR: SYSRESETREQS Mask */

#define SCB_AIRCR_SYSRESETREQ_POS           2U                                            /*!< SCB AIRCR: SYSRESETREQ Position */
#define SCB_AIRCR_SYSRESETREQ_MSK          (1UL << SCB_AIRCR_SYSRESETREQ_POS)             /*!< SCB AIRCR: SYSRESETREQ Mask */

#define SCB_AIRCR_VECTCLRACTIVE_POS         1U                                            /*!< SCB AIRCR: VECTCLRACTIVE Position */
#define SCB_AIRCR_VECTCLRACTIVE_MSK        (1UL << SCB_AIRCR_VECTCLRACTIVE_POS)           /*!< SCB AIRCR: VECTCLRACTIVE Mask */

/** \brief SCB System Control Register Definitions */
#define SCB_SCR_SEVONPEND_POS               4U                                            /*!< SCB SCR: SEVONPEND Position */
#define SCB_SCR_SEVONPEND_MSK              (1UL << SCB_SCR_SEVONPEND_POS)                 /*!< SCB SCR: SEVONPEND Mask */

#define SCB_SCR_SLEEPDEEPS_POS              3U                                            /*!< SCB SCR: SLEEPDEEPS Position */
#define SCB_SCR_SLEEPDEEPS_MSK             (1UL << SCB_SCR_SLEEPDEEPS_POS)                /*!< SCB SCR: SLEEPDEEPS Mask */

#define SCB_SCR_SLEEPDEEP_POS               2U                                            /*!< SCB SCR: SLEEPDEEP Position */
#define SCB_SCR_SLEEPDEEP_MSK              (1UL << SCB_SCR_SLEEPDEEP_POS)                 /*!< SCB SCR: SLEEPDEEP Mask */

#define SCB_SCR_SLEEPONEXIT_POS             1U                                            /*!< SCB SCR: SLEEPONEXIT Position */
#define SCB_SCR_SLEEPONEXIT_MSK            (1UL << SCB_SCR_SLEEPONEXIT_POS)               /*!< SCB SCR: SLEEPONEXIT Mask */

/** \brief SCB Configuration Control Register Definitions */
#define SCB_CCR_TRD_POS                    20U                                            /*!< SCB CCR: TRD Position */
#define SCB_CCR_TRD_MSK                    (1UL << SCB_CCR_TRD_POS)                       /*!< SCB CCR: TRD Mask */

#define SCB_CCR_LOB_POS                    19U                                            /*!< SCB CCR: LOB Position */
#define SCB_CCR_LOB_MSK                    (1UL << SCB_CCR_LOB_POS)                       /*!< SCB CCR: LOB Mask */

#define SCB_CCR_BP_POS                     18U                                            /*!< SCB CCR: BP Position */
#define SCB_CCR_BP_MSK                     (1UL << SCB_CCR_BP_POS)                        /*!< SCB CCR: BP Mask */

#define SCB_CCR_IC_POS                     17U                                            /*!< SCB CCR: IC Position */
#define SCB_CCR_IC_MSK                     (1UL << SCB_CCR_IC_POS)                        /*!< SCB CCR: IC Mask */

#define SCB_CCR_DC_POS                     16U                                            /*!< SCB CCR: DC Position */
#define SCB_CCR_DC_MSK                     (1UL << SCB_CCR_DC_POS)                        /*!< SCB CCR: DC Mask */

#define SCB_CCR_STKOFHFNMIGN_POS           10U                                            /*!< SCB CCR: STKOFHFNMIGN Position */
#define SCB_CCR_STKOFHFNMIGN_MSK           (1UL << SCB_CCR_STKOFHFNMIGN_POS)              /*!< SCB CCR: STKOFHFNMIGN Mask */

#define SCB_CCR_BFHFNMIGN_POS               8U                                            /*!< SCB CCR: BFHFNMIGN Position */
#define SCB_CCR_BFHFNMIGN_MSK              (1UL << SCB_CCR_BFHFNMIGN_POS)                 /*!< SCB CCR: BFHFNMIGN Mask */

#define SCB_CCR_DIV_0_TRP_POS               4U                                            /*!< SCB CCR: DIV_0_TRP Position */
#define SCB_CCR_DIV_0_TRP_MSK              (1UL << SCB_CCR_DIV_0_TRP_POS)                 /*!< SCB CCR: DIV_0_TRP Mask */

#define SCB_CCR_UNALIGN_TRP_POS             3U                                            /*!< SCB CCR: UNALIGN_TRP Position */
#define SCB_CCR_UNALIGN_TRP_MSK            (1UL << SCB_CCR_UNALIGN_TRP_POS)               /*!< SCB CCR: UNALIGN_TRP Mask */

#define SCB_CCR_USERSETMPEND_POS            1U                                            /*!< SCB CCR: USERSETMPEND Position */
#define SCB_CCR_USERSETMPEND_MSK           (1UL << SCB_CCR_USERSETMPEND_POS)              /*!< SCB CCR: USERSETMPEND Mask */

/** \brief SCB System Handler Control and State Register Definitions */
#define SCB_SHCSR_HARDFAULTPENDED_POS      21U                                            /*!< SCB SHCSR: HARDFAULTPENDED Position */
#define SCB_SHCSR_HARDFAULTPENDED_MSK      (1UL << SCB_SHCSR_HARDFAULTPENDED_POS)         /*!< SCB SHCSR: HARDFAULTPENDED Mask */

#define SCB_SHCSR_SECUREFAULTPENDED_POS    20U                                            /*!< SCB SHCSR: SECUREFAULTPENDED Position */
#define SCB_SHCSR_SECUREFAULTPENDED_MSK    (1UL << SCB_SHCSR_SECUREFAULTPENDED_POS)       /*!< SCB SHCSR: SECUREFAULTPENDED Mask */

#define SCB_SHCSR_SECUREFAULTENA_POS       19U                                            /*!< SCB SHCSR: SECUREFAULTENA Position */
#define SCB_SHCSR_SECUREFAULTENA_MSK       (1UL << SCB_SHCSR_SECUREFAULTENA_POS)          /*!< SCB SHCSR: SECUREFAULTENA Mask */

#define SCB_SHCSR_USGFAULTENA_POS          18U                                            /*!< SCB SHCSR: USGFAULTENA Position */
#define SCB_SHCSR_USGFAULTENA_MSK          (1UL << SCB_SHCSR_USGFAULTENA_POS)             /*!< SCB SHCSR: USGFAULTENA Mask */

#define SCB_SHCSR_BUSFAULTENA_POS          17U                                            /*!< SCB SHCSR: BUSFAULTENA Position */
#define SCB_SHCSR_BUSFAULTENA_MSK          (1UL << SCB_SHCSR_BUSFAULTENA_POS)             /*!< SCB SHCSR: BUSFAULTENA Mask */

#define SCB_SHCSR_MEMFAULTENA_POS          16U                                            /*!< SCB SHCSR: MEMFAULTENA Position */
#define SCB_SHCSR_MEMFAULTENA_MSK          (1UL << SCB_SHCSR_MEMFAULTENA_POS)             /*!< SCB SHCSR: MEMFAULTENA Mask */

#define SCB_SHCSR_SVCALLPENDED_POS         15U                                            /*!< SCB SHCSR: SVCALLPENDED Position */
#define SCB_SHCSR_SVCALLPENDED_MSK         (1UL << SCB_SHCSR_SVCALLPENDED_POS)            /*!< SCB SHCSR: SVCALLPENDED Mask */

#define SCB_SHCSR_BUSFAULTPENDED_POS       14U                                            /*!< SCB SHCSR: BUSFAULTPENDED Position */
#define SCB_SHCSR_BUSFAULTPENDED_MSK       (1UL << SCB_SHCSR_BUSFAULTPENDED_POS)          /*!< SCB SHCSR: BUSFAULTPENDED Mask */

#define SCB_SHCSR_MEMFAULTPENDED_POS       13U                                            /*!< SCB SHCSR: MEMFAULTPENDED Position */
#define SCB_SHCSR_MEMFAULTPENDED_MSK       (1UL << SCB_SHCSR_MEMFAULTPENDED_POS)          /*!< SCB SHCSR: MEMFAULTPENDED Mask */

#define SCB_SHCSR_USGFAULTPENDED_POS       12U                                            /*!< SCB SHCSR: USGFAULTPENDED Position */
#define SCB_SHCSR_USGFAULTPENDED_MSK       (1UL << SCB_SHCSR_USGFAULTPENDED_POS)          /*!< SCB SHCSR: USGFAULTPENDED Mask */

#define SCB_SHCSR_SYSTICKACT_POS           11U                                            /*!< SCB SHCSR: SYSTICKACT Position */
#define SCB_SHCSR_SYSTICKACT_MSK           (1UL << SCB_SHCSR_SYSTICKACT_POS)              /*!< SCB SHCSR: SYSTICKACT Mask */

#define SCB_SHCSR_PENDSVACT_POS            10U                                            /*!< SCB SHCSR: PENDSVACT Position */
#define SCB_SHCSR_PENDSVACT_MSK            (1UL << SCB_SHCSR_PENDSVACT_POS)               /*!< SCB SHCSR: PENDSVACT Mask */

#define SCB_SHCSR_MONITORACT_POS            8U                                            /*!< SCB SHCSR: MONITORACT Position */
#define SCB_SHCSR_MONITORACT_MSK           (1UL << SCB_SHCSR_MONITORACT_POS)              /*!< SCB SHCSR: MONITORACT Mask */

#define SCB_SHCSR_SVCALLACT_POS             7U                                            /*!< SCB SHCSR: SVCALLACT Position */
#define SCB_SHCSR_SVCALLACT_MSK            (1UL << SCB_SHCSR_SVCALLACT_POS)               /*!< SCB SHCSR: SVCALLACT Mask */

#define SCB_SHCSR_NMIACT_POS                5U                                            /*!< SCB SHCSR: NMIACT Position */
#define SCB_SHCSR_NMIACT_MSK               (1UL << SCB_SHCSR_NMIACT_POS)                  /*!< SCB SHCSR: NMIACT Mask */

#define SCB_SHCSR_SECUREFAULTACT_POS        4U                                            /*!< SCB SHCSR: SECUREFAULTACT Position */
#define SCB_SHCSR_SECUREFAULTACT_MSK       (1UL << SCB_SHCSR_SECUREFAULTACT_POS)          /*!< SCB SHCSR: SECUREFAULTACT Mask */

#define SCB_SHCSR_USGFAULTACT_POS           3U                                            /*!< SCB SHCSR: USGFAULTACT Position */
#define SCB_SHCSR_USGFAULTACT_MSK          (1UL << SCB_SHCSR_USGFAULTACT_POS)             /*!< SCB SHCSR: USGFAULTACT Mask */

#define SCB_SHCSR_HARDFAULTACT_POS          2U                                            /*!< SCB SHCSR: HARDFAULTACT Position */
#define SCB_SHCSR_HARDFAULTACT_MSK         (1UL << SCB_SHCSR_HARDFAULTACT_POS)            /*!< SCB SHCSR: HARDFAULTACT Mask */

#define SCB_SHCSR_BUSFAULTACT_POS           1U                                            /*!< SCB SHCSR: BUSFAULTACT Position */
#define SCB_SHCSR_BUSFAULTACT_MSK          (1UL << SCB_SHCSR_BUSFAULTACT_POS)             /*!< SCB SHCSR: BUSFAULTACT Mask */

#define SCB_SHCSR_MEMFAULTACT_POS           0U                                            /*!< SCB SHCSR: MEMFAULTACT Position */
#define SCB_SHCSR_MEMFAULTACT_MSK          (1UL /*<< SCB_SHCSR_MEMFAULTACT_POS*/)         /*!< SCB SHCSR: MEMFAULTACT Mask */

/** \brief SCB Configurable Fault Status Register Definitions */
#define SCB_CFSR_USGFAULTSR_POS            16U                                            /*!< SCB CFSR: Usage Fault Status Register Position */
#define SCB_CFSR_USGFAULTSR_MSK            (0xFFFFUL << SCB_CFSR_USGFAULTSR_POS)          /*!< SCB CFSR: Usage Fault Status Register Mask */

#define SCB_CFSR_BUSFAULTSR_POS             8U                                            /*!< SCB CFSR: Bus Fault Status Register Position */
#define SCB_CFSR_BUSFAULTSR_MSK            (0xFFUL << SCB_CFSR_BUSFAULTSR_POS)            /*!< SCB CFSR: Bus Fault Status Register Mask */

#define SCB_CFSR_MEMFAULTSR_POS             0U                                            /*!< SCB CFSR: Memory Manage Fault Status Register Position */
#define SCB_CFSR_MEMFAULTSR_MSK            (0xFFUL /*<< SCB_CFSR_MEMFAULTSR_POS*/)        /*!< SCB CFSR: Memory Manage Fault Status Register Mask */

/** \brief SCB MemManage Fault Status Register Definitions (part of SCB Configurable Fault Status Register) */
#define SCB_CFSR_MMARVALID_POS             (SCB_CFSR_MEMFAULTSR_POS + 7U)                 /*!< SCB CFSR (MMFSR): MMARVALID Position */
#define SCB_CFSR_MMARVALID_MSK             (1UL << SCB_CFSR_MMARVALID_POS)                /*!< SCB CFSR (MMFSR): MMARVALID Mask */

#define SCB_CFSR_MLSPERR_POS               (SCB_CFSR_MEMFAULTSR_POS + 5U)                 /*!< SCB CFSR (MMFSR): MLSPERR Position */
#define SCB_CFSR_MLSPERR_MSK               (1UL << SCB_CFSR_MLSPERR_POS)                  /*!< SCB CFSR (MMFSR): MLSPERR Mask */

#define SCB_CFSR_MSTKERR_POS               (SCB_CFSR_MEMFAULTSR_POS + 4U)                 /*!< SCB CFSR (MMFSR): MSTKERR Position */
#define SCB_CFSR_MSTKERR_MSK               (1UL << SCB_CFSR_MSTKERR_POS)                  /*!< SCB CFSR (MMFSR): MSTKERR Mask */

#define SCB_CFSR_MUNSTKERR_POS             (SCB_CFSR_MEMFAULTSR_POS + 3U)                 /*!< SCB CFSR (MMFSR): MUNSTKERR Position */
#define SCB_CFSR_MUNSTKERR_MSK             (1UL << SCB_CFSR_MUNSTKERR_POS)                /*!< SCB CFSR (MMFSR): MUNSTKERR Mask */

#define SCB_CFSR_DACCVIOL_POS              (SCB_CFSR_MEMFAULTSR_POS + 1U)                 /*!< SCB CFSR (MMFSR): DACCVIOL Position */
#define SCB_CFSR_DACCVIOL_MSK              (1UL << SCB_CFSR_DACCVIOL_POS)                 /*!< SCB CFSR (MMFSR): DACCVIOL Mask */

#define SCB_CFSR_IACCVIOL_POS              (SCB_CFSR_MEMFAULTSR_POS + 0U)                 /*!< SCB CFSR (MMFSR): IACCVIOL Position */
#define SCB_CFSR_IACCVIOL_MSK              (1UL /*<< SCB_CFSR_IACCVIOL_POS*/)             /*!< SCB CFSR (MMFSR): IACCVIOL Mask */

/** \brief SCB BusFault Status Register Definitions (part of SCB Configurable Fault Status Register) */
#define SCB_CFSR_BFARVALID_POS            (SCB_CFSR_BUSFAULTSR_POS + 7U)                  /*!< SCB CFSR (BFSR): BFARVALID Position */
#define SCB_CFSR_BFARVALID_MSK            (1UL << SCB_CFSR_BFARVALID_POS)                 /*!< SCB CFSR (BFSR): BFARVALID Mask */

#define SCB_CFSR_LSPERR_POS               (SCB_CFSR_BUSFAULTSR_POS + 5U)                  /*!< SCB CFSR (BFSR): LSPERR Position */
#define SCB_CFSR_LSPERR_MSK               (1UL << SCB_CFSR_LSPERR_POS)                    /*!< SCB CFSR (BFSR): LSPERR Mask */

#define SCB_CFSR_STKERR_POS               (SCB_CFSR_BUSFAULTSR_POS + 4U)                  /*!< SCB CFSR (BFSR): STKERR Position */
#define SCB_CFSR_STKERR_MSK               (1UL << SCB_CFSR_STKERR_POS)                    /*!< SCB CFSR (BFSR): STKERR Mask */

#define SCB_CFSR_UNSTKERR_POS             (SCB_CFSR_BUSFAULTSR_POS + 3U)                  /*!< SCB CFSR (BFSR): UNSTKERR Position */
#define SCB_CFSR_UNSTKERR_MSK             (1UL << SCB_CFSR_UNSTKERR_POS)                  /*!< SCB CFSR (BFSR): UNSTKERR Mask */

#define SCB_CFSR_IMPRECISERR_POS          (SCB_CFSR_BUSFAULTSR_POS + 2U)                  /*!< SCB CFSR (BFSR): IMPRECISERR Position */
#define SCB_CFSR_IMPRECISERR_MSK          (1UL << SCB_CFSR_IMPRECISERR_POS)               /*!< SCB CFSR (BFSR): IMPRECISERR Mask */

#define SCB_CFSR_PRECISERR_POS            (SCB_CFSR_BUSFAULTSR_POS + 1U)                  /*!< SCB CFSR (BFSR): PRECISERR Position */
#define SCB_CFSR_PRECISERR_MSK            (1UL << SCB_CFSR_PRECISERR_POS)                 /*!< SCB CFSR (BFSR): PRECISERR Mask */

#define SCB_CFSR_IBUSERR_POS              (SCB_CFSR_BUSFAULTSR_POS + 0U)                  /*!< SCB CFSR (BFSR): IBUSERR Position */
#define SCB_CFSR_IBUSERR_MSK              (1UL << SCB_CFSR_IBUSERR_POS)                   /*!< SCB CFSR (BFSR): IBUSERR Mask */

/** \brief SCB UsageFault Status Register Definitions (part of SCB Configurable Fault Status Register) */
#define SCB_CFSR_DIVBYZERO_POS            (SCB_CFSR_USGFAULTSR_POS + 9U)                  /*!< SCB CFSR (UFSR): DIVBYZERO Position */
#define SCB_CFSR_DIVBYZERO_MSK            (1UL << SCB_CFSR_DIVBYZERO_POS)                 /*!< SCB CFSR (UFSR): DIVBYZERO Mask */

#define SCB_CFSR_UNALIGNED_POS            (SCB_CFSR_USGFAULTSR_POS + 8U)                  /*!< SCB CFSR (UFSR): UNALIGNED Position */
#define SCB_CFSR_UNALIGNED_MSK            (1UL << SCB_CFSR_UNALIGNED_POS)                 /*!< SCB CFSR (UFSR): UNALIGNED Mask */

#define SCB_CFSR_STKOF_POS                (SCB_CFSR_USGFAULTSR_POS + 4U)                  /*!< SCB CFSR (UFSR): STKOF Position */
#define SCB_CFSR_STKOF_MSK                (1UL << SCB_CFSR_STKOF_POS)                     /*!< SCB CFSR (UFSR): STKOF Mask */

#define SCB_CFSR_NOCP_POS                 (SCB_CFSR_USGFAULTSR_POS + 3U)                  /*!< SCB CFSR (UFSR): NOCP Position */
#define SCB_CFSR_NOCP_MSK                 (1UL << SCB_CFSR_NOCP_POS)                      /*!< SCB CFSR (UFSR): NOCP Mask */

#define SCB_CFSR_INVPC_POS                (SCB_CFSR_USGFAULTSR_POS + 2U)                  /*!< SCB CFSR (UFSR): INVPC Position */
#define SCB_CFSR_INVPC_MSK                (1UL << SCB_CFSR_INVPC_POS)                     /*!< SCB CFSR (UFSR): INVPC Mask */

#define SCB_CFSR_INVSTATE_POS             (SCB_CFSR_USGFAULTSR_POS + 1U)                  /*!< SCB CFSR (UFSR): INVSTATE Position */
#define SCB_CFSR_INVSTATE_MSK             (1UL << SCB_CFSR_INVSTATE_POS)                  /*!< SCB CFSR (UFSR): INVSTATE Mask */

#define SCB_CFSR_UNDEFINSTR_POS           (SCB_CFSR_USGFAULTSR_POS + 0U)                  /*!< SCB CFSR (UFSR): UNDEFINSTR Position */
#define SCB_CFSR_UNDEFINSTR_MSK           (1UL << SCB_CFSR_UNDEFINSTR_POS)                /*!< SCB CFSR (UFSR): UNDEFINSTR Mask */

/** \brief SCB Hard Fault Status Register Definitions */
#define SCB_HFSR_DEBUGEVT_POS              31U                                            /*!< SCB HFSR: DEBUGEVT Position */
#define SCB_HFSR_DEBUGEVT_MSK              (1UL << SCB_HFSR_DEBUGEVT_POS)                 /*!< SCB HFSR: DEBUGEVT Mask */

#define SCB_HFSR_FORCED_POS                30U                                            /*!< SCB HFSR: FORCED Position */
#define SCB_HFSR_FORCED_MSK                (1UL << SCB_HFSR_FORCED_POS)                   /*!< SCB HFSR: FORCED Mask */

#define SCB_HFSR_VECTTBL_POS                1U                                            /*!< SCB HFSR: VECTTBL Position */
#define SCB_HFSR_VECTTBL_MSK               (1UL << SCB_HFSR_VECTTBL_POS)                  /*!< SCB HFSR: VECTTBL Mask */

/** \brief SCB Debug Fault Status Register Definitions */
#define SCB_DFSR_PMU_POS                    5U                                            /*!< SCB DFSR: PMU Position */
#define SCB_DFSR_PMU_MSK                   (1UL << SCB_DFSR_PMU_POS)                      /*!< SCB DFSR: PMU Mask */

#define SCB_DFSR_EXTERNAL_POS               4U                                            /*!< SCB DFSR: EXTERNAL Position */
#define SCB_DFSR_EXTERNAL_MSK              (1UL << SCB_DFSR_EXTERNAL_POS)                 /*!< SCB DFSR: EXTERNAL Mask */

#define SCB_DFSR_VCATCH_POS                 3U                                            /*!< SCB DFSR: VCATCH Position */
#define SCB_DFSR_VCATCH_MSK                (1UL << SCB_DFSR_VCATCH_POS)                   /*!< SCB DFSR: VCATCH Mask */

#define SCB_DFSR_DWTTRAP_POS                2U                                            /*!< SCB DFSR: DWTTRAP Position */
#define SCB_DFSR_DWTTRAP_MSK               (1UL << SCB_DFSR_DWTTRAP_POS)                  /*!< SCB DFSR: DWTTRAP Mask */

#define SCB_DFSR_BKPT_POS                   1U                                            /*!< SCB DFSR: BKPT Position */
#define SCB_DFSR_BKPT_MSK                  (1UL << SCB_DFSR_BKPT_POS)                     /*!< SCB DFSR: BKPT Mask */

#define SCB_DFSR_HALTED_POS                 0U                                            /*!< SCB DFSR: HALTED Position */
#define SCB_DFSR_HALTED_MSK                (1UL /*<< SCB_DFSR_HALTED_POS*/)               /*!< SCB DFSR: HALTED Mask */

/** \brief SCB Non-Secure Access Control Register Definitions */
#define SCB_NSACR_CP11_POS                 11U                                            /*!< SCB NSACR: CP11 Position */
#define SCB_NSACR_CP11_MSK                 (1UL << SCB_NSACR_CP11_POS)                    /*!< SCB NSACR: CP11 Mask */

#define SCB_NSACR_CP10_POS                 10U                                            /*!< SCB NSACR: CP10 Position */
#define SCB_NSACR_CP10_MSK                 (1UL << SCB_NSACR_CP10_POS)                    /*!< SCB NSACR: CP10 Mask */

#define SCB_NSACR_CP7_POS                   7U                                            /*!< SCB NSACR: CP7 Position */
#define SCB_NSACR_CP7_MSK                  (1UL << SCB_NSACR_CP7_POS)                     /*!< SCB NSACR: CP7 Mask */

#define SCB_NSACR_CP6_POS                   6U                                            /*!< SCB NSACR: CP6 Position */
#define SCB_NSACR_CP6_MSK                  (1UL << SCB_NSACR_CP6_POS)                     /*!< SCB NSACR: CP6 Mask */

#define SCB_NSACR_CP5_POS                   5U                                            /*!< SCB NSACR: CP5 Position */
#define SCB_NSACR_CP5_MSK                  (1UL << SCB_NSACR_CP5_POS)                     /*!< SCB NSACR: CP5 Mask */

#define SCB_NSACR_CP4_POS                   4U                                            /*!< SCB NSACR: CP4 Position */
#define SCB_NSACR_CP4_MSK                  (1UL << SCB_NSACR_CP4_POS)                     /*!< SCB NSACR: CP4 Mask */

#define SCB_NSACR_CP3_POS                   3U                                            /*!< SCB NSACR: CP3 Position */
#define SCB_NSACR_CP3_MSK                  (1UL << SCB_NSACR_CP3_POS)                     /*!< SCB NSACR: CP3 Mask */

#define SCB_NSACR_CP2_POS                   2U                                            /*!< SCB NSACR: CP2 Position */
#define SCB_NSACR_CP2_MSK                  (1UL << SCB_NSACR_CP2_POS)                     /*!< SCB NSACR: CP2 Mask */

#define SCB_NSACR_CP1_POS                   1U                                            /*!< SCB NSACR: CP1 Position */
#define SCB_NSACR_CP1_MSK                  (1UL << SCB_NSACR_CP1_POS)                     /*!< SCB NSACR: CP1 Mask */

#define SCB_NSACR_CP0_POS                   0U                                            /*!< SCB NSACR: CP0 Position */
#define SCB_NSACR_CP0_MSK                  (1UL /*<< SCB_NSACR_CP0_POS*/)                 /*!< SCB NSACR: CP0 Mask */

/** \brief SCB Debug Feature Register 0 Definitions */
#define SCB_ID_DFR_UDE_POS                 28U                                            /*!< SCB ID_DFR: UDE Position */
#define SCB_ID_DFR_UDE_MSK                 (0xFUL << SCB_ID_DFR_UDE_POS)                  /*!< SCB ID_DFR: UDE Mask */

#define SCB_ID_DFR_MProfDbg_POS            20U                                            /*!< SCB ID_DFR: MProfDbg Position */
#define SCB_ID_DFR_MProfDbg_MSK            (0xFUL << SCB_ID_DFR_MProfDbg_POS)             /*!< SCB ID_DFR: MProfDbg Mask */

/** \brief SCB Cache Level ID Register Definitions */
#define SCB_CLIDR_LOUU_POS                 27U                                            /*!< SCB CLIDR: LoUU Position */
#define SCB_CLIDR_LOUU_MSK                 (7UL << SCB_CLIDR_LOUU_POS)                    /*!< SCB CLIDR: LoUU Mask */

#define SCB_CLIDR_LOC_POS                  24U                                            /*!< SCB CLIDR: LoC Position */
#define SCB_CLIDR_LOC_MSK                  (7UL << SCB_CLIDR_LOC_POS)                     /*!< SCB CLIDR: LoC Mask */

#define SCB_CLIDR_IC_POS                   0U                                             /*!< SCB CLIDR: IC Position */
#define SCB_CLIDR_IC_MSK                   (1UL << SCB_CLIDR_IC_POS)                      /*!< SCB CLIDR: IC Mask */

#define SCB_CLIDR_DC_POS                   1U                                             /*!< SCB CLIDR: DC Position */
#define SCB_CLIDR_DC_MSK                   (1UL << SCB_CLIDR_DC_POS)                      /*!< SCB CLIDR: DC Mask */

#define SCB_CLIDR_CTYPE1_POS               0U
#define SCB_CLIDR_CTYPE1_MSK               (7UL << SCB_CLIDR_CTYPE1_POS)

/** \brief SCB Cache Type Register Definitions */
#define SCB_CTR_FORMAT_POS                 29U                                            /*!< SCB CTR: Format Position */
#define SCB_CTR_FORMAT_MSK                 (7UL << SCB_CTR_FORMAT_POS)                    /*!< SCB CTR: Format Mask */

#define SCB_CTR_CWG_POS                    24U                                            /*!< SCB CTR: CWG Position */
#define SCB_CTR_CWG_MSK                    (0xFUL << SCB_CTR_CWG_POS)                     /*!< SCB CTR: CWG Mask */

#define SCB_CTR_ERG_POS                    20U                                            /*!< SCB CTR: ERG Position */
#define SCB_CTR_ERG_MSK                    (0xFUL << SCB_CTR_ERG_POS)                     /*!< SCB CTR: ERG Mask */

#define SCB_CTR_DMINLINE_POS               16U                                            /*!< SCB CTR: DminLine Position */
#define SCB_CTR_DMINLINE_MSK               (0xFUL << SCB_CTR_DMINLINE_POS)                /*!< SCB CTR: DminLine Mask */

#define SCB_CTR_IMINLINE_POS                0U                                            /*!< SCB CTR: ImInLine Position */
#define SCB_CTR_IMINLINE_MSK               (0xFUL /*<< SCB_CTR_IMINLINE_POS*/)            /*!< SCB CTR: ImInLine Mask */

/** \brief SCB Cache Size ID Register Definitions */
#define SCB_CCSIDR_WT_POS                  31U                                            /*!< SCB CCSIDR: WT Position */
#define SCB_CCSIDR_WT_MSK                  (1UL << SCB_CCSIDR_WT_POS)                     /*!< SCB CCSIDR: WT Mask */

#define SCB_CCSIDR_WB_POS                  30U                                            /*!< SCB CCSIDR: WB Position */
#define SCB_CCSIDR_WB_MSK                  (1UL << SCB_CCSIDR_WB_POS)                     /*!< SCB CCSIDR: WB Mask */

#define SCB_CCSIDR_RA_POS                  29U                                            /*!< SCB CCSIDR: RA Position */
#define SCB_CCSIDR_RA_MSK                  (1UL << SCB_CCSIDR_RA_POS)                     /*!< SCB CCSIDR: RA Mask */

#define SCB_CCSIDR_WA_POS                  28U                                            /*!< SCB CCSIDR: WA Position */
#define SCB_CCSIDR_WA_MSK                  (1UL << SCB_CCSIDR_WA_POS)                     /*!< SCB CCSIDR: WA Mask */

#define SCB_CCSIDR_NUMSETS_POS             13U                                            /*!< SCB CCSIDR: NumSets Position */
#define SCB_CCSIDR_NUMSETS_MSK             (0x7FFFUL << SCB_CCSIDR_NUMSETS_POS)           /*!< SCB CCSIDR: NumSets Mask */

#define SCB_CCSIDR_ASSOCIATIVITY_POS        3U                                            /*!< SCB CCSIDR: Associativity Position */
#define SCB_CCSIDR_ASSOCIATIVITY_MSK       (0x3FFUL << SCB_CCSIDR_ASSOCIATIVITY_POS)      /*!< SCB CCSIDR: Associativity Mask */

#define SCB_CCSIDR_LINESIZE_POS             0U                                            /*!< SCB CCSIDR: LineSize Position */
#define SCB_CCSIDR_LINESIZE_MSK            (7UL /*<< SCB_CCSIDR_LINESIZE_POS*/)           /*!< SCB CCSIDR: LineSize Mask */

/** \brief SCB Cache Size Selection Register Definitions */
#define SCB_CSSELR_LEVEL_POS                1U                                            /*!< SCB CSSELR: Level Position */
#define SCB_CSSELR_LEVEL_MSK               (7UL << SCB_CSSELR_LEVEL_POS)                  /*!< SCB CSSELR: Level Mask */

#define SCB_CSSELR_IND_POS                  0U                                            /*!< SCB CSSELR: InD Position */
#define SCB_CSSELR_IND_MSK                 (1UL /*<< SCB_CSSELR_IND_POS*/)                /*!< SCB CSSELR: InD Mask */

/** \brief SCB Software Triggered Interrupt Register Definitions */
#define SCB_STIR_INTID_POS                  0U                                            /*!< SCB STIR: INTID Position */
#define SCB_STIR_INTID_MSK                 (0x1FFUL /*<< SCB_STIR_INTID_POS*/)            /*!< SCB STIR: INTID Mask */

/** \brief SCB RAS Fault Status Register Definitions */
#define SCB_RFSR_V_POS                     31U                                            /*!< SCB RFSR: V Position */
#define SCB_RFSR_V_MSK                     (1UL << SCB_RFSR_V_POS)                        /*!< SCB RFSR: V Mask */

#define SCB_RFSR_IS_POS                    16U                                            /*!< SCB RFSR: IS Position */
#define SCB_RFSR_IS_MSK                    (0x7FFFUL << SCB_RFSR_IS_POS)                  /*!< SCB RFSR: IS Mask */

#define SCB_RFSR_UET_POS                    0U                                            /*!< SCB RFSR: UET Position */
#define SCB_RFSR_UET_MSK                   (3UL /*<< SCB_RFSR_UET_POS*/)                  /*!< SCB RFSR: UET Mask */

/** \brief SCB D-Cache Invalidate by Set-way Register Definitions */
#define SCB_DCISW_WAY_POS                  30U                                            /*!< SCB DCISW: Way Position */
#define SCB_DCISW_WAY_MSK                  (3UL << SCB_DCISW_WAY_POS)                     /*!< SCB DCISW: Way Mask */

#define SCB_DCISW_SET_POS                   5U                                            /*!< SCB DCISW: Set Position */
#define SCB_DCISW_SET_MSK                  (0x1FFUL << SCB_DCISW_SET_POS)                 /*!< SCB DCISW: Set Mask */

/** \brief SCB D-Cache Clean by Set-way Register Definitions */
#define SCB_DCCSW_WAY_POS                  30U                                            /*!< SCB DCCSW: Way Position */
#define SCB_DCCSW_WAY_MSK                  (3UL << SCB_DCCSW_WAY_POS)                     /*!< SCB DCCSW: Way Mask */

#define SCB_DCCSW_SET_POS                   5U                                            /*!< SCB DCCSW: Set Position */
#define SCB_DCCSW_SET_MSK                  (0x1FFUL << SCB_DCCSW_SET_POS)                 /*!< SCB DCCSW: Set Mask */

/** \brief SCB D-Cache Clean and Invalidate by Set-way Register Definitions */
#define SCB_DCCISW_WAY_POS                 30U                                            /*!< SCB DCCISW: Way Position */
#define SCB_DCCISW_WAY_MSK                 (3UL << SCB_DCCISW_WAY_POS)                    /*!< SCB DCCISW: Way Mask */

#define SCB_DCCISW_SET_POS                  5U                                            /*!< SCB DCCISW: Set Position */
#define SCB_DCCISW_SET_MSK                 (0x1FFUL << SCB_DCCISW_SET_POS)                /*!< SCB DCCISW: Set Mask */


/** \brief SCB U-Cache Invalidate by Set-way Register Definitions */
#define SCB_DCISW_UC_WAY_POS               31U                                            /*!< SCB DCISW: Way Position */
#define SCB_DCISW_UC_WAY_MSK               (1UL << SCB_DCISW_UC_WAY_POS)                  /*!< SCB DCISW: Way Mask */

#define SCB_DCISW_UC_SET_POS               5U                                             /*!< SCB DCISW: Set Position */
#define SCB_DCISW_UC_SET_MSK               (0x3FFUL << SCB_DCISW_UC_SET_POS)              /*!< SCB DCISW: Set Mask */

/** \brief SCB U-Cache Clean by Set-way Register Definitions */
#define SCB_DCCSW_UC_WAY_POS               31U                                            /*!< SCB DCCSW: Way Position */
#define SCB_DCCSW_UC_WAY_MSK               (1UL << SCB_DCCSW_UC_WAY_POS)                  /*!< SCB DCCSW: Way Mask */

#define SCB_DCCSW_UC_SET_POS               5U                                             /*!< SCB DCCSW: Set Position */
#define SCB_DCCSW_UC_SET_MSK               (0x3FFUL << SCB_DCCSW_UC_SET_POS)              /*!< SCB DCCSW: Set Mask */

/** \brief SCB U-Cache Clean and Invalidate by Set-way Register Definitions */
#define SCB_DCCISW_UC_WAY_POS              31U                                            /*!< SCB DCCISW: Way Position */
#define SCB_DCCISW_UC_WAY_MSK              (1UL << SCB_DCCISW_UC_WAY_POS)                 /*!< SCB DCCISW: Way Mask */

#define SCB_DCCISW_UC_SET_POS              5U                                             /*!< SCB DCCISW: Set Position */
#define SCB_DCCISW_UC_SET_MSK              (0x3FFUL << SCB_DCCISW_UC_SET_POS)             /*!< SCB DCCISW: Set Mask */
/*@} end of group CMSIS_SCB */

#define SCS_BASE_ADDR (0xE000E000UL)                         /*!< System Control Space Base Address */
#define SCB_BASE_ADDR (SCS_BASE_ADDR +  0x0D00UL)            /*!< System Control Block Base Address */
#define SYS_CTRL_BLK ((SCB_TYPE *)     SCB_BASE_ADDR) /*!< SCB configuration struct */

/* CP10 Access Bits */
#define CPACR_CP10_POS         20U
#define CPACR_CP10_MSK         (3UL << CPACR_CP10_POS)
#define CPACR_CP10_NO_ACCESS   (0UL << CPACR_CP10_POS)
#define CPACR_CP10_PRIV_ACCESS (1UL << CPACR_CP10_POS)
#define CPACR_CP10_RESERVED    (2UL << CPACR_CP10_POS)
#define CPACR_CP10_FULL_ACCESS (3UL << CPACR_CP10_POS)

/* CP11 Access Bits */
#define CPACR_CP11_POS         22U
#define CPACR_CP11_MSK         (3UL << CPACR_CP11_POS)
#define CPACR_CP11_NO_ACCESS   (0UL << CPACR_CP11_POS)
#define CPACR_CP11_PRIV_ACCESS (1UL << CPACR_CP11_POS)
#define CPACR_CP11_RESERVED    (2UL << CPACR_CP11_POS)
#define CPACR_CP11_FULL_ACCESS (3UL << CPACR_CP11_POS)
