
set(incs .  ./assets ./draw_osd/include)

set(srcs ap_main.c
    draw_osd/src/draw_osd_cli.c
    )

list(APPEND srcs
    assets/bk_img.c
    assets/bk_font.c
    assets/blend_dsc.c
)


set(priv_req  media_service multimedia bk_cli bk_display bk_draw_osd)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)