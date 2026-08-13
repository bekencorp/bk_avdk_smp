
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

set(priv_requires bk_common littlefs lwip_intf_v2_1 bk_vfs)
if (CONFIG_ALI_MQTT)
	list(APPEND priv_requires http)
endif()

#TODO fix main
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_wifi bk_cli
	PRIV_REQUIRES ${priv_requires}
	)
