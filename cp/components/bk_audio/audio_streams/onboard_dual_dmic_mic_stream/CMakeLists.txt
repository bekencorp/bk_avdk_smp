set(incs .)

set(srcs)

if (CONFIG_ADK_ONBOARD_DUAL_DMIC_MIC_STREAM)

if (CONFIG_ADK_ONBOARD_DUAL_DMIC_MIC_STREAM_V2)
    list(APPEND srcs
            v2.0/onboard_dual_dmic_mic_stream_v2.c
        )
else()
    list(APPEND srcs
            v1.0/onboard_dual_dmic_mic_stream.c
        )
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)