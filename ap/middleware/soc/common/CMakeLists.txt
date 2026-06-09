armino_build_get_property(target ARMINO_SOC)

set(incs
	soc/include
	hal/include
	)

set(srcs
	hal/timer_hal.c
	hal/dma_hal.c
	hal/icu_hal.c
	hal/i2c_hal.c
	hal/uart_hal.c
	)

if (CONFIG_AON_GPIO)
	list(APPEND srcs
		hal/gpio_hal.c
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

if(CONFIG_TRNG_SUPPORT)
	list(APPEND srcs
		"hal/trng_hal.c"
	)
endif()

if(CONFIG_IPI)
	list(APPEND srcs
		"hal/ipi_hal.c"
	)
endif()

if(CONFIG_MAILBOX)
	if (NOT CONFIG_MAILBOX_V2_0)
	list(APPEND srcs
		"hal/mailbox_hal.c"
	)
	endif()
endif()

if(CONFIG_QSPI)
	if (NOT CONFIG_SOC_BK7259)
		list(APPEND srcs
			"hal/qspi_hal.c"
		)
	endif()
endif()

if(CONFIG_PSRAM)
	list(APPEND srcs
		"hal/psram_hal.c"
	)
endif()


if (CONFIG_SDIO_V2P0)
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

if(CONFIG_CKMN)
	list(APPEND srcs
		"hal/ckmn_hal.c"
	)
endif()

if(CONFIG_LCD)
	list(APPEND srcs
		"hal/lcd_disp_hal.c"
	)
endif()

if(CONFIG_DMA2D)
	list(APPEND srcs
		"hal/dma2d_hal.c"
	)
endif()

if(CONFIG_HIGH_PERFORMANCE_DMA)
	list(APPEND srcs
		"hal/hpdma_hal.c"
	)
endif()

if(CONFIG_JPEGDEC_HW)
	list(APPEND srcs
		"hal/jpeg_dec_hal.c"
	)
endif()

if(CONFIG_LIN)
	list(APPEND srcs
		"hal/lin_hal.c"
	)
endif()

if(CONFIG_CAN)
	list(APPEND srcs
		"hal/can_hal.c"
	)
endif()

if(CONFIG_SLCD)
	list(APPEND srcs
		"hal/slcd_hal.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES ${target})
