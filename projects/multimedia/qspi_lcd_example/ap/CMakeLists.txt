set(incs .)
set(srcs ap_main.c)

set(priv_req media_service bk_display bk_frame_buffer bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
