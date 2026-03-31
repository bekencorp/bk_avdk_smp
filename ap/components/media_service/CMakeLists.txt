set(incs include)
set(srcs)

if(CONFIG_MEDIA_SERVICE)

list(APPEND srcs src/media_service.c
	src/bk_flexa_mjpeg_bond.c
	src/bk_flexa_isp_bond.c
)

list(APPEND incs bk_private)
list(APPEND srcs
	osi_wrapper/video_osi_wrapper.c
	osi_wrapper/audio_osi_wrapper.c
	osi_wrapper/nano_osi_wrapper.c
	osi_wrapper/isp_socket_wrapper.c
	osi_wrapper/isp_i2c_wrapper.c
)

endif() #CONFIG_MEDIA_SERVICE

set(priv_req bk_common bk_vfs bk_display bk_rtos lwip_intf_v2_1 bk_h264d bk_h264e bk_encoder bk_decoder bk_gpu avdk_driver)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES)