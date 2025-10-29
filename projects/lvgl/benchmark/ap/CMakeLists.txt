set(incs .)

set(srcs
    ap_main.c
    )

if (CONFIG_LV_USE_DEMO_BENCHMARK)
list(APPEND srcs
    lv_demo_benchmark.c
    assets/img_benchmark_cogwheel_alpha16.c
    assets/img_benchmark_cogwheel_argb.c
    assets/img_benchmark_cogwheel_chroma_keyed.c
    assets/img_benchmark_cogwheel_indexed16.c
    assets/img_benchmark_cogwheel_rgb.c
    assets/img_benchmark_cogwheel_rgb565a8.c
    assets/lv_font_bechmark_montserrat_12_compr_az.c
    assets/lv_font_bechmark_montserrat_16_compr_az.c
    assets/lv_font_bechmark_montserrat_28_compr_az.c
)
endif()


set(priv_req  media_service multimedia)

if ("${ARMINO_SOC}" STREQUAL "bk7258_ap")
list(APPEND priv_req  lvgl bk_display bk_cli media_service)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
