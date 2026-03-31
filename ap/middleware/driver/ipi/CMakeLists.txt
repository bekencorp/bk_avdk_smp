if (CONFIG_IPI)
	list(APPEND incs ipi)
	list(APPEND srcs ipi/ipi_driver.c)

	if (CONFIG_CLI AND CONFIG_IPI_TEST)
		list(APPEND srcs ipi/ipi_test.c)
	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)

