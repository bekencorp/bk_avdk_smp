set(incs .)

set(srcs ap_main.c fisheye_cli.c)

set(priv_req media_service avdk_utils bk_peripheral fisheye)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
