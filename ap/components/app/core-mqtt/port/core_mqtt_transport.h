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

#ifndef CORE_MQTT_TRANSPORT_H
#define CORE_MQTT_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "transport_interface.h"

typedef struct NetworkContext {
	int socket;
	bool blocking_recv;
	uint32_t recv_timeout_ms;
#if CONFIG_COREMQTT_TLS
	bool use_tls;
#endif
} NetworkContext_t;

int core_mqtt_transport_connect(NetworkContext_t *ctx, const char *host, uint16_t port,
				bool use_tls);
void core_mqtt_transport_set_connack_mode(NetworkContext_t *ctx);
void core_mqtt_transport_set_nonblock_mode(NetworkContext_t *ctx, bool nonblock);
void core_mqtt_transport_abort(NetworkContext_t *ctx);
void core_mqtt_transport_disconnect(NetworkContext_t *ctx);
int32_t core_mqtt_transport_recv(NetworkContext_t *ctx, void *buf, size_t len);
int32_t core_mqtt_transport_send(NetworkContext_t *ctx, const void *buf, size_t len);
int32_t core_mqtt_transport_writev(NetworkContext_t *ctx, TransportOutVector_t *pIoVec,
				     size_t ioVecCount);
int32_t core_mqtt_transport_drain(NetworkContext_t *ctx, void *buf, size_t len);
void core_mqtt_transport_get_stats(uint32_t *tx_bytes, uint32_t *rx_bytes);

#endif /* CORE_MQTT_TRANSPORT_H */
