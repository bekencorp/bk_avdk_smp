# Armino (BEKEN) component build for libwebsockets
# FreeRTOS + mbedTLS, no project(), only collect srcs/incs and generate config

set(LWS_PLAT_FREERTOS 1)
set(LWS_WITH_MBEDTLS 1)
set(LWS_WITH_SSL 1)
set(LWS_WITH_NETWORK 1)
set(LWS_WITH_CLIENT 1)
set(LWS_WITH_SERVER 1)
set(LWS_ROLE_H1 1)
set(LWS_ROLE_H2 1)
set(LWS_ROLE_WS 1)
set(LWS_ROLE_RAW 1)
set(LWS_ROLE_RAW_FILE 1)
set(LWS_WITH_FILE_OPS 1)
set(LWS_WITH_HTTP2 1)
set(LWS_WITH_POLL 1)
set(LWS_WITH_SYS_STATE 1)
set(LWS_WITH_SYS_SMD 1)
set(LWS_WITH_SECURE_STREAMS 1)
set(LWS_WITH_TLS 1)
set(LWS_WITH_TLS_SESSIONS 1)
set(LWS_WITH_CONMON 1)
set(LWS_WITH_WOL 1)
set(LWS_WITHOUT_EXTENSIONS 1)
set(LWS_WITH_CUSTOM_HEADERS 1)
if(CONFIG_IPV6)
	set(LWS_WITH_IPV6 1)
endif()
set(LWS_WITH_LEJP 1)
set(LWS_WITH_LHP 1)
set(LWS_WITH_JSONRPC 1)
set(LWS_WITH_UPNG 1)
set(LWS_WITH_GZINFLATE 1)
set(LWS_WITH_JPEG 1)
set(LWS_WITH_DLO 1)
set(LWS_WITH_LWSAC 1)
set(LWS_WITHOUT_BUILTIN_SHA1 0)
set(LWS_SUPPRESS_DEPRECATED_API_WARNINGS 1)
set(LWS_MAX_SMP 1)
set(LWS_LOGGING_BITFIELD_SET 0)
set(LWS_LOGGING_BITFIELD_CLEAR 0)
set(LWS_LIBRARY_VERSION_MAJOR 4)
set(LWS_LIBRARY_VERSION_MINOR 5)
set(LWS_LIBRARY_VERSION_PATCH 99)
set(LWS_LIBRARY_VERSION_PATCH_ELABORATED "99-armino")
set(LWS_LIBRARY_VERSION "${LWS_LIBRARY_VERSION_MAJOR}.${LWS_LIBRARY_VERSION_MINOR}.${LWS_LIBRARY_VERSION_PATCH_ELABORATED}")
set(LWS_INSTALL_PREFIX "/usr")
set(LWS_INSTALL_LIB_DIR "lib")
set(LWS_BUILD_HASH "armino")
# Default client CA cert path (used by TLS when no CA is specified; "." = current dir)
set(LWS_OPENSSL_CLIENT_CERTS ".")
#set(LWIP_PROVIDE_ERRNO 1)

# For config generation - basic headers (embedded toolchain has these)
set(LWS_HAVE_STDINT_H 1)
set(LWS_HAVE_STRING_H 1)
set(LWS_HAVE_STRINGS_H 1)
set(LWS_HAVE_STDLIB_H 1)
set(LWS_HAVE_SYS_TYPES_H 1)
set(LWS_HAVE_SYS_STAT_H 1)
set(LWS_HAVE_SYS_SOCKET_H 1)
set(LWS_HAVE_NETINET_IN_H 1)
set(LWS_HAVE_MALLOC 1)
set(LWS_HAVE_GETENV 1)
set(LWS_HAVE_STRERROR 1)
set(LWS_HAVE_MEMSET 1)
set(LWS_HAVE_REALLOC 1)
set(LWS_HAVE_SNPRINTF 1)
set(LWS_HAVE_PTHREAD_H 1)
set(LWS_HAVE_INTTYPES_H 1)
set(LWS_HAS_INTPTR_T 1)
# mbedTLS-specific
set(LWS_HAVE_mbedtls_net_init 1)
set(LWS_HAVE_mbedtls_ssl_conf_alpn_protocols 1)
set(LWS_HAVE_mbedtls_ssl_get_alpn_protocol 1)
set(LWS_HAVE_mbedtls_ssl_session_save 1)
set(LWS_HAVE_mbedtls_ssl_set_hs_ca_chain 1)
set(LWS_HAVE_mbedtls_ssl_set_hs_own_cert 1)
set(LWS_HAVE_mbedtls_ssl_set_hs_authmode 1)
set(LWS_HAVE_mbedtls_ssl_set_verify 1)
set(LWS_HAVE_mbedtls_x509_crt_parse_file 1)
set(LWS_HAVE_MBEDTLS_NET_SOCKETS 1)
set(LWS_HAVE_MBEDTLS_SSL_NEW_SESSION_TICKET 1)

# Generate config headers only when building (add_subdirectory), not during get_requirements.
# In get_requirements CMAKE_CURRENT_SOURCE_DIR is the build dir, so we skip to avoid wrong paths.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL COMPONENT_DIR)
	configure_file(
		${COMPONENT_DIR}/cmake/lws_config.h.in
		${CMAKE_CURRENT_BINARY_DIR}/lws_config.h
		)
	configure_file(
		${COMPONENT_DIR}/cmake/lws_config_private.h.in
		${CMAKE_CURRENT_BINARY_DIR}/lws_config_private.h
		)
endif()

set(srcs)
set(incs
	.
	include
	configs
	${CMAKE_CURRENT_BINARY_DIR}
	lib
	lib/core
	lib/plat/freertos
	lib/misc
	lib/system
	lib/system/smd
	lib/system/metrics
	lib/system/async-dns
	lib/tls
	lib/tls/mbedtls
	lib/tls/mbedtls/wrapper/include
	lib/tls/mbedtls/wrapper/include/internal
	lib/tls/mbedtls/wrapper/include/platform
	lib/tls/mbedtls/wrapper/include/openssl
	lib/core-net
	lib/roles
	lib/roles/http
	lib/roles/http/compression
	lib/roles/h1
	lib/roles/h2
	lib/roles/ws
	lib/event-libs
	lib/event-libs/poll
	lib/secure-streams
	lib/misc/jrpc
)

# Core
list(APPEND srcs
	lib/core/lws_dll2.c
	lib/core/alloc.c
	lib/core/buflist.c
	lib/core/context.c
	lib/core/lws_map.c
	lib/core/adapt.c
	lib/core/libwebsockets.c
	lib/core/logs.c
	lib/core/vfs.c
)

# Plat freertos
list(APPEND srcs
	lib/plat/freertos/freertos-fds.c
	lib/plat/freertos/freertos-init.c
	lib/plat/freertos/freertos-misc.c
	lib/plat/freertos/freertos-pipe.c
	lib/plat/freertos/freertos-service.c
	lib/plat/freertos/freertos-sockets.c
	lib/plat/freertos/freertos-file.c
	lib/misc/romfs.c
)

# Misc
list(APPEND srcs
	lib/misc/base64-decode.c
	lib/misc/prng.c
	lib/misc/lws-crc32.c
	lib/misc/lws-ring.c
	lib/misc/cache-ttl/lws-cache-ttl.c
	lib/misc/cache-ttl/heap.c
	lib/misc/upng-gzip.c
	lib/misc/upng.c
	lib/misc/jpeg.c
	lib/misc/dlo/dlo.c
	lib/misc/dlo/dlo-rect.c
	lib/misc/dlo/dlo-font-mcufont.c
	lib/misc/dlo/dlo-text.c
	lib/misc/dlo/dlo-png.c
	lib/misc/dlo/dlo-jpeg.c
	lib/misc/dlo/dlo-lhp.c
	lib/misc/sha-1.c
	lib/misc/lejp.c
	lib/misc/lhp.c
	lib/misc/lhp-ss.c
	lib/misc/jrpc/jrpc.c
	lib/misc/lwsac/lwsac.c
)

# System
list(APPEND srcs
	lib/system/system.c
	lib/system/smd/smd.c
)

# TLS mbedtls
list(APPEND srcs
	lib/tls/tls.c
	lib/tls/tls-network.c
	lib/tls/tls-sessions.c
	lib/tls/tls-server.c
	lib/tls/tls-client.c
	lib/tls/mbedtls/mbedtls-tls.c
	lib/tls/mbedtls/mbedtls-extensions.c
	lib/tls/mbedtls/mbedtls-x509.c
	lib/tls/mbedtls/mbedtls-ssl.c
	lib/tls/mbedtls/mbedtls-session.c
	lib/tls/mbedtls/mbedtls-server.c
	lib/tls/mbedtls/mbedtls-client.c
	lib/tls/mbedtls/wrapper/library/ssl_cert.c
	lib/tls/mbedtls/wrapper/library/ssl_lib.c
	lib/tls/mbedtls/wrapper/library/ssl_methods.c
	lib/tls/mbedtls/wrapper/library/ssl_pkey.c
	lib/tls/mbedtls/wrapper/library/ssl_stack.c
	lib/tls/mbedtls/wrapper/library/ssl_x509.c
	lib/tls/mbedtls/wrapper/platform/ssl_pm.c
	lib/tls/mbedtls/wrapper/platform/ssl_port.c
)

# Core-net
list(APPEND srcs
	lib/core-net/dummy-callback.c
	lib/core-net/output.c
	lib/core-net/close.c
	lib/core-net/network.c
	lib/core-net/vhost.c
	lib/core-net/pollfd.c
	lib/core-net/service.c
	lib/core-net/sorted-usec-list.c
	lib/core-net/wsi.c
	lib/core-net/wsi-timeout.c
	lib/core-net/adopt.c
	lib/core-net/latency.c
	lib/roles/pipe/ops-pipe.c
	lib/core-net/state.c
	lib/core-net/wol.c
	lib/core-net/client/client.c
	lib/core-net/client/connect.c
	lib/core-net/client/connect2.c
	lib/core-net/client/connect3.c
	lib/core-net/client/connect4.c
	lib/core-net/client/sort-dns.c
	lib/core-net/client/conmon.c
)

# Roles
list(APPEND srcs
	lib/roles/http/client/client-http.c
	lib/roles/http/header.c
	lib/roles/http/date.c
	lib/roles/http/parsers.c
	lib/roles/http/server/server.c
	lib/roles/http/server/lws-spa.c
	lib/roles/h1/ops-h1.c
	lib/roles/h2/http2.c
	lib/roles/h2/hpack.c
	lib/roles/h2/ops-h2.c
	lib/roles/ws/ops-ws.c
	lib/roles/ws/client-ws.c
	lib/roles/ws/client-parser-ws.c
	lib/roles/ws/server-ws.c
	lib/roles/raw-skt/ops-raw-skt.c
	lib/roles/raw-file/ops-raw-file.c
	lib/roles/listen/ops-listen.c
)

# Event poll
list(APPEND srcs
	lib/event-libs/poll/poll.c
)

# Secure streams
list(APPEND srcs
	lib/secure-streams/secure-streams.c
	lib/secure-streams/policy-common.c
	lib/secure-streams/system/captive-portal-detect/captive-portal-detect.c
	lib/secure-streams/protocols/ss-raw.c
	lib/secure-streams/policy-json.c
	lib/secure-streams/system/fetch-policy/fetch-policy.c
	lib/secure-streams/protocols/ss-h1.c
	lib/secure-streams/protocols/ss-h2.c
	lib/secure-streams/protocols/ss-ws.c
)

# Media: no sources for minimal embedded (no ALSA/V4L2/transcode)
