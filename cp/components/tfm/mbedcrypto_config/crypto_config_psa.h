/*
 * PSA crypto capability set of the secure world PSA server.
 *
 * Starts from the medium profile and adds AES-GCM and AES-CBC, so the
 * non-secure world gets the same algorithms whether PSA is served by the
 * secure world or by a local non-secure server.
 */

#ifndef CRYPTO_CONFIG_PSA_H
#define CRYPTO_CONFIG_PSA_H

#include "../tfm/lib/ext/mbedcrypto/mbedcrypto_config/crypto_config_profile_medium.h"

#ifndef PSA_WANT_ALG_GCM
#define PSA_WANT_ALG_GCM 1
#endif

// #ifndef PSA_WANT_ALG_SHA_384
// #define PSA_WANT_ALG_SHA_384 1
// #endif

// #ifndef PSA_WANT_ALG_SHA_512
// #define PSA_WANT_ALG_SHA_512 1
// #endif

#ifndef PSA_WANT_ALG_CBC_NO_PADDING
#define PSA_WANT_ALG_CBC_NO_PADDING 1
#endif

#endif /* CRYPTO_CONFIG_PSA_H */
