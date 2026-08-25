// Copyright 2025-2026 Beken
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

#include "core_mqtt_tls.h"

#include <string.h>
#include <errno.h>

#include "lwip/sockets.h"

#include <components/log.h>
#include <os/os.h>
#include <os/str.h>
#include "sdkconfig.h"

#ifndef MBEDTLS_CONFIG_FILE
#define MBEDTLS_CONFIG_FILE "mbedtls_psa_crypto_config.h"
#endif

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"

#define TAG "core_mqtt_tls"
#define CORE_MQTT_TLS_IO_RETRY_MAX  5000U

typedef struct {
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	bool active;
} CoreMqttTlsContext_t;

static CoreMqttTlsContext_t s_tls;

static int core_mqtt_tls_bio_send(void *ctx, const unsigned char *buf, size_t len)
{
	int fd = (int)(intptr_t)ctx;
	int ret;

	if (fd < 0 || !buf || len == 0)
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;

	ret = (int)send(fd, buf, len, 0);
	if (ret >= 0)
		return ret;

	if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
		return MBEDTLS_ERR_SSL_WANT_WRITE;

	return MBEDTLS_ERR_NET_SEND_FAILED;
}

static int core_mqtt_tls_bio_recv(void *ctx, unsigned char *buf, size_t len)
{
	int fd = (int)(intptr_t)ctx;
	int ret;

	if (fd < 0 || !buf || len == 0)
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;

	ret = (int)recv(fd, buf, len, 0);
	if (ret > 0)
		return ret;
	if (ret == 0)
		return MBEDTLS_ERR_NET_CONN_RESET;

	if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
		return MBEDTLS_ERR_SSL_WANT_READ;
	if (errno == ETIMEDOUT)
		return MBEDTLS_ERR_SSL_TIMEOUT;

	return MBEDTLS_ERR_NET_RECV_FAILED;
}

bool core_mqtt_tls_is_active(void)
{
	return s_tls.active;
}

int core_mqtt_tls_setup(NetworkContext_t *ctx, const char *hostname)
{
	const char pers[] = "bk7259_core_mqtt";
	int ret;

	if (!ctx || ctx->socket < 0 || !hostname)
		return -1;

	if (s_tls.active)
		core_mqtt_tls_teardown();

	mbedtls_ssl_init(&s_tls.ssl);
	mbedtls_ssl_config_init(&s_tls.conf);
	mbedtls_entropy_init(&s_tls.entropy);
	mbedtls_ctr_drbg_init(&s_tls.ctr_drbg);

	ret = mbedtls_ctr_drbg_seed(&s_tls.ctr_drbg, mbedtls_entropy_func, &s_tls.entropy,
				    (const unsigned char *)pers, sizeof(pers) - 1U);
	if (ret != 0) {
		BK_LOGE(TAG, "ctr_drbg_seed failed -0x%04x\r\n", -ret);
		goto fail;
	}

	ret = mbedtls_ssl_config_defaults(&s_tls.conf, MBEDTLS_SSL_IS_CLIENT,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		BK_LOGE(TAG, "ssl_config_defaults failed -0x%04x\r\n", -ret);
		goto fail;
	}

#if CONFIG_COREMQTT_TLS_VERIFY_SERVER
	mbedtls_ssl_conf_authmode(&s_tls.conf, MBEDTLS_SSL_VERIFY_REQUIRED);
#else
	mbedtls_ssl_conf_authmode(&s_tls.conf, MBEDTLS_SSL_VERIFY_NONE);
#endif
	mbedtls_ssl_conf_rng(&s_tls.conf, mbedtls_ctr_drbg_random, &s_tls.ctr_drbg);
	/* Do not block mqtt_ssl_read(); non-blocking socket + WANT_READ polling. */
	mbedtls_ssl_conf_read_timeout(&s_tls.conf, 0);

	ret = mbedtls_ssl_setup(&s_tls.ssl, &s_tls.conf);
	if (ret != 0) {
		BK_LOGE(TAG, "ssl_setup failed -0x%04x\r\n", -ret);
		goto fail;
	}

	ret = mbedtls_ssl_set_hostname(&s_tls.ssl, hostname);
	if (ret != 0) {
		BK_LOGE(TAG, "ssl_set_hostname failed -0x%04x\r\n", -ret);
		goto fail;
	}

	mbedtls_ssl_set_bio(&s_tls.ssl, (void *)(intptr_t)ctx->socket,
			    core_mqtt_tls_bio_send, core_mqtt_tls_bio_recv, NULL);

	while ((ret = mbedtls_ssl_handshake(&s_tls.ssl)) != 0) {
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;
		{
			char errbuf[96];

			mbedtls_strerror(ret, errbuf, sizeof(errbuf));
			BK_LOGE(TAG, "ssl_handshake failed -0x%04x %s\r\n", -ret, errbuf);
		}
		goto fail;
	}

	s_tls.active = true;
	BK_LOGI(TAG, "TLS handshake ok host=%s cipher=%s\r\n",
		hostname, mbedtls_ssl_get_ciphersuite(&s_tls.ssl));
	return 0;

fail:
	core_mqtt_tls_teardown();
	return -1;
}

void core_mqtt_tls_teardown(void)
{
	if (s_tls.active) {
		(void)mbedtls_ssl_close_notify(&s_tls.ssl);
		s_tls.active = false;
	}

	mbedtls_ssl_free(&s_tls.ssl);
	mbedtls_ssl_config_free(&s_tls.conf);
	mbedtls_entropy_free(&s_tls.entropy);
	mbedtls_ctr_drbg_free(&s_tls.ctr_drbg);
}

int32_t core_mqtt_tls_send(NetworkContext_t *ctx, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t sent_total = 0;
	uint32_t retry = 0;
	int ret;

	(void)ctx;

	if (!s_tls.active || !buf || len == 0)
		return -1;

	while (sent_total < len) {
		ret = mbedtls_ssl_write(&s_tls.ssl, p + sent_total, len - sent_total);
		if (ret > 0) {
			sent_total += (size_t)ret;
			retry = 0;
			continue;
		}
		if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ) {
			if (++retry > CORE_MQTT_TLS_IO_RETRY_MAX) {
				BK_LOGE(TAG, "ssl_write retry exhausted sent=%u/%u\r\n",
					(unsigned)sent_total, (unsigned)len);
				return sent_total > 0 ? (int32_t)sent_total : -1;
			}
			rtos_delay_milliseconds(1);
			continue;
		}

		BK_LOGE(TAG, "ssl_write failed -0x%04x sent=%u/%u\r\n",
			-ret, (unsigned)sent_total, (unsigned)len);
		return sent_total > 0 ? (int32_t)sent_total : -1;
	}

	return (int32_t)sent_total;
}

int32_t core_mqtt_tls_recv(NetworkContext_t *ctx, void *buf, size_t len, bool blocking)
{
	int ret;

	(void)ctx;

	if (!s_tls.active || !buf || len == 0)
		return -1;

	ret = mbedtls_ssl_read(&s_tls.ssl, buf, len);
	if (ret > 0)
		return ret;
	if (ret == 0) {
		BK_LOGW(TAG, "TLS peer closed connection\r\n");
		return -1;
	}
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
		return 0;
	if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
		return 0;

	BK_LOGW(TAG, "ssl_read failed -0x%04x\r\n", -ret);
	return -1;
}
