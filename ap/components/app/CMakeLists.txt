
#TODO should move all public include dir to modules

set(incs .)
set(srcs )

if (CONFIG_FTP_SERVER)
list(APPEND incs ../app/ftp)
endif()

if (CONFIG_LITTLEFS)
	list(APPEND srcs
		littlefs_demo/lfs_example_onchip_flash.c
	)
endif()

if (CONFIG_HOST_NONE)
	list(APPEND incs)
endif()

if (CONFIG_NTP_SYNC_RTC)
	list(APPEND srcs
	time/time.c
	time/ntp.c
	time/app_time_intf.c
	)
endif()

if (CONFIG_FTP_SERVER)
	list(APPEND srcs
		ftp/ftpd.c
	)
endif()

if (CONFIG_ALI_MQTT)
	list(APPEND incs
		ali_mqtt/mqtt
		ali_mqtt/mqtt/utils
		ali_mqtt/mqtt/MQTTPacket
	)
	list(APPEND srcs
		ali_mqtt/mqtt/mqtt_client.c
		ali_mqtt/mqtt/mqtt_instance.c
		ali_mqtt/mqtt/mqtt_adapter.c
		ali_mqtt/mqtt/utils/utils_list.c
		ali_mqtt/mqtt/utils/device.c
		ali_mqtt/mqtt/utils/report.c
		ali_mqtt/mqtt/utils/ca.c
		ali_mqtt/mqtt/utils/guider.c
		ali_mqtt/mqtt/mqtt_test.c
		ali_mqtt/mqtt/MQTTPacket/MQTTConnectClient.c
		ali_mqtt/mqtt/MQTTPacket/MQTTDeserializePublish.c
		ali_mqtt/mqtt/MQTTPacket/MQTTPacket.c
		ali_mqtt/mqtt/MQTTPacket/MQTTSerializePublish.c
		ali_mqtt/mqtt/MQTTPacket/MQTTSubscribeClient.c
		ali_mqtt/mqtt/MQTTPacket/MQTTUnsubscribeClient.c
	)
	set(ali_mqtt_c_compile_options
		"-DMQTT_COMM_ENABLED"
		"-DMQTT_DIRECT"
		"-DIOTX_WITHOUT_TLS"
		"-DOTA_SIGNAL_CHANNEL=1"
	)
	armino_build_set_property(C_COMPILE_OPTIONS "${ali_mqtt_c_compile_options}" APPEND)
endif()

if (CONFIG_PAHO_MQTT)
	list(APPEND incs
		paho-mqtt/client
		paho-mqtt/client/src
		paho-mqtt/mqtt_ui
		paho-mqtt/mqtt_ui/tcp_mqtt
		paho-mqtt/mqtt_ui/ssl_mqtt
	)
	list(APPEND srcs
		paho-mqtt/client/paho_mqtt_udp.c
		paho-mqtt/mqtt_ui/mqtt_client_core.c
		paho-mqtt/mqtt_ui/mqtt_client_com_port.c
		paho-mqtt/mqtt_ui/tcp_mqtt/tcp_mqtt_client_port.c
		paho-mqtt/paho_mqtt_test.c
	)
	# MQTTClient.c exports mqtt_subscribe/mqtt_publish, which ali_mqtt also defines.
	if (NOT CONFIG_ALI_MQTT)
		list(APPEND srcs
			paho-mqtt/client/src/MQTTClient.c
			paho-mqtt/client/src/MQTTFreeRTOS.c
		)
	endif()
	# MQTTPacket sources overlap with ali_mqtt; link one copy when both enabled.
	if (CONFIG_ALI_MQTT)
		list(APPEND incs ali_mqtt/mqtt/MQTTPacket)
	else()
		list(APPEND incs paho-mqtt/packet/src)
		list(APPEND srcs
			paho-mqtt/packet/src/MQTTConnectClient.c
			paho-mqtt/packet/src/MQTTDeserializePublish.c
			paho-mqtt/packet/src/MQTTPacket.c
			paho-mqtt/packet/src/MQTTSerializePublish.c
			paho-mqtt/packet/src/MQTTSubscribeClient.c
			paho-mqtt/packet/src/MQTTUnsubscribeClient.c
			paho-mqtt/packet/src/MQTTFormat.c
		)
	endif()
	set(paho_mqtt_c_compile_options
		"-DMQTT_COMM_ENABLED"
		"-DMQTT_DIRECT"
		"-DIOTX_WITHOUT_TLS"
		"-DOTA_SIGNAL_CHANNEL=1"
		"-DMAX_PACKET_ID=65535"
		"-DPAHOMQTT_PIPE_MODE"
	)
	if (CONFIG_PAHO_MQTT_TLS)
		list(APPEND srcs
			paho-mqtt/mqtt_ui/ssl_mqtt/ssl_mqtt_client_port.c
			paho-mqtt/mqtt_ui/ssl_mqtt/sl_tls.c
		)
		list(APPEND paho_mqtt_c_compile_options "-DMQTT_USING_TLS")
	endif()
	armino_build_set_property(C_COMPILE_OPTIONS "${paho_mqtt_c_compile_options}" APPEND)
endif()

set(priv_requires bk_common littlefs lwip_intf_v2_1 bk_vfs)
if (CONFIG_ALI_MQTT)
	list(APPEND priv_requires http)
endif()
if (CONFIG_PAHO_MQTT)
	list(APPEND priv_requires http)
endif()
if (CONFIG_PAHO_MQTT_TLS)
	list(APPEND priv_requires psa_mbedtls)
endif()

#TODO fix main
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_wifi bk_cli
	PRIV_REQUIRES ${priv_requires}
	)
