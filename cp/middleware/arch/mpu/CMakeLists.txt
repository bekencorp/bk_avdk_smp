armino_build_get_property(target ARMINO_SOC)

set(incs)
set(srcs)

list(APPEND incs
	./
)

list(APPEND srcs
	mpu.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

