/*
 * PSA extension API for builds where the PSA server lives in the secure world.
 *
 * The local PSA core is not compiled in such builds, so the few Mbed TLS
 * extension entry points it would export are provided here instead.
 */
#include "common.h"

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)

#include <psa/crypto.h>

/* Key store, RNG and drivers all live in the secure world, so the client has
 * no state of its own to release. */
void mbedtls_psa_crypto_free(void)
{
}

#endif /* MBEDTLS_PSA_CRYPTO_CLIENT && !MBEDTLS_PSA_CRYPTO_C */
