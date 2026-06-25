set(incs . video_player_cli/include)
set(srcs ap_main.c 
        video_player_cli/src/video_player_cli.c
        video_player_cli/src/video_player_common.c
        video_player_cli/src/video_play_callbacks.c
        video_player_cli/src/video_play_engine_cli.c
        video_player_cli/src/video_play_playlist_cli.c
        video_player_cli/src/audio_player_device.c
        video_player_cli/src/video_recorder_cli.c
        video_player_cli/src/audio_recorder_device.c
        )

set(priv_req media_service bk_vfs multimedia_device_service avdk_utils bk_peripheral bk_video_recorder bk_encoder)

if (CONFIG_AVI_PLAYER)
    list(APPEND priv_req bk_video_player)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
