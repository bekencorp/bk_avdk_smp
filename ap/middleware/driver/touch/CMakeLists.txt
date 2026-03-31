if (CONFIG_TOUCH)
	list(APPEND incs touch)

	list(APPEND srcs touch/touch_driver_v1_1.c)

	if (CONFIG_CLI AND CONFIG_TOUCH_TEST)
		list(APPEND srcs touch/touch_test.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
