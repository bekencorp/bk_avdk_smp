set(incs include .)
set(srcs)

if (CONFIG_INTEGRATION_DOORBELL)
	list(APPEND srcs
		src/doorbell_core.c
		src/doorbell_boarding.c
		src/doorbell_network.c
		src/doorbell_network_transfer.c
		src/doorbell_devices.c
		src/doorbell_cmd.c
		src/bk_smart_config.c
	)

if (CONFIG_VOICE_SERVICE)
	list(APPEND srcs
		src/doorbell_audio_device.c
	)
endif()

endif()

set(priv_req media_utils bk_network_transfer avdk_utils lwip_intf_v2_1 bk_wifi_driver cs2p2p bt doorbell_device_service bk_peripheral media_service bk_asr_service)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)
