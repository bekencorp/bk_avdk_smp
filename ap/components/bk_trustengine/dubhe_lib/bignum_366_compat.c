/*
 * Mbed TLS 3.6 bignum APIs used by rsa_alt_helpers, for use with dubhe_lib/bignum.c.
 * Copied from mbedtls/library/bignum.c (3.6.6).
 */
#include "common.h"

#if defined(MBEDTLS_BIGNUM_C)

#include "mbedtls/bignum.h"
#include "bignum_core.h"
#include "mbedtls/constant_time.h"
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"
#include <string.h>

extern int mbedtls_mpi_grow(mbedtls_mpi *X, size_t nblimbs);

#define ciL    (sizeof(mbedtls_mpi_uint))

int mbedtls_mpi_gcd_modinv_odd(mbedtls_mpi *G,
                               mbedtls_mpi *I,
                               const mbedtls_mpi *A,
                               const mbedtls_mpi *N)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi local_g;
    mbedtls_mpi_uint *T = NULL;
    const size_t T_factor = I != NULL ? 5 : 4;
    const mbedtls_mpi_uint zero = 0;

    if (mbedtls_mpi_cmp_int(A, 0) < 0 ||
        mbedtls_mpi_cmp_mpi(A, N) > 0 ||
        mbedtls_mpi_get_bit(N, 0) != 1 ||
        (I != NULL && mbedtls_mpi_cmp_int(N, 1) == 0)) {
        return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
    }

    if (A == N || (I != NULL && (I == N || G == N))) {
        return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
    }

    mbedtls_mpi_init(&local_g);

    if (G == NULL) {
        G = &local_g;
    }

    MBEDTLS_MPI_CHK(mbedtls_mpi_grow(G, N->n));
    if (I != NULL) {
        MBEDTLS_MPI_CHK(mbedtls_mpi_grow(I, N->n));
    }

    T = mbedtls_calloc(sizeof(mbedtls_mpi_uint) * N->n, T_factor);
    if (T == NULL) {
        ret = MBEDTLS_ERR_MPI_ALLOC_FAILED;
        goto cleanup;
    }

    mbedtls_mpi_uint *Ip = I != NULL ? I->p : NULL;
    const mbedtls_mpi_uint *Ap = A->p != NULL ? A->p : &zero;
    size_t An = A->n >= N->n ? N->n : A->p != NULL ? A->n : 1;
    mbedtls_mpi_core_gcd_modinv_odd(G->p, Ip, Ap, An, N->p, N->n, T);

    G->s = 1;
    if (I != NULL) {
        I->s = 1;
    }

    if (G->n > N->n) {
        memset(G->p + N->n, 0, ciL * (G->n - N->n));
    }
    if (I != NULL && I->n > N->n) {
        memset(I->p + N->n, 0, ciL * (I->n - N->n));
    }

cleanup:
    mbedtls_mpi_free(&local_g);
    mbedtls_free(T);
    return ret;
}

int mbedtls_mpi_inv_mod_odd(mbedtls_mpi *X,
                            const mbedtls_mpi *A,
                            const mbedtls_mpi *N)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi T, G;

    mbedtls_mpi_init(&T);
    mbedtls_mpi_init(&G);

    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&T, A, N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(&G, &T, &T, N));
    if (mbedtls_mpi_cmp_int(&G, 1) != 0) {
        ret = MBEDTLS_ERR_MPI_NOT_ACCEPTABLE;
        goto cleanup;
    }

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(X, &T));

cleanup:
    mbedtls_mpi_free(&T);
    mbedtls_mpi_free(&G);

    return ret;
}

int mbedtls_mpi_inv_mod_even_in_range(mbedtls_mpi *X,
                                      mbedtls_mpi const *A,
                                      mbedtls_mpi const *N)
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_mpi I, G;

    mbedtls_mpi_init(&I);
    mbedtls_mpi_init(&G);

    MBEDTLS_MPI_CHK(mbedtls_mpi_mod_mpi(&I, N, A));
    MBEDTLS_MPI_CHK(mbedtls_mpi_gcd_modinv_odd(&G, &I, &I, A));
    if (mbedtls_mpi_cmp_int(&G, 1) != 0) {
        ret = MBEDTLS_ERR_MPI_NOT_ACCEPTABLE;
        goto cleanup;
    }

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&I, &I, N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_int(&I, &I, 1));
    MBEDTLS_MPI_CHK(mbedtls_mpi_div_mpi(&G, NULL, &I, A));
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(X, N, &G));

cleanup:
    mbedtls_mpi_free(&I);
    mbedtls_mpi_free(&G);
    return ret;
}

#endif /* MBEDTLS_BIGNUM_C */
