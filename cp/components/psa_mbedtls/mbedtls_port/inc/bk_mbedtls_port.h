#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Create the mbedtls/PSA global mutexes, safe to call more than once. */
void bk_mbedtls_threading_init(void);

#ifdef __cplusplus
}
#endif
