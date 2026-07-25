set(incs . include src)
set(srcs
    ap_main.c
    src/motion_detect_demo.c
)

set(priv_req
    media_service
    bk_cli
    bk_camera
    bk_display
    bk_gpu
    bk_frame_buffer
    avdk_utils
    multimedia_device_service
    bk_peripheral
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


