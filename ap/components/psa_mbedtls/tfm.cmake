# NS PSA Crypto client is provided by the tfm component (tfm_crypto_api.c).
# psa_mbedtls must not compile a local PSA Crypto server or duplicate classic
# crypto sources for CONFIG_TFM_CRYPTO.
set(tfm_srcs)
