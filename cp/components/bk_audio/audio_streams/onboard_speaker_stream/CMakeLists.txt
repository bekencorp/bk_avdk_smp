set(incs .)

set(srcs)

if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM)


if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2)
    list(APPEND srcs
        v2.0/onboard_speaker_stream_v2.c
    )
else()
    list(APPEND srcs
        v1.0/onboard_speaker_stream.c
    )

    if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM_TEST)
    list(APPEND srcs
        v1.0/onboard_speaker_stream_test.c
    )
    endif()
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)