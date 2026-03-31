set(incs .)
set(srcs ap_main.c frame_buffer_test.c)

set(priv_req bk_network_provisioning bk_frame_buffer avdk_utils)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
