set(incs . include display)

set(srcs
	src/ap_main.c
	src/isp_cli.c
	src/isp_api_test.c
	src/isp_func_test.c
	src/isp_mini_code.c
	src/isp_tuning_test.c
	src/isp_dump_test.c
	src/isp_display_pipeline.c
	src/isp_vc_route.c
	display/display.c
)

set(priv_req media_service bk_cli avdk_utils lwip_intf_v2_1 bk_gpu bk_display bk_camera bk_peripheral)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
