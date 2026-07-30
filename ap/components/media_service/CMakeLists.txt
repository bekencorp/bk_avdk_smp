set(incs include)
set(srcs)

if(CONFIG_MEDIA_SERVICE)

list(APPEND srcs src/media_service.c
	src/bk_flexa_isp_h264e_bond.c
	src/bk_flexa_isp_jpege_bond.c
	src/bk_flexa_mjpegd_h264e_bond.c
)

if(CONFIG_VG_LITE_GPU)
list(APPEND srcs
	src/bk_flexa_isp_gpu_bond.c
	src/bk_flexa_mjpegd_gpu_bond.c
	src/bk_flexa_h264d_gpu_bond.c
)
endif()

list(APPEND incs bk_private)
list(APPEND srcs
	osi_wrapper/video_osi_wrapper.c
	osi_wrapper/audio_osi_wrapper.c
	osi_wrapper/nano_osi_wrapper.c
	osi_wrapper/isp_socket_wrapper.c
	osi_wrapper/isp_i2c_wrapper.c
)

endif() #CONFIG_MEDIA_SERVICE

set(priv_req bk_common bk_vfs bk_rtos lwip_intf_v2_1 bk_encoder bk_decoder avdk_driver bk_peripheral)

if(CONFIG_BK_DISPLAY)
list(APPEND priv_req bk_display)
endif()

if(CONFIG_VG_LITE_GPU)
list(APPEND priv_req bk_gpu)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES)

# Discover nested independent components living under this directory
# (e.g. h264d_fbpool/). The build only recurses into a component's subdirs when
# this marker string is present in its CMakeLists.txt.
armino_component_register_call_subdirs()