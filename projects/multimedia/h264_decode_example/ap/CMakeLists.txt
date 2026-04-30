set(incs . include ./h264_decode/include ./h264_decode/common ./h264_decode_stress/include)
set(srcs
    ap_main.c
    h264_decode/src/h264_decode_cli.c
    h264_decode/src/h264_decode_flexa_test.c
    h264_decode/src/vcdec_h264_driver_test.c
    h264_decode/common/h264_decode_stream_1280x720.c
    h264_decode/common/h264_decode_stream_256x128.c
    h264_decode_stress/src/h264_decode_stress.c
    h264_decode_stress/src/h264_decode_stress_stream.c
)

set(priv_req media_service bk_cli avdk_utils bk_h264d psram_stress_service)

if (CONFIG_BK_DECODER)
list(APPEND srcs
    h264_decode/src/vcdec_h264_test.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
