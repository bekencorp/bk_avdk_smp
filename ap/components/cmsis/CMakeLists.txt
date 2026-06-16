set(incs)
set(srcs)

if (CONFIG_CMSIS)
	list(APPEND incs
		CMSIS_5/CMSIS/Core/Include
	)

endif()

if (CONFIG_SOC_SMP)
armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common coredump bk_rtos)
else()
armino_component_register(SRCS "${srcs}"
			INCLUDE_DIRS "${incs}"
			PRIV_REQUIRES bk_common coredump)
endif()

add_definitions(-D__ARM_ARCH_8M_MAIN__=1)
