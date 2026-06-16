list(APPEND incs ./)

list(APPEND srcs
	common/driver.c
)

if (CONFIG_SUPPORT_INIT_EXPORT)
list(APPEND srcs
	common/peripheral.c
)
endif ()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)
