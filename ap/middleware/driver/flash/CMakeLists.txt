if (CONFIG_FLASH)
	list(APPEND incs flash)
	list(APPEND srcs
		flash/flash_partition.c
		flash/flash_bypass.c
		flash/flash_notify.c
		flash/CheckSumUtils.c
	)

	list(APPEND srcs flash/flash_driver.c)

	# flash_test.c hosts both CLI test commands AND ble_callback_deal_handler(),
	# which OTA (ota_base_drv.c) uses in production to coordinate BLE-vs-flash
	# timing. So compile it whenever OTA is enabled, even if FLASH_TEST is off.
	# Unused CLI command symbols are stripped by -Wl,--gc-sections.
	if (CONFIG_OTA_FUNCTION OR (CONFIG_CLI AND CONFIG_FLASH_TEST))
		list(APPEND srcs flash/flash_test.c)
	endif()

	if (CONFIG_CLI AND CONFIG_FLASH_TEST)
		list(APPEND srcs flash/flash_wr_test.c)
	endif()

	if(CONFIG_CLI AND CONFIG_FLASH_API_TEST)
		list(APPEND srcs flash/cli_flash_api.c)
	endif()
	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
