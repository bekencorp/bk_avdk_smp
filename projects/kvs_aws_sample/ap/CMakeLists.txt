# kvs_aws_sample AP: AWS KVS WebRTC P2P Master (static media from disk)
# Requires CONFIG_KVS_AWS=y in menuconfig. Optional: CONFIG_KVS_AWS_DATA_CHANNEL for Data Channel.
#
# Optional: use IoT Core credentials instead of IAM Access Key (see kvs_aws_sample.rst " A").
# Set to ON when building, e.g.: cmake ... -DKVS_USE_IOT_CREDENTIALS=ON
if(NOT DEFINED KVS_USE_IOT_CREDENTIALS)
	set(KVS_USE_IOT_CREDENTIALS OFF)
endif()
if(KVS_USE_IOT_CREDENTIALS)
	message(STATUS "kvs_aws_sample: building with IOT_CORE_ENABLE_CREDENTIALS (IoT credential provider)")
endif()

set(incs .)
set(srcs
    ap_main.c
    kvs_cli.c
    Common.c
    StaticMedia.c
    kvsWebRTCClientMaster.c
    kvsWebRTCClientViewer.c
    newlib_syscalls.c
)

set(priv_req bk_common lwip_intf_v2_1 bk_cli bk_vfs bk_rtos psa_mbedtls libwebsockets usrsctp libsrtp2 kvs_aws)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

if(KVS_USE_IOT_CREDENTIALS)
	target_compile_definitions(${COMPONENT_LIB} PRIVATE IOT_CORE_ENABLE_CREDENTIALS)
endif()

