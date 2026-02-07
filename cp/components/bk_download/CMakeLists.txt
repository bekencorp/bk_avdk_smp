set(incs include)
set(srcs)

if (CONFIG_DL_QSPI_TEST_ENABLE AND CONFIG_QSPI)
	list(APPEND srcs
		dl_test.c
	)
endif()

if (CONFIG_DL_QSPI_ENABLE AND CONFIG_QSPI)
	list(APPEND srcs
		dl_protocol.c
		dl_crc32.c
		dl_uart.c
		dl_main.c
		)
endif()

armino_component_register(SRCS "${srcs}" 
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES bk_common driver
	)

