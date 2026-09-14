set(incs
	.
	./audio_test
	./asr_service_test
	./player_service_test
	./voice_service_test
	../common
)

set(srcs
	ap_main.c
)

if (CONFIG_AUD_PM_FAST_COLD OR CONFIG_AUD_PM_FAST_HOT)
	list(APPEND srcs audio_test/cli_aud_fb_test.c)
endif()

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs asr_service_test/cli_asr_service.c)
endif()

if (CONFIG_PLAYER_SERVICE)
	list(APPEND srcs player_service_test/cli_player_service.c)
endif()

if (CONFIG_VOICE_SERVICE_TEST)
	list(APPEND srcs voice_service_test/cli_voice_service.c)
endif()

set(priv_req media_service avdk_utils)

if (CONFIG_ASR_SERVICE)
	list(APPEND priv_req bk_asr_service)
endif()

if (CONFIG_BEKEN_KWS)
	list(APPEND incs ${CMAKE_CURRENT_LIST_DIR}/../../../../ap/components/bk_asr_service/bk_kws)
endif()

if (CONFIG_VOICE_SERVICE)
	list(APPEND priv_req bk_voice_service)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
