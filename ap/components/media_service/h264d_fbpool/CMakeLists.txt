set(incs include)
set(srcs)
set(priv_req bk_common)

list(APPEND srcs
	src/h264d_fbpool.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
