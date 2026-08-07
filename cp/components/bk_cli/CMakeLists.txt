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
	cli_mem.c
	cli_sram.c
	cli_os.c
	cli_rpc.c
	cli_reg.c
	)

if (CONFIG_SOC_SMP)
	list(APPEND srcs
		cli_cp_hotplug.c
		cli_basepri_stress.c
	)
endif()

	list(APPEND srcs
    	cli_fpb.c
    	cli_dwt.c
	)



if (CONFIG_PM_ENABLE)
	list(APPEND srcs
		cli_pwr.c
		cli_wfi_basepri_stress.c
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

if (CONFIG_FATFS)
	list(APPEND srcs "cli_fatfs.c")
endif()

if (CONFIG_VFS)
	list(APPEND srcs
		cli_vfs.c
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


if (CONFIG_MAC802154_TEST)
    list(APPEND srcs
		cli_mac802154.c
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

if (CONFIG_CALENDAR)
	list(APPEND srcs "cli_calendar.c")
endif()

if (CONFIG_CPU_PERF_TEST)
	list(APPEND srcs "cli_cpu_perf.c")
endif()

if (CONFIG_OTP_V1 AND CONFIG_OTP_TEST)
	list(APPEND srcs "cli_package_type.c")
endif()

elseif (CONFIG_SHELL_ASYNCLOG)
	list(APPEND srcs "cli_main.c")
endif() #if(CONFIG_CLI)

if (CONFIG_SHELL_ASYNCLOG OR CONFIG_BKREG)
	list(APPEND srcs
		shell_task.c
		shell_uart.c
	)
	if ((CONFIG_MAILBOX))
		list(APPEND srcs
			shell_mailbox_ipc.c
		)
	endif()
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

if (CONFIG_CKMN)
	list(APPEND srcs cli_ckmn.c)
endif()

if (CONFIG_TFM_MPC_NSC)
	list(APPEND srcs cli_mpc.c)
endif()

if (CONFIG_TFM_INT_TARGET_NSC)
	list(APPEND srcs cli_int_target.c)
endif()

if (CONFIG_LIN)
	list(APPEND srcs cli_lin.c)
endif()

if (CONFIG_SCR)
	list(APPEND srcs cli_scr.c)
endif()

if (CONFIG_MAC802154_TEST)
	list(APPEND srcs cli_mac802154.c)
endif()
set(PRIV_REQUIRES_LIST bk_common psa_mbedtls temp_detect bk_netif bk_ps bk_init bk_libs bk_wifi compal cmsis bk_bluetooth bk_phy lwip_intf_v2_1 console at )
# tfm exposes the NS-facing NSC headers (tfm/interface/include/tfm_*_nsc.h) used
# by cli_reg.c / cli_mpc.c under CONFIG_TFM_*_NSC, and links libtfm_s_veneers.a.
list(APPEND PRIV_REQUIRES_LIST tfm)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES utf8 app driver bk_system iperf adc_key controller_if
	#PRIV_REQUIRES airkiss bk_common bk_netif bk_ps
	PRIV_REQUIRES ${PRIV_REQUIRES_LIST}
	)

armino_component_register_call_subdirs()
