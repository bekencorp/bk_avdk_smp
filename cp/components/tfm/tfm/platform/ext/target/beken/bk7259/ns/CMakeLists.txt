string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")
set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(platform_ns)

target_include_directories(platform_region_defs
    INTERFACE
        partition
        stub
)

target_include_directories(platform_ns
    PUBLIC
        ${ROOT_PATH}/tf-m-tests/tests_reg/build_spe/api_ns/platform/ext/common
        ${ROOT_PATH}/tf-m-tests/tests_reg/build_spe/api_ns/platform/include
        platform/ext/common/
        platform/include/
)

target_include_directories(platform_ns
    PUBLIC
        .
        board
        common/idk/port
        common/cmsis_core
        common/native_drivers
        common/secure
        common/partition
        driver
        idk
        include
        partition
        stub
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
        ${IDK_PATH}/middleware/driver/gpio
        ${IDK_PATH}/middleware/driver/${SOC}
        ${IDK_PATH}/middleware/arch/cm33/include
        ${IDK_PATH}/components/bk_common/include
        ${IDK_PATH}/middleware/driver/spinlock
        ${IDK_PATH}/components/bk_ps/include
        ${IDK_PATH}/components/temp_detect
        ${IDK_PATH}/middleware/driver/io_matrix/v2.0

)

target_sources(platform_ns
    PRIVATE
        startup_bk7259_ns.c
        #board/target_cfg.c
        #board/tfm_interrupts.c
        board/wdt.c
        common/cmsis_drivers/Driver_Flash.c
        common/cmsis_drivers/Driver_USART.c
        common/cmsis_core/system_cmsdk_bk7236.c
        common/cmsis_core/cache.c
        common/cmsis_core/mpu.c
        common/idk/port/mem_port.c
        common/idk/port/os_port.c
        ${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver.c
        ${IDK_PATH}/middleware/driver/sys_ctrl/sys_driver_common.c
        ${IDK_PATH}/middleware/soc/${SOC}/hal/sys_hal.c
        ${IDK_PATH}/middleware/soc/${SOC}/hal/sys_pm_hal.c
        # ${IDK_PATH}/middleware/driver/gpio/gpio_driver_base.c
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
        ${IDK_PATH}/middleware/soc/${SOC}/hal/adc_hal.c
        ${IDK_PATH}/middleware/soc/${SOC}/hal/aon_pmu_hal.c
        ${IDK_PATH}/middleware/driver/pwr_clk/pwr_clk.c
		${IDK_PATH}/middleware/driver/saradc/adc_driver.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/bk_sensor.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/temp_detect.c
		${IDK_PATH}/middleware/driver/saradc/temp_detect/volt_detect.c
		${IDK_PATH}/middleware/driver/common/drv_model.c
        ${TFM_BEKEN}/common/idk/port/printf.c 
        ${TFM_BEKEN}/common/idk/port/printf_base.c 
        ${TFM_BEKEN}/common/idk/port/str_arch.c
        ${IDK_PATH}/middleware/driver/io_matrix/v2.0/io_matrix_driver.c
		${IDK_PATH}/middleware/driver/io_matrix/v2.0/gpio_adapter.c
		${IDK_PATH}/middleware/soc/common/hal/iomx_hal.c

)

target_link_libraries(platform_ns
     PUBLIC
        ${CUR_DIR}/ext/accelerator/libtest_ns_crypto_hw.a
)