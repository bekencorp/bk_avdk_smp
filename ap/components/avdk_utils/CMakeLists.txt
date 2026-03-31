set(incs include)
set(srcs
	src/avdk_monitor.c
	src/avdk_utils.c
	src/sw_i2c.c
	src/avdk_pixel_types.c
)

set(priv_req )


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
