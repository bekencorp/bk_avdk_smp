set(incs include  ./asr_service_test)
set(srcs ap_main.c)

list(APPEND srcs
    asr_service_test/cli_asr_service.c
)
set(priv_req media_service avdk_utils bk_asr_service)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
