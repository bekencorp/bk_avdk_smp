set(incs .)
set(srcs ap_main.c)

#if (CONFIG_AUDIO)
list(APPEND srcs
    aud_test.c
    aud_test_data.c
)
#endif()

if(CONFIG_AUD_RSP_BENCH_CLI)
list(APPEND srcs rsp_bench_cli.c)
endif()

if(CONFIG_MEDIA_SERVICE)
list(APPEND priv_req media_service)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
