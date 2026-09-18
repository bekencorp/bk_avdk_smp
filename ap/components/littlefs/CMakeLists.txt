set(incs "")
set(srcs "")

if(CONFIG_LITTLEFS)
	list(APPEND incs .)
	list(APPEND srcs
		"lfs.c"
		"lfs_util.c"
		"lfs_flashbd.c"
		)
endif()

set(priv_req bk_common)
if (CONFIG_QSPI_NOR_FLASH OR CONFIG_QSPI_NAND_FLASH)
	# QSPI flash block device (lfs_flashbd.c) calls bk_qspi_flash_*, implemented
	# in the bk_external_flash component.
	list(APPEND priv_req bk_external_flash)
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES "${priv_req}"
		)
