set(incs . include rgb_lcd/include)


set(srcs 
	ap_main.c
	src/lcd_example_rgb.c
)

set(priv_req  media_service bk_network_provisioning avdk_utils bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
