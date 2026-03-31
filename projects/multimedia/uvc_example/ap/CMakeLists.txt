set(incs . include)

set(srcs
	ap_main.c
	src/uvc_test.c
)

set(priv_req bk_uvc)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")