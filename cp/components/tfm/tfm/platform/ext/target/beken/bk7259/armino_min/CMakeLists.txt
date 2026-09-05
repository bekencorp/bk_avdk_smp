# BK7259 TF-M armino_min: self-contained minimal drivers for the secure world.
#
# These replace the gpio_map.h-coupled / full-feature SDK drivers
# (cp/middleware/driver/{uart,flash,otp,ckmn,saradc}, cp/middleware/soc/.../hal
# of the same) that common/idk/CMakeLists.txt used to pull in for bk7259.
#
# The clock / analog bring-up (sys_drv_early_init, sys_hal_switch_cpu_bus_freq,
# sys_drv_init, sys_drv_get_chip_id) is intentionally NOT reimplemented here and
# stays sourced from the SDK sys_hal.c / sys_driver.c (they program chip-specific
# PLL/BUCK/XTAL analog registers and DVFS voltage tables that are unsafe to
# minimize without silicon validation). sys_min.c only carries the lightweight
# PPC critical-section int shim. See docs/bk7259_security_detailed_design.md 3.4.

set(armino_min_dir ${CMAKE_CURRENT_LIST_DIR})

# Single shared flash core (physically one copy in the CP tree). Both the shared
# config table (flash_core_config.c) and the portable erase/write/SR/line-mode
# algorithm (bk_flash_core.c) are compiled here; flash_min.c now delegates to the
# core and only keeps the secure-world glue (XIP cbus, DBUS regions, remap, OTA).
get_filename_component(flash_core_dir
	"${armino_min_dir}/../../../../../../../../../middleware/soc/common/flash_core" ABSOLUTE)

set(armino_min_srcs
	${armino_min_dir}/hal/uart_min.c
	${armino_min_dir}/hal/flash_min.c
	${armino_min_dir}/hal/efuse_min.c
	${armino_min_dir}/hal/otp_min.c
	${armino_min_dir}/hal/sys_min.c
	${armino_min_dir}/hal/ckmn_min.c
	${armino_min_dir}/hal/prro_min.c
	${armino_min_dir}/hal/stub_sca.c
	${flash_core_dir}/flash_core_config.c
	${flash_core_dir}/bk_flash_core.c
)

# Scope the flash-core build knobs to the core TU only (avoid leaking into other
# secure sources). QUAD_ENABLE keeps XIP four-line mode.
set_source_files_properties(${flash_core_dir}/bk_flash_core.c
	PROPERTIES COMPILE_DEFINITIONS "CONFIG_FLASH_QUAD_ENABLE=1")

target_include_directories(platform_s PUBLIC ${armino_min_dir}/include ${flash_core_dir})
target_sources(platform_s PRIVATE ${armino_min_srcs})

if (BL2)
	target_include_directories(platform_bl2 PUBLIC ${armino_min_dir}/include ${flash_core_dir})
	target_sources(platform_bl2 PRIVATE ${armino_min_srcs})
endif()

# platform_ns is built in a separate (installed) NS tree and may not exist in
# the SPE configure pass; guard so this file is reusable from both.
if (TARGET platform_ns)
	target_include_directories(platform_ns PUBLIC ${armino_min_dir}/include ${flash_core_dir})
	target_sources(platform_ns PRIVATE ${armino_min_srcs})
endif()
