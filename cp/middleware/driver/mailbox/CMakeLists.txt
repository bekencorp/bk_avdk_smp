if (CONFIG_MAILBOX)
	if (CONFIG_MAILBOX_V2_0)
	list(APPEND srcs
		"mailbox/mbox0_drv.c"
		"mailbox/mbox0_adapter.c"
		)
	if (CONFIG_SOC_SMP)
	list(APPEND srcs
		"mailbox/mbox0_cross_core.c"
		)
	endif()
	endif()

	if ((CONFIG_CPU_CNT GREATER 1))
		list(APPEND srcs
			mailbox/mailbox_channel.c
			mailbox/mb_ipc_cmd.c
			mailbox/mb_ipc.c
			mailbox/mb_ipc_heartbeat.c
			mailbox/bk_api_ipc.c
			mailbox/bk_api_ipc_test.c
			mailbox/bk_api_rpc_server.c
			mailbox/mb_chnl_buff.c
			spinlock/amp_res_lock.c
			spinlock/spinlock.c
		)

		list(APPEND srcs
			mailbox/mb_uart_driver.c
		)

		if (CONFIG_IPC_STRESS_TEST)
			list(APPEND srcs
				mailbox/mb_ipc_stress_srv.c
			)
		endif()

	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)
