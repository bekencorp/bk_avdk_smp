/*
 * MD helpers for builds where the PSA server lives in the secure world.
 *
 * The classic MD layer keeps using the local (hardware) engine: PSA dispatch is
 * reported as unavailable so mbedtls_md_* behaves exactly as without PSA.
 */
#include "common.h"

#if defined(MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(MBEDTLS_PSA_CRYPTO_C)

#include <psa/crypto.h>
#include "md_psa.h"
#include "psa_util_internal.h"

/* Only the hardware md.c lacks this helper; the upstream md.c already has it. */
#if defined(MBEDTLS_MD_LIGHT) && CONFIG_TRUSTENGINE
int mbedtls_md_error_from_psa(psa_status_t status)
{
    return PSA_TO_MBEDTLS_ERR_LIST(status, psa_to_md_errors,
                                   psa_generic_status_to_mbedtls);
}
#endif /* MBEDTLS_MD_LIGHT && CONFIG_TRUSTENGINE */

#if defined(MBEDTLS_MD_SOME_PSA)
int psa_can_do_hash(psa_algorithm_t hash_alg)
{
    (void) hash_alg;
    return 0;
}
#endif /* MBEDTLS_MD_SOME_PSA */

#if defined(MBEDTLS_BLOCK_CIPHER_SOME_PSA)
int psa_can_do_cipher(psa_key_type_t key_type, psa_algorithm_t cipher_alg)
{
    (void) key_type;
    (void) cipher_alg;
    return 0;
}
#endif /* MBEDTLS_BLOCK_CIPHER_SOME_PSA */

#endif /* MBEDTLS_PSA_CRYPTO_CLIENT && !MBEDTLS_PSA_CRYPTO_C */
