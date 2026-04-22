#ifndef __LIBWEBSOCKETS_MBEDTLS_CONFIG__
#define __LIBWEBSOCKETS_MBEDTLS_CONFIG__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Enable X.509 certificate creation for lws_x509_create_self_signed (mbedtls-x509.c) */
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CRT_WRITE_C

/* Enable X.509 parse from file/path (ssl_pm.c: x509_pm_load_file / x509_pm_load_path) */
#define MBEDTLS_FS_IO

/* Enable ALPN (ssl_pm.c: _ssl_set_alpn_list, SSL_get0_alpn_selected) */
#define MBEDTLS_SSL_ALPN

#ifdef __cplusplus
}
#endif

#endif /* __LIBWEBSOCKETS_MBEDTLS_CONFIG__ */
