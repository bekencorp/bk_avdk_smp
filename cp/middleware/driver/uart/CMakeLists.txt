if (CONFIG_UART)
	list(APPEND incs uart)

	list(APPEND srcs
		uart/uart_driver.c
		uart/uart_statis.c
		uart/printf.c
	)

	if (CONFIG_CLI AND CONFIG_UART_TEST)
		list(APPEND srcs uart/uart_test.c)
	endif()

	if (CONFIG_CLI AND CONFIG_UART_API_TEST)
		list(APPEND srcs uart/cli_uart_api.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
