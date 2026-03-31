
armino_build_get_property(target ARMINO_SOC)

set(incs)
set(srcs)

if (CONFIG_CMSIS)
	list(APPEND incs
		CMSIS_5/CMSIS/Core/Include
	)

if (CONFIG_SOC_BK7236XX)
if (CONFIG_SOC_SMP)
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu0.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu1.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/startup_cpu2.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu0.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu1.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/system_cpu2.c
		CMSIS_5/Device/Beken/bk7236xx/Source/smp/smp.c
	)
else()
	list(APPEND srcs
		CMSIS_5/Device/Beken/bk7236xx/Source/startup_bk7236.c
		CMSIS_5/Device/Beken/bk7236xx/Source/system_bk7236xx.c
	)
endif()

	list(APPEND incs
		CMSIS_5/Device/Beken/bk7236xx/Include
		CMSIS_5/Device/Beken/bk7236xx/Include/Template
		CMSIS_5/Device/Beken/armstar
	)
endif()

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
