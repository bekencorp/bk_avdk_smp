
set(incs 
    ./bk_multimedia
    ./kvs_webrtc
)

set(srcs
	ap_main.c
	kvs_webrtc/kvs_webrtc_master.c
	kvs_webrtc/kvs_webrtc_cli.c
	kvs_webrtc/kvs_common.c
	kvs_webrtc/newlib_syscalls.c
	bk_multimedia/bk_multimedia_test_cli.c
	bk_multimedia/bk_multimedia.c
)

if (CONFIG_AUD_PARAM_CTRL)
	list(APPEND incs ./audio_param)
	list(APPEND srcs
		audio_param/audio_para.c
	)
endif()

set(priv_req multimedia_device_service avdk_utils media_service smart_lock bk_peripheral bk_network_transfer kvs_aws libwebsockets usrsctp libsrtp2 bk_cli bk_vfs bk_rtos psa_mbedtls lwip_intf_v2_1)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")