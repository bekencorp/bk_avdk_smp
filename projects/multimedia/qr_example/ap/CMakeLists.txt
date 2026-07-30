set(incs . include src)
set(srcs
    ap_main.c
    src/qr_demo.c
    src/qr_wifi.c
)

set(priv_req
    media_service
    bk_cli
    bk_camera
    bk_display
    bk_gpu
    bk_frame_buffer
    bk_event
    bk_wifi_driver
    avdk_utils
    json
    lwip_intf_v2_1
    multimedia_device_service
    bk_peripheral
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


