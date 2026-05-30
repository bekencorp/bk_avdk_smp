set(incs . include ./jpeg_encode/include ./jpeg_encode/common)

set(srcs ap_main.c
    jpeg_encode/src/jpeg_encode_cli.c
    jpeg_encode/src/bk_jpeg_encode_test.c
    jpeg_encode/common/jpeg_encode_nv12_256x128.c
    )

set(priv_req media_service bk_cli avdk_utils bk_encoder)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
