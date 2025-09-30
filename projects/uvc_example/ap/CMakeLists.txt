set(incs . uvc_test/include)
set(srcs ap_main.c
    uvc_test/src/uvc_main.c
    uvc_test/src/uvc_common.c)

set(priv_req  media_service bk_camera bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
