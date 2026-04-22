# Armino (beken) component build for usrsctp
# Same source set as usrsctplib/CMakeLists.txt, built with SCTP_USE_LWIP + SCTP_USE_MBEDTLS_SHA1
# Dependencies: bk_common, lwip_intf_v2_1, os_source, bk_rtos, psa_mbedtls

set(usrsctp_lib_dir ${CMAKE_CURRENT_SOURCE_DIR}/usrsctplib)

set(incs
	.
	${usrsctp_lib_dir}
	${usrsctp_lib_dir}/netinet
	${usrsctp_lib_dir}/netinet6
	${usrsctp_lib_dir}/port
)

# Library sources (from usrsctplib/CMakeLists.txt), plus sctp_lwip_thread_safe.c for SCTP_USE_LWIP
set(srcs
	usrsctplib/netinet/sctp_asconf.c
	usrsctplib/netinet/sctp_auth.c
	usrsctplib/netinet/sctp_bsd_addr.c
	usrsctplib/netinet/sctp_callout.c
	usrsctplib/netinet/sctp_cc_functions.c
	usrsctplib/netinet/sctp_crc32.c
	usrsctplib/netinet/sctp_indata.c
	usrsctplib/netinet/sctp_input.c
	usrsctplib/netinet/sctp_output.c
	usrsctplib/netinet/sctp_pcb.c
	usrsctplib/netinet/sctp_peeloff.c
	usrsctplib/netinet/sctp_sha1.c
	usrsctplib/netinet/sctp_ss_functions.c
	usrsctplib/netinet/sctp_sysctl.c
	usrsctplib/netinet/sctp_timer.c
	usrsctplib/netinet/sctp_userspace.c
	usrsctplib/netinet/sctp_usrreq.c
	usrsctplib/netinet/sctputil.c
	usrsctplib/netinet/sctp_lwip_thread_safe.c
	usrsctplib/netinet6/sctp6_usrreq.c
	usrsctplib/user_environment.c
	usrsctplib/user_mbuf.c
	usrsctplib/user_recv_thread.c
	usrsctplib/user_socket.c
	usrsctplib/port/pthread_rwlock_compat.c
)
