if (CONFIG_SYS_SW_REGS)
	list(APPEND incs sys_sw_regs)
	list(APPEND srcs sys_sw_regs/sys_sw_regs.c)

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()
