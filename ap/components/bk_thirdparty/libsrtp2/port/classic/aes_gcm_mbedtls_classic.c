/*
 * aes_gcm_mbedtls_classic.c
 *
 * AES Galois Counter Mode using the classic Mbed TLS API
 * (mbedtls_gcm_*), as an alternative to the PSA-based backend
 * (aes_gcm_mbedtls.c). Selected when CONFIG_PSA_MBEDTLS is enabled
 * but CONFIG_MBEDTLS_USE_PSA_CRYPTO is not.
 *
 * Does not require psa_crypto_init().
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <mbedtls/gcm.h>

#include "srtp.h"
#include "cipher.h"
#include "datatypes.h"
#include "crypto_types.h"
#include "err.h" /* for srtp_debug */
#include "alloc.h"
#include "cipher_types.h"
#include "cipher_test_cases.h"

#define MAX_AD_SIZE 2048
#define GCM_IV_LEN 12
#define GCM_AUTH_TAG_LEN 16
#define GCM_AUTH_TAG_LEN_8 8

#define FUNC_ENTRY() debug_print(srtp_mod_aes_gcm, "%s entry", __func__);

srtp_debug_module_t srtp_mod_aes_gcm = {
    false,                    /* debugging is off by default */
    "aes gcm mbedtls classic" /* printable module name       */
};

/*
 * Private context for the classic Mbed TLS AES-GCM backend.
 */
typedef struct {
    size_t key_size;            /* AES key size in bytes (16/32)    */
    size_t tag_len;             /* AEAD tag length                  */
    size_t aad_size;            /* number of buffered AAD bytes     */
    size_t iv_len;              /* IV length                        */
    uint8_t iv[GCM_IV_LEN];     /* IV buffer                        */
    uint8_t aad[MAX_AD_SIZE];   /* AAD buffer                       */
    srtp_cipher_direction_t dir;
    mbedtls_gcm_context gcm;    /* the cipher context               */
} classic_aes_gcm_ctx_t;

/*
 * static function declarations.
 */
static srtp_err_status_t srtp_aes_gcm_classic_alloc(srtp_cipher_t **c,
                                                    size_t key_len,
                                                    size_t tlen);

static srtp_err_status_t srtp_aes_gcm_classic_dealloc(srtp_cipher_t *c);

static srtp_err_status_t srtp_aes_gcm_classic_context_init(void *cv,
                                                           const uint8_t *key);

static srtp_err_status_t srtp_aes_gcm_classic_set_iv(
    void *cv,
    uint8_t *iv,
    srtp_cipher_direction_t direction);

static srtp_err_status_t srtp_aes_gcm_classic_set_aad(void *cv,
                                                      const uint8_t *aad,
                                                      size_t aad_len);

static srtp_err_status_t srtp_aes_gcm_classic_encrypt(void *cv,
                                                      const uint8_t *src,
                                                      size_t src_len,
                                                      uint8_t *dst,
                                                      size_t *dst_len);

static srtp_err_status_t srtp_aes_gcm_classic_decrypt(void *cv,
                                                      const uint8_t *src,
                                                      size_t src_len,
                                                      uint8_t *dst,
                                                      size_t *dst_len);

/*
 * Name of this crypto engine
 */
static const char srtp_aes_gcm_128_classic_description[] =
    "AES-128 GCM using mbedtls (classic)";
static const char srtp_aes_gcm_256_classic_description[] =
    "AES-256 GCM using mbedtls (classic)";

/* clang-format off */
const srtp_cipher_type_t srtp_aes_gcm_128 = {
    srtp_aes_gcm_classic_alloc,
    srtp_aes_gcm_classic_dealloc,
    srtp_aes_gcm_classic_context_init,
    srtp_aes_gcm_classic_set_aad,
    srtp_aes_gcm_classic_encrypt,
    srtp_aes_gcm_classic_decrypt,
    srtp_aes_gcm_classic_set_iv,
    srtp_aes_gcm_128_classic_description,
    &srtp_aes_gcm_128_test_case_0,
    SRTP_AES_GCM_128
};

const srtp_cipher_type_t srtp_aes_gcm_256 = {
    srtp_aes_gcm_classic_alloc,
    srtp_aes_gcm_classic_dealloc,
    srtp_aes_gcm_classic_context_init,
    srtp_aes_gcm_classic_set_aad,
    srtp_aes_gcm_classic_encrypt,
    srtp_aes_gcm_classic_decrypt,
    srtp_aes_gcm_classic_set_iv,
    srtp_aes_gcm_256_classic_description,
    &srtp_aes_gcm_256_test_case_0,
    SRTP_AES_GCM_256
};
/* clang-format on */

/*
 * This function allocates a new instance of this crypto engine.
 * The key_len parameter should be one of 28 or 44 for
 * AES-128-GCM or AES-256-GCM respectively.  Note that the
 * key length includes the 12 byte salt value that is used when
 * initializing the KDF.
 */
static srtp_err_status_t srtp_aes_gcm_classic_alloc(srtp_cipher_t **c,
                                                    size_t key_len,
                                                    size_t tlen)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *gcm;

    debug_print(srtp_mod_aes_gcm, "allocating cipher with key length %zu",
                key_len);
    debug_print(srtp_mod_aes_gcm, "allocating cipher with tag length %zu",
                tlen);

    /*
     * Verify the key_len is valid for one of: AES-128/256
     */
    if (key_len != SRTP_AES_GCM_128_KEY_LEN_WSALT &&
        key_len != SRTP_AES_GCM_256_KEY_LEN_WSALT) {
        return srtp_err_status_bad_param;
    }

    if (tlen != GCM_AUTH_TAG_LEN && tlen != GCM_AUTH_TAG_LEN_8) {
        return srtp_err_status_bad_param;
    }

    /* allocate memory a cipher of type aes_gcm */
    *c = (srtp_cipher_t *)srtp_crypto_alloc(sizeof(srtp_cipher_t));
    if (*c == NULL) {
        return srtp_err_status_alloc_fail;
    }

    gcm = (classic_aes_gcm_ctx_t *)srtp_crypto_alloc(
        sizeof(classic_aes_gcm_ctx_t));
    if (gcm == NULL) {
        srtp_crypto_free(*c);
        *c = NULL;
        return srtp_err_status_alloc_fail;
    }

    mbedtls_gcm_init(&gcm->gcm);

    /* set pointers */
    (*c)->state = gcm;

    /* setup cipher attributes */
    switch (key_len) {
    case SRTP_AES_GCM_128_KEY_LEN_WSALT:
        (*c)->type = &srtp_aes_gcm_128;
        (*c)->algorithm = SRTP_AES_GCM_128;
        gcm->key_size = SRTP_AES_128_KEY_LEN;
        gcm->tag_len = tlen;
        break;
    case SRTP_AES_GCM_256_KEY_LEN_WSALT:
        (*c)->type = &srtp_aes_gcm_256;
        (*c)->algorithm = SRTP_AES_GCM_256;
        gcm->key_size = SRTP_AES_256_KEY_LEN;
        gcm->tag_len = tlen;
        break;
    }

    /* set key size        */
    (*c)->key_len = key_len;

    return srtp_err_status_ok;
}

/*
 * This function deallocates a GCM session
 */
static srtp_err_status_t srtp_aes_gcm_classic_dealloc(srtp_cipher_t *c)
{
    classic_aes_gcm_ctx_t *ctx;
    FUNC_ENTRY();
    ctx = (classic_aes_gcm_ctx_t *)c->state;
    if (ctx) {
        mbedtls_gcm_free(&ctx->gcm);
        /* zeroize the key material */
        octet_string_set_to_zero(ctx, sizeof(classic_aes_gcm_ctx_t));
        srtp_crypto_free(ctx);
    }

    /* free memory */
    srtp_crypto_free(c);

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_aes_gcm_classic_context_init(void *cv,
                                                           const uint8_t *key)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *c = (classic_aes_gcm_ctx_t *)cv;
    int ret;
    c->dir = srtp_direction_any;
    c->aad_size = 0;

    debug_print(srtp_mod_aes_gcm, "key:  %s",
                srtp_octet_string_hex_string(key, c->key_size));

    switch (c->key_size) {
    case SRTP_AES_256_KEY_LEN:
    case SRTP_AES_128_KEY_LEN:
        break;
    default:
        return srtp_err_status_bad_param;
    }

    ret = mbedtls_gcm_setkey(&c->gcm, MBEDTLS_CIPHER_ID_AES, key,
                             (unsigned int)(c->key_size << 3));
    if (ret != 0) {
        debug_print(srtp_mod_aes_gcm, "setkey error: %d", ret);
        return srtp_err_status_init_fail;
    }

    return srtp_err_status_ok;
}

static srtp_err_status_t srtp_aes_gcm_classic_set_iv(
    void *cv,
    uint8_t *iv,
    srtp_cipher_direction_t direction)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *c = (classic_aes_gcm_ctx_t *)cv;

    if (direction != srtp_direction_encrypt &&
        direction != srtp_direction_decrypt) {
        return srtp_err_status_bad_param;
    }
    c->dir = direction;

    debug_print(srtp_mod_aes_gcm, "setting iv: %s",
                srtp_octet_string_hex_string(iv, GCM_IV_LEN));

    c->iv_len = GCM_IV_LEN;
    memcpy(c->iv, iv, c->iv_len);

    /* a new IV starts a new message; reset buffered AAD */
    c->aad_size = 0;

    return srtp_err_status_ok;
}

/*
 * This function processes the AAD
 *
 * Parameters:
 *	cv	Crypto context
 *	aad	Additional data to process for AEAD cipher suites
 *	aad_len	length of aad buffer
 */
static srtp_err_status_t srtp_aes_gcm_classic_set_aad(void *cv,
                                                      const uint8_t *aad,
                                                      size_t aad_len)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *c = (classic_aes_gcm_ctx_t *)cv;

    debug_print(srtp_mod_aes_gcm, "setting AAD: %s",
                srtp_octet_string_hex_string(aad, aad_len));

    if (aad_len + c->aad_size > MAX_AD_SIZE) {
        return srtp_err_status_bad_param;
    }

    memcpy(c->aad + c->aad_size, aad, aad_len);
    c->aad_size += aad_len;

    return srtp_err_status_ok;
}

/*
 * This function encrypts a buffer using AES GCM mode
 */
static srtp_err_status_t srtp_aes_gcm_classic_encrypt(void *cv,
                                                      const uint8_t *src,
                                                      size_t src_len,
                                                      uint8_t *dst,
                                                      size_t *dst_len)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *c = (classic_aes_gcm_ctx_t *)cv;
    int ret;

    if (c->dir != srtp_direction_encrypt) {
        return srtp_err_status_bad_param;
    }

    if (*dst_len < src_len + c->tag_len) {
        return srtp_err_status_buffer_small;
    }

    /* ciphertext is written to dst, the tag is appended right after it */
    ret = mbedtls_gcm_crypt_and_tag(&c->gcm, MBEDTLS_GCM_ENCRYPT, src_len,
                                    c->iv, c->iv_len, c->aad, c->aad_size, src,
                                    dst, c->tag_len, dst + src_len);

    c->aad_size = 0;
    if (ret != 0) {
        debug_print(srtp_mod_aes_gcm, "mbedtls error code:  %d", ret);
        return srtp_err_status_bad_param;
    }

    *dst_len = src_len + c->tag_len;

    return srtp_err_status_ok;
}

/*
 * This function decrypts a buffer using AES GCM mode
 */
static srtp_err_status_t srtp_aes_gcm_classic_decrypt(void *cv,
                                                      const uint8_t *src,
                                                      size_t src_len,
                                                      uint8_t *dst,
                                                      size_t *dst_len)
{
    FUNC_ENTRY();
    classic_aes_gcm_ctx_t *c = (classic_aes_gcm_ctx_t *)cv;
    int ret;
    size_t ct_len;

    if (c->dir != srtp_direction_decrypt) {
        return srtp_err_status_bad_param;
    }

    if (src_len < c->tag_len) {
        return srtp_err_status_bad_param;
    }

    if (*dst_len < src_len - c->tag_len) {
        return srtp_err_status_buffer_small;
    }

    debug_print(srtp_mod_aes_gcm, "AAD: %s",
                srtp_octet_string_hex_string(c->aad, c->aad_size));

    /* the tag is the last tag_len bytes of the input buffer */
    ct_len = src_len - c->tag_len;
    ret = mbedtls_gcm_auth_decrypt(&c->gcm, ct_len, c->iv, c->iv_len, c->aad,
                                   c->aad_size, src + ct_len, c->tag_len, src,
                                   dst);

    c->aad_size = 0;
    if (ret != 0) {
        debug_print(srtp_mod_aes_gcm, "mbedtls error code:  %d", ret);
        return srtp_err_status_auth_fail;
    }

    *dst_len = ct_len;

    return srtp_err_status_ok;
}
