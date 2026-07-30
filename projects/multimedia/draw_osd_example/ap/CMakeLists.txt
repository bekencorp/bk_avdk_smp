set(incs .  ./assets ./draw_osd/include ./uvc/include ./mipi/include)

set(srcs ap_main.c
    draw_osd/src/draw_osd_cli.c
    )

list(APPEND srcs
    assets/bk_img.c
    assets/bk_font.c
    assets/blend_dsc.c
)

set(priv_req media_service bk_cli avdk_utils bk_peripheral bk_draw_osd)

# 共享显示层：hx8399c LCD + DPU（MIPI/UVC 两条通路都复用它开屏，放在 draw_osd/ 与 CLI 同级）
list(APPEND srcs
    draw_osd/src/display.c
)

# UVC 通路：UVC 主机 -> MJPEG 解码 -> GPU 叠加 OSD -> MIPI 显示
# defconfig 固定开了 CONFIG_USB_CAMERA / CONFIG_BK_DECODER，这里直接编入
# uvc_pipeline.c 合并了原 encode_frame_que/decode_test/uvc_test/pipeline_test 四个文件
list(APPEND srcs
    uvc/src/uvc_pipeline.c
    uvc/src/osd_uvc.c
)
list(APPEND priv_req bk_uvc bk_gpu bk_decoder)

# 本地 MIPI 摄像头通路：GC2053 CSI -> ISP MP NV12 -> GPU -> MIPI 显示
# 复用共享的 draw_osd/src/display.c 开屏；相机/ISP 走 bk_camera，ISP->GPU bond 在 media_service
# mipi_pipeline.c 合并了原 mipi_camera.c（相机段），对标 uvc_pipeline.c
list(APPEND srcs
    mipi/src/mipi_pipeline.c
    mipi/src/osd_mipi.c
)
list(APPEND priv_req bk_camera)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
