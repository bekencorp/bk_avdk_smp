// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cli.h"
#include "mbedtls_test.h"

#include <driver/aon_rtc.h>
#include <stdint.h>
#include <string.h>

#include "mbedtls/bignum.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "mbedtls/x509_crt.h"

#define MBEDTLS_TLS_RANDOM_LEN  32U
#define MBEDTLS_TLS_PARAMS_LEN  256U
#define MBEDTLS_TLS_KEY_BUF_LEN 256U

extern int bk_rand(void);

static const char s_tls_test_key_pem[] =
	"-----BEGIN PRIVATE KEY-----\r\n"
	"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgSmg4ZmkOnnfbkr6e\r\n"
	"J9olSiOQcI3In90o7Z5mc5jBNNKhRANCAARI/h6GgB26LzS91sy44FOL1Xk3Uw28\r\n"
	"e6DKlLtz57PH6qT1HriuQbsqyJUGtwaNS2GuTjBRtUfU4pxtR3x8QvAp\r\n"
	"-----END PRIVATE KEY-----\r\n";

static const char s_tls_test_chain_pem[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIB7zCCAZWgAwIBAgIEf5abBDAKBggqhkjOPQQDAjCBjjELMAkGA1UEBhMCVVMx\r\n"
	"EzARBgNVBAgMCldhc2hpbmd0b24xEDAOBgNVBAcMB1NlYXR0bGUxGDAWBgNVBAoM\r\n"
	"D0FtYXpvbi5jb20gSW5jLjEaMBgGA1UECwwRRGV2aWNlIE1hbmFnZW1lbnQxIjAg\r\n"
	"BgNVBAMMGURBSyBDQSBmb3IgQTEwMU5VVlZOUzc2MzQwHhcNMjAwNTIxMDQwODIy\r\n"
	"WhcNNDAwNTIxMDQwODIyWjAfMR0wGwYJKwYBBAGlawEDDA5BMTAxTlVWVk5TNzYz\r\n"
	"NDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABEj+HoaAHbovNL3WzLjgU4vVeTdT\r\n"
	"Dbx7oMqUu3Pns8fqpPUeuK5BuyrIlQa3Bo1LYa5OMFG1R9TinG1HfHxC8CmjTzBN\r\n"
	"MB0GA1UdDgQWBBRXoaVEB5rcaYsf3zF/SIesWIoHxTAfBgNVHSMEGDAWgBTPYP0y\r\n"
	"cVxp8p67ybjjw1nHIeXPzjALBgNVHQ8EBAMCA6gwCgYIKoZIzj0EAwIDSAAwRQIh\r\n"
	"AIEuXNXw+A8RIJrbGIFf1TzlQ0/mYplWKwFuC27jpWdGAiAInPm8uB9QaTM3Nr8z\r\n"
	"zp4O5vZQrCYMRFcvUreNIglTrw==\r\n"
	"-----END CERTIFICATE-----\r\n"
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDVDCCAvqgAwIBAgIOAMh0yNgkaQl0BGq/jbUwCgYIKoZIzj0EAwIwgaYxCzAJ\r\n"
	"BgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxl\r\n"
	"MRgwFgYDVQQKDA9BbWF6b24uY29tIEluYy4xGjAYBgNVBAsMEURldmljZSBNYW5h\r\n"
	"Z2VtZW50MTowOAYDVQQDDDFBbWF6b25ESEEgdHV5YV9Qcm9kdWN0X0ExMDFOVVZW\r\n"
	"TlM3NjM0IENlcnRpZmljYXRlMB4XDTIwMDQyOTE2MjU1NloXDTQwMDQyOTE2MjU1\r\n"
	"NlowgY4xCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQH\r\n"
	"DAdTZWF0dGxlMRgwFgYDVQQKDA9BbWF6b24uY29tIEluYy4xGjAYBgNVBAsMEURl\r\n"
	"dmljZSBNYW5hZ2VtZW50MSIwIAYDVQQDDBlEQUsgQ0EgZm9yIEExMDFOVVZWTlM3\r\n"
	"NjM0MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEiD3cM+GTfNsfcJSO1zHbafKe\r\n"
	"MbdsHCU5vHzdD3IURcteJgnvQqjSMce/A82EmfL8G6GcHgq63j1Cik7dYPL5qKOC\r\n"
	"ASAwggEcMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFM9g/TJxXGnynrvJ\r\n"
	"uOPDWcch5c/OMIHWBgNVHSMEgc4wgcuAFBKrjoX+A7DTDyRzm5td9WGz91TYoYGi\r\n"
	"pIGfMIGcMQswCQYDVQQGEwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UE\r\n"
	"BwwHU2VhdHRsZTEYMBYGA1UECgwPQW1hem9uLmNvbSBJbmMuMRowGAYDVQQLDBFE\r\n"
	"ZXZpY2UgTWFuYWdlbWVudDEwMC4GA1UEAwwnQW1hem9uREhBIFR1eWFfTWFudWZh\r\n"
	"Y3R1cmVyIENlcnRpZmljYXRlgg4AyHTIzjPtVZWdZb0jGDAOBgNVHQ8BAf8EBAMC\r\n"
	"AoQwCgYIKoZIzj0EAwIDSAAwRQIgBA7X5ncLg8Uwht+segzN0zSi2AwDpcOWIa4D\r\n"
	"9DcA3WoCIQCgsqkBjYr60zfyF0irOgJrlh+HgulKudfc/ESTSJs0aQ==\r\n"
	"-----END CERTIFICATE-----\r\n"
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDWDCCAv2gAwIBAgIOAMh0yM4z7VWVnWW9IxgwCgYIKoZIzj0EAwIwgZwxCzAJ\r\n"
	"BgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxl\r\n"
	"MRgwFgYDVQQKDA9BbWF6b24uY29tIEluYy4xGjAYBgNVBAsMEURldmljZSBNYW5h\r\n"
	"Z2VtZW50MTAwLgYDVQQDDCdBbWF6b25ESEEgVHV5YV9NYW51ZmFjdHVyZXIgQ2Vy\r\n"
	"dGlmaWNhdGUwHhcNMjAwNDI5MTYyNTUyWhcNNDAwNDI5MTYyNTUyWjCBpjELMAkG\r\n"
	"A1UEBhMCVVMxEzARBgNVBAgMCldhc2hpbmd0b24xEDAOBgNVBAcMB1NlYXR0bGUx\r\n"
	"GDAWBgNVBAoMD0FtYXpvbi5jb20gSW5jLjEaMBgGA1UECwwRRGV2aWNlIE1hbmFn\r\n"
	"ZW1lbnQxOjA4BgNVBAMMMUFtYXpvbkRIQSB0dXlhX1Byb2R1Y3RfQTEwMU5VVlZO\r\n"
	"Uzc2MzQgQ2VydGlmaWNhdGUwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQPXnAv\r\n"
	"MfkLIPa36y1tytRiQnM6EML+KAS4yuDjN+WiqsjzAbmmrJISd0FOdaiKZLRy1CqQ\r\n"
	"8LPlnLeWN+ez75N6o4IBFTCCAREwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQU\r\n"
	"EquOhf4DsNMPJHObm131YbP3VNgwgc4GA1UdIwSBxjCBw4AU8RzI5oQ+OsSvsUub\r\n"
	"ORRuOEETp/ShgZqkgZcwgZQxFDASBgNVBAoMC0FtYXpvbiBJbmMuMSMwIQYDVQQL\r\n"
	"DBpEZXZpY2UgTWFuYWdlbWVudCBTZXJ2aWNlczEQMA4GA1UEBwwHU2VhdHRsZTET\r\n"
	"MBEGA1UECAwKV2FzaGluZ3RvbjELMAkGA1UEBhMCVVMxIzAhBgNVBAMMGkFtYXpv\r\n"
	"bkRIQSBSb290IENlcnRpZmljYXRlgg4AxujSrs5VP8cNqAB3ZjAOBgNVHQ8BAf8E\r\n"
	"BAMCAYYwCgYIKoZIzj0EAwIDSQAwRgIhALak4BjOMpaXW+ZnrMnfJQMfhyfDCw4r\r\n"
	"n2k6AlIJOb6KAiEAtZd1Xbp/qn7h16tosTdbeJCO9AYBURUDpiR1f6tKI4Q=\r\n"
	"-----END CERTIFICATE-----\r\n"
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDQDCCAuagAwIBAgIOAMbo0q7OVT/HDagAd2YwCgYIKoZIzj0EAwIwgZQxFDAS\r\n"
	"BgNVBAoMC0FtYXpvbiBJbmMuMSMwIQYDVQQLDBpEZXZpY2UgTWFuYWdlbWVudCBT\r\n"
	"ZXJ2aWNlczEQMA4GA1UEBwwHU2VhdHRsZTETMBEGA1UECAwKV2FzaGluZ3RvbjEL\r\n"
	"MAkGA1UEBhMCVVMxIzAhBgNVBAMMGkFtYXpvbkRIQSBSb290IENlcnRpZmljYXRl\r\n"
	"MB4XDTE5MTIwOTIwMjU0MFoXDTM5MTIwOTIwMjU0MFowgZwxCzAJBgNVBAYTAlVT\r\n"
	"MRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxlMRgwFgYDVQQK\r\n"
	"DA9BbWF6b24uY29tIEluYy4xGjAYBgNVBAsMEURldmljZSBNYW5hZ2VtZW50MTAw\r\n"
	"LgYDVQQDDCdBbWF6b25ESEEgVHV5YV9NYW51ZmFjdHVyZXIgQ2VydGlmaWNhdGUw\r\n"
	"WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARBmDa+6dYNG2/KBpLv08bn6e1lmoKf\r\n"
	"1hv0e8W3tQW97u1kFpiq3BF1MaQqrAiV0vqFItWOeWCXwV7oy3hz+6Q7o4IBEDCC\r\n"
	"AQwwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQU8RzI5oQ+OsSvsUubORRuOEET\r\n"
	"p/QwgckGA1UdIwSBwTCBvoAUYYvcTyRKpW7uCmW8vHSaUZXqq2ChgZqkgZcwgZQx\r\n"
	"FDASBgNVBAoMC0FtYXpvbiBJbmMuMSMwIQYDVQQLDBpEZXZpY2UgTWFuYWdlbWVu\r\n"
	"dCBTZXJ2aWNlczEQMA4GA1UEBwwHU2VhdHRsZTETMBEGA1UECAwKV2FzaGluZ3Rv\r\n"
	"bjELMAkGA1UEBhMCVVMxIzAhBgNVBAMMGkFtYXpvbkRIQSBSb290IENlcnRpZmlj\r\n"
	"YXRlggkAqkHFg3IfxuIwDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMCA0gAMEUC\r\n"
	"IQDnRZkmmDemoO+Mk7yutOIpp0mF2Cj4+1Y35uHLnobBfQIgOuZdBZK4Pab8TD2g\r\n"
	"9TzvlsmQkQqm1VTNyqwykH0o+5g=\r\n"
	"-----END CERTIFICATE-----\r\n"
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDNjCCAtugAwIBAgIJAKpBxYNyH8biMAoGCCqGSM49BAMCMIGUMRQwEgYDVQQK\r\n"
	"DAtBbWF6b24gSW5jLjEjMCEGA1UECwwaRGV2aWNlIE1hbmFnZW1lbnQgU2VydmljZXMxEDAOBgNVBAcMB1NlYXR0bGUxEzARBgNVBAgMCldhc2hpbmd0b24xCzAJBgNV\r\n"
	"BAYTAlVTMSMwIQYDVQQDDBpBbWF6b25ESEEgUm9vdCBDZXJ0aWZpY2F0ZTAgFw0x\r\n"
	"NzExMTYwNjI3MDlaGA8yMTE3MTEwODA2MjcwOVowgZQxFDASBgNVBAoMC0FtYXpv\r\n"
	"biBJbmMuMSMwIQYDVQQLDBpEZXZpY2UgTWFuYWdlbWVudCBTZXJ2aWNlczEQMA4G\r\n"
	"A1UEBwwHU2VhdHRsZTETMBEGA1UECAwKV2FzaGluZ3RvbjELMAkGA1UEBhMCVVMx\r\n"
	"IzAhBgNVBAMMGkFtYXpvbkRIQSBSb290IENlcnRpZmljYXRlMFkwEwYHKoZIzj0C\r\n"
	"AQYIKoZIzj0DAQcDQgAEv7+Zfvkc+qvUaKgaGxQMZoDFHQ18Z5OSXB4BNkYRszgR\r\n"
	"PQ82o54KVb0RKkkq0e3niRn1gUZ8jozePEPrpPV5yaOCARAwggEMMA8GA1UdEwEB\r\n"
	"/wQFMAMBAf8wHQYDVR0OBBYEFGGL3E8kSqVu7gplvLx0mlGV6qtgMIHJBgNVHSME\r\n"
	"gcEwgb6AFGGL3E8kSqVu7gplvLx0mlGV6qtgoYGapIGXMIGUMRQwEgYDVQQKDAtB\r\n"
	"bWF6b24gSW5jLjEjMCEGA1UECwwaRGV2aWNlIE1hbmFnZW1lbnQgU2VydmljZXMx\r\n"
	"EDAOBgNVBAcMB1NlYXR0bGUxEzARBgNVBAgMCldhc2hpbmd0b24xCzAJBgNVBAYT\r\n"
	"AlVTMSMwIQYDVQQDDBpBbWF6b25ESEEgUm9vdCBDZXJ0aWZpY2F0ZYIJAKpBxYNy\r\n"
	"H8biMA4GA1UdDwEB/wQEAwIBhjAKBggqhkjOPQQDAgNJADBGAiEAoCM4t1cMuTeu\r\n"
	"8yIlw/1BUIUb1Q4MYXp+LyfjcbmVz8ECIQDuKW8gQZjUS8Z7GcgNYnFup3UjTznj\r\n"
	"5ja4/PmvDx0Glw==\r\n"
	"-----END CERTIFICATE-----\r\n";

static int mbedtls_tls_rng(void *rng_state, unsigned char *output, size_t len)
{
	uint32_t rnd = 0;

	(void)rng_state;
	if (output == NULL) {
		return -1;
	}

	for (size_t i = 0; i < len; i++) {
		if ((i & 0x3U) == 0U) {
			rnd = (uint32_t)bk_rand();
		}
		output[i] = (unsigned char)((rnd >> ((i & 0x3U) * 8U)) & 0xFFU);
	}

	return 0;
}

static double mbedtls_tls_perf_ops(uint32_t loops, uint64_t elapsed_us)
{
	if (elapsed_us == 0U) {
		return 0.0;
	}

	return ((double)loops * 1000000.0) / (double)elapsed_us;
}

static void mbedtls_tls_fill_pattern(unsigned char *buf, size_t len)
{
	if (buf == NULL) {
		return;
	}

	for (size_t i = 0; i < len; i++) {
		buf[i] = (unsigned char)(i & 0xFFU);
	}
}

static void mbedtls_tls_fill_increment(unsigned char *buf, size_t len)
{
	if (buf == NULL) {
		return;
	}

	for (size_t i = 0; i < len; i++) {
		buf[i] = (unsigned char)i;
	}
}

int mbedtls_tls_server_certificate_test(uint32_t loops)
{
	mbedtls_x509_crt cacert;
	mbedtls_x509_crt srvcert;
	uint32_t flags = 0;
	uint64_t start_us;
	uint64_t elapsed_us;
	uint64_t parse_us = 0;
	uint64_t verify_us = 0;
	int ret = 0;

	if (loops == 0U) {
		loops = 1U;
	}

	mbedtls_x509_crt_init(&cacert);
	mbedtls_x509_crt_init(&srvcert);

	ret = mbedtls_x509_crt_parse(&cacert,
								 (const unsigned char *)s_tls_test_chain_pem,
								 sizeof(s_tls_test_chain_pem));
	if (ret != 0) {
		goto cleanup;
	}

	start_us = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < loops; i++) {
		uint64_t step_us;

		mbedtls_x509_crt_free(&srvcert);
		mbedtls_x509_crt_init(&srvcert);
		flags = 0;

		step_us = bk_aon_rtc_get_us();
		ret = mbedtls_x509_crt_parse(&srvcert,
									 (const unsigned char *)s_tls_test_chain_pem,
									 sizeof(s_tls_test_chain_pem));
		parse_us += bk_aon_rtc_get_us() - step_us;
		if (ret != 0) {
			goto cleanup;
		}

		step_us = bk_aon_rtc_get_us();
		ret = mbedtls_x509_crt_verify(&srvcert, &cacert, NULL, NULL, &flags, NULL, NULL);
		verify_us += bk_aon_rtc_get_us() - step_us;
		if (ret != 0) {
			goto cleanup;
		}
	}
	elapsed_us = bk_aon_rtc_get_us() - start_us;

	CLI_LOGD("tls_server_certificate loops=%u elapsed_us=%llu rate=%.2fops/s\r\n",
			 (unsigned int)loops,
			 (unsigned long long)elapsed_us,
			 mbedtls_tls_perf_ops(loops, elapsed_us));
	CLI_LOGD("tls_server_certificate_sub loops=%u parse_us=%llu verify_us=%llu\r\n",
			 (unsigned int)loops,
			 (unsigned long long)parse_us,
			 (unsigned long long)verify_us);

cleanup:
	if ((ret != 0) && (flags != 0U)) {
		char vrfy_buf[192];

		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", flags);
		CLI_LOGE("tls_server_certificate flags=0x%08x %s\r\n",
				 (unsigned int)flags,
				 vrfy_buf);
	}

	mbedtls_x509_crt_free(&srvcert);
	mbedtls_x509_crt_free(&cacert);
	return ret;
}

int mbedtls_tls_server_key_exchange_test(uint32_t loops)
{
	mbedtls_x509_crt srvcert;
	mbedtls_pk_context sign_key;
	mbedtls_ecdh_context server_ecdh;
	unsigned char client_random[MBEDTLS_TLS_RANDOM_LEN];
	unsigned char server_random[MBEDTLS_TLS_RANDOM_LEN];
	unsigned char params[MBEDTLS_TLS_PARAMS_LEN];
	unsigned char signed_data[(MBEDTLS_TLS_RANDOM_LEN * 2U) + MBEDTLS_TLS_PARAMS_LEN];
	unsigned char hash[32];
	unsigned char sig[MBEDTLS_PK_SIGNATURE_MAX_SIZE];
	size_t params_len = 0;
	size_t sig_len = 0;
	uint64_t start_us;
	uint64_t elapsed_us;
	uint64_t params_parse_us = 0;
	uint64_t hash_us = 0;
	uint64_t sig_verify_us = 0;
	int ret = 0;

	if (loops == 0U) {
		loops = 1U;
	}

	mbedtls_x509_crt_init(&srvcert);
	mbedtls_pk_init(&sign_key);
	mbedtls_ecdh_init(&server_ecdh);
	memset(sig, 0, sizeof(sig));
	mbedtls_tls_fill_increment(client_random, sizeof(client_random));
	mbedtls_tls_fill_pattern(server_random, sizeof(server_random));

	ret = mbedtls_x509_crt_parse(&srvcert,
								 (const unsigned char *)s_tls_test_chain_pem,
								 sizeof(s_tls_test_chain_pem));
	if (ret != 0) {
		goto cleanup;
	}

	ret = mbedtls_pk_parse_key(&sign_key,
							   (const unsigned char *)s_tls_test_key_pem,
							   sizeof(s_tls_test_key_pem),
							   NULL,
							   0,
							   mbedtls_tls_rng,
							   NULL);
	if (ret != 0) {
		goto cleanup;
	}

	ret = mbedtls_ecdh_setup(&server_ecdh, MBEDTLS_ECP_DP_SECP256R1);
	if (ret != 0) {
		goto cleanup;
	}

	ret = mbedtls_ecdh_make_params(&server_ecdh,
								   &params_len,
								   params,
								   sizeof(params),
								   mbedtls_tls_rng,
								   NULL);
	if (ret != 0) {
		goto cleanup;
	}

	memcpy(signed_data, client_random, sizeof(client_random));
	memcpy(signed_data + sizeof(client_random), server_random, sizeof(server_random));
	memcpy(signed_data + sizeof(client_random) + sizeof(server_random), params, params_len);

	ret = mbedtls_sha256(signed_data, (sizeof(client_random) + sizeof(server_random) + params_len), hash, 0);
	if (ret != 0) {
		goto cleanup;
	}

	ret = mbedtls_pk_sign(&sign_key,
						  MBEDTLS_MD_SHA256,
						  hash,
						  0,
						  sig,
						  sizeof(sig),
						  &sig_len,
						  mbedtls_tls_rng,
						  NULL);
	if (ret != 0) {
		goto cleanup;
	}

	start_us = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < loops; i++) {
		mbedtls_ecdh_context client_ecdh;
		const unsigned char *p = params;
		uint64_t step_us;

		mbedtls_ecdh_init(&client_ecdh);

		step_us = bk_aon_rtc_get_us();
		ret = mbedtls_ecdh_read_params(&client_ecdh, &p, params + params_len);
		params_parse_us += bk_aon_rtc_get_us() - step_us;
		if ((ret == 0) && (p != (params + params_len))) {
			ret = -1;
		}
		if (ret == 0) {
			step_us = bk_aon_rtc_get_us();
			ret = mbedtls_sha256(signed_data,
								 (sizeof(client_random) + sizeof(server_random) + params_len),
								 hash,
								 0);
			hash_us += bk_aon_rtc_get_us() - step_us;
		}
		if (ret == 0) {
			step_us = bk_aon_rtc_get_us();
			ret = mbedtls_pk_verify(&srvcert.pk, MBEDTLS_MD_SHA256, hash, 0, sig, sig_len);
			sig_verify_us += bk_aon_rtc_get_us() - step_us;
		}

		mbedtls_ecdh_free(&client_ecdh);
		if (ret != 0) {
			goto cleanup;
		}
	}
	elapsed_us = bk_aon_rtc_get_us() - start_us;

	CLI_LOGD("tls_server_key_exchange loops=%u elapsed_us=%llu rate=%.2fops/s\r\n",
			 (unsigned int)loops,
			 (unsigned long long)elapsed_us,
			 mbedtls_tls_perf_ops(loops, elapsed_us));
	CLI_LOGD("tls_server_key_exchange_sub loops=%u params_parse_us=%llu hash_us=%llu sig_verify_us=%llu\r\n",
			 (unsigned int)loops,
			 (unsigned long long)params_parse_us,
			 (unsigned long long)hash_us,
			 (unsigned long long)sig_verify_us);

cleanup:
	mbedtls_ecdh_free(&server_ecdh);
	mbedtls_pk_free(&sign_key);
	mbedtls_x509_crt_free(&srvcert);
	return ret;
}

int mbedtls_tls_client_key_exchange_test(uint32_t loops)
{
	mbedtls_ecp_keypair server_key;
	unsigned char client_pub[MBEDTLS_TLS_KEY_BUF_LEN];
	unsigned char client_secret[MBEDTLS_MPI_MAX_SIZE];
	unsigned char server_secret[MBEDTLS_MPI_MAX_SIZE];
	size_t client_pub_len = 0;
	size_t client_secret_len = 0;
	size_t server_secret_len = 0;
	uint64_t start_us;
	uint64_t elapsed_us;
	uint64_t local_make_pub_us = 0;
	uint64_t local_secret_us = 0;
	int ret = 0;

	if (loops == 0U) {
		loops = 1U;
	}

	mbedtls_ecp_keypair_init(&server_key);
	memset(client_pub, 0, sizeof(client_pub));
	memset(client_secret, 0, sizeof(client_secret));
	memset(server_secret, 0, sizeof(server_secret));

	ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &server_key, mbedtls_tls_rng, NULL);
	if (ret != 0) {
		goto cleanup;
	}

	start_us = bk_aon_rtc_get_us();
	for (uint32_t i = 0; i < loops; i++) {
		mbedtls_ecdh_context client_ecdh;
		uint64_t step_us;

		mbedtls_ecdh_init(&client_ecdh);

		ret = mbedtls_ecdh_get_params(&client_ecdh, &server_key, MBEDTLS_ECDH_THEIRS);
		if (ret == 0) {
			step_us = bk_aon_rtc_get_us();
			ret = mbedtls_ecdh_make_public(&client_ecdh,
										   &client_pub_len,
										   client_pub,
										   sizeof(client_pub),
										   mbedtls_tls_rng,
										   NULL);
			local_make_pub_us += bk_aon_rtc_get_us() - step_us;
		}
		if (ret == 0) {
			step_us = bk_aon_rtc_get_us();
			ret = mbedtls_ecdh_calc_secret(&client_ecdh,
										   &client_secret_len,
										   client_secret,
										   sizeof(client_secret),
										   mbedtls_tls_rng,
										   NULL);
			local_secret_us += bk_aon_rtc_get_us() - step_us;
		}

		mbedtls_ecdh_free(&client_ecdh);
		if (ret != 0) {
			goto cleanup;
		}
	}
	elapsed_us = bk_aon_rtc_get_us() - start_us;

	CLI_LOGD("tls_client_key_exchange loops=%u elapsed_us=%llu rate=%.2fops/s\r\n",
			 (unsigned int)loops,
			 (unsigned long long)elapsed_us,
			 mbedtls_tls_perf_ops(loops, elapsed_us));
	CLI_LOGD("tls_client_key_exchange_sub loops=%u local_make_pub_us=%llu local_secret_us=%llu\r\n",
			 (unsigned int)loops,
			 (unsigned long long)local_make_pub_us,
			 (unsigned long long)local_secret_us);

	if ((ret == 0) && (client_pub_len > 0U)) {
		mbedtls_ecdh_context server_ecdh;

		mbedtls_ecdh_init(&server_ecdh);
		ret = mbedtls_ecdh_get_params(&server_ecdh, &server_key, MBEDTLS_ECDH_OURS);
		if (ret == 0) {
			ret = mbedtls_ecdh_read_public(&server_ecdh, client_pub, client_pub_len);
		}
		if (ret == 0) {
			ret = mbedtls_ecdh_calc_secret(&server_ecdh,
										   &server_secret_len,
										   server_secret,
										   sizeof(server_secret),
										   mbedtls_tls_rng,
										   NULL);
		}
		if ((ret == 0) &&
			((server_secret_len != client_secret_len) ||
			 (memcmp(server_secret, client_secret, client_secret_len) != 0))) {
			ret = -1;
		}
		mbedtls_ecdh_free(&server_ecdh);
	}

cleanup:
	mbedtls_ecp_keypair_free(&server_key);
	return ret;
}
