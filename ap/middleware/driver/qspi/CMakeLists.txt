if (CONFIG_QSPI)
	list(APPEND incs qspi)
	list(APPEND srcs
		qspi/qspi_driver.c
		qspi/qspi_statis.c
	)

	if (CONFIG_QSPI_MST_PSRAM)
		list(APPEND srcs qspi/qspi_psram.c)
	endif()

	if (CONFIG_QSPI_NAND_FLASH)
		list(APPEND srcs qspi/qspi_nand_flash.c)
		list(APPEND srcs qspi/qspi_nand_bbm.c)
	elseif (CONFIG_QSPI_NOR_FLASH)
		list(APPEND srcs qspi/qspi_flash.c)
	endif()

	if (CONFIG_CLI AND CONFIG_QSPI_TEST)
		list(APPEND srcs qspi/qspi_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
