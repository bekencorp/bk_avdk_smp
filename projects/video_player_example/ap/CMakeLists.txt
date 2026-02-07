set(incs . video_player_cli/include)
set(srcs ap_main.c 
        video_player_cli/src/video_player_cli.c
        video_player_cli/src/video_player_common.c
        video_player_cli/src/video_recorder_cli.c
        video_player_cli/src/module_test_cli.c
        video_player_cli/src/video_play_callbacks.c
        video_player_cli/src/video_play_engine_cli.c
        video_player_cli/src/video_play_playlist_cli.c
        video_player_cli/src/audio_player_device.c
        video_player_cli/src/audio_recorder_device.c
        )

set(GLOBAL_FUNCTION_SYMBOLS "")

set(priv_req  media_service multimedia bk_camera bk_cli bk_display bk_voice_service bk_video_recorder)

# bk_video_player is built as an independent component. Add it explicitly so the example can
# compile even if other dependency chains change.
if (CONFIG_AVI_PLAYER)
    list(APPEND priv_req bk_video_player)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

foreach(symbol ${GLOBAL_FUNCTION_SYMBOLS})
	target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ${symbol}")
endforeach()

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)