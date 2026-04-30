set(incs
    .
    include
)

set(srcs
    ap_main.c
    src/h264d_gpu_display_demo.c
    src/h264d_gpu_display_display.c
    src/h264d_gpu_display_gpu.c
    src/h264_decode_stream_1280x720.c
)

set(priv_req
    media_service
    bk_cli
    bk_camera
    bk_decoder
    bk_gpu
    bk_display
    avdk_utils
    bk_peripheral
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
