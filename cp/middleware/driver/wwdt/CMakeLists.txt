if (CONFIG_SUPPORT_WWDT)
	list(APPEND incs wwdt)
	list(APPEND srcs wwdt/wwdt_driver.c)

	if (CONFIG_CLI AND CONFIG_WWDT_TEST)
		list(APPEND srcs wwdt/wwdt_test.c)
	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)