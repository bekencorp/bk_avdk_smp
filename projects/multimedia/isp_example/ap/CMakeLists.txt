set(incs . include )

set(srcs
	src/ap_main.c
	src/isp_cli.c
	src/isp_api_test.c
	src/isp_func_test.c
	src/isp_mini_code.c
	src/isp_tuning_test.c
	src/isp_dump_test.c
)

set(priv_req media_service bk_cli avdk_utils lwip_intf_v2_1)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")