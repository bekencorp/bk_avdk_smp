set(incs . include )

set(srcs
	src/ap_main.c
	src/dvp_cli.c
	src/dvp_func_test.c
)

set(priv_req media_service bk_cli avdk_utils)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")