set(incs . include ./pp/include)
set(srcs
    ap_main.c
    pp/src/pp_cli.c
)

set(priv_req media_service bk_cli avdk_utils psram_stress_service bk_peripheral)

if (CONFIG_BK_DECODER)
set(h264_decode_common_dir
    ${CMAKE_CURRENT_LIST_DIR}/../../h264_decode_example/ap/h264_decode/common)
list(APPEND srcs
    pp/src/pp_test.c
    ${h264_decode_common_dir}/h264_decode_stream_1280x720.c
    ${h264_decode_common_dir}/h264_decode_h264_parser.c
)
list(APPEND incs ${h264_decode_common_dir})
endif()

if (CONFIG_PP_EXAMPLE_ENABLE_MIPI_DISPLAY)
list(APPEND srcs
    pp/src/pp_dpu.c
    pp/src/pp_gpu_blit.c
)
list(APPEND priv_req
    bk_display
    bk_gpu
    bk_peripheral
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
