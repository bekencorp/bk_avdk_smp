set(incs include  . ./jpeg_decode/include)
set(srcs ap_main.c
    jpeg_decode/src/jpeg_decode_cli.c
    jpeg_decode/src/jpeg_decode_test_common.c
    jpeg_decode/src/jpeg_decode_regular_test.c
    jpeg_decode/src/jpeg_decode_error_test.c
    jpeg_decode/data/jpeg_data_420_864_479.c
    jpeg_decode/data/jpeg_data_420_864_480.c
    jpeg_decode/data/jpeg_data_420_865_480.c
    jpeg_decode/data/jpeg_data_422_864_479.c
    jpeg_decode/data/jpeg_data_422_864_480.c
    jpeg_decode/data/jpeg_data_422_865_480.c
    )

set(priv_req  media_service multimedia)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)