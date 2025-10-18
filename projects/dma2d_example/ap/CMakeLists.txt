set(incs include  .  src/blend_assets  ./dma2d/include)

set(srcs ap_main.c
    dma2d/src/dma2d_cli.c
    dma2d/src/dma2d_fill_test.c
    dma2d/src/dma2d_memcpy_test.c
    dma2d/src/dma2d_pfc_test.c
    dma2d/src/dma2d_blend_test.c
    )

list(APPEND srcs
    dma2d/src/blend_assets/bk_img.c
    dma2d/src/blend_assets/blend_dsc.c
)

set(priv_req  media_service multimedia bk_dma2d bk_draw_osd)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
