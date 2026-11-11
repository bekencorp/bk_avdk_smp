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

if(CONFIG_AUD_OPUS_BENCH_CLI)
list(APPEND srcs opus_bench_cli.c)
endif()

if(CONFIG_AUD_CODEC_LOOPBACK_CLI)
list(APPEND srcs codec_loopback_test/cli_codec_loopback.c)
list(APPEND incs codec_loopback_test)
list(APPEND priv_req bk_voice_service)
endif()

if(CONFIG_MEDIA_SERVICE)
list(APPEND priv_req media_service)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
