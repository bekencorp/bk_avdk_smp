set(incs .  ./assets ./draw_osd/include)

set(srcs ap_main.c
    draw_osd/src/draw_osd_cli.c
    draw_osd/src/draw_osd_complex_test.c
    draw_osd/src/osd_disp_compat.c
    )

list(APPEND srcs
    assets/bk_img.c
    assets/bk_font.c
    assets/blend_dsc.c
)

set(priv_req media_service bk_cli avdk_utils bk_peripheral bk_draw_osd)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
