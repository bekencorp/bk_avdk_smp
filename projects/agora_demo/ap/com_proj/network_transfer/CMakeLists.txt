set(incs . common)
set(srcs "")

if (CONFIG_BK_NETWORK_TRANSFER)

list(APPEND srcs
    network_transfer.c
)

if (CONFIG_AGORA_IOT_SDK)
list(APPEND incs
            agora_rtc
            agora_rtc/agent_utils
)

list(APPEND srcs
	agora_rtc/bk_agora_api.c
	agora_rtc/agora_rtc_engine.c
	agora_rtc/agora_agent_engine.c
    agora_rtc/agent_utils/AgoraWebClientUtils.c
)
endif()

if (CONFIG_BK_WSS_TRANS)
list(APPEND incs
            bk_wss
)

list(APPEND srcs
	bk_wss/bk_wss.c
	bk_wss/bk_wss_main.c
	bk_wss/bk_wss_debug.c
)
endif()

if (CONFIG_VOLC_RTC_EN)
list(APPEND incs
            volc_rtc
            volc_rtc/agent_utils
)

list(APPEND srcs
    volc_rtc/bk_volc_api.c
    volc_rtc/volc_rtc_engine.c
    volc_rtc/volc_agent_engine.c
    volc_rtc/agent_utils/RtcWebClientUtils.c
)
endif()

if (CONFIG_LINGXIN_AI_EN)
list(APPEND srcs
    lingxin_wss/lingxin_wss_main.c
)
endif()

if (CONFIG_ALIYUN_MMI_EN)
list(APPEND srcs
    aliyun_wss/ali_wss_main.c
    aliyun_wss/ali_wss.c
)
endif()

endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common bk_uid bk_init lwip_intf_v2_1 psa_mbedtls json VolcEngineRTCLite agora-iot-sdk bk_smart_config bk_app_event bk_factory_config bk_vfs
)