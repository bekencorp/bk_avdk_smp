set(incs .)
set(srcs cp_main.c)

if (CONFIG_VND_CAL AND CONFIG_OVERRIDE_VND_CAL)
	list(APPEND srcs
		${CMAKE_CURRENT_LIST_DIR}/../../pm/cp/vnd_cal.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
