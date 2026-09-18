if (CONFIG_QSPI)
	list(APPEND incs qspi)
	list(APPEND srcs
		qspi/qspi_driver.c
		qspi/qspi_statis.c
	)

	if (CONFIG_QSPI_MST_PSRAM)
		list(APPEND srcs qspi/qspi_psram.c)
	endif()

	# NOR/NAND flash protocol engines, BBM and device tables moved to
	# components/bk_external_flash/qspi_flash. Only the QSPI controller,
	# statistics and QSPI-PSRAM stay in middleware.

	if (CONFIG_CLI AND CONFIG_QSPI_TEST)
		list(APPEND srcs qspi/qspi_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
