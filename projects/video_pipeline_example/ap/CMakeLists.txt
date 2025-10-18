set(incs include  . ./video_pipeline/include)
set(srcs ap_main.c
    video_pipeline/src/video_pipeline_cli.c
    video_pipeline/src/video_pipeline_regular_test.c
    video_pipeline/src/video_pipeline_error_test.c
    video_pipeline/src/video_pipeline_test_common.c
    video_pipeline/data/jpeg_data_420_864_479.c
    video_pipeline/data/jpeg_data_420_864_480.c
    video_pipeline/data/jpeg_data_420_865_480.c
    video_pipeline/data/jpeg_data_422_864_479.c
    video_pipeline/data/jpeg_data_422_864_480.c
    video_pipeline/data/jpeg_data_422_865_480.c
    )

set(priv_req  media_service multimedia)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)