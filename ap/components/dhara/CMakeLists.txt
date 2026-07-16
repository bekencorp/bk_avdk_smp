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

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common driver
		)
