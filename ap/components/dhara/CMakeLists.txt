set(incs "")
set(srcs "")

if(CONFIG_DHARA_FTL)
	list(APPEND incs . dhara)
	list(APPEND srcs
		"dhara/error.c"
		"dhara/journal.c"
		"dhara/map.c"
		"dhara_nand_qspi.c"
		"nand_ftl.c"
		)
endif()

set(priv_req bk_common driver)
if (CONFIG_QSPI_NAND_FLASH)
	# nand_ftl.c calls bk_qspi_flash_*, implemented in the bk_external_flash component.
	list(APPEND priv_req bk_external_flash)
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES "${priv_req}"
		)
