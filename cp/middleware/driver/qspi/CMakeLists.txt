if (CONFIG_QSPI)
	list(APPEND incs qspi)
	list(APPEND srcs
		qspi/qspi_driver.c
		qspi/qspi_statis.c
		qspi/qspi_psram.c
		qspi/qspi_flash.c
	)

	if (CONFIG_CLI AND CONFIG_QSPI_TEST)
		list(APPEND srcs qspi/qspi_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
