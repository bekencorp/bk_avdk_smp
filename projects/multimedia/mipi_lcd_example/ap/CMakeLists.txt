set(incs . include )

set(srcs 
	ap_main.c
	src/lcd_example_mipi.c
)

set(priv_req  media_service bk_network_provisioning  bk_cli avdk_utils bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
