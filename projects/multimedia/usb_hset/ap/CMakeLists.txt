set(incs .)

set(srcs ap_main.c usb_hset_cdc.c)

set(priv_req bk_usb)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
