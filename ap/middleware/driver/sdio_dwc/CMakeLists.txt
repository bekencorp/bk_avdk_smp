if (CONFIG_SUPPORT_SDIO_DWC AND CONFIG_SDIO_V3P0)
	list(APPEND incs
		sdio_dwc/v3p0
		sdio_dwc/v3p0/include
	)

	list(APPEND srcs
		sdio_dwc/v3p0/sdio_host_driver.c
		sdio_dwc/v3p0/sd_card_driver.c
	)

	if (CONFIG_SDIO_DWC_TEST)
		list(APPEND srcs
			sdio_dwc/v3p0/sdcard_test.c
			sdio_dwc/v3p0/sdio_dwc_test.c
		)
	endif ()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
