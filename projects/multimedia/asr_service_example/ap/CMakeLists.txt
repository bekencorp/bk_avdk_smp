set(incs include  ./asr_service_test)
set(srcs ap_main.c)

list(APPEND srcs
    asr_service_test/cli_asr_service.c
)
set(priv_req media_service avdk_utils bk_asr_service)
if (CONFIG_VOICE_SERVICE)
    list(APPEND priv_req bk_voice_service)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
