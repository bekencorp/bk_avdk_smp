set(incs include include/bk_private)
set(srcs)

list(APPEND incs
	.
	)

if (CONFIG_CLI_USER_CONFIG)
list(APPEND incs ${CONFIG_CLI_USER_CONFIG_DIR})
endif()

if (CONFIG_CLI)
list(APPEND srcs
	cli_event.c
	cli_common.c
	cli_main.c
	cli_misc.c
	cli_heap.c
	cli_mem.c
	cli_sram.c
	cli_sram_bus_bench.c
	cli_os.c
	cli_rpc.c
	cli_reg.c
	cli_exception.c
	)

if (CONFIG_SOC_SMP)
	list(APPEND srcs
		cli_ap_hotplug.c
	)
endif()

if (CONFIG_ARCH_CORTEX_M)
	list(APPEND srcs
    	# cli_fpb.c
    	cli_dwt.c
	)
endif()

if (CONFIG_BUTTON OR CONFIG_ADC_KEY)
	list(APPEND srcs
		cli_key.c
	)
endif()


if (CONFIG_PM_CLI_ENABLE)
	list(APPEND srcs
		cli_pwr.c
	)
endif()

if (CONFIG_WIFI_CLI_ENABLE)
	list(APPEND srcs
		cli_wifi.c
	)
endif()

if (CONFIG_AIRKISS_TEST)
	list(APPEND srcs
		cli_airkiss.c
	)
endif()

if (CONFIG_IPERF_TEST)
	list(APPEND srcs
		cli_iperf.c
	)
endif()


if (CONFIG_SDIO_SLAVE)
	list(APPEND srcs
		cli_sdio_slave.c
	)
endif()


if (CONFIG_GSENSOR_TEST_EN)
	list(APPEND srcs
		cli_gsensor.c
	)
endif()



if (CONFIG_FATFS)
	list(APPEND srcs "cli_fatfs.c")
endif()

if (CONFIG_DHARA_FTL)
	list(APPEND srcs "cli_nand_ftl.c")
endif()

if (CONFIG_VFS)
	list(APPEND srcs
		cli_vfs.c
	)
endif()

if (CONFIG_SDIO_HOST)
	list(APPEND srcs
		cli_sdio_host.c
	)
endif()

if (CONFIG_LWIP)
	list(APPEND srcs
		cli_lwip.c
	)
endif()

if (CONFIG_BK_NETIF)
	list(APPEND srcs
		cli_netif.c
	)
endif()

if ((CONFIG_CPU_CNT GREATER 1))
	list(APPEND srcs
		rpc_client_gpio.c
	)
endif()

if (CONFIG_USB)
	list(APPEND srcs "cli_usb.c")
endif()


	if (CONFIG_OTP_V1 AND CONFIG_OTP_TEST)
		list(APPEND srcs
			"cli_otp.c"
			"cli_package_type.c"
		)
	endif()




if (CONFIG_AT_CMD)
	list(APPEND srcs "cli_at.c")
endif()

if (CONFIG_EASY_FLASH)
	list(APPEND srcs
		"cli_easyflash.c"
	)
endif()

if (CONFIG_TOUCH_TEST)
	list(APPEND srcs "cli_touch.c")
endif()

if (CONFIG_CALENDAR)
	list(APPEND srcs "cli_calendar.c")
endif()

if (CONFIG_CPU_PERF_TEST)
	list(APPEND srcs "cli_cpu_perf.c")
endif()

if (CONFIG_GET_UID_TEST)
	list(APPEND srcs cli_uid.c)
endif()

elseif (CONFIG_SHELL_ASYNCLOG)
	list(APPEND srcs "cli_main.c")
endif() #if(CONFIG_CLI)

if (CONFIG_SHELL_ASYNCLOG)
	list(APPEND srcs
			shell_task.c
			shell_uart.c
	)
	if ((CONFIG_MAILBOX))
		list(APPEND srcs
			shell_mailbox_cp1.c
		)
	endif()
endif()

if (CONFIG_BATTERY_TEST)
	list(APPEND srcs cli_battery.c)
endif()


if (CONFIG_QRCODEGEN_TEST)
	list(APPEND srcs cli_qrcodegen.c)
endif()

if (CONFIG_MPC_TEST)
	list(APPEND srcs cli_mpc.c)
endif()

if (CONFIG_PRRO_TEST)
	list(APPEND srcs cli_prro.c)
endif()

if (CONFIG_INTERRUPT_TEST)
	list(APPEND srcs cli_interrupt.c)
endif()


if (CONFIG_FLASHDB_DEMO)
	list(APPEND srcs cli_flashdb.c)
endif()

if (CONFIG_PUF_TEST)
	list(APPEND srcs cli_puf.c)
endif()

if (CONFIG_TFM_MPC_NSC)
	list(APPEND srcs cli_mpc.c)
endif()

if (CONFIG_TFM_INT_TARGET_NSC)
	list(APPEND srcs cli_int_target.c)
endif()




if (CONFIG_BK_MODEM)
	list(APPEND srcs cli_modem.c)
endif()

set(PRIV_REQUIRES_LIST bk_common psa_mbedtls bk_netif bk_ps fatfs bk_vfs bk_init bk_libs bk_wifi compal ota cmsis flashdb bk_wifi_driver lwip_intf_v2_1 console bk_batt_monitor app)
if (CONFIG_DHARA_FTL)
	list(APPEND PRIV_REQUIRES_LIST dhara)
endif()

armino_component_register(SRCS "cli_gsensor.c" "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES utf8 app driver bk_system iperf fatfs http adc_key key bk_websocket core cortex-m demos
	#PRIV_REQUIRES airkiss bk_common bk_netif bk_ps
	PRIV_REQUIRES ${PRIV_REQUIRES_LIST}
	)

if(DEFINED ENV{COREMARK_CONTEXTS})
	set(coremark_contexts "$ENV{COREMARK_CONTEXTS}")
else()
	set(coremark_contexts "2")
endif()

if(DEFINED ENV{COREMARK_OPT_FLAG})
	set(coremark_opt_flag "$ENV{COREMARK_OPT_FLAG}")
else()
	set(coremark_opt_flag "-Os")
endif()

if(DEFINED ENV{COREMARK_TEST_ITERATIONS})
	set(coremark_iterations "$ENV{COREMARK_TEST_ITERATIONS}")
else()
	set(coremark_iterations "12000")
endif()

if(DEFINED ENV{COREMARK_CPU_FREQ_MHZ})
	set(coremark_cpu_freq_mhz "$ENV{COREMARK_CPU_FREQ_MHZ}")
else()
	set(coremark_cpu_freq_mhz "480")
endif()

# Enable M55 MVE (Helium) for CoreMark if requested
if(DEFINED ENV{COREMARK_ENABLE_MVE} AND "$ENV{COREMARK_ENABLE_MVE}" STREQUAL "1")
	set(coremark_mve_flags "-march=armv8.1-m.main+mve")
	set(coremark_opt_label "${coremark_opt_flag}+MVE")
else()
	set(coremark_mve_flags "")
	set(coremark_opt_label "${coremark_opt_flag}")
endif()

# Suppress aggressive inlining by GCC14 if requested (mimics GCC10 code structure)
if(DEFINED ENV{COREMARK_NO_INLINE} AND "$ENV{COREMARK_NO_INLINE}" STREQUAL "1")
	set(coremark_inline_flags "-fno-inline-functions;-fno-inline-small-functions")
	set(coremark_opt_label "${coremark_opt_label}+NoInline")
elseif(DEFINED ENV{COREMARK_INLINE_LIMIT})
	# GCC10 default -finline-limit=30; GCC14 default is much higher (~600)
	# Use = syntax so GCC doesn't misparse the value as a linker input file
	set(coremark_inline_flags
	    "--param=max-inline-insns-single=$ENV{COREMARK_INLINE_LIMIT}"
	    "--param=max-inline-insns-auto=$ENV{COREMARK_INLINE_LIMIT}")
	set(coremark_opt_label "${coremark_opt_label}+InlineLimit$ENV{COREMARK_INLINE_LIMIT}")
else()
	set(coremark_inline_flags "")
endif()

# Place CoreMark hot functions in PSRAM code section if requested
if(DEFINED ENV{COREMARK_PSRAM_CODE} AND "$ENV{COREMARK_PSRAM_CODE}" STREQUAL "1")
	set(coremark_psram_flag "-DCOREMARK_PSRAM_CODE=1")
	set(coremark_opt_label "${coremark_opt_label}+PSRAM")
else()
	set(coremark_psram_flag "")
endif()

# Place CoreMark hot functions in internal SRAM (.iram) to eliminate Flash XIP stall
if(DEFINED ENV{COREMARK_IRAM_CODE} AND "$ENV{COREMARK_IRAM_CODE}" STREQUAL "1")
	set(coremark_iram_flag "-DCOREMARK_IRAM_CODE=1")
	set(coremark_opt_label "${coremark_opt_label}+IRAM")
else()
	set(coremark_iram_flag "")
endif()

set(coremark_freq_flag "-DCOREMARK_CPU_FREQ_MHZ=${coremark_cpu_freq_mhz}")
set(coremark_opt_label "${coremark_opt_label}+${coremark_cpu_freq_mhz}M")

set(coremark_data_flags "")
if(DEFINED ENV{COREMARK_SRAM3_DATA} AND "$ENV{COREMARK_SRAM3_DATA}" STREQUAL "1")
	list(APPEND coremark_data_flags "-DCOREMARK_SRAM3_DATA=1")
	set(coremark_opt_label "${coremark_opt_label}+SRAM3DATA")
elseif(DEFINED ENV{COREMARK_SRAM4_DATA} AND "$ENV{COREMARK_SRAM4_DATA}" STREQUAL "1")
	list(APPEND coremark_data_flags "-DCOREMARK_SRAM4_DATA=1")
	set(coremark_opt_label "${coremark_opt_label}+SRAM4DATA")
elseif(DEFINED ENV{COREMARK_DTCM_DATA} AND "$ENV{COREMARK_DTCM_DATA}" STREQUAL "1")
	list(APPEND coremark_data_flags "-DCOREMARK_DTCM_DATA=1")
	set(coremark_opt_label "${coremark_opt_label}+DTCMDATA")
endif()

set(coremark_measure_flags "")
if(DEFINED ENV{COREMARK_SEGMENT_ENABLE} AND "$ENV{COREMARK_SEGMENT_ENABLE}" STREQUAL "1")
	list(APPEND coremark_measure_flags "-DCOREMARK_SEGMENT_ENABLE=1")
	set(coremark_opt_label "${coremark_opt_label}+SEG")
endif()
if(DEFINED ENV{COREMARK_PMU_ENABLE} AND "$ENV{COREMARK_PMU_ENABLE}" STREQUAL "1")
	list(APPEND coremark_measure_flags "-DCOREMARK_PMU_ENABLE=1")
	set(coremark_opt_label "${coremark_opt_label}+PMU")
endif()

component_compile_options(
	${coremark_opt_flag}
	${coremark_mve_flags}
	${coremark_inline_flags}
	${coremark_psram_flag}
	${coremark_iram_flag}
	${coremark_freq_flag}
	${coremark_data_flags}
	${coremark_measure_flags}
	-DCOREMARK_CONTEXTS=${coremark_contexts}
	-DCOREMARK_TEST_ITERATIONS=${coremark_iterations}
	-DCOREMARK_OPT_LABEL="${coremark_opt_label}"
	-DCOREMARK_COMPILER_FLAGS="${coremark_opt_label}"
)

armino_component_register_call_subdirs()
