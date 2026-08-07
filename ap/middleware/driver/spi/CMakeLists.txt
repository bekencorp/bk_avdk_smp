if (CONFIG_SPI)
	list(APPEND incs spi)
	list(APPEND srcs spi/spi_driver.c spi/spi_statis.c)

	if (CONFIG_SPI_MST_FLASH)
		list(APPEND srcs spi/spi_flash.c)
	endif()

	if (CONFIG_CLI AND CONFIG_SPI_TEST)
		list(APPEND srcs spi/spi_test.c)
	endif()

	if (CONFIG_CLI AND CONFIG_SPI_API_TEST)
		list(APPEND srcs spi/cli_spi_api.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()