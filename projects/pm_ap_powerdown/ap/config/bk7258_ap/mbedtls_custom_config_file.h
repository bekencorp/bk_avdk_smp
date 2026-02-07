/*
 * Copyright 2020-2025 Beken

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#pragma once
 
 #define MBEDTLS_HAVE_ASM
 #define MBEDTLS_ENTROPY_HARDWARE_ALT
 #define MBEDTLS_AES_ROM_TABLES
 #define MBEDTLS_AES_FEWER_TABLES
 #define MBEDTLS_CIPHER_MODE_CBC
 #define MBEDTLS_CIPHER_MODE_CTR
 #define MBEDTLS_CIPHER_PADDING_PKCS7
 #define MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS
 #define MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN
 #define MBEDTLS_CIPHER_PADDING_ZEROS
 #define MBEDTLS_ECP_DP_SECP256R1_ENABLED
 #define MBEDTLS_ECP_DP_SECP384R1_ENABLED
 #define MBEDTLS_ECP_NIST_OPTIM
 #define MBEDTLS_ECDH_LEGACY_CONTEXT
 #define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
 #define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
 #define MBEDTLS_ERROR_STRERROR_DUMMY
 #define MBEDTLS_NO_PLATFORM_ENTROPY
 #define MBEDTLS_PKCS1_V15
 
 #define MBEDTLS_SSL_ALL_ALERT_MESSAGES
 #define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
 #define MBEDTLS_SSL_PROTO_TLS1_2
 #define MBEDTLS_SSL_PROTO_DTLS
 #define MBEDTLS_SSL_SERVER_NAME_INDICATION
 #define MBEDTLS_AES_C
 #define MBEDTLS_ASN1_PARSE_C
 #define MBEDTLS_ASN1_WRITE_C
 #define MBEDTLS_BASE64_C
 #define MBEDTLS_BIGNUM_C
 #define MBEDTLS_CCM_C
 #define MBEDTLS_GCM_C
 #define MBEDTLS_NIST_KW_C
 #define MBEDTLS_CIPHER_C
 #define MBEDTLS_CMAC_C
 #define MBEDTLS_CTR_DRBG_C
 #define MBEDTLS_DEBUG_C
 #define MBEDTLS_ECDH_C
 #define MBEDTLS_ECDSA_C
 #define MBEDTLS_ECP_C
 #define MBEDTLS_ENTROPY_C
 #define MBEDTLS_ECP_FIXED_POINT_OPTIM 0
 #define MBEDTLS_HKDF_C
 #define MBEDTLS_MD5_C
 #define MBEDTLS_OID_C
 #define MBEDTLS_PEM_PARSE_C
 #define MBEDTLS_PK_C
 #define MBEDTLS_PK_PARSE_C
 #define MBEDTLS_PK_WRITE_C
 #define MBEDTLS_PKCS5_C
 #define MBEDTLS_PLATFORM_C
 #define MBEDTLS_RSA_C
 #define MBEDTLS_SHA1_C
 #define MBEDTLS_SHA256_C
 //#define MBEDTLS_SHA512_C
 #define MBEDTLS_SSL_CLI_C
 #define MBEDTLS_SSL_TLS_C
 #define MBEDTLS_X509_USE_C
 #define MBEDTLS_X509_CRT_PARSE_C
 #define MBEDTLS_NET_C
 #define MBEDTLS_SSL_OUT_CONTENT_LEN             8192
 #define MBEDTLS_SSL_DTLS_MAX_BUFFERING          16384
 
 #ifndef MBEDTLS_PLATFORM_SNPRINTF_ALT
 #define MBEDTLS_PLATFORM_SNPRINTF_ALT

 
 #define MBEDTLS_THREADING_ALT
 #define MBEDTLS_THREADING_C
 #define MBEDTLS_PLATFORM_MEMORY
 #if defined( MBEDTLS_PLATFORM_MEMORY )
 extern void *tls_mbedtls_mem_calloc(size_t n, size_t size);
 extern void tls_mbedtls_mem_free(void *ptr);
 #define MBEDTLS_PLATFORM_STD_CALLOC             tls_mbedtls_mem_calloc
 #define MBEDTLS_PLATFORM_STD_FREE               tls_mbedtls_mem_free
 #endif
 #define MBEDTLS_PLATFORM_STD_SNPRINTF        snprintf
 #define os_calloc(nmemb,size)   ((size) && (nmemb) > (~( unsigned int) 0)/(size))?0:os_zalloc((nmemb)*(size))
 //#define MBEDTLS_PLATFORM_CALLOC_MACRO        os_calloc /**< Default allocator macro to use, can be undefined. See MBEDTLS_PLATFORM_STD_CALLOC for requirements. */
 //#define MBEDTLS_PLATFORM_FREE_MACRO            os_free /**< Default free macro to use, can be undefined. See MBEDTLS_PLATFORM_STD_FREE for requirements. */
 #define MBEDTLS_PLATFORM_PRINTF_MACRO        os_printf /**< Default printf macro to use, can be undefined */
 
 #endif //CONFIG_FULL_MBEDTLS
 
 #ifdef CRYPTO_NV_SEED
 #include "tfm_mbedcrypto_config_extra_nv_seed.h"
 #endif /* CRYPTO_NV_SEED */
 
 #if defined(CRYPTO_HW_ACCELERATOR) && !defined(CONFIG_TFM_CRYPTO)
 #include "mbedtls_accelerator_config_ns.h"
 #elif defined(MBEDTLS_ENTROPY_NV_SEED)
 #include "mbedtls_entropy_nv_seed_config.h"
 #endif