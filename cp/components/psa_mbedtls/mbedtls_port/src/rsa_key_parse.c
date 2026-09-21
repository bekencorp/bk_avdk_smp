/*
 * RSA PKCS#1 DER key parsing for PSA (TE RSA has no parse helpers).
 */
#include "common.h"

#if defined(MBEDTLS_RSA_C)

#include "mbedtls/rsa.h"
#include "rsa_internal.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"

#include <string.h>

static int asn1_get_nonzero_mpi(unsigned char **p,
                                const unsigned char *end,
                                mbedtls_mpi *X)
{
    int ret;

    ret = mbedtls_asn1_get_mpi(p, end, X);
    if (ret != 0) {
        return ret;
    }

    if (mbedtls_mpi_cmp_int(X, 0) == 0) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    return 0;
}

int mbedtls_rsa_parse_key(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen)
{
    int ret, version;
    size_t len;
    unsigned char *p, *end;

    mbedtls_mpi T;
    mbedtls_mpi_init(&T);

    p = (unsigned char *) key;
    end = p + keylen;

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) != 0) {
        return ret;
    }

    if (end != p + len) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    if ((ret = mbedtls_asn1_get_int(&p, end, &version)) != 0) {
        return ret;
    }

    if (version != 0) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, &T, NULL, NULL,
                                  NULL, NULL)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, NULL,
                                  NULL, &T)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, NULL,
                                  &T, NULL)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, &T, NULL,
                                  NULL, NULL)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_rsa_import(rsa, NULL, NULL, &T,
                                  NULL, NULL)) != 0) {
        goto cleanup;
    }

#if !defined(MBEDTLS_RSA_NO_CRT) && !defined(MBEDTLS_RSA_ALT)
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->DP, &T)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->DQ, &T)) != 0) {
        goto cleanup;
    }

    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = mbedtls_mpi_copy(&rsa->QP, &T)) != 0) {
        goto cleanup;
    }
#else
    if ((ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0 ||
        (ret = asn1_get_nonzero_mpi(&p, end, &T)) != 0) {
        goto cleanup;
    }
#endif

    if ((ret = mbedtls_rsa_complete(rsa)) != 0 ||
        (ret = mbedtls_rsa_check_pubkey(rsa)) != 0) {
        goto cleanup;
    }

    if (p != end) {
        ret = MBEDTLS_ERR_ASN1_LENGTH_MISMATCH;
    }

cleanup:

    mbedtls_mpi_free(&T);

    if (ret != 0) {
        mbedtls_rsa_free(rsa);
    }

    return ret;
}

int mbedtls_rsa_parse_pubkey(mbedtls_rsa_context *rsa, const unsigned char *key, size_t keylen)
{
    unsigned char *p = (unsigned char *) key;
    unsigned char *end = (unsigned char *) (key + keylen);
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len;

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len,
                                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) != 0) {
        return ret;
    }

    if (end != p + len) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER)) != 0) {
        return ret;
    }

    if ((ret = mbedtls_rsa_import_raw(rsa, p, len, NULL, 0, NULL, 0,
                                      NULL, 0, NULL, 0)) != 0) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    p += len;

    if ((ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER)) != 0) {
        return ret;
    }

    if ((ret = mbedtls_rsa_import_raw(rsa, NULL, 0, NULL, 0, NULL, 0,
                                      NULL, 0, p, len)) != 0) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    p += len;

    if (mbedtls_rsa_complete(rsa) != 0 ||
        mbedtls_rsa_check_pubkey(rsa) != 0) {
        return MBEDTLS_ERR_RSA_BAD_INPUT_DATA;
    }

    if (p != end) {
        return MBEDTLS_ERR_ASN1_LENGTH_MISMATCH;
    }

    return 0;
}

int mbedtls_rsa_write_key(const mbedtls_rsa_context *rsa, unsigned char *start,
                          unsigned char **p)
{
    size_t len = 0;
    int ret;

    mbedtls_mpi T;

    mbedtls_mpi_init(&T);

    if ((ret = mbedtls_rsa_export_crt(rsa, NULL, NULL, &T)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export_crt(rsa, NULL, &T, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export_crt(rsa, &T, NULL, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, &T, NULL, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, NULL, &T, NULL, NULL, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, &T, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, NULL, &T)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, &T, NULL, NULL, NULL, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

end_of_export:

    mbedtls_mpi_free(&T);
    if (ret < 0) {
        return ret;
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, 0));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                                                     MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE));

    return (int) len;
}

int mbedtls_rsa_write_pubkey(const mbedtls_rsa_context *rsa, unsigned char *start,
                             unsigned char **p)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    size_t len = 0;
    mbedtls_mpi T;

    mbedtls_mpi_init(&T);

    if ((ret = mbedtls_rsa_export(rsa, NULL, NULL, NULL, NULL, &T)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

    if ((ret = mbedtls_rsa_export(rsa, &T, NULL, NULL, NULL, NULL)) != 0 ||
        (ret = mbedtls_asn1_write_mpi(p, start, &T)) < 0) {
        goto end_of_export;
    }
    len += ret;

end_of_export:

    mbedtls_mpi_free(&T);
    if (ret < 0) {
        return ret;
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_CONSTRUCTED |
                                                     MBEDTLS_ASN1_SEQUENCE));

    return (int) len;
}

#endif /* MBEDTLS_RSA_C */
