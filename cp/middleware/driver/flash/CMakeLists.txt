if (CONFIG_FLASH)
	list(APPEND incs flash)
	list(APPEND srcs
		flash/flash_partition.c
		flash/flash_notify.c
		flash/CheckSumUtils.c
	)

	list(APPEND srcs flash/flash_driver.c flash/flash_driver_ext.c)

	if (CONFIG_CLI AND CONFIG_FLASH_TEST)
		list(APPEND srcs flash/flash_test.c flash/flash_wr_test.c)
	endif()

	if(CONFIG_CLI AND CONFIG_FLASH_API_TEST)
		list(APPEND srcs flash/cli_flash_api.c)
	endif()
	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
