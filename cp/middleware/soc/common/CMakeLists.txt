armino_build_get_property(target ARMINO_SOC)

set(incs
	soc/include
	hal/include
	)

set(srcs
	hal/timer_hal.c
	hal/dma_hal.c
	hal/icu_hal.c
	# hal/spi_hal.c
	hal/uart_hal.c
	# hal/adc_hal.c
	)


	
if (CONFIG_AON_WDT)
	list(APPEND srcs
		hal/aon_wdt_hal.c
	)
endif()

if (CONFIG_FLASH)
	list(APPEND srcs
		hal/flash_hal.c
	)
endif()

if (CONFIG_FFT)
	list(APPEND srcs
		hal/fft_hal.c
		)
endif()

if (CONFIG_I2S)
if (NOT CONFIG_SOC_BK7259)
	list(APPEND srcs
		hal/i2s_hal.c
		)
endif()
endif()

if(CONFIG_EFUSE)
	list(APPEND srcs
		hal/efuse_hal.c
	)
endif()

if(CONFIG_MAILBOX)
	if (NOT CONFIG_MAILBOX_V2_0)
	list(APPEND srcs
		"hal/mailbox_hal.c"
	)
	endif()
endif()

if(CONFIG_AON_RTC)
	list(APPEND srcs
		"hal/aon_rtc_hal_64bit.c"
	)
endif()

if (CONFIG_SDIO_V2P0 OR CONFIG_SDIO_V2P1)
	list(APPEND srcs
		"hal/sdio_hal.c"
	)
endif()


if(CONFIG_SBC)
	list(APPEND srcs
		"hal/sbc_hal.c"
	)
endif()

if(CONFIG_CALENDAR)
	list(APPEND srcs
		"hal/calendar_hal.c"
	)
endif()

if(CONFIG_SDIO_HOST)
	list(APPEND srcs
		"hal/sdio_host_hal.c"
	)
endif()

if(CONFIG_SDMADC)
	list(APPEND srcs
		"hal/sdmadc_hal.c"
	)
endif()

if(CONFIG_OTP_V1)
	list(APPEND srcs
		"hal/otp_hal.c"
	)
endif()

#if(CONFIG_CKMN)
	list(APPEND srcs
		"hal/ckmn_hal.c"
	)
#endif()



if(CONFIG_LIN)
	list(APPEND srcs
		"hal/lin_hal.c"
	)
endif()

if(CONFIG_IPI)
	list(APPEND srcs
		"hal/ipi_hal.c"
	)
endif()

if(CONFIG_SLCD)
	list(APPEND srcs
		"hal/slcd_hal.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES ${target})
