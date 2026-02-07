set(incs .)

set(srcs
    ap_main.c
    )

if (CONFIG_LV_USE_DEMO_MUSIC)
list(APPEND srcs
    cli_lv_demo_music.c
    lv_demo_music.c
    lv_demo_music_main.c
    lv_demo_music_list.c
    lv_demo_music_list_mgr.c
    assets/img_lv_demo_music_btn_corner_large.c
    assets/img_lv_demo_music_btn_list_pause_large.c
    assets/img_lv_demo_music_btn_list_pause.c
    assets/img_lv_demo_music_btn_list_play_large.c
    assets/img_lv_demo_music_btn_list_play.c
    assets/img_lv_demo_music_btn_loop_large.c
    assets/img_lv_demo_music_btn_loop.c
    assets/img_lv_demo_music_btn_next_large.c
    assets/img_lv_demo_music_btn_next.c
    assets/img_lv_demo_music_btn_pause_large.c
    assets/img_lv_demo_music_btn_pause.c
    assets/img_lv_demo_music_btn_play_large.c
    assets/img_lv_demo_music_btn_play.c
    assets/img_lv_demo_music_btn_prev_large.c
    assets/img_lv_demo_music_btn_prev.c
    assets/img_lv_demo_music_btn_rnd_large.c
    assets/img_lv_demo_music_btn_rnd.c
    assets/img_lv_demo_music_corner_left_large.c
    assets/img_lv_demo_music_corner_left.c
    assets/img_lv_demo_music_corner_right_large.c
    assets/img_lv_demo_music_corner_right.c
    assets/img_lv_demo_music_cover_1_large.c
    assets/img_lv_demo_music_cover_1.c
    assets/img_lv_demo_music_cover_2_large.c
    assets/img_lv_demo_music_cover_2.c
    assets/img_lv_demo_music_cover_3_large.c
    assets/img_lv_demo_music_cover_3.c
    assets/img_lv_demo_music_icon_1_large.c
    assets/img_lv_demo_music_icon_1.c
    assets/img_lv_demo_music_icon_2_large.c
    assets/img_lv_demo_music_icon_2.c
    assets/img_lv_demo_music_icon_3_large.c
    assets/img_lv_demo_music_icon_3.c
    assets/img_lv_demo_music_icon_4_large.c
    assets/img_lv_demo_music_icon_4.c
    assets/img_lv_demo_music_list_border_large.c
    assets/img_lv_demo_music_list_border.c
    assets/img_lv_demo_music_logo.c
    assets/img_lv_demo_music_slider_knob_large.c
    assets/img_lv_demo_music_slider_knob.c
    assets/img_lv_demo_music_wave_bottom_large.c
    assets/img_lv_demo_music_wave_bottom.c
    assets/img_lv_demo_music_wave_top_large.c
    assets/img_lv_demo_music_wave_top.c
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
