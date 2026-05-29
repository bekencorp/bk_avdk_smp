list(APPEND incs wdt)
list(APPEND srcs wdt/task_wdt.c)

if (CONFIG_TASK_WDT)
	if (CONFIG_CLI AND CONFIG_TASK_WDT_TEST)
		list(APPEND srcs wdt/task_wdt_test.c)
	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)