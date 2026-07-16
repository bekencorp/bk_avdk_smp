set(incs "")
set(srcs "")

if (CONFIG_FATFS)
	list(APPEND incs .)
	list(APPEND srcs
		"ffunicode.c"
		"ffsystem.c"
		"disk_io.c"
		"ff.c"
		"test_fatfs.c"
		"driver_udisk.c"
		"cc936.c"
		"ccsbcs.c"
		)
endif()

set(priv_req bk_common bk_usb)
if (CONFIG_QSPI_NAND_FLASH AND (CONFIG_FATFS_QSPI_0_FLASH OR CONFIG_FATFS_QSPI_1_FLASH))
	list(APPEND priv_req dhara)
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES "${priv_req}"
		)
