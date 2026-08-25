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

#include "core_mqtt_transport.h"

#include <string.h>
#include <errno.h>

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#include <components/log.h>
#include <os/os.h>
#include <os/str.h>
#include "sdkconfig.h"

#if CONFIG_COREMQTT_TLS
#include "core_mqtt_tls.h"
#endif

#define TAG "core_mqtt_net"
#define CORE_MQTT_RECV_TIMEOUT_MS       200U
#define CORE_MQTT_CONNACK_TIMEOUT_MS    2000U
#define CORE_MQTT_TCP_CONNECT_TIMEOUT_MS 8000U
#define CORE_MQTT_WRITEV_BUF_SIZE       512U
#define CORE_MQTT_RX_STAGING_SIZE       512U
#define CORE_MQTT_TLS_IO_RETRY_MAX      5000U

static uint32_t s_tx_bytes;
static uint32_t s_rx_bytes;
static uint8_t s_rx_staging[CORE_MQTT_RX_STAGING_SIZE];
static size_t s_rx_staging_len;
static size_t s_rx_staging_pos;

static void core_mqtt_transport_rx_reset(void)
{
	s_rx_staging_len = 0;
	s_rx_staging_pos = 0;
}

static int core_mqtt_transport_set_nonblock(int sock, bool nonblock)
{
#if defined(FIONBIO)
	unsigned long flag = nonblock ? 1UL : 0UL;
	return ioctlsocket(sock, FIONBIO, &flag);
#else
	int flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0)
		return -1;
	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	return fcntl(sock, F_SETFL, flags);
#endif
}

static int core_mqtt_transport_apply_recv_timeout(int sock, uint32_t timeout_ms)
{
	struct timeval tv;

	if (timeout_ms == 0)
		return 0;

	tv.tv_sec = timeout_ms / 1000U;
	tv.tv_usec = (timeout_ms % 1000U) * 1000U;
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static bool core_mqtt_transport_wait_readable(int sock, uint32_t timeout_ms)
{
	fd_set rfds;
	struct timeval tv;
	int ret;

	if (sock < 0)
		return false;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	tv.tv_sec = timeout_ms / 1000U;
	tv.tv_usec = (timeout_ms % 1000U) * 1000U;
	ret = select(sock + 1, &rfds, NULL, NULL, &tv);
	return (ret > 0) && FD_ISSET(sock, &rfds);
}

static bool core_mqtt_transport_wait_writable(int sock, uint32_t timeout_ms)
{
	fd_set wfds;
	fd_set efds;
	struct timeval tv;
	int ret;

	if (sock < 0)
		return false;

	FD_ZERO(&wfds);
	FD_ZERO(&efds);
	FD_SET(sock, &wfds);
	FD_SET(sock, &efds);
	tv.tv_sec = timeout_ms / 1000U;
	tv.tv_usec = (timeout_ms % 1000U) * 1000U;
	ret = select(sock + 1, NULL, &wfds, &efds, &tv);
	if (ret <= 0)
		return false;
	if (FD_ISSET(sock, &efds))
		return false;

	return FD_ISSET(sock, &wfds);
}

static int core_mqtt_transport_tcp_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	int so_error = 0;
	socklen_t err_len = sizeof(so_error);
	int ret;

	if (core_mqtt_transport_set_nonblock(sock, true) != 0)
		return -1;

	ret = connect(sock, addr, addrlen);
	if (ret == 0)
		return 0;

	if (errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EALREADY) {
		BK_LOGE(TAG, "connect failed errno=%d\r\n", errno);
		return -1;
	}

	if (!core_mqtt_transport_wait_writable(sock, CORE_MQTT_TCP_CONNECT_TIMEOUT_MS)) {
		BK_LOGE(TAG, "connect timeout %ums\r\n", CORE_MQTT_TCP_CONNECT_TIMEOUT_MS);
		return -1;
	}

	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &err_len) != 0 || so_error != 0) {
		BK_LOGE(TAG, "connect failed so_error=%d\r\n", so_error);
		return -1;
	}

	return 0;
}

static int32_t core_mqtt_transport_send_all(NetworkContext_t *ctx, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t sent_total = 0;
	int32_t bytes;

	if (!ctx || ctx->socket < 0 || !buf || len == 0)
		return -1;

#if CONFIG_COREMQTT_TLS
	if (ctx->use_tls && core_mqtt_tls_is_active()) {
		uint32_t retry = 0;

		while (sent_total < len) {
			bytes = core_mqtt_tls_send(ctx, p + sent_total, len - sent_total);
			if (bytes > 0) {
				sent_total += (size_t)bytes;
				retry = 0;
				continue;
			}
			if (bytes == 0) {
				if (++retry > CORE_MQTT_TLS_IO_RETRY_MAX)
					break;
				rtos_delay_milliseconds(1);
				continue;
			}
			return bytes;
		}

		if (sent_total > 0)
			s_tx_bytes += (uint32_t)sent_total;
		return (int32_t)sent_total;
	}
#endif

	while (sent_total < len) {
		bytes = (int32_t)send(ctx->socket, p + sent_total, len - sent_total, 0);
		if (bytes > 0) {
			sent_total += (size_t)bytes;
			continue;
		}
		if (bytes == 0)
			return (int32_t)sent_total;

		if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
			if (sent_total > 0)
				break;
			return 0;
		}

		BK_LOGE(TAG, "send failed errno=%d sent=%u/%u\r\n",
			errno, (unsigned)sent_total, (unsigned)len);
		return -1;
	}

	s_tx_bytes += (uint32_t)sent_total;
	return (int32_t)sent_total;
}

int core_mqtt_transport_connect(NetworkContext_t *ctx, const char *host, uint16_t port,
				bool use_tls)
{
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	struct addrinfo *res = NULL;
	char port_str[8];
	char ip_str[16];
	int sock = -1;
	int ret;
	int opt = 1;

	if (!ctx || !host)
		return -1;

	core_mqtt_transport_disconnect(ctx);

#if CONFIG_COREMQTT_TLS
	ctx->use_tls = use_tls;
#else
	(void)use_tls;
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	os_snprintf(port_str, sizeof(port_str), "%u", port);
	ret = getaddrinfo(host, port_str, &hints, &result);
	if (ret != 0 || result == NULL) {
		BK_LOGE(TAG, "getaddrinfo failed: %s ret=%d\r\n", host, ret);
		return -1;
	}

	for (res = result; res != NULL; res = res->ai_next) {
		if (res->ai_family == AF_INET)
			break;
	}

	if (res == NULL) {
		BK_LOGE(TAG, "no IPv4 address for %s\r\n", host);
		freeaddrinfo(result);
		return -1;
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock < 0) {
		BK_LOGE(TAG, "socket failed\r\n");
		freeaddrinfo(result);
		return -1;
	}

	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

	os_strlcpy(ip_str,
		   inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr),
		   sizeof(ip_str));
	BK_LOGI(TAG, "tcp connect %s:%u ip=%s\r\n", host, port, ip_str);

	ret = core_mqtt_transport_tcp_connect(sock, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(result);
	if (ret < 0) {
		close(sock);
		return -1;
	}

	if (core_mqtt_transport_set_nonblock(sock, false) != 0) {
		BK_LOGE(TAG, "socket setup failed\r\n");
		close(sock);
		return -1;
	}

	ctx->socket = sock;
	ctx->blocking_recv = true;
	if (ctx->recv_timeout_ms == 0)
		ctx->recv_timeout_ms = CORE_MQTT_RECV_TIMEOUT_MS;
	core_mqtt_transport_rx_reset();
	s_tx_bytes = 0;
	s_rx_bytes = 0;

#if CONFIG_COREMQTT_TLS
	if (use_tls) {
		/* Keep recv blocking without SO_RCVTIMEO during TLS handshake. */
		core_mqtt_transport_apply_recv_timeout(sock, 0);
		if (core_mqtt_tls_setup(ctx, host) != 0) {
			close(sock);
			ctx->socket = -1;
			return -1;
		}
		core_mqtt_transport_apply_recv_timeout(sock, ctx->recv_timeout_ms);
		BK_LOGI(TAG, "MQTTS connected to %s:%u ip=%s socket=%d\r\n",
			host, port, ip_str, sock);
		return 0;
	}
#endif

	core_mqtt_transport_apply_recv_timeout(sock, ctx->recv_timeout_ms);

	BK_LOGI(TAG, "connected to %s:%u ip=%s socket=%d\r\n", host, port, ip_str, sock);
	return 0;
}

void core_mqtt_transport_set_connack_mode(NetworkContext_t *ctx)
{
	if (!ctx || ctx->socket < 0)
		return;

	ctx->blocking_recv = true;
	ctx->recv_timeout_ms = CORE_MQTT_CONNACK_TIMEOUT_MS;
	core_mqtt_transport_set_nonblock(ctx->socket, false);
	core_mqtt_transport_apply_recv_timeout(ctx->socket, ctx->recv_timeout_ms);
}

void core_mqtt_transport_set_nonblock_mode(NetworkContext_t *ctx, bool nonblock)
{
	if (!ctx || ctx->socket < 0)
		return;

	ctx->blocking_recv = !nonblock;
	if (core_mqtt_transport_set_nonblock(ctx->socket, nonblock) != 0)
		BK_LOGW(TAG, "set nonblock=%d failed\r\n", nonblock);

	if (nonblock) {
		core_mqtt_transport_apply_recv_timeout(ctx->socket, 0);
	} else {
		if (ctx->recv_timeout_ms == 0)
			ctx->recv_timeout_ms = CORE_MQTT_RECV_TIMEOUT_MS;
		core_mqtt_transport_apply_recv_timeout(ctx->socket, ctx->recv_timeout_ms);
	}
}

void core_mqtt_transport_abort(NetworkContext_t *ctx)
{
	int sock;

	if (!ctx)
		return;

#if CONFIG_COREMQTT_TLS
	if (ctx->use_tls)
		core_mqtt_tls_teardown();
#endif

	sock = ctx->socket;
	ctx->socket = -1;
	if (sock >= 0) {
		shutdown(sock, SHUT_RDWR);
		close(sock);
	}
}

void core_mqtt_transport_disconnect(NetworkContext_t *ctx)
{
	if (!ctx)
		return;

#if CONFIG_COREMQTT_TLS
	if (ctx->use_tls)
		core_mqtt_tls_teardown();
#endif

	if (ctx->socket >= 0) {
		close(ctx->socket);
		ctx->socket = -1;
	}
	ctx->blocking_recv = false;
	core_mqtt_transport_rx_reset();
}

int32_t core_mqtt_transport_recv(NetworkContext_t *ctx, void *buf, size_t len)
{
	uint8_t *out = buf;
	size_t copied = 0;
	int32_t bytes;

	if (!ctx || ctx->socket < 0 || !buf || len == 0)
		return -1;

	while (copied < len) {
		if (s_rx_staging_pos < s_rx_staging_len) {
			size_t avail = s_rx_staging_len - s_rx_staging_pos;
			size_t need = len - copied;
			size_t n = avail < need ? avail : need;

			memcpy(out + copied, s_rx_staging + s_rx_staging_pos, n);
			s_rx_staging_pos += n;
			copied += n;
			continue;
		}

		s_rx_staging_pos = 0;
		s_rx_staging_len = 0;

		if (ctx->blocking_recv) {
			if (!core_mqtt_transport_wait_readable(ctx->socket, ctx->recv_timeout_ms))
				break;
		}

#if CONFIG_COREMQTT_TLS
		if (ctx->use_tls && core_mqtt_tls_is_active()) {
			bytes = core_mqtt_tls_recv(ctx, s_rx_staging, sizeof(s_rx_staging),
						   ctx->blocking_recv);
		} else
#endif
		{
			bytes = (int32_t)recv(ctx->socket, s_rx_staging, sizeof(s_rx_staging),
					      ctx->blocking_recv ? 0 : MSG_DONTWAIT);
		}
		if (bytes > 0) {
			s_rx_staging_len = (size_t)bytes;
			s_rx_staging_pos = 0;
			continue;
		}
		if (bytes == 0)
			break;

		if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR ||
		    errno == ETIMEDOUT)
			break;

		BK_LOGW(TAG, "recv failed errno=%d\r\n", errno);
		return copied > 0 ? (int32_t)copied : -1;
	}

	if (copied > 0) {
		s_rx_bytes += (uint32_t)copied;
		return (int32_t)copied;
	}

	return 0;
}

int32_t core_mqtt_transport_send(NetworkContext_t *ctx, const void *buf, size_t len)
{
	return core_mqtt_transport_send_all(ctx, buf, len);
}

int32_t core_mqtt_transport_drain(NetworkContext_t *ctx, void *buf, size_t len)
{
	int32_t total = 0;
	int32_t bytes;

	if (!ctx || !buf || len == 0)
		return 0;

	while (total < (int32_t)len) {
		bytes = core_mqtt_transport_recv(ctx, (uint8_t *)buf + total, len - total);
		if (bytes > 0) {
			total += bytes;
			continue;
		}
		if (bytes == 0)
			break;
		return -1;
	}

	return total;
}

int32_t core_mqtt_transport_writev(NetworkContext_t *ctx, TransportOutVector_t *pIoVec,
				   size_t ioVecCount)
{
	uint8_t buf[CORE_MQTT_WRITEV_BUF_SIZE];
	size_t total = 0;
	size_t offset = 0;
	size_t i;

	if (!ctx || !pIoVec || ioVecCount == 0)
		return -1;

	for (i = 0; i < ioVecCount; i++)
		total += pIoVec[i].iov_len;

	if (total == 0 || total > sizeof(buf))
		return -1;

	for (i = 0; i < ioVecCount; i++) {
		memcpy(buf + offset, pIoVec[i].iov_base, pIoVec[i].iov_len);
		offset += pIoVec[i].iov_len;
	}

	return core_mqtt_transport_send_all(ctx, buf, total);
}

void core_mqtt_transport_get_stats(uint32_t *tx_bytes, uint32_t *rx_bytes)
{
	if (tx_bytes)
		*tx_bytes = s_tx_bytes;
	if (rx_bytes)
		*rx_bytes = s_rx_bytes;
}
