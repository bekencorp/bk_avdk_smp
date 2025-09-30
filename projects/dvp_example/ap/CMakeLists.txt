set(incs . dvp_test/include)
set(srcs ap_main.c
    dvp_test/src/dvp_frame_list.c
    dvp_test/src/dvp_func_test.c
    dvp_test/src/dvp_api_test.c
    dvp_test/src/dvp_main.c)

set(priv_req  media_service multimedia bk_camera bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
