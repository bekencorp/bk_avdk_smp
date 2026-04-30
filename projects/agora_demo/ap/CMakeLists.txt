set(incs include .)

list(APPEND incs
    com_proj/bk_app_event
    com_proj/bk_factory_config
    com_proj/bk_key_app
    com_proj/bk_smart_config
    com_proj/bk_smart_config/include
    com_proj/network_transfer
    com_proj/network_transfer/common
    com_proj/audio_engine
    com_proj/video_engine
)

if (CONFIG_BK_VIDEO_ENGINE AND (CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA OR CONFIG_USB_CAMERA))
    list(APPEND incs
        ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/include
        ${ARMINO_PATH}/components/avdk_driver/include
        ${ARMINO_PATH}/components/bk_peripheral/include
    )
endif()

if (CONFIG_AGORA_IOT_SDK)
    list(APPEND incs
        com_proj/network_transfer/agora_rtc
        com_proj/network_transfer/agora_rtc/agent_utils
    )
endif()

set(srcs ap_main.c)


list(APPEND srcs
    com_proj/bk_app_event/app_event.c
)


if (CONFIG_BK_FACTORY_CONFIG)
    list(APPEND srcs
        com_proj/bk_factory_config/bk_factory_config.c
    )
endif()

list(APPEND srcs
    com_proj/bk_key_app/key_app_service.c
)


if (CONFIG_BK_SMART_CONFIG)
    list(APPEND srcs
        com_proj/bk_smart_config/src/core/bk_smart_config_core.c
    )
endif()


if (CONFIG_BK_NETWORK_TRANSFER)
    list(APPEND srcs
        com_proj/network_transfer/network_transfer.c
    )
    

    if (CONFIG_AGORA_IOT_SDK)
        list(APPEND srcs
            com_proj/network_transfer/agora_rtc/bk_agora_api.c
            com_proj/network_transfer/agora_rtc/agora_rtc_engine.c
            com_proj/network_transfer/agora_rtc/agora_agent_engine.c
            com_proj/network_transfer/agora_rtc/agent_utils/AgoraWebClientUtils.c
        )
    endif()
endif()

if (CONFIG_BK_AUDIO_ENGINE)
list(APPEND srcs
	com_proj/audio_engine/audio_engine.c
	com_proj/audio_engine/audio_engine_cli.c
)
endif()

if (CONFIG_AE_SUPPORT_PROMPT_TONE)
list(APPEND srcs
	com_proj/audio_engine/audio_engine_prompt_tone.c
)
endif()

if (CONFIG_BK_VIDEO_ENGINE)
list(APPEND srcs
    com_proj/video_engine/video_engine.c
)

    if (CONFIG_USB_CAMERA)
        list(APPEND srcs
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/app_uvc.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/devices_mgmt.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/doorbell_img_manager.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/encode_frame_que.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/app_jpeg_decode.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/app_h264_encode.c
        )
    endif()

    if (CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA)
        list(APPEND srcs
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/app_camera.c
            ${CMAKE_SOURCE_DIR}/../common_components/doorbell_device_service/src/app_codec.c
        )
    endif()
endif()


set(priv_req bk_network_provisioning media_service easy_flash bk_cli psa_mbedtls
    key bk_common bk_init lwip_intf_v2_1 json agora-iot-sdk bk_vfs ota bk_batt_monitor bk_uid bk_pan
    bk_voice_service bk_player_service bk_asr_service)

if (CONFIG_USB_CAMERA)
    list(APPEND priv_req bk_uvc bk_usb bk_frame_buffer bk_peripheral bk_decoder bk_encoder bk_h264e avdk_driver)
endif()

if (CONFIG_BK_VIDEO_ENGINE AND CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA)
    list(APPEND priv_req avdk_utils avdk_driver bk_peripheral bk_h264e bk_camera bk_frame_buffer)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")