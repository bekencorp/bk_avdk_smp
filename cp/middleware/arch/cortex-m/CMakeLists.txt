armino_build_get_property(target ARMINO_SOC)

set(incs)
set(srcs)

list(APPEND incs
	./
	./include
)

if(CONFIG_SOC_SMP)
list(APPEND srcs
	startup_smp_core0.c
	startup_smp_core1.c
)
else()
list(APPEND srcs
	startup.c
)
endif()

list(APPEND srcs
	system.c
	prep_c.c
	dwt.c
	arch.c
	cache.c
	l2_cache.c
	#fault_s.S
	#fault.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

