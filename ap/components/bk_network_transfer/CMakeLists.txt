set(incs .
	common/transfer_list
	common/bk_ntwk_pack
	common/bk_video_drop_policy
	common/bk_video_fps_strategy
	common/bk_ntwk_sdp
	bk_network_service/bk_ntwk_socket
	bk_network_service/bk_net_tcp_service
	bk_network_service/bk_net_udp_service
	bk_network_service/bk_net_cs2_service
	bk_network_service/bk_ntwk_kvs_service
)

set(srcs)

if (CONFIG_BK_NTWK_TRANSFER)
list(APPEND srcs
	common/transfer_list/trans_list.c
	common/bk_ntwk_pack/ntwk_pack.c
	common/bk_ntwk_pack/ntwk_fragmentation.c
	common/bk_ntwk_sdp/ntwk_sdp.c
	common/bk_video_drop_policy/video_drop.c
	bk_network_service/bk_ntwk_socket/ntwk_socket.c
	bk_network_service/bk_ntwk_tcp_service/ntwk_tcp_service.c
	bk_network_service/bk_ntwk_tcp_service/bk_tcp_service.c
	bk_network_service/bk_ntwk_udp_service/ntwk_udp_service.c
	bk_network_service/bk_ntwk_udp_service/bk_udp_service.c
	network_transfer_internal.c
	network_transfer.c
	)

if (CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE)
list(APPEND srcs
	common/bk_video_fps_strategy/video_fps.c
)
endif()

if (CONFIG_NTWK_H264_DROP_POLICY)
list(APPEND srcs
	common/bk_video_drop_policy/h264_backpressure_drop.c
)
endif()

if (CONFIG_CS2_P2P_SERVER OR CONFIG_CS2_P2P_CLIENT)
list(APPEND srcs
	bk_network_service/bk_ntwk_cs2_service/ntwk_cs2_service.c
	bk_network_service/bk_ntwk_cs2_service/bk_cs2_service.c
)
endif()

if (CONFIG_KVS_NTWK_BRIDGE)
	list(APPEND srcs
		bk_network_service/bk_ntwk_kvs_service/ntwk_kvs_service.c
		bk_network_service/bk_ntwk_kvs_service/bk_kvs_service.c
	)
endif()
endif()

set(req bk_common media_utils lwip_intf_v2_1 avdk_utils bk_wifi_driver cs2p2p)

if (CONFIG_KVS_NTWK_BRIDGE)
	list(APPEND req kvs_aws)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES "${req}"
	)

# Armino REQUIRES wires the static lib but not always CMake usage propagation; link the target so
# INTERFACE_INCLUDE_DIRECTORIES from kvs_aws (see bk_thirdparty/kvs_aws/CMakeLists.txt incs) reach this build.
if(CONFIG_KVS_NTWK_BRIDGE)
	target_link_libraries(${COMPONENT_LIB} PRIVATE armino::kvs_aws)
endif()
 