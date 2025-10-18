set(incs include  . ./rgb_lcd/include)
set(srcs ap_main.c
    rgb_lcd/src/rgb_lcd_cli.c
    rgb_lcd/src/rgb_lcd_api_test.c
    rgb_lcd/src/rgb_lcd_display_test.c
)

set(priv_req  media_service multimedia bk_display)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)