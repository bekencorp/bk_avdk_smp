if (CONFIG_HSPL)
	list(APPEND incs hspl)
	list(APPEND srcs hspl/hspl_driver.c)
	list(APPEND srcs hspl/hspl_res_lock.c)

	if (CONFIG_CLI AND CONFIG_HSPL_TEST)
		list(APPEND srcs hspl/hspl_test.c)
	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)

