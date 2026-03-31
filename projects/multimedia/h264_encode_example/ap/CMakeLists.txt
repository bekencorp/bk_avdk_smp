set(incs . include ./h264_encode/include ./h264_stress/include ./mjpeg_encode)
set(srcs ap_main.c
    h264_encode/src/h264_encode_cli.c
    h264_encode/src/h264_encode_test_common.c
    h264_encode/src/h264_encode_regular_test.c
    h264_encode/src/h264_encode_error_test.c
    h264_encode/src/h264e_api_cli.c
    h264_stress/src/h264_encode_stress.c
    mjpeg_encode/mjpeg_encode_stress.c
    )

set(priv_req media_service bk_cli avdk_utils bk_h264e psram_stress_service)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
