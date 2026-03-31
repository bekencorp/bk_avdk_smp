if (CONFIG_SCR)
	list(APPEND incs scr)
	list(APPEND srcs scr/scr_driver_v1_26.c)

	if (CONFIG_CLI AND CONFIG_SCR_TEST)
		list(APPEND srcs scr/scr_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
