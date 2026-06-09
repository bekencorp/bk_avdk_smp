/*
 * hmac_mbedtls_classic.c
 *
 * Implementation of hmac srtp_auth_type_t that uses the classic
 * Mbed TLS message-digest API (mbedtls_md_hmac_*), as an alternative
 * to the PSA-based backend (hmac_mbedtls.c). Selected when
 * CONFIG_PSA_MBEDTLS is enabled but CONFIG_MBEDTLS_USE_PSA_CRYPTO is not.
 *
 * Does not require psa_crypto_init().
 *
 * Note: requires MBEDTLS_MD_C and MBEDTLS_SHA1_C in the mbedtls config.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mbedtls/md.h>

#include "auth.h"
#include "alloc.h"
#include "err.h" /* for srtp_debug */
#include "auth_test_cases.h"

#define SHA1_DIGEST_SIZE 20

typedef struct {
    mbedtls_md_context_t ctx;
} classic_hmac_ctx_t;

/* the debug module for authentication */
srtp_debug_module_t srtp_mod_hmac = {
    false,                       /* debugging is off by default */
    "hmac sha-1 mbedtls classic" /* printable name for module   */
};

static srtp_err_status_t srtp_hmac_classic_alloc(srtp_auth_t **a,
                                                 size_t key_len,
                                                 size_t out_len)
{
    extern const srtp_auth_type_t srtp_hmac;
    classic_hmac_ctx_t *hmac_ctx;
    const mbedtls_md_info_t *md_info;

    debug_print(srtp_mod_hmac, "allocating auth func with key length %zu",
                key_len);
    debug_print(srtp_mod_hmac, "                          tag length %zu",
                out_len);

    /* check output length - should be less than 20 bytes */
    if (out_len > SHA1_DIGEST_SIZE) {
        return srtp_err_status_bad_param;
    }

    *a = (srtp_auth_t *)srtp_crypto_alloc(sizeof(srtp_auth_t));
    if (*a == NULL) {
        return srtp_err_status_alloc_fail;
    }

    /* allocate the mbedtls md context */
    (*a)->state = srtp_crypto_alloc(sizeof(classic_hmac_ctx_t));
    if ((*a)->state == NULL) {
        srtp_crypto_free(*a);
        *a = NULL;
        return srtp_err_status_alloc_fail;
    }

    hmac_ctx = (classic_hmac_ctx_t *)((*a)->state);
    mbedtls_md_init(&hmac_ctx->ctx);

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (md_info == NULL) {
        srtp_crypto_free((*a)->state);
        srtp_crypto_free(*a);
        *a = NULL;
        return srtp_err_status_auth_fail;
    }

    /* 1 = HMAC mode */
    if (mbedtls_md_setup(&hmac_ctx->ctx, md_info, 1) != 0) {
        mbedtls_md_free(&hmac_ctx->ctx);
        srtp_crypto_free((*a)->state);
        srtp_crypto_free(*a);
        *a = NULL;
        return srtp_err_status_auth_fail;
    }

    /* set pointers */
    (*a)->type = &srtp_hmac;
    (*a)->out_len = out_len;
    (*a)->key_len = key_len;
    (*a)->prefix_len = 0;

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_hmac_classic_dealloc(srtp_auth_t *a)
{
    classic_hmac_ctx_t *hmac_ctx;
    hmac_ctx = (classic_hmac_ctx_t *)a->state;

    mbedtls_md_free(&hmac_ctx->ctx);
    srtp_crypto_free(hmac_ctx);

    /* zeroize entire state */
    octet_string_set_to_zero(a, sizeof(srtp_auth_t));

    /* free memory */
    srtp_crypto_free(a);

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_hmac_classic_init(void *statev,
                                                const uint8_t *key,
                                                size_t key_len)
{
    classic_hmac_ctx_t *state = (classic_hmac_ctx_t *)statev;

    if (mbedtls_md_hmac_starts(&state->ctx, key, key_len) != 0) {
        return srtp_err_status_auth_fail;
    }

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_hmac_classic_start(void *statev)
{
    classic_hmac_ctx_t *state = (classic_hmac_ctx_t *)statev;

    /* reset reuses the key set by mbedtls_md_hmac_starts() */
    if (mbedtls_md_hmac_reset(&state->ctx) != 0) {
        return srtp_err_status_auth_fail;
    }

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_hmac_classic_update(void *statev,
                                                  const uint8_t *message,
                                                  size_t msg_octets)
{
    classic_hmac_ctx_t *state = (classic_hmac_ctx_t *)statev;

    debug_print(srtp_mod_hmac, "input: %s",
                srtp_octet_string_hex_string(message, msg_octets));

    if (mbedtls_md_hmac_update(&state->ctx, message, msg_octets) != 0) {
        return srtp_err_status_auth_fail;
    }

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_hmac_classic_compute(void *statev,
                                                   const uint8_t *message,
                                                   size_t msg_octets,
                                                   size_t tag_len,
                                                   uint8_t *result)
{
    classic_hmac_ctx_t *state = (classic_hmac_ctx_t *)statev;
    uint8_t hash_value[SHA1_DIGEST_SIZE];
    size_t i;

    /* check tag length, return error if we can't provide the value expected */
    if (tag_len > SHA1_DIGEST_SIZE) {
        return srtp_err_status_bad_param;
    }

    /* hash the remaining message, then finalize */
    if (mbedtls_md_hmac_update(&state->ctx, message, msg_octets) != 0) {
        return srtp_err_status_auth_fail;
    }

    if (mbedtls_md_hmac_finish(&state->ctx, hash_value) != 0) {
        return srtp_err_status_auth_fail;
    }

    /* copy hash_value to *result */
    for (i = 0; i < tag_len; i++) {
        result[i] = hash_value[i];
    }

    debug_print(srtp_mod_hmac, "output: %s",
                srtp_octet_string_hex_string(hash_value, tag_len));

    return srtp_err_status_ok;
}

static const char srtp_hmac_classic_description[] =
    "hmac sha-1 authentication function using mbedtls (classic)";

/*
 * srtp_auth_type_t hmac is the hmac metaobject
 */
const srtp_auth_type_t srtp_hmac = {
    srtp_hmac_classic_alloc,       /* */
    srtp_hmac_classic_dealloc,     /* */
    srtp_hmac_classic_init,        /* */
    srtp_hmac_classic_compute,     /* */
    srtp_hmac_classic_update,      /* */
    srtp_hmac_classic_start,       /* */
    srtp_hmac_classic_description, /* */
    &srtp_hmac_test_case_0,        /* */
    SRTP_HMAC_SHA1                 /* */
};
