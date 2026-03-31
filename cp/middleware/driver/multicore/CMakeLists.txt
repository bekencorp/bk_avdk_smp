if (CONFIG_SUPPORT_MULTICORE)
	list(APPEND incs multicore)
	list(APPEND srcs multicore/multicore_driver.c)
endif()

if (CONFIG_CLI AND CONFIG_MULTICORE_TEST)
	list(APPEND srcs multicore/multicore_test.c)
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)
