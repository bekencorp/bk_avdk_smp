set(incs .)
set(srcs ap_main.c draw_tiger.c)

set(priv_req bk_network_provisioning bk_frame_buffer avdk_utils bk_gpu media_service bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
