set(incs include)
set(srcs "")

if (CONFIG_AUDIO_PLAY)
	list(APPEND srcs
		src/audio_play.c
		src/spk_service.c
		src/hint_service.c
	)
	if (CONFIG_CLI)
		list(APPEND srcs
			src/audio_play_cli.c
			src/hint_prompt_clips.c
		)
	endif()
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common include avdk_utils bk_audio
)

