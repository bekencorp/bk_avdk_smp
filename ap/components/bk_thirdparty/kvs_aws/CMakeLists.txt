#
# kvs_aws: AWS Kinesis Video Streams WebRTC C SDK (armino / m3_7259_v2)
# Builds: PIC (utils/state/client/heap/mkvgen/view/trace) + kvsCommonLws + WebRTC client/signaling
# Depends: psa_mbedtls, libsrtp2, libwebsockets, bk_vfs, bk_rtos, bk_common, os_source, lwip_intf_v2_1, driver
# Optional: usrsctp when CONFIG_KVS_AWS_DATA_CHANNEL
#
# When CONFIG_KVS_AWS=n, registers with empty srcs/incs (same pattern as libsrtp2/libwebsockets).
#

set(incs)
set(srcs)

if(CONFIG_KVS_AWS)
	set(KVS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
	set(PIC_ROOT "${KVS_ROOT}/pic/src")
	set(COMMON_LWS_ROOT "${KVS_ROOT}/common_lws")
	set(WEBRTC_ROOT "${KVS_ROOT}/webrtc_src")

	list(APPEND incs
		${KVS_ROOT}
		${KVS_ROOT}/webrtc_src/include
		${KVS_ROOT}/webrtc_src/source
		${KVS_ROOT}/configs
		${COMMON_LWS_ROOT}/include
		${COMMON_LWS_ROOT}/Common
		${COMMON_LWS_ROOT}/Common/Lws
		${PIC_ROOT}
		${PIC_ROOT}/utils/include
		${PIC_ROOT}/client/include
		${PIC_ROOT}/common/include
		${PIC_ROOT}/state/include
		${PIC_ROOT}/heap/include
		${PIC_ROOT}/duration/src
		${PIC_ROOT}/mkvgen/include
		${PIC_ROOT}/view/include
		${PIC_ROOT}/trace/include
	)
	if(CONFIG_KVS_AWS_DATA_CHANNEL)
		# usrsctp.h lives in sibling component usrsctp/usrsctplib
		list(APPEND incs ${CMAKE_CURRENT_SOURCE_DIR}/../usrsctp/usrsctplib)
	endif()

	# PIC sources (utils, state, client, heap, duration, mkvgen, view, trace)
	file(GLOB PIC_UTILS_SRCS "${PIC_ROOT}/utils/src/*.c")
	file(GLOB PIC_STATE_SRCS "${PIC_ROOT}/state/src/*.c")
	file(GLOB PIC_CLIENT_SRCS "${PIC_ROOT}/client/src/*.c")
	file(GLOB PIC_HEAP_SRCS "${PIC_ROOT}/heap/src/*.c")
	file(GLOB PIC_DURATION_SRCS "${PIC_ROOT}/duration/src/*.c")
	file(GLOB PIC_MKVGEN_SRCS "${PIC_ROOT}/mkvgen/src/*.c")
	file(GLOB PIC_VIEW_SRCS "${PIC_ROOT}/view/src/*.c")
	file(GLOB PIC_TRACE_SRCS "${PIC_ROOT}/trace/src/*.c")
	list(APPEND srcs ${PIC_UTILS_SRCS} ${PIC_STATE_SRCS} ${PIC_CLIENT_SRCS} ${PIC_HEAP_SRCS}
		${PIC_DURATION_SRCS} ${PIC_MKVGEN_SRCS} ${PIC_VIEW_SRCS} ${PIC_TRACE_SRCS})

	# kvsCommonLws: Common (exclude Curl/jsmn) + Lws
	file(GLOB COMMON_BASE "${COMMON_LWS_ROOT}/Common/*.c")
	file(GLOB COMMON_LWS "${COMMON_LWS_ROOT}/Common/Lws/*.c")
	list(FILTER COMMON_BASE EXCLUDE REGEX ".*/Curl/.*")
	list(FILTER COMMON_BASE EXCLUDE REGEX ".*/jsmn\\.c$")
	list(APPEND srcs ${COMMON_BASE} ${COMMON_LWS})

	# WebRTC client + signaling (no OpenSSL; DataChannel optional)
	file(GLOB WEBRTC_CRYPTO "${WEBRTC_ROOT}/source/Crypto/*.c")
	file(GLOB WEBRTC_ICE "${WEBRTC_ROOT}/source/Ice/*.c")
	file(GLOB WEBRTC_RTCP "${WEBRTC_ROOT}/source/Rtcp/*.c")
	file(GLOB WEBRTC_RTP "${WEBRTC_ROOT}/source/Rtp/*.c")
	file(GLOB WEBRTC_RTP_CODECS "${WEBRTC_ROOT}/source/Rtp/Codecs/*.c")
	file(GLOB WEBRTC_SDP "${WEBRTC_ROOT}/source/Sdp/*.c")
	file(GLOB WEBRTC_SRTP "${WEBRTC_ROOT}/source/Srtp/*.c")
	file(GLOB WEBRTC_STUN "${WEBRTC_ROOT}/source/Stun/*.c")
	file(GLOB WEBRTC_SIGNALING "${WEBRTC_ROOT}/source/Signaling/*.c")
	file(GLOB WEBRTC_METRICS "${WEBRTC_ROOT}/source/Metrics/*.c")
	list(FILTER WEBRTC_CRYPTO EXCLUDE REGEX ".*_openssl\\.c$")
	if(CONFIG_KVS_GET_CA_FROM_ARRAY)
	list(APPEND srcs ${WEBRTC_ROOT}/source/Platform/beken/kvs_embedded_ca_cert.c)
	endif()
	list(APPEND srcs
		${WEBRTC_CRYPTO}
		${WEBRTC_ICE}
		${WEBRTC_ROOT}/source/Threadpool/ThreadPoolContext.c
		${WEBRTC_ROOT}/source/PeerConnection/JitterBuffer.c
		${WEBRTC_ROOT}/source/PeerConnection/jsmn.c
		${WEBRTC_ROOT}/source/PeerConnection/PeerConnection.c
		${WEBRTC_ROOT}/source/PeerConnection/Retransmitter.c
		${WEBRTC_ROOT}/source/PeerConnection/Rtcp.c
		${WEBRTC_ROOT}/source/PeerConnection/Rtp.c
		${WEBRTC_ROOT}/source/PeerConnection/SessionDescription.c
		${WEBRTC_RTCP}
		${WEBRTC_RTP}
		${WEBRTC_RTP_CODECS}
		${WEBRTC_SDP}
		${WEBRTC_SRTP}
		${WEBRTC_STUN}
		${WEBRTC_SIGNALING}
		${WEBRTC_METRICS}
	)
	if(CONFIG_KVS_AWS_DATA_CHANNEL)
		list(APPEND srcs
			${WEBRTC_ROOT}/source/PeerConnection/DataChannel.c
			${WEBRTC_ROOT}/source/Sctp/Sctp.c
		)
	endif()
endif()

# Dependencies (same as libwebsockets-style: bk_common, os_source, lwip_intf_v2_1, bk_vfs, bk_rtos, driver, psa_mbedtls)
set(kvs_priv_req psa_mbedtls libsrtp2 libwebsockets bk_vfs bk_rtos bk_common os_source lwip_intf_v2_1 driver)
if(CONFIG_KVS_AWS_DATA_CHANNEL)
	list(APPEND kvs_priv_req usrsctp)
endif()

armino_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS ${incs}
	PRIV_REQUIRES ${kvs_priv_req}
)

if(CONFIG_KVS_AWS)
	# SSL via psa_mbedtls (PRIV_REQUIRES psa_mbedtls)
	# LWS_AMAZON_RTOS: use <FreeRTOS.h> not <freertos/FreeRTOS.h> so include works with os_source include path (same as libwebsockets).
	target_compile_definitions(${COMPONENT_LIB} PRIVATE
		KVS_USE_MBEDTLS
		#CONFIG_PSA_MBEDTLS
		#LWS_WITH_MBEDTLS
		KVS_BUILD_WITH_LWS
		LWS_AMAZON_RTOS
		MBEDTLS_USER_CONFIG_FILE=\"config_mbedtls.h\"
		#CONFIG_KVS_PTHREAD_CANCEL_DETACH
		# Thread pool: WebRTC client singleton + async STUN DNS; signaling uses pool instead of per-message THREAD_CREATE
		ENABLE_KVS_THREADPOOL
		KVS_DEFAULT_STACK_SIZE_BYTES=30720
		# Embedded: reduce signaling file cache entries to save heap (~7KB per entry). 4 allows a few channels; use 1 for single-channel.
		KVS_SIGNALING_CACHE_ENTRY_COUNT=4
	)
	if(CONFIG_KVS_AWS_DATA_CHANNEL)
		target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_DATA_CHANNEL)
	endif()

	# Relax warnings for third-party code (same idea as libsrtp2/usrsctp)
	target_compile_options(${COMPONENT_LIB} PRIVATE
		-Wno-unused-parameter
		-Wno-maybe-uninitialized
		-Wno-sign-compare
	)
endif()
