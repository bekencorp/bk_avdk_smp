/**
 * AWS KVS / TLS CA bundle compiled when CONFIG_KVS_GET_CA_FROM_ARRAY is enabled.
 * PEM matches projects/kvs_aws_sample/ap/certs/cert.pem when kept in sync.
 */
#pragma once

#if CONFIG_KVS_GET_CA_FROM_ARRAY

extern const char kvs_embedded_ca_pem[];

#endif
