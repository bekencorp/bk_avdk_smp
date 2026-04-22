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
	flash
	uart
	sys_ctrl
	gpio
	general_dma
	icu
	ipi
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
	pwm
	)

list(APPEND incs
	${armino_path}/$ENV{PROJECT_DIR}/config/${target}/
)

set(srcs
	sys_ctrl/sys_driver.c
	sys_ctrl/sys_driver_common.c
	sys_ctrl/sys_ps_driver.c
	sys_ctrl/sys_int_driver.c
	sys_ctrl/sys_clock_driver.c
	icu/interrupt_base.c
	pmu/aon_pmu_driver.c
	saradc/cli_adc_api.c
	saradc/cli_sadc_api.c
	saradc/adc_cmd_api.c
	saradc/saradc_server.c
	saradc/saradc_notify.c
	pwr_clk/pwr_clk.c
	pwr_clk/rosc_32k.c
	pwr_clk/rosc_ppm.c
	chip_support/chip_support.c
	)

if (CONFIG_SARADC_V1P2)
	list(APPEND incs
		saradc/v1_2/
	)

	list(APPEND srcs
		saradc/v1_2/adc_driver.c
		saradc/v1_2/adc_legacy_api.c
		saradc/v1_2/adc_ate.c
		saradc/v1_2/adc_test.c
	)
elseif (CONFIG_SARADC_V1P1)
	list(APPEND incs
		saradc/v1_1/
	)

	list(APPEND srcs
		saradc/v1_1/adc_driver.c
		saradc/v1_1/adc_statis.c
	)
endif()

	list(APPEND srcs
		common/driver.c
		common/drv_model.c
 		#icu/icu_driver.c
		sys_ctrl/sys_bt_driver.c
		sys_ctrl/sys_wifi_driver.c
		##sys_ctrl/sys_audio_driver.c
		sys_ctrl/sys_psram_driver.c
		sys_ctrl/sys_touch_driver.c
		#pwr_clk/low_pwr_core.c
		#pwr_clk/low_pwr_misc.c
		#sys_ctrl/sys_ctrl.c
		#timer/timer_driver.c
		#wdt/wdt_driver.c
		#gpio/gpio_driver_base.c
		#uart/uart_driver.c
		#uart/uart_statis.c
		#uart/printf.c
	)

if (CONFIG_SOC_BK7259)
	list(APPEND srcs
		sys_ctrl/sys_audio_driver.c
	)
endif()

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

if (CONFIG_WAKEUP)
	list(APPEND incs
		wakeup
	)

	list(APPEND srcs
		wakeup/wakeup_driver.c
	)
endif()

if (NOT CMAKE_BUILD_EARLY_EXPANSION)
	add_subdirectory(common)
	add_subdirectory(rtc)
	add_subdirectory(trng)
	add_subdirectory(efuse)
	add_subdirectory(deep_lv)
	add_subdirectory(wdt)
	add_subdirectory(timer)
	add_subdirectory(gpio)
	add_subdirectory(flash)
	add_subdirectory(uart)
	add_subdirectory(general_dma)
	# add_subdirectory(saradc)
	add_subdirectory(pwm)
	add_subdirectory(spi)
	add_subdirectory(i3c)
	# add_subdirectory(i2c)
	add_subdirectory(ckmn)
	add_subdirectory(qspi)
	#add_subdirectory(scr)
	add_subdirectory(touch)
	#add_subdirectory(vault)
	#add_subdirectory(i2s)
	add_subdirectory(psram)
	#add_subdirectory(h264)
	#add_subdirectory(device)
	add_subdirectory(lin)
	add_subdirectory(io_matrix)
	#add_subdirectory(wwdt)
	add_subdirectory(multicore)
	add_subdirectory(mailbox)
	add_subdirectory(ipi)
	add_subdirectory(hspl)
	add_subdirectory(irda)
	add_subdirectory(sys_sw_regs)
	add_subdirectory(aspl)
endif()

if (CONFIG_USB)
	list(APPEND srcs
		sys_ctrl/sys_usb_driver.c
	)
endif()

if (CONFIG_EFUSE)
	list(APPEND srcs
		efuse/efuse_driver.c
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

if (CONFIG_JPEGDEC_HW)
	list(APPEND srcs
		"jpeg_dec/jpeg_dec_driver.c"
		)
endif()







if (CONFIG_AUDIO)

if (CONFIG_SBC)
	list(APPEND srcs
		"sbc/sbc_driver.c"
		)
endif()

	list(APPEND srcs
		"audio/aud/aud_common_driver.c"
		#"audio/uac_driver.c"
		)

if (CONFIG_LPVAD)
	list(APPEND srcs
		"lpvad/aud_lp_vad.c"
		)
endif()

if (CONFIG_AUDIO_ADC)
if (CONFIG_SOC_BK7259)
	list(APPEND srcs
		"audio/aud/aud_adc_driver_bk7259.c"
		)
else()
	list(APPEND srcs
		"audio/aud/aud_adc_driver.c"
		)
endif()
endif()

if (CONFIG_AUDIO_DMIC)
	list(APPEND srcs
		"audio/aud/aud_dmic_driver.c"
		)
endif()

if (CONFIG_AUDIO_DTMF)
	list(APPEND srcs
		"audio/aud/aud_dtmf_driver.c"
		)
endif()

if (CONFIG_AUDIO_DAC)

if (CONFIG_SOC_BK7259)
	list(APPEND srcs
		"audio/aud/aud_dac_driver_bk7259.c"
		)
else()
	list(APPEND srcs
		"audio/aud/aud_dac_driver.c"
		)
endif()
endif()
endif() #if (CONFIG_AUDIO)

if (CONFIG_AUDIO_RING_BUFF)
	list(APPEND srcs
		"audio/audio_ring_buff.c"
		)
endif()

if (CONFIG_OTP_V1)
	list(APPEND srcs
		"otp/otp_driver_v1_1.c"
		)
endif()

if (CONFIG_MAC802154_ENABLE)
	list(APPEND srcs
		sys_ctrl/sys_thread_driver.c
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

if (CONFIG_SARADC)
if (CONFIG_CLI_SARADC_TEST)
	list(APPEND srcs
		saradc/cli_adc.c
		)
endif()
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
#if (CONFIG_CKMN)
	list(APPEND incs
		ckmn
	)

	list(APPEND srcs
		ckmn/ckmn_driver.c
	)
#endif()

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

if (CONFIG_XDAC)
	list(APPEND srcs
		"xdac/xdac_driver.c"
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"

	REQUIRES core cortex-m mpu
	PRIV_REQUIRES bk_log easy_flash cmsis os_source unity console bk_phy temp_detect
)
