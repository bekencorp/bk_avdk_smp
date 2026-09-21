/*
 * Mbed TLS configuration of the secure world crypto service.
 *
 * Starts from the medium profile and allows AES-192/256 keys, so the non-secure
 * world sees the same AES key sizes whether PSA is served by the secure world
 * or by a local non-secure server.
 */

#ifndef TFM_MBEDCRYPTO_CONFIG_H
#define TFM_MBEDCRYPTO_CONFIG_H

#include "../tfm/lib/ext/mbedcrypto/mbedcrypto_config/tfm_mbedcrypto_config_profile_medium.h"

#undef MBEDTLS_AES_ONLY_128_BIT_KEY_LENGTH

#endif /* TFM_MBEDCRYPTO_CONFIG_H */
