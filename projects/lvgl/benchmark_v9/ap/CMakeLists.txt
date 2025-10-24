set(incs .
    )

set(srcs
    ap_main.c
    )

if (CONFIG_LV_USE_DEMO_BENCHMARK)
list(APPEND srcs
    assets/img_benchmark_avatar.c
    assets/img_benchmark_lvgl_logo_argb.c
    assets/img_benchmark_lvgl_logo_rgb.c
    # assets/lv_font_benchmark_montserrat_12_aligned.c
    # assets/lv_font_benchmark_montserrat_14_aligned.c
    # assets/lv_font_benchmark_montserrat_16_aligned.c
    # assets/lv_font_benchmark_montserrat_18_aligned.c
    # assets/lv_font_benchmark_montserrat_20_aligned.c
    # assets/lv_font_benchmark_montserrat_24_aligned.c
    # assets/lv_font_benchmark_montserrat_26_aligned.c
    assets/img_clothes.c
    assets/img_demo_widgets_avatar.c
    assets/img_demo_widgets_needle.c
    assets/img_lvgl_logo.c
    lv_demo_benchmark.c
    lv_demo_widgets.c
)
endif()

set(priv_req  media_service multimedia)

if ("${ARMINO_SOC}" STREQUAL "bk7258_ap")
list(APPEND priv_req lvgl bk_display bk_cli media_service)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
