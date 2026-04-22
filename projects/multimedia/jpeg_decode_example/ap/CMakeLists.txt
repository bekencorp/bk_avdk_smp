set(incs . include ./jpeg_decode/include)
set(srcs
    ap_main.c
    jpeg_decode/src/jpeg_decode_cli.c
    jpeg_decode_stress/src/jpeg_decode_stress.c
)

set(priv_req media_service bk_cli avdk_utils bk_h264d psram_stress_service)

if (CONFIG_BK_DECODER)
list(APPEND srcs
    jpeg_decode/src/vcdec_jpeg_test.c
    jpeg_decode/common/jpeg_176_144.c
    jpeg_decode_stress/src/jpeg_data_1920_1080.c
)
list(APPEND incs ./jpeg_decode/common ./jpeg_decode_stress/include)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
