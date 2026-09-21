set(incs)
set(srcs)

if (CONFIG_PSA_MBEDTLS)
include(sw.cmake)
include(hw.cmake)
include(tfm.cmake)

list(APPEND incs
	${hw_incs}
	mbedtls_port/inc
	mbedtls_port/mbedtls/include
	mbedtls_port/mbedtls/library
	mbedtls_port/configs

	mbedtls/include
	mbedtls/library
	mbedtls/tests/src
	mbedtls/tests/include
)

list(APPEND srcs
	${sw_srcs}
	${hw_srcs}
	${tfm_srcs}

	mbedtls_port/src/tls_mem.c
	mbedtls_port/mbedtls/library/platform.c
	mbedtls_port/mbedtls/library/threading.c
	mbedtls_port/mbedtls/programs/test/selftest.c

	mbedtls/tests/src/certs.c

	# src_x509
	mbedtls/library/pkcs7.c
	mbedtls/library/x509.c
	mbedtls/library/x509_create.c
	mbedtls/library/x509_crl.c
	mbedtls/library/x509_crt.c
	mbedtls/library/x509_csr.c
	mbedtls/library/x509write.c
	mbedtls/library/x509write_crt.c
	mbedtls/library/x509write_csr.c

	# src_tls
	mbedtls/library/debug.c
	mbedtls/library/mps_reader.c
	mbedtls/library/mps_trace.c
	mbedtls/library/ssl_cache.c
	mbedtls/library/ssl_ciphersuites.c
	mbedtls/library/ssl_client.c
	mbedtls/library/ssl_cookie.c
	mbedtls/library/ssl_debug_helpers_generated.c
	mbedtls/library/ssl_msg.c
	mbedtls/library/ssl_ticket.c
	mbedtls/library/ssl_tls.c
	mbedtls/library/ssl_tls12_client.c
	mbedtls/library/ssl_tls12_server.c
	mbedtls/library/ssl_tls13_keys.c
	mbedtls/library/ssl_tls13_server.c
	mbedtls/library/ssl_tls13_client.c
	mbedtls/library/ssl_tls13_generic.c

	mbedtls/library/version.c
	mbedtls/library/version_features.c

	# src_crypto
	mbedtls/library/aes.c
	mbedtls/library/aesni.c
	mbedtls/library/aesce.c
	mbedtls/library/aria.c
	mbedtls/library/asn1parse.c
	mbedtls/library/asn1write.c
	mbedtls/library/base64.c
	mbedtls/library/bignum_core.c
	mbedtls/library/bignum_mod.c
	mbedtls/library/bignum_mod_raw.c
	mbedtls/library/block_cipher.c
	mbedtls/library/camellia.c
	mbedtls/library/ccm.c
	mbedtls/library/chacha20.c
	mbedtls/library/chachapoly.c
	mbedtls/library/constant_time.c
	mbedtls/library/des.c
	mbedtls/library/ecjpake.c
	mbedtls/library/ecp_curves_new.c
	mbedtls/library/entropy.c
	mbedtls/library/error.c
	mbedtls/library/gcm.c
	mbedtls/library/hkdf.c
	mbedtls/library/hmac_drbg.c
	mbedtls/library/md5.c
	mbedtls/library/nist_kw.c
	mbedtls/library/padlock.c
	mbedtls/library/ripemd160.c
	mbedtls/library/rsa_alt_helpers.c
	mbedtls/library/sha1.c
	mbedtls/library/sha256.c
	mbedtls/library/sha512.c
	mbedtls/library/sha3.c
	mbedtls/library/pem.c
	mbedtls/library/pk.c
	mbedtls/library/pk_ecc.c
	mbedtls/library/pk_wrap.c
	mbedtls/library/pkcs12.c
	mbedtls/library/pkcs5.c
	mbedtls/library/pkwrite.c
	mbedtls/library/platform_util.c
	mbedtls/library/poly1305.c
	mbedtls/library/cmac.c
)

if (CONFIG_LWIP)
	list(APPEND srcs
		mbedtls_port/src/tls_connect.c
		mbedtls_port/mbedtls/library/net_sockets.c
	)
endif()

if (CONFIG_MBEDTLS_USE_PSA_CRYPTO)
	list(APPEND srcs
		mbedtls/library/psa_util.c
		mbedtls/library/psa_crypto_cipher.c
		mbedtls/library/psa_crypto_slot_management.c
		mbedtls/library/psa_crypto_driver_wrappers_no_static.c
		mbedtls/library/psa_crypto_hash.c
		mbedtls/library/psa_crypto_aead.c
		mbedtls/library/psa_crypto_mac.c
		mbedtls/library/psa_crypto_random.c
	)
	# psa_mpi_helper.c fills in a bignum helper that the TrustEngine bignum.c
	# leaves out; the upstream bignum.c compiled for the software backend already
	# defines it, so including it there would be a duplicate definition.
	# md_psa_client.c is needed by both backends (it stubs out PSA dispatch in the
	# classic layer) and guards its own hardware-only helper internally.
	if (CONFIG_TFM_CRYPTO)
		list(APPEND srcs
			mbedtls/library/psa_crypto_client.c
			mbedtls_port/src/md_psa_client.c
		)
	else()
		list(APPEND srcs
			# Ported copy: hooks the threading init into psa_crypto_init() so the
			# PSA mutexes exist even when no CLI or TLS code initialises them.
			mbedtls_port/mbedtls/library/psa_crypto.c
			mbedtls/library/psa_crypto_client.c
			mbedtls/library/psa_crypto_rsa.c
			mbedtls/library/psa_crypto_ecp.c
			mbedtls/library/psa_crypto_ffdh.c
		)
		if (CONFIG_TRUSTENGINE)
			list(APPEND srcs
				mbedtls_port/src/psa_mpi_helper.c
				mbedtls_port/src/rsa_key_parse.c
			)
		endif()
	endif()
endif()

if (CONFIG_PSA_MBEDTLS_TEST)
	list(APPEND incs
		cli/psa_tests
		#mbedtls/tests/src
		#mbedtls/tests/include
	)

	list(APPEND srcs
		cli/psa_tests/aes_cbc.c
		cli/psa_tests/aes_gcm.c
		cli/psa_tests/cli_crypto.c
		cli/psa_tests/ecdh.c
		cli/psa_tests/ecdsa.c
		cli/psa_tests/hmac.c
		cli/psa_tests/sha256.c
		cli/psa_tests/sha384.c
		cli/psa_tests/key_manage.c
		cli/psa_tests/platform_misc.c
		cli/psa_tests/psa_api_test.c
	)

	# The TLS sample needs the socket layer, which only exists with lwIP.
	if (CONFIG_LWIP)
		list(APPEND srcs
			cli/psa_tests/psa_tls_client.c
		)
	endif()
endif()

if (CONFIG_CLASSIC_MBEDTLS_TEST)
	list(APPEND incs
		cli/mbedtls_tests
	)

	list(APPEND srcs
		cli/mbedtls_tests/mbedtls_sha_test.c
		cli/mbedtls_tests/mbedtls_aes_test.c
		cli/mbedtls_tests/cli_mbedtls.c
		cli/mbedtls_tests/mbedtls_ecdsa_test.c
		cli/mbedtls_tests/mbedtls_rsa_test.c
		cli/mbedtls_tests/mbedtls_multi_thread_test.c
		cli/mbedtls_tests/mbedtls_rand_test.c
		#cli/mbedtls_tests/psa_tls_client.c
	)
endif()

if (NOT CONFIG_TRUSTENGINE)
	list(APPEND srcs
		mbedtls_port/src/tls_hardware.c
	)
endif()

endif()

set(psa_mbedtls_priv_requires bk_common lwip_intf_v2_1)
if (CONFIG_MBEDTLS_USE_PSA_CRYPTO AND CONFIG_TFM_CRYPTO)
	list(APPEND psa_mbedtls_priv_requires tfm)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES ${psa_mbedtls_priv_requires}
)

if (CONFIG_PSA_MBEDTLS)
set(PSA_MBEDTLS_CONFIG_FILE_PATH  "${COMPONENT_DIR}/mbedtls_port/configs/mbedtls_psa_crypto_config.h")
target_compile_definitions(${COMPONENT_LIB}
	PUBLIC
		MBEDTLS_CONFIG_FILE="${PSA_MBEDTLS_CONFIG_FILE_PATH}"
		${hw_defs}
)
endif()
