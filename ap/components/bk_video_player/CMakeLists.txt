set(incs include)
set(srcs "")

if (CONFIG_AVI_PLAYER)
    list(APPEND srcs
        src/avi_player.c
        src/avi_player_jpeg_decode.c
    )
endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common include multimedia avdk_utils lvgl
)

