set(incs .)

set(srcs ap_main.c)

set(priv_req avdk_utils bk_peripheral)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
