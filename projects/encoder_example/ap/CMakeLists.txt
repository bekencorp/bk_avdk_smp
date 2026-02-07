set(incs .)
set(srcs ap_main.c
    yuv_test_buf.c
    jpeg_encoder_test.c
)

set(priv_req  media_service bk_cli bk_encoder)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")