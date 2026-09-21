set(incs
	.
)
set(srcs
	cp_main.c
	cp_asr_cli.c
	cli_voice_service.c
	cli_player_service.c
)

if (CONFIG_AUD_PARAM_CTRL)
	list(APPEND incs
		./audio_param
	)
	list(APPEND srcs
		audio_param/audio_param_adapter.c
		audio_param/audio_para.c
	)
endif()

if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
	list(APPEND srcs
		vnd_cal.c
	)
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_audio bk_voice_service bk_player_service bk_aud_param_ctrl bk_asr_service)
