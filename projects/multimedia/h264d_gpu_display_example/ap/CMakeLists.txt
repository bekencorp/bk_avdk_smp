set(incs
    .
    include
)

set(srcs
    ap_main.c
    src/h264d_gpu_display_demo.c
    src/h264d_gpu_display_h264_parser.c
    src/h264d_gpu_display_gpu.c
)

set(priv_req
    media_service
    bk_cli
    bk_camera
    bk_decoder
    bk_gpu
    avdk_utils
    bk_peripheral
)

if (CONFIG_H264D_GPU_DISPLAY_ENABLE_MIPI_DISPLAY)
list(APPEND srcs
    src/h264d_gpu_display_dpu.c
)
list(APPEND priv_req
    bk_display
)
endif()

if (CONFIG_H264D_GPU_DISPLAY_ENABLE_ISP_PIP)
list(APPEND srcs
    src/h264d_gpu_display_isp.c
)
# bk_isp_camera_* / bk_camera_sensor_* / bk_camera_bus_* symbols are exposed
# transitively via media_service + bk_camera (already in priv_req above).
endif()

if (CONFIG_H264D_GPU_DISPLAY_TEST_STREAM_1280X720)
list(APPEND srcs
    src/h264_decode_stream_1280x720.c
)
else()
list(APPEND srcs
    src/h264_decode_stream_720x1280.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
