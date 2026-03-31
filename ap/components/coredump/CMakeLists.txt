set(incs)
set(srcs)
set(priv_incs)

if (CONFIG_CM_BACKTRACE)
	list(APPEND incs
		cm_backtrace
		cm_backtrace/Languages/en-US
	)

	list(APPEND srcs
		cm_backtrace/cm_backtrace.c
	)
endif()

list(APPEND srcs
	bk_coredump.c
	bk_coredump_arm.c
	bk_coredump_uart.c
	bk_coredump_mem.c
	trap_test.c
	bk_traceback.c
	memdump_strategies/memdump_minimal.c
	memdump_strategies/memdump_all.c
)

list(APPEND priv_incs .)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_INCLUDE_DIRS "${priv_incs}"
	PRIV_REQUIRES os_source
)

