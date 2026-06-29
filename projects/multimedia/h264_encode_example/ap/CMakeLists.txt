set(incs . include ./h264_encode/include ./h264_encode/common)
set(srcs ap_main.c
    h264_encode/src/h264_encode_cli.c
    h264_encode/src/vcenc_h264_test.c
    h264_encode/common/h264_encode_stream_256x128.c
    )

set(priv_req media_service bk_cli avdk_utils psram_stress_service)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
