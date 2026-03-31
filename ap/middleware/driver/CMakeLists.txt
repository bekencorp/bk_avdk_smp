armino_build_get_property(target ARMINO_SOC)

# TODO
# 0. Too big components and makefile, need optimize it
# 2. optimize it when do driver optimization

set(incs
	.
	include
	include/bk_private
	include/bk_private/legacy
	common
	reset_reason
	pwm
	flash
	uart
	sys_ctrl
	gpio
	general_dma
	icu
	i2c
	ipi
	sdcard
	saradc
	pmu
	mailbox
	spinlock
	hspl
	touch
	sbc
	rtc
	i2s
	fft
	chip_support
	../compal/common_io/include
	wdt
	timer
	scr
	slcd
	pwr_clk
	)



set(srcs
	sys_ctrl/sys_driver.c
	sys_ctrl/sys_driver_common.c
	sys_ctrl/sys_ps_driver.c
	sys_ctrl/sys_int_driver.c
	sys_ctrl/sys_clock_driver.c


	icu/interrupt_base.c

	pmu/aon_pmu_driver.c


	pwr_clk/pwr_clk.c
	#pwr_clk/pm_ap_misc.c
	#pwr_clk/pm_ap_core.c
	chip_support/chip_support.c
	)
if (CONFIG_SARADC)
	list(APPEND srcs
		saradc/saradc_client.c
		saradc/saradc_example.c
		saradc/saradc_notify.c
	)



endif()

	list(APPEND srcs
		common/driver.c
		common/drv_model.c
		#timer/timer_driver.c
		wdt/wdt_driver.c
		#sys_ctrl/sys_ctrl.c
		sys_ctrl/sys_wifi_driver.c
		sys_ctrl/sys_bt_driver.c
		sys_ctrl/sys_audio_driver.c
		sys_ctrl/sys_video_driver.c
		#sys_ctrl/sys_psram_driver.c
		#sys_ctrl/sys_touch_driver.c
		#gpio/gpio_driver_base.c
		#uart/uart_driver.c
		#uart/uart_statis.c
		#uart/printf.c
	)

if (CONFIG_RESET_REASON)
	list(APPEND srcs reset_reason/reset_reason.c)
endif()

if (CONFIG_AON_WDT)
	list(APPEND incs
		aon_wdt
	)

	list(APPEND srcs
		aon_wdt/aon_wdt_driver.c
	)
endif()


if (NOT CMAKE_BUILD_EARLY_EXPANSION)
	add_subdirectory(common)
	add_subdirectory(rtc)
	add_subdirectory(trng)
	add_subdirectory(timer)
	add_subdirectory(gpio)
	add_subdirectory(io_matrix)
	add_subdirectory(flash)
	add_subdirectory(uart)
	add_subdirectory(general_dma)
	add_subdirectory(pwm)
	add_subdirectory(spi)
	add_subdirectory(i2c)
	add_subdirectory(qspi)
	#add_subdirectory(sdio_host)
	#add_subdirectory(sdio)
	#add_subdirectory(touch)
	#add_subdirectory(i2s)
	add_subdirectory(psram)
	add_subdirectory(irda)
	#add_subdirectory(wwdt)
	add_subdirectory(mailbox)
	add_subdirectory(multicore)
	add_subdirectory(ipi)
	add_subdirectory(hspl)
	add_subdirectory(h264)
	add_subdirectory(device)
	add_subdirectory(yuv_buf)
	add_subdirectory(lin)
	add_subdirectory(hp_dma)
	add_subdirectory(sys_sw_regs)
	add_subdirectory(aspl)
endif()

if (CONFIG_USB)
	list(APPEND srcs
		sys_ctrl/sys_usb_driver.c
	)
endif()

if (CONFIG_FFT)
	list(APPEND srcs
		fft/fft.c
		)
endif()

if (CONFIG_I2S)
	list(APPEND incs
		"i2s"
	)

	list(APPEND srcs
		i2s/i2s_driver.c
	)
endif()

if(CONFIG_SLCD)
	list(APPEND srcs
		"slcd/slcd_driver.c"
	)
endif()

if (CONFIG_LCD_QSPI)
	list(APPEND srcs
		"lcd/lcd_qspi_driver.c"
		)
endif()

if (CONFIG_LCD_SPI)
	list(APPEND srcs
		"lcd/lcd_spi_driver.c"
		)
endif()

if (CONFIG_JPEGENC_HW)
	list(APPEND srcs
		"jpeg_enc/jpeg_driver.c"
		"jpeg_enc/jpeg_statis.c"
		)
endif() #if (CONFIG_JPEGENC_HW)


if (CONFIG_CALENDAR)
	list(APPEND srcs
		"calendar/calendar_driver.c"
		)
endif() #if (CONFIG_CALENDAR)

if (CONFIG_SDIO_V1P0)
	list(APPEND incs
		"sdio/v1p0"
	)

	list(APPEND srcs
		"sdio/v1p0/sdio.c"
		"sdio/v1p0/sdma.c"
		"sdio/v1p0/sutil.c"
		)
endif() #if (CONFIG_SDIO_V1P0)

if (CONFIG_SDIO_V2P0 AND CONFIG_SDIO_SLAVE)
	list(APPEND incs
		"sdio/v2p0"
	)

	list(APPEND srcs
		"sdio/v2p0/sdio_slave_driver.c"
		"sdio/v2p0/sdio_utils.c"
		"sdio/v2p0/sdio_test.c"
		)
endif() #if (CONFIG_SDIO_V2P0)

if (CONFIG_SDIO_V3P0)
	list(APPEND incs
		"sdio_dwc/v3p0"
		"sdio_dwc/v3p0/include"
	)
	list(APPEND srcs
		"sdio_dwc/v3p0/sdio_host_driver.c"
		"sdio_dwc/v3p0/sd_card_driver.c"
		"sdio_dwc/v3p0/sdcard_test.c"
		"sdio_dwc/v3p0/sdio_dwc_test.c"
		)
endif() #if (CONFIG_SDIO_HOST)

if (CONFIG_AUDIO)

if (CONFIG_SBC)
	list(APPEND srcs
		"sbc/sbc_driver.c"
		)
endif()

if (CONFIG_AUD_DRIVER_V1)
	list(APPEND srcs
		"audio/aud/V1/aud_common_driver.c"
		)
	if (CONFIG_AUDIO_ADC)
		list(APPEND srcs
			"audio/aud/V1/aud_adc_driver.c"
			)
	endif()
	if (CONFIG_AUDIO_DAC)
		list(APPEND srcs
			"audio/aud/V1/aud_dac_driver.c"
			)
	endif()
	if (CONFIG_AUDIO_DMIC)
		list(APPEND srcs
			"audio/aud/V1/aud_dmic_driver.c"
			)
	endif()
	if (CONFIG_AUDIO_DTMF)
		list(APPEND srcs
			"audio/aud/V1/aud_dtmf_driver.c"
			)
	endif()
elseif (CONFIG_AUD_DRIVER_V2)
	list(APPEND srcs
		"audio/aud/V2/aud_common_driver.c"
		)
	if (CONFIG_AUDIO_ADC)
			list(APPEND srcs
				"audio/aud/V2/aud_adc_driver.c"
				)
	endif()
	if (CONFIG_AUDIO_DAC)
			list(APPEND srcs
				"audio/aud/V2/aud_dac_driver.c"
				)
	endif()
endif() # CONFIG_AUD_DRIVER_V1 / V2

endif() #if (CONFIG_AUDIO)

if (CONFIG_AUDIO_RING_BUFF)
	list(APPEND srcs
		"audio/audio_ring_buff.c"
		)
endif()

if (CONFIG_TOUCH)
	list(APPEND srcs
		"touch/touch_driver_v1_1.c"
		)
endif()


if (CONFIG_OTP_V1)
	list(APPEND srcs
		"otp/otp_driver_v1_1.c"
		)
endif()


	list(APPEND incs
		"${target}"
	)

	list(APPEND srcs
		"${target}/gpio_driver.c"
		"${target}/interrupt.c"
		"${target}/interrupt_statis.c"
		)

if (CONFIG_SDMADC)
	list(APPEND srcs
		"sdmadc/sdmadc_driver.c"
		)
endif()

# Ethernet Support
if (CONFIG_ETH)
	list(APPEND incs
		eth
		eth/phy
	)

	# MAC and PHY src
	list(APPEND srcs
		eth/eth_mac.c
		eth/eth_mac_ex.c
		eth/lan8742.c
		eth/phy/phy.c
		eth/phy/smsc.c
		eth/phy/micrel_ksz90x1.c
		eth/phy/miiphyutil.c
		eth/phy/realtek.c
	)
endif()

# ckmn Support
if (CONFIG_CKMN)
	list(APPEND incs
		ckmn
	)

	list(APPEND srcs
		ckmn/ckmn_driver.c
	)
endif()

if (CONFIG_SCR)
    list(APPEND srcs
		"scr/scr_driver_v1_26.c"
	)
endif()

if (CONFIG_CAN)
	list(APPEND incs can)
	list(APPEND srcs can/can_driver.c can/can_statis.c)

	if (CONFIG_CAN_TEST)
		list(APPEND srcs can/can_test.c)
	endif()

	if (CONFIG_CAN_DEMO)
		list(APPEND srcs can/can_demo.c)
	endif()
endif()

if (CONFIG_PM_DEMO_ENABLE)
	list(APPEND srcs
		pwr_clk/pm_ap_demo.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES core cortex-m mpu
	PRIV_REQUIRES bk_log easy_flash cmsis os_source bk_usb part_table unity avdk_utils bk_wifi console
)
