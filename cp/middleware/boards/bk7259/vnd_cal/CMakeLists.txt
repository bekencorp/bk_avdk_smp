if (CONFIG_VND_CAL)
if (NOT CONFIG_OVERRIDE_VND_CAL)
set(incs
	.
	include
	)
set(srcs
	vnd_cal.c
	)
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_libs)
