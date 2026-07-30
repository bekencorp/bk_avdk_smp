add_subdirectory(port)

set(platform_s_inc
	${IDK_PATH}/include
	${IDK_PATH}/include/driver
	${IDK_PATH}/include/components
	${IDK_PATH}/middleware/driver/common
	${IDK_PATH}/middleware/driver/sys_ctrl
	${IDK_PATH}/middleware/soc/common/soc/include
	${IDK_PATH}/middleware/soc/common/hal/include
	${IDK_PATH}/middleware/soc/${SOC}/hal
	${IDK_PATH}/middleware/soc/${SOC}/soc
	${IDK_PATH}/middleware/driver/include/bk_private
	${IDK_PATH}/middleware/driver/include/bk_private/legacy
	${IDK_PATH}/middleware/soc/${SOC}
	${IDK_PATH}/include/soc/${SOC}
	${IDK_PATH}/middleware/driver/gpio
	${IDK_PATH}/middleware/driver/gpio/v2px
	${IDK_PATH}/middleware/driver/${SOC}
	${IDK_PATH}/components/bk_common/include
	${IDK_PATH}/middleware/driver/spinlock
	${IDK_PATH}/components/bk_ps/include
	${IDK_PATH}/components/bk_pm/include
	${IDK_PATH}/../ap/middleware/driver/sys_sw_regs
	${IDK_PATH}/middleware/driver/io_matrix/v2.0
	${IDK_PATH}/middleware/driver/mpc
	${IDK_PATH}/middleware/driver/aspl
)
# Arch include differs: bk7259 = M52 (armv8.1-m), others = cm33.
if (${SOC} STREQUAL "bk7259")
	list(APPEND platform_s_inc ${IDK_PATH}/middleware/arch/cortex-m/include)
	# The kept SDK sys bring-up (sys_hal.c / sys_pm_hal.c / pwr_clk.c) pulls in
	# pwr_clk.h -> ram_regions.h (generated memory-layout header in the
	# project-level partitions dir) and sys_pm_hal.c -> sys_sw_regs.h. Provide
	# both dirs so they resolve without dragging in the full app build tree.
	list(APPEND platform_s_inc
		${IDK_PATH}/middleware/driver/sys_sw_regs
		${PROJECT_PARTITIONS_DIR})
else()
	list(APPEND platform_s_inc ${IDK_PATH}/middleware/arch/cm33/include)
endif()
target_include_directories(platform_s PUBLIC ${platform_s_inc})

if (${SOC} STREQUAL "bk7259")
# BK7259 self-contained secure world: the full-feature non-secure peripheral
# drivers (uart/flash/efuse/otp/ckmn) are NO LONGER compiled here; they are
# provided by ../../bk7259/armino_min/ (see HLD D11 / DD 3.4). What remains are
# the genuine SoC bring-up subsystems still referenced from the SDK:
#   - sys clock/voltage/DVFS bring-up (sys_driver.c / sys_hal.c / sys_pm_hal.c +
#     sys_clock_driver.c): heavy analog/PLL/DVFS init, not a "peripheral driver";
#     not yet self-contained (depends on 13k-line sys_ll.h + analog tables).
#   - MPC stack (mpc_driver.c / mpc_hal.c): ported per prior decision.
#   - gpio_driver.c / timer / adc_hal / aon_pmu_hal / pwr_clk: kept until their
#     secure-world call sites are pruned or migrated to armino_min.
target_sources(platform_s
	PRIVATE
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver_common.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
		${IDK_PATH}/middleware/driver/${SOC}/gpio_driver.c
		${IDK_PATH}/middleware/driver/mpc/mpc_driver.c
		${IDK_PATH}/middleware/soc/common/hal/mpc_hal.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_clock_driver.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_int_driver.c
		${IDK_PATH}/middleware/driver/timer/timer_driver.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/timer_hal.c
		${IDK_PATH}/middleware/soc/common/hal/timer_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/adc_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/aon_pmu_hal.c
		${IDK_PATH}/middleware/driver/pwr_clk/pwr_clk.c
)
# The kept SDK sys bring-up is normally compiled with GNU C (gnu99). The TF-M
# platform build uses strict -std=c99, under which the GNU `asm` keyword (used
# by sys_hal.c DSB/ISB barriers in the DVFS path) is not recognised. Compile
# these SDK files with gnu11 so their inline asm / GNU extensions parse.
set_source_files_properties(
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
	PROPERTIES COMPILE_OPTIONS "-std=gnu11"
)
# The secure-world config tree does not source components/bk_pm/Kconfig, so the
# PM Kconfig value symbols it introduced (used as literals in sys_pm_hal.c) are
# undefined here. Provide the Kconfig default (matches the CP-core sdkconfig).
target_compile_definitions(platform_s PRIVATE CONFIG_AON_LDO_SEL=4)
if (BL2)
	target_compile_definitions(platform_bl2 PRIVATE CONFIG_AON_LDO_SEL=4)
endif()
else()
target_sources(platform_s
	PRIVATE
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver_common.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_video_driver.c	
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
		# $<$<STREQUAL:${SOC},bk7236>:${IDK_PATH}/middleware/driver/gpio/gpio_driver_base.c>
		${IDK_PATH}/middleware/driver/${SOC}/gpio_driver.c
		# ${IDK_PATH}/middleware/soc/common/hal/gpio_hal.c
		# ${IDK_PATH}/middleware/soc/${SOC}/hal/gpio_ll.c
		${IDK_PATH}/middleware/driver/mpc/mpc_driver.c
		${IDK_PATH}/middleware/soc/common/hal/mpc_hal.c
		${IDK_PATH}/middleware/driver/ppc/prro_driver.c
		${IDK_PATH}/middleware/soc/common/hal/prro_hal.c
		${IDK_PATH}/middleware/driver/uart/uart_driver.c
		${IDK_PATH}/middleware/soc/common/hal/uart_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/uart_ll.c
		${IDK_PATH}/middleware/driver/flash/flash_driver.c
		${IDK_PATH}/middleware/soc/common/hal/flash_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/flash_ll.c
		${IDK_PATH}/middleware/driver/sys_ctrl/sys_clock_driver.c
        ${IDK_PATH}/middleware/driver/sys_ctrl/sys_int_driver.c
		${IDK_PATH}/middleware/driver/timer/timer_driver.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/timer_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/timer_ll.c
		${IDK_PATH}/middleware/soc/common/hal/timer_hal.c
		${IDK_PATH}/middleware/driver/ckmn/ckmn_driver.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/ckmn_ll.c
		${IDK_PATH}/middleware/soc/common/hal/ckmn_hal.c
        ${IDK_PATH}/middleware/soc/${SOC}/hal/adc_hal.c
		${IDK_PATH}/middleware/soc/${SOC}/hal/aon_pmu_hal.c
		${IDK_PATH}/middleware/driver/pwr_clk/pwr_clk.c
		${IDK_PATH}/middleware/driver/saradc/adc_driver.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/bk_sensor.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/temp_detect.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/volt_detect.c
		${IDK_PATH}/middleware/driver/common/drv_model.c
		${IDK_PATH}/middleware/soc/common/hal/otp_hal.c
		${IDK_PATH}/middleware/driver/otp/otp_driver_v1_1.c
		$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/anti_tamper/anti_tamper_driver.c>
		$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/soc/common/hal/anti_tamper_hal.c>
		$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/io_matrix/v2.0/io_matrix_driver.c>
		$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/io_matrix/v2.0/gpio_adapter.c>
		$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/soc/common/hal/iomx_hal.c>
)
endif()

if (PLATFORM_FLIH_IRQ_TEST_SUPPORT AND NOT (${SOC} STREQUAL "bk7259"))
target_sources(platform_s
	PRIVATE
		${IDK_PATH}/middleware/driver/timer/timer_driver_s.c
)
endif()

if (BL2)
	target_include_directories(platform_bl2 PUBLIC ${platform_s_inc})

	if (${SOC} STREQUAL "bk7259")
	# See platform_s note above: peripheral drivers replaced by armino_min;
	# only SoC bring-up (sys clock/DVFS), MPC, gpio, timer, adc, aon_pmu,
	# pwr_clk remain referenced from the SDK for now.
	target_sources(platform_bl2
		PRIVATE
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver.c
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver_common.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
			${IDK_PATH}/middleware/driver/${SOC}/gpio_driver.c
			${IDK_PATH}/middleware/driver/mpc/mpc_driver.c
			${IDK_PATH}/middleware/soc/common/hal/mpc_hal.c
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_clock_driver.c
			${IDK_PATH}/middleware/driver/timer/timer_driver.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/timer_hal.c
			${IDK_PATH}/middleware/soc/common/hal/timer_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/adc_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/aon_pmu_hal.c
			${IDK_PATH}/middleware/driver/pwr_clk/pwr_clk.c
	)
	else()
	target_sources(platform_bl2
		PRIVATE
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver.c
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver_common.c
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_video_driver.c		
			${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
			# $<$<STREQUAL:${SOC},bk7236>:${IDK_PATH}/middleware/driver/gpio/gpio_driver_base.c>
			${IDK_PATH}/middleware/driver/${SOC}/gpio_driver.c
			# ${IDK_PATH}/middleware/soc/common/hal/gpio_hal.c
			# ${IDK_PATH}/middleware/soc/${SOC}/hal/gpio_ll.c
			${IDK_PATH}/middleware/driver/mpc/mpc_driver.c
			${IDK_PATH}/middleware/soc/common/hal/mpc_hal.c
			${IDK_PATH}/middleware/driver/ppc/prro_driver.c
			${IDK_PATH}/middleware/soc/common/hal/prro_hal.c
			${IDK_PATH}/middleware/driver/uart/uart_driver.c
			${IDK_PATH}/middleware/soc/common/hal/uart_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/uart_ll.c
			${IDK_PATH}/middleware/driver/flash/flash_driver.c
			${IDK_PATH}/middleware/soc/common/hal/flash_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/flash_ll.c
			${IDK_PATH}/middleware/driver/sys_ctrl/sys_clock_driver.c
			# ${IDK_PATH}/middleware/driver/sys_ctrl/sys_int_driver.c
			${IDK_PATH}/middleware/driver/timer/timer_driver.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/timer_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/timer_ll.c
			${IDK_PATH}/middleware/soc/common/hal/timer_hal.c
			${IDK_PATH}/middleware/driver/ckmn/ckmn_driver.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/ckmn_ll.c
			${IDK_PATH}/middleware/soc/common/hal/ckmn_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/adc_hal.c
			${IDK_PATH}/middleware/soc/${SOC}/hal/aon_pmu_hal.c
			${IDK_PATH}/middleware/driver/pwr_clk/pwr_clk.c
			${IDK_PATH}/middleware/driver/saradc/adc_driver.c
			${IDK_PATH}/middleware/driver/saradc/temp_detect/bk_sensor.c
			${IDK_PATH}/middleware/driver/saradc/temp_detect/temp_detect.c
			${IDK_PATH}/middleware/driver/saradc/temp_detect/volt_detect.c
			${IDK_PATH}/middleware/soc/common/hal/otp_hal.c
			${IDK_PATH}/middleware/driver/otp/otp_driver_v1_1.c
			$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/anti_tamper/anti_tamper_driver.c>
			$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/soc/common/hal/anti_tamper_hal.c>
			$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/io_matrix/v2.0/io_matrix_driver.c>
			$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/driver/io_matrix/v2.0/gpio_adapter.c>
			$<$<OR:$<STREQUAL:${SOC},bk7236n>,$<STREQUAL:${SOC},bk7239n>>:${IDK_PATH}/middleware/soc/common/hal/iomx_hal.c>
	)
	endif()
endif()
