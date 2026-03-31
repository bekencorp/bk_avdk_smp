/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "soc/bk7259/reg_base.h"
#include "partitions.h"
#include "sdkconfig.h"
#include "ram_regions.h"


OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE  = (1 << 10);
PERSIST_MEM_AREA_SIZE = 4;
REBOOT_TAG_AREA_SIZE = 4;

#if CONFIG_MPU
__MPU_PROTECT_SIZE = 0x20;
#else
__MPU_PROTECT_SIZE = 0x0;
#endif


/*****************************************************************************
The follow four shared memory address area mapping to the same physical memory
    0x08000000 ~ 0x080a0000  ---- instruction area, recommend put sram code
    0x18000000 ~ 0x180a0000  ---- instruction area, recommend put sram code
    0x28000000 ~ 0x280a0000  ---- data area, recommend put sram data
    0x38000000 ~ 0x380a0000  ---- data area, recommend put sram data
******************************************************************************/

__AP_APP_IRAM_OFFSET = 0x0; /* 0x20000000;*/

__AP_APP_IRAM_BASE = CONFIG_AP_RAM_ADDR - __AP_APP_IRAM_OFFSET;  /*0x08100000 = 0x28100000 - __CP_APP_IRAM_OFFSET*/
__AP_APP_IRAM_SIZE = CONFIG_AP_RAM_SIZE;

MEMORY
{
  FLASH (rx)            : ORIGIN = (SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET), LENGTH = CONFIG_AP_VIRTUAL_PARTITION_SIZE
  IRAM (rx)             : ORIGIN = __AP_APP_IRAM_BASE, LENGTH = __AP_APP_IRAM_SIZE

  RAM (rwx)             : ORIGIN = CONFIG_AP_RAM_ADDR, LENGTH = CONFIG_AP_RAM_SIZE
  SWAP (rwx)            : ORIGIN = CONFIG_SWAP_ADDR, LENGTH = CONFIG_SWAP_SIZE

  DTCM (rwx)            : ORIGIN = SOC_DTCM_DATA_BASE + 0, LENGTH = CONFIG_DTCM_SIZE
#if CONFIG_PSRAM
#if (CONFIG_PSRAM_AS_SYS_MEMORY)
  PSRAM_HEAP (rwx)      : ORIGIN = CONFIG_AP_PSRAM_HEAP_ADDR, LENGTH = CONFIG_AP_PSRAM_HEAP_SIZE
#endif
#if (CONFIG_AP_PSRAM_CODE_SECTION_ADDR)
  PSRAM_CODE_SECTION (rwx)   : ORIGIN = CONFIG_AP_PSRAM_CODE_SECTION_ADDR, LENGTH = CONFIG_AP_PSRAM_CODE_SECTION_SIZE
#endif
#if (CONFIG_AP_PSRAM_DATA_SECTION_ADDR)
  PSRAM_DATA_SECTION (rwx)   : ORIGIN = CONFIG_AP_PSRAM_DATA_SECTION_ADDR, LENGTH = CONFIG_AP_PSRAM_DATA_SECTION_SIZE
#endif
#endif
}

#if (CONFIG_SOC_SMP)
ENTRY(Reset_Handler_Core0)
#else
ENTRY(Reset_Handler)
#endif

SECTIONS
{
    ASSERT((. == ALIGN(512)), "vector table address align fault.")
    .vectors :
    {
#if (CONFIG_SOC_SMP)
        __vector_core0_table = .;
        KEEP(*(.vectors_core0))
#else
        __vector_table = .;
        KEEP(*(.vectors))
#endif
        *(.fix.reset_entry)
    } > FLASH

    .gnu.sgstubs ALIGN(32) : ALIGN(32)
    {
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > FLASH

    .cli_cmds_table ALIGN(32) : ALIGN(32)
    {
        PROVIDE(_cli_cmds_start = .);
        KEEP(*(.cli_cmdtabl))
        KEEP(*(.cli_cmdtabl*))
        PROVIDE(_cli_cmds_end = .);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        _stext = .;

        . = ALIGN(8);
        __periph_init_start = .;
        KEEP(*(SORT(.dev_init_fn*)))
        __periph_init_end = .;
        . = ALIGN(8);

        . = ALIGN(8);
        __comp_init_start = .;
        KEEP(*(SORT(.comp_init_fn*)))
        __comp_init_end = .;
        . = ALIGN(8);

        . = ALIGN(8);
        __app_init_start = .;
        KEEP(*(SORT(.app_init_fn*)))
        __app_init_end = .;
        . = ALIGN(8);

#if CONFIG_ALL_CODE_IN_PSRAM
    EXCLUDE_FILE(*) *(.text .text.* .rodata .rodata.*)
#else /*CONFIG_ALL_CODE_IN_PSRAM*/
#if CONFIG_SPE
	EXCLUDE_FILE (
        tasks.c.obj
        queue.c.obj
        list.c.obj
        timers.c.obj
        port.c.obj
        rtos_pub.c.obj
        heap_4.c.obj
        mem_arch.c.obj
        gpio_driver_base.c.obj
        flash_driver.c.obj
        ckmn_driver.c.obj
        arch_interrupt.c.obj
        hal_machw.c.obj
        interrupts.c.obj
        rw_task.c.obj
        ke_event.c.obj
        me_strategy.c.obj
        chan.c.obj
        vif_mgmt.c.obj
        rxl_cntrl.c.obj
        rxl_hwdesc.c.obj
        mm_time.c.obj
        rwnx.c.obj
        rwnx_rx.c.obj
        ps.c.obj
        mm.c.obj
        rf_cntrl.c.obj
        mcc.c.obj
        phy_karst_bk7236.c.obj
        crm.c.obj
        bk7236_cal.c.obj
        manual_cal_bk7236.c.obj
        bk_cal_common.c.obj
        nv_parameters.c.obj
        macif_fhost.c.obj
        rc_driver.c.obj
        sys_wifi_driver.c.obj
        wifi_v2.c.obj
        bk_feature.c.obj
        pm.c.obj
        sys_ps_driver.c.obj
        sys_pm_hal.c.obj
        bk_wifi_adapter.c.obj
        net_param.c.obj
        wifi_api_wrapper.c.obj
        bk_rf_adapter.c.obj
        bk_phy_adapter.c.obj
        shell_task.c.obj
        shell_uart.c.obj
        uart_driver.c.obj
        uart_hal.c.obj
    #if CONFIG_PSRAM
        *libbk_cli.a:*.c.obj
        *libtensorflow-microlite.a:*.o
    #endif
	) *(.text .text.* .rodata .rodata.*)
#else /*CONFIG_SPE*/
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
#endif /*CONFIG_SPE*/
#endif /*CONFIG_ALL_CODE_IN_PSRAM*/
        . = ALIGN(4);
        __devconfig_start = .;
        *(".devconfig.*")
        KEEP(*(SORT_BY_NAME(".devconfig*")))
        __devconfig_end = .;

        . = ALIGN(4);
        __apps_start = .;
        KEEP (*(.apps_data))
        __apps_end = .;
        _etext = .;
        . = ALIGN(4);
    } > FLASH

    .a_device_null :
    {
      KEEP(*(.a_deviceobj_null))
    } > FLASH

    .a_devices :
    {
      __device_start = .;
      KEEP(*(.a_deviceobj_*))
      __device_end = .;
    } > FLASH

    .a_init_entries :
    {
      __a_init_start = .;
      KEEP(*(.a_init_entry_*))
      __a_init_end = .;
    } > FLASH


    .sensor_detect :
    {
      __camera_sensor_detect_array_start = .;
      KEEP(*(.camera_sensor_detect_function_list.*))
      __camera_sensor_detect_array_end = .;
    } > FLASH

    .lcd_panel_device_list :
    {
      __lcd_panel_device_array_start = .;
      KEEP(*(.lcd_panel_device_list.*))
      __lcd_panel_device_array_end = .;
    } > FLASH

    .avdk_ipc :
    {
      __start_ipc_chan_reg = .;
      KEEP(*(.ipc_chan_reg*))
      __stop_ipc_chan_reg = .;
    } > FLASH

#if !CONFIG_ALL_CODE_IN_PSRAM
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;
#endif

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__iram_flash_begin)
        LONG (__iram_start__)
        LONG ((__iram_end__ - __iram_start__) / 4)

        LONG (__data_flash_begin)
        LONG (__data_start__)
        LONG ((__data_end__ - __data_start__) / 4)

        LONG (__dtcm_content)
        LONG (__dtcm_start__)
        LONG ((__dtcm_end__ - __dtcm_start__) / 4)

#if CONFIG_PSRAM
        LONG (__psram_code_lma)
        LONG (__psram_code_start__)
        LONG ((__psram_code_end__ - __psram_code_start__) / 4)
        LONG (__psram_data_lma)
        LONG (__psram_data_start__)
        LONG ((__psram_data_end__ - __psram_data_start__) / 4)
#endif
        __copy_table_end__ = .;
    } > FLASH

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (_bss_start)
        LONG ((_bss_end - _bss_start) / 4)
        LONG (_heap_start)
        LONG ((_heap_end - _heap_start) / 4)
        /* Add each additional bss section here */
    #if CONFIG_PSRAM
        LONG (__psram_bss_start__)
        LONG ((__psram_bss_end__ - __psram_bss_start__) / 4)
    #endif
        __zero_table_end__ = .;
    } > FLASH

    .dtcm :
    {
        . = ALIGN(4);

        PROVIDE(__dtcm_content = LOADADDR(.dtcm));
        __dtcm_start__ = .;
#if 0        
#if (CONFIG_SOC_SMP)
        . = ALIGN(512);
        KEEP(*(.vectors_iram))

        . = ALIGN(512);
        __vector_core1_table = .;
        KEEP(*(.vectors_core1))
#else
        . = ALIGN(512);
        KEEP(*(.vectors_iram))
#endif


        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        . = ALIGN(4);

        __itcm_start__ = .;
        *(.itcm)
        *(.itcm_section*)
        *(.itcm_sec_code*)
        *(.interrupt)
        __itcm_end__ = .;
        . = ALIGN(4);

#endif
        __dtcm_end__ = .;
    } > DTCM AT > FLASH

    .iram :
    {
        . = ALIGN(512);
        PROVIDE(__iram_flash_begin = LOADADDR(.iram));
        __iram_start__ = .;
        *(.iram)
	
        __itcm_start__ = .;
        *(.itcm)
        *(.itcm_section*)
        *(.itcm_sec_code*)
        *(.interrupt)
        __itcm_end__ = .;



#if CONFIG_SPE
        tasks.c.obj*(.text .text.* .rodata .rodata.*)
        queue.c.obj*(.text .text.* .rodata .rodata.*)
        list.c.obj*(.text .text.* .rodata .rodata.*)
        timers.c.obj*(.text .text.* .rodata .rodata.*)
        port.c.obj*(.text .text.* .rodata .rodata.*)
        rtos_pub.c.obj*(.text .text.* .rodata .rodata.*)
        heap_4.c.obj*(.text .text.* .rodata .rodata.*)
        mem_arch.c.obj*(.text .text.* .rodata .rodata.*)
        gpio_driver_base.c.obj*(.text .text.* .rodata .rodata.*)
        flash_driver.c.obj*(.text .text.* .rodata .rodata.*)
        ckmn_driver.c.obj*(.text .text.* .rodata .rodata.*)
        arch_interrupt.c.obj*(.text .text.* .rodata .rodata.*)
        hal_machw.c.obj*(.text .text.* .rodata .rodata.*)
        interrupts.c.obj*(.text .text.* .rodata .rodata.*)
        rw_task.c.obj*(.text .text.* .rodata .rodata.*)
        ke_event.c.obj*(.text .text.* .rodata .rodata.*)
        me_strategy.c.obj*(.text .text.* .rodata .rodata.*)
        chan.c.obj*(.text .text.* .rodata .rodata.*)
        vif_mgmt.c.obj*(.text .text.* .rodata .rodata.*)
        rxl_cntrl.c.obj*(.text .text.* .rodata .rodata.*)
        rxl_hwdesc.c.obj*(.text .text.* .rodata .rodata.*)
        mm_time.c.obj*(.text .text.* .rodata .rodata.*)
        rwnx.c.obj*(.text .text.* .rodata .rodata.*)
        rwnx_rx.c.obj*(.text .text.* .rodata .rodata.*)
        ps.c.obj*(.text .text.* .rodata .rodata.*)
        mm.c.obj*(.text .text.* .rodata .rodata.*)
        rf_cntrl.c.obj*(.text .text.* .rodata .rodata.*)
        mcc.c.obj*(.text .text.* .rodata .rodata.*)
        phy_karst_bk7236.c.obj*(.text .text.* .rodata .rodata.*)
        crm.c.obj*(.text .text.* .rodata .rodata.*)
        bk7236_cal.c.obj*(.text .text.* .rodata .rodata.*)
        manual_cal_bk7236.c.obj*(.text .text.* .rodata .rodata.*)
        bk_cal_common.c.obj*(.text .text.* .rodata .rodata.*)
        nv_parameters.c.obj*(.text .text.* .rodata .rodata.*)
        macif_fhost.c.obj*(.text .text.* .rodata .rodata.*)
        rc_driver.c.obj*(.text .text.* .rodata .rodata.*)
        sys_wifi_driver.c.obj*(.text .text.* .rodata .rodata.*)
        wifi_v2.c.obj*(.text .text.* .rodata .rodata.*)
        bk_feature.c.obj*(.text .text.* .rodata .rodata.*)
        pm.c.obj*(.text .text.* .rodata .rodata.*)
        sys_ps_driver.c.obj*(.text .text.* .rodata .rodata.*)
        sys_pm_hal.c.obj*(.text .text.* .rodata .rodata.*)
        bk_wifi_adapter.c.obj*(.text .text.* .rodata .rodata.*)
        net_param.c.obj*(.text .text.* .rodata .rodata.*)
        wifi_api_wrapper.c.obj*(.text .text.* .rodata .rodata.*)
        bk_rf_adapter.c.obj*(.text .text.* .rodata .rodata.*)
        bk_phy_adapter.c.obj*(.text .text.* .rodata .rodata.*)
        shell_task.c.obj*(.text .text.* .rodata .rodata.*)
        shell_uart.c.obj*(.text .text.* .rodata .rodata.*)
        uart_driver.c.obj*(.text .text.* .rodata .rodata.*)
        uart_hal.c.obj*(.text .text.* .rodata .rodata.*)
#endif
        . = ALIGN(4);
        __iram_end__ = .;
    } > IRAM AT > FLASH

    .ramcode :
    {
        . = . +  SIZEOF(.iram);
        . = ALIGN(512);
    } > RAM AT > FLASH

    .data :
    {
        PROVIDE(__etext = LOADADDR(.data));
        PROVIDE(__data_flash_begin = LOADADDR(.data));

        . = ALIGN(4);
        __data_start__ = .;

        . = ALIGN(512);
        KEEP(*(.vectors_iram))
	
#if CONFIG_SOC_SMP
        . = ALIGN(512);
        __vector_core1_table = .;
        KEEP(*(.vectors_core1))
#endif

#ifdef CONFIG_GCOV
        /* added in template for gcov: */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
#endif
        *(.sram_spinlock_section)
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        *(.data)
        *(".data.*")
        *(.sdata)

        *(.video_spec_data*)
        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH

    _data_flash_begin = __data_flash_begin;
    _data_ram_begin = __data_start__;
    _data_ram_end = .;

    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        *(".uninitialized.*")
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM

    .PERSIST_MEM : ALIGN(4)
    {
        __persist_mem__ = .;
        . += PERSIST_MEM_AREA_SIZE;
    } > RAM

    .REBOOT_TAG : ALIGN(4)
    {
        __reboot_tag__ = .;
        . += REBOOT_TAG_AREA_SIZE;
    } > RAM

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    bss_size = _bss_end - _bss_start;

#if (CONFIG_SOC_SMP)
	_msp_total_size = __MSP_STACK_SIZE * 2;
#else
	_msp_total_size = __MSP_STACK_SIZE;
#endif

    .heap (COPY) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + (ORIGIN(RAM) + LENGTH(RAM) - _msp_total_size - _heap_start - 8); /* 16 bytes for boundary protection */
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

/*multiple cores require independent stack spaces*/
#if (CONFIG_SOC_SMP)
    .stack_core1 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2) (COPY) :
    {
        . = ALIGN(8);
        _sstack_core1 = .;
        __StackLimitCore1 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCore1 = .;
        _estack_core1 = .;
    } > RAM

    .stack_core0 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_core0 = .;
        __StackLimitCore0 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCore0 = .;
        _estack_core0 = .;
    } > RAM
#else
    .stack (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        __StackLimitCore0 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCore0 = .;
        __StackTop = .;
        _estack = .;
    } > RAM
#endif

    .swap (NOLOAD):
    {
        . = ALIGN(8);
        _swap_start = .;
        _shared_memory_start = .;
        * (.shared_memory)
        * (.shared_memory*)
        _shared_memory_end = .;
        * (.swap_data)
        * (.swap_data*)
        . = ALIGN(8);
        _swap_end = .;
    } > SWAP

#if CONFIG_PSRAM
    .psram.code :
    {
        . = ALIGN(4);
        PROVIDE(__psram_code_lma = LOADADDR(.psram.code));
        __psram_code_start__ = .;

        /* Explicitly marked PSRAM code */
        *(.psram.code)
        *(.psram.code*)

        *libbk_cli.a:*.c.obj(.text .text.* .rodata .rodata.*)
        *libtensorflow-microlite.a:*.o(.text .text.* .rodata .rodata.*)

#if CONFIG_ALL_CODE_IN_PSRAM
         *(.text .text.* .rodata .rodata.*)
         /* ARM exception tables should be in PSRAM when all code is in PSRAM */
         *(.ARM.extab* .gnu.linkonce.armextab.*)
         . = ALIGN(4);
         __exidx_start = .;
         *(.ARM.exidx* .gnu.linkonce.armexidx.*)
         __exidx_end = .;
#endif /*CONFIG_ALL_CODE_IN_PSRAM*/
        . = ALIGN(4);
        __psram_code_end__ = .;
    } > PSRAM_CODE_SECTION AT > FLASH


    .psram.data :
    {
        . = ALIGN(4);
        PROVIDE(__psram_data_lma = LOADADDR(.psram.data));
        __psram_data_start__ = .;

        *(.psram.data)
        *(.psram.data*)

        . = ALIGN(4);
        __psram_data_end__ = .;
    } > PSRAM_DATA_SECTION AT > FLASH

    .psram.bss (NOLOAD):
    {
        . = ALIGN(4);
        __psram_bss_start__ = .;
        *(.psram.bss)
        *(.psram.bss*)
        . = ALIGN(4);
        __psram_bss_end__ = .;
    } > PSRAM_DATA_SECTION

#endif

#if CONFIG_PSRAM
    s_rom_end = LOADADDR(.psram.data) + SIZEOF(.psram.data);
#else
    s_rom_end = LOADADDR(.data) + SIZEOF(.data);
#endif
    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;

    ASSERT((s_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 90K!!!")
    ASSERT((_shared_memory_end - _shared_memory_start) == 1024, ".shared_memory sections must be 1024 bytes!!!")
}
//eof

