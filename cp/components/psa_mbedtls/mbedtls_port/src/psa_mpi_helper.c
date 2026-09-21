#include "common.h"

#if defined(MBEDTLS_PSA_CRYPTO_C)

#include "mbedtls/bignum.h"
#include "bignum_core.h"
#include "constant_time_internal.h"

int mbedtls_mpi_lt_mpi_ct(const mbedtls_mpi *X,
                          const mbedtls_mpi *Y,
                          unsigned *ret)
{
	mbedtls_ct_condition_t different_sign, X_is_negative, Y_is_negative, result;

	if (X->n != Y->n) {
		return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;
	}

	X_is_negative = mbedtls_ct_bool((X->s & 2) >> 1);
	Y_is_negative = mbedtls_ct_bool((Y->s & 2) >> 1);

	different_sign = mbedtls_ct_bool_ne(X_is_negative, Y_is_negative);
	result = mbedtls_ct_bool_and(different_sign, X_is_negative);

	{
		void * const p[2] = { X->p, Y->p };
		size_t i = mbedtls_ct_size_if_else_0(X_is_negative, 1);
		mbedtls_ct_condition_t lt = mbedtls_mpi_core_lt_ct(p[i], p[i ^ 1], X->n);

		result = mbedtls_ct_bool_or(result,
					    mbedtls_ct_bool_and(mbedtls_ct_bool_not(different_sign), lt));
	}

	*ret = mbedtls_ct_uint_if_else_0(result, 1);

	return 0;
}

#endif /* MBEDTLS_PSA_CRYPTO_C */
