armino_build_get_property(target ARMINO_SOC)
armino_build_get_property(core ARMINO_CORE)

if(NOT "${target}" STREQUAL "bk7259")
    return()
endif()

set(incs
	soc
	hal
	./
	./misc
	../../../components/bk_pm/include
	)
set(srcs "misc/stub.c")

list(APPEND incs
	$ENV{PROJECT_DIR}/partitions/${target}/
	$ENV{PROJECT_DIR}/partitions
	$ENV{PROJECT_DIR}/cp/config/${target}/
)

if(CONFIG_SPE)
list(APPEND srcs	"mpu_cfg.c")
else()
list(APPEND srcs	"mpu_cfg_s.c")
endif()

list(APPEND srcs
	"memory.c"
	"exception.c"
	"misc/arch_delay.c"
	"misc/core_sleep.c"
	"misc/stack_base.c"
	"vt_relocate.c"
	"soc_init.c"
	"hal/psram_ll.c"
	"hal/pwm_hal_debug.c"
	"hal/timer_hal_debug.c"
	"hal/timer_hal.c"
	"hal/reset_reason_hal.c"
	"hal/gpio_hal.c"
	"hal/gpio_hal_debug.c"
	"hal/adc_hal.c"
	"hal/dma_hal_debug.c"
	"hal/uart_hal_debug.c"
	"hal/uart_ll.c"
	"hal/wdt_hal_debug.c"
	"hal/wdt_hal.c"
	"hal/wdt_ll.c"
	"hal/efuse_hal_debug.c"
	# "hal/adc_hal_debug.c"
	# "hal/adc_ll.c"
	"hal/sys_hal.c"
	"hal/sys_hal_debug.c"
	# "hal/sys_aonp_hal_debug.c"
	"hal/sys_types.c"
	"hal/sys_pm_hal.c"
	"hal/sys_pm_hal_ctrl.c"
	"hal/sys_pm_hal_debug.c"
	#"hal/i2s_hal_debug.c"
	# "hal/jpeg_hal_debug.c"
	# "hal/jpeg_ll.c"
	"hal/flash_hal_debug.c"
	"hal/aon_rtc_hal_debug.c"
	"hal/aon_pmu_hal.c"
	"hal/aon_pmu_hal_debug.c"
	#"hal/sdio_hal_debug.c"
	"hal/i2s_hal.c"
	"hal/ckmn_hal_debug.c"
	"hal/lin_hal_debug.c"
	"hal/scr_hal.c"
	"hal/scr_hal_debug.c"
	"hal/irda_hal.c"
	"hal/mcheck_hal.c"
	"hal/multicore_hal.c"
	"hal/wwdt_hal.c"
	"hal/pwm_hal_v2p2.c"
)

if (CONFIG_MAILBOX_V2_0)
list(APPEND srcs
	"hal/mbox0_hal.c"
)
endif()

# Crash-dump-to-flash writer: only build when the dump output method is Flash.
# Avoids pulling in the BK_PARTITION_OTA reference when dumps go to UART.
if (CONFIG_DUMP_BY_FLASH)
list(APPEND srcs
	"misc/soc_flash_dump.c"
)
endif()

list(APPEND srcs
	"core_init.c"
)


if(CONFIG_AUDIO AND CONFIG_SOC_BK7259)
	list(APPEND srcs
		#"hal/aud_hal.c"
		"hal/aud_hal_bk7259.c"
	)
endif()

if(CONFIG_PSRAM)
	list(APPEND srcs
		"hal/psram_hal.c"
	)
endif()

if(CONFIG_XDAC)
	list(APPEND srcs
		"hal/xdac_hal.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES coredump mpu REQUIRES partitions os_source)

armino_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
armino_build_get_property(config_dir CONFIG_DIR)

#TODO add support to generate ELF with/without bootloader

set(LD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${core})

set(LD_FILE ${LD_DIR}/${target}_bsp.ld)

if(EXISTS "$ENV{PROJECT_DIR}/${target}_bsp.ld")
	set(LD_FILE $ENV{PROJECT_DIR}/${target}_bsp.ld)
endif()

LOGI("[Select ld file]: ${LD_FILE}.")

set(ld_header_dependencies
	${armino_path}/include/soc/${target}/reg_base.h
	$ENV{PARTITIONS_DIR}/partitions_gen.h
)

add_custom_command(
	OUTPUT ${target}_out.ld
	COMMAND "${CMAKE_C_COMPILER}" -P -x c -E - < ${LD_FILE} -o ${target}_out.ld
			-I ${armino_path}/include/ -I ${config_dir}
			-I ${armino_path}/middleware/boards/${target}/partitions
			-I $ENV{SECURITY_PREBUILD_DIR}/
			-I $ENV{PARTITIONS_DIR}
	MAIN_DEPENDENCY ${LD_DIR}/${target}.ld ${sdkconfig_header}
	DEPENDS ${ld_header_dependencies}
	COMMENT "Generating linker script..."
	VERBATIM)

add_custom_target(${target}_linker_script DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}_out.ld)
add_dependencies(${COMPONENT_LIB} ${target}_linker_script)

target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/${target}_out.ld")
