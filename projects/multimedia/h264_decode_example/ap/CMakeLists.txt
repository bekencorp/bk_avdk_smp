set(incs . include ./h264_decode/include ./h264_decode/common)
set(srcs
    ap_main.c
    h264_decode/src/h264_decode_cli.c
    h264_decode/common/h264_decode_h264_parser.c
    h264_decode/common/h264_decode_stream_1280x720.c
    h264_decode/common/h264_decode_stream_1280x720_ibbp.c
)

set(priv_req media_service h264d_fbpool bk_cli avdk_utils psram_stress_service)

if (CONFIG_BK_DECODER)
list(APPEND srcs
    h264_decode/src/vcdec_h264_test_common.c
    h264_decode/src/vcdec_h264_frame_test.c
    h264_decode/src/vcdec_h264_flexa_test.c
    h264_decode/src/vcdec_h264_frame_zerocopy_test.c
    h264_decode/src/vcdec_h264_rgb_test.c
    h264_decode/src/vcdec_h264_boot_demo.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
