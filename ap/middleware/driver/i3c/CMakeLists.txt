if (CONFIG_I3C)
	armino_build_get_property(armino_path ARMINO_PATH)
	armino_build_get_property(target ARMINO_SOC)
	list(APPEND incs i3c)
	list(APPEND incs ${armino_path}/middleware/soc/${target}/hal)
	list(APPEND incs ${armino_path}/middleware/soc/${target}/soc)
	list(APPEND srcs
		i3c/i3c_common.c
		i3c/i3c_master.c
		i3c/i3c_slave.c
	)

	if (CONFIG_I3C_TEST)
		list(APPEND srcs i3c/i3c_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
