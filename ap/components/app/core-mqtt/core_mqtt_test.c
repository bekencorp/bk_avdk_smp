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

#include <string.h>
#include <stdio.h>

#include "core_mqtt.h"
#include "core_mqtt_transport.h"
#include "core_mqtt_test.h"

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/system.h>
#include <components/event.h>
#include <components/netif.h>
#include <modules/wifi_types.h>
#include "common/bk_err.h"
#include "sdkconfig.h"

#define TAG "core_mqtt"

#define CORE_MQTT_DEFAULT_PORT        1883
#define CORE_MQTT_DEFAULT_TLS_PORT    8883
#define CORE_MQTT_CLIENT_ID_SIZE      32
#define CORE_MQTT_HOST_SIZE           128
#define CORE_MQTT_CONNECT_TIMEOUT_MS  15000U
#define CORE_MQTT_ACK_PROPS_BUF_SIZE  128U
#define CORE_MQTT_PROCESS_LOOP_DELAY_MS     20U
#define CORE_MQTT_DESTROY_WAIT_MS     3000U
#define CORE_MQTT_RECONNECT_DELAY_MS  5000U
#define CORE_MQTT_NETWORK_POLL_MS     100U

static MQTTContext_t s_mqtt_ctx;
static NetworkContext_t s_net_ctx = { .socket = -1, .blocking_recv = false, .recv_timeout_ms = 200 };
static TransportInterface_t s_transport;
static MQTTFixedBuffer_t s_fixed_buffer;
static uint8_t s_network_buffer[CONFIG_COREMQTT_NETWORK_BUFFER_SIZE];
static MQTTPubAckInfo_t s_outgoing_publishes[CONFIG_COREMQTT_MAX_INFLIGHT];
static MQTTPubAckInfo_t s_incoming_publishes[CONFIG_COREMQTT_MAX_INFLIGHT];
static uint8_t s_ack_props_buf[CORE_MQTT_ACK_PROPS_BUF_SIZE];

static char s_client_id[CORE_MQTT_CLIENT_ID_SIZE];
static char s_host[CORE_MQTT_HOST_SIZE];
static uint16_t s_port = CORE_MQTT_DEFAULT_PORT;
static bool s_use_tls = false;
static char *s_sub_topic = NULL;
static char *s_pub_topic = NULL;
static char *s_username = NULL;
static char *s_password = NULL;

static beken_mutex_t s_mqtt_mutex = NULL;
static beken_thread_t s_mqtt_thread = NULL;
static volatile int s_mqtt_started = 0;
static volatile int s_mqtt_connected = 0;
static volatile int s_mqtt_running = 0;
static volatile int s_wifi_has_ip = 0;
static volatile int s_wifi_cb_registered = 0;

static void core_mqtt_sync_wifi_ip_state(void)
{
	netif_ip4_config_t ip4 = { 0 };

	if (bk_netif_get_ip4_config(NETIF_IF_STA, &ip4) == BK_OK &&
	    ip4.ip[0] != '\0' && os_strcmp(ip4.ip, "0.0.0.0") != 0) {
		s_wifi_has_ip = 1;
	} else {
		s_wifi_has_ip = 0;
	}
}

static bk_err_t core_mqtt_wifi_event_cb(void *arg, event_module_t event_module,
					int event_id, void *event_data)
{
	(void)arg;

	if (event_module == EVENT_MOD_WIFI &&
	    event_id == EVENT_WIFI_STA_DISCONNECTED) {
		s_wifi_has_ip = 0;
	} else if (event_module == EVENT_MOD_NETIF &&
		   event_id == EVENT_NETIF_GOT_IP4) {
		netif_event_got_ip4_t *got_ip = (netif_event_got_ip4_t *)event_data;

		if (got_ip && got_ip->netif_if == NETIF_IF_STA)
			s_wifi_has_ip = 1;
	}

	return BK_OK;
}

static void core_mqtt_wifi_cb_register(void)
{
	if (s_wifi_cb_registered)
		return;

	core_mqtt_sync_wifi_ip_state();
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
					     core_mqtt_wifi_event_cb, NULL));
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
					     core_mqtt_wifi_event_cb, NULL));
	s_wifi_cb_registered = 1;
}

static void core_mqtt_wifi_cb_unregister(void)
{
	if (!s_wifi_cb_registered)
		return;

	(void)bk_event_unregister_cb(EVENT_MOD_WIFI, EVENT_WIFI_STA_DISCONNECTED,
				     core_mqtt_wifi_event_cb);
	(void)bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
				     core_mqtt_wifi_event_cb);
	s_wifi_cb_registered = 0;
	s_wifi_has_ip = 0;
}

static void core_mqtt_wait_for_network(void)
{
	while (s_mqtt_running && !s_wifi_has_ip)
		rtos_delay_milliseconds(CORE_MQTT_NETWORK_POLL_MS);
}

static char *core_mqtt_get_client_id(void)
{
	uint8_t mac[BK_MAC_ADDR_LEN] = {0};

	if (s_client_id[0] == '\0') {
		bk_get_mac(mac, MAC_TYPE_BASE);
		os_snprintf(s_client_id, sizeof(s_client_id), "bk7259_%02x%02x%02x%02x%02x%02x",
			    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return s_client_id;
}

static void core_mqtt_release_config(void)
{
	if (s_sub_topic) {
		os_free(s_sub_topic);
		s_sub_topic = NULL;
	}
	if (s_pub_topic) {
		os_free(s_pub_topic);
		s_pub_topic = NULL;
	}
	if (s_username) {
		os_free(s_username);
		s_username = NULL;
	}
	if (s_password) {
		os_free(s_password);
		s_password = NULL;
	}
}

static int core_mqtt_save_string(char **dst, const char *src)
{
	char *buf = NULL;
	size_t len;

	if (!src || src[0] == '\0' || (src[0] == '-' && src[1] == '\0'))
		return 0;

	len = os_strlen(src);
	buf = os_malloc(len + 1);
	if (!buf)
		return -1;

	os_memcpy(buf, src, len);
	buf[len] = '\0';
	*dst = buf;
	return 0;
}

static int core_mqtt_save_password(char **dst, const char *src)
{
	const char *pwd = src;

	if (pwd && os_strcmp(pwd, CORE_MQTT_JWT_PASSWORD_PLACEHOLDER) == 0) {
		pwd = CORE_MQTT_JWT_PASSWORD;
		BK_LOGI(TAG, "password placeholder %s -> JWT\r\n",
			CORE_MQTT_JWT_PASSWORD_PLACEHOLDER);
	}

	return core_mqtt_save_string(dst, pwd);
}

static int core_mqtt_strip_uri_scheme(const char **host_in, bool *use_tls)
{
	const char *p = *host_in;

	if (!p || !use_tls)
		return -1;

	*use_tls = false;
	if (os_strncmp(p, "mqtts://", 8) == 0) {
		*use_tls = true;
		p += 8;
	} else if (os_strncmp(p, "ssl://", 6) == 0) {
		*use_tls = true;
		p += 6;
	} else if (os_strncmp(p, "tcp://", 6) == 0) {
		p += 6;
	}

	*host_in = p;
	return 0;
}

static int core_mqtt_parse_host_port(const char *host_in)
{
	char *colon = NULL;
	char host_buf[CORE_MQTT_HOST_SIZE];
	const char *p = host_in;
	bool want_tls = false;

	if (!host_in || host_in[0] == '\0')
		return -1;

	s_use_tls = false;
	if (core_mqtt_strip_uri_scheme(&p, &want_tls) != 0)
		return -1;

#if CONFIG_COREMQTT_TLS
	s_use_tls = want_tls;
#elif want_tls
	BK_LOGW(TAG, "MQTTS URI ignored: rebuild with CONFIG_COREMQTT_TLS=y\r\n");
#endif

	os_strlcpy(host_buf, p, sizeof(host_buf));
	colon = os_strchr(host_buf, ':');
	if (colon) {
		*colon = '\0';
		s_port = (uint16_t)os_strtoul(colon + 1, NULL, 10);
		if (s_port == 0)
			s_port = s_use_tls ? CORE_MQTT_DEFAULT_TLS_PORT : CORE_MQTT_DEFAULT_PORT;
	} else {
#if CONFIG_COREMQTT_TLS
		s_port = s_use_tls ? CORE_MQTT_DEFAULT_TLS_PORT : CORE_MQTT_DEFAULT_PORT;
#else
		s_port = CORE_MQTT_DEFAULT_PORT;
#endif
	}

#if CONFIG_COREMQTT_TLS
	if (!s_use_tls && s_port == CORE_MQTT_DEFAULT_TLS_PORT)
		s_use_tls = true;
#endif

	os_strlcpy(s_host, host_buf, sizeof(s_host));
	return 0;
}

static uint32_t core_mqtt_get_time_ms(void)
{
	return (uint32_t)rtos_get_time();
}

static bool core_mqtt_event_callback(MQTTContext_t *pContext,
				     MQTTPacketInfo_t *pPacketInfo,
				     MQTTDeserializedInfo_t *pDeserializedInfo,
				     MQTTSuccessFailReasonCode_t *pReasonCode,
				     MQTTPropBuilder_t *pSendPropsBuffer,
				     MQTTPropBuilder_t *pGetPropsBuffer)
{
	(void)pContext;
	(void)pReasonCode;
	(void)pSendPropsBuffer;
	(void)pGetPropsBuffer;

	if (!pPacketInfo)
		return true;

	switch (pPacketInfo->type) {
	case MQTT_PACKET_TYPE_CONNACK:
		BK_LOGI(TAG, "CONNACK received\r\n");
		break;

	case MQTT_PACKET_TYPE_SUBACK:
		BK_LOGI(TAG, "SUBACK received packetId=%u\r\n",
			pDeserializedInfo ? pDeserializedInfo->packetIdentifier : 0U);
		break;

	case MQTT_PACKET_TYPE_PUBLISH:
		if (pDeserializedInfo && pDeserializedInfo->pPublishInfo) {
			MQTTPublishInfo_t *pub = pDeserializedInfo->pPublishInfo;

			BK_LOGI(TAG, "topic=%.*s payload=%.*s\r\n",
				(int)pub->topicNameLength, pub->pTopicName ? pub->pTopicName : "",
				(int)pub->payloadLength,
				pub->pPayload ? (const char *)pub->pPayload : "");
		}
		break;

	case MQTT_PACKET_TYPE_PUBACK:
		BK_LOGI(TAG, "PUBACK received packetId=%u\r\n",
			pDeserializedInfo ? pDeserializedInfo->packetIdentifier : 0U);
		break;

	default:
		break;
	}

	return true;
}

static int core_mqtt_do_subscribe(void)
{
	MQTTSubscribeInfo_t subscription = { 0 };
	uint16_t packet_id;
	MQTTStatus_t status;

	if (!s_sub_topic)
		return -1;

	subscription.qos = MQTTQoS1;
	subscription.pTopicFilter = s_sub_topic;
	subscription.topicFilterLength = os_strlen(s_sub_topic);

	packet_id = MQTT_GetPacketId(&s_mqtt_ctx);
	status = MQTT_Subscribe(&s_mqtt_ctx, &subscription, 1, packet_id, NULL);
	if (status != MQTTSuccess) {
		BK_LOGE(TAG, "subscribe failed status=%s\r\n", MQTT_Status_strerror(status));
		return -1;
	}

	BK_LOGI(TAG, "subscribe sent topic=%s id=%u\r\n", s_sub_topic, packet_id);
	return 0;
}

static bool core_mqtt_process_loop_once(uint32_t *tx_bytes, uint32_t *rx_bytes)
{
	MQTTStatus_t status;
	bool was_waiting_ping = s_mqtt_ctx.waitingForPingResp;

	status = MQTT_ProcessLoop(&s_mqtt_ctx);

	if (!was_waiting_ping && s_mqtt_ctx.waitingForPingResp)
		BK_LOGD(TAG, "PINGREQ sent\r\n");
	else if (was_waiting_ping && !s_mqtt_ctx.waitingForPingResp)
		BK_LOGD(TAG, "PINGRESP received\r\n");

	if (status == MQTTSuccess || status == MQTTNeedMoreBytes ||
	    status == MQTTNoDataAvailable)
		return true;

	core_mqtt_transport_get_stats(tx_bytes, rx_bytes);
	if (status == MQTTKeepAliveTimeout) {
		BK_LOGW(TAG, "keep-alive timeout tx=%lu rx=%lu\r\n",
			(unsigned long)*tx_bytes, (unsigned long)*rx_bytes);
	} else {
		BK_LOGW(TAG, "process loop status=%s tx=%lu rx=%lu\r\n",
			MQTT_Status_strerror(status),
			(unsigned long)*tx_bytes, (unsigned long)*rx_bytes);
	}

	return false;
}

static void core_mqtt_cleanup_session(void)
{
	if (s_mqtt_connected) {
		(void)MQTT_Disconnect(&s_mqtt_ctx, NULL, NULL);
		s_mqtt_connected = 0;
	}

	core_mqtt_transport_disconnect(&s_net_ctx);
}

static int core_mqtt_establish_session(void)
{
	MQTTConnectInfo_t connect_info = { 0 };
	MQTTStatus_t status;
	bool session_present = false;
	const char *client_id = core_mqtt_get_client_id();
	uint32_t tx_bytes = 0;
	uint32_t rx_bytes = 0;

	if (core_mqtt_transport_connect(&s_net_ctx, s_host, s_port, s_use_tls) != 0)
		return -1;

	if (!s_mqtt_running)
		return -1;

	memset(&s_mqtt_ctx, 0, sizeof(s_mqtt_ctx));
	memset(&s_transport, 0, sizeof(s_transport));

	s_transport.pNetworkContext = &s_net_ctx;
	s_transport.send = core_mqtt_transport_send;
	s_transport.recv = core_mqtt_transport_recv;
	s_transport.writev = core_mqtt_transport_writev;

	s_fixed_buffer.pBuffer = s_network_buffer;
	s_fixed_buffer.size = sizeof(s_network_buffer);

	status = MQTT_Init(&s_mqtt_ctx, &s_transport, core_mqtt_get_time_ms,
			   core_mqtt_event_callback, &s_fixed_buffer);
	if (status != MQTTSuccess) {
		BK_LOGE(TAG, "MQTT_Init failed status=%s\r\n", MQTT_Status_strerror(status));
		goto fail;
	}

	status = MQTT_InitStatefulQoS(&s_mqtt_ctx,
				      s_outgoing_publishes,
				      CONFIG_COREMQTT_MAX_INFLIGHT,
				      s_incoming_publishes,
				      CONFIG_COREMQTT_MAX_INFLIGHT,
				      s_ack_props_buf,
				      sizeof(s_ack_props_buf));
	if (status != MQTTSuccess) {
		BK_LOGE(TAG, "MQTT_InitStatefulQoS failed status=%s\r\n",
			MQTT_Status_strerror(status));
		goto fail;
	}

	connect_info.cleanSession = true;
	connect_info.keepAliveSeconds = 60;
	connect_info.pClientIdentifier = client_id;
	connect_info.clientIdentifierLength = os_strlen(client_id);
	if (s_username) {
		connect_info.pUserName = s_username;
		connect_info.userNameLength = os_strlen(s_username);
	}
	if (s_password) {
		connect_info.pPassword = s_password;
		connect_info.passwordLength = os_strlen(s_password);
	}

	BK_LOGI(TAG, "connect host=%s port=%u tls=%d client=%s mqtt=5\r\n",
		s_host, s_port, (int)s_use_tls, client_id);

	core_mqtt_transport_set_connack_mode(&s_net_ctx);

	status = MQTT_Connect(&s_mqtt_ctx, &connect_info, NULL,
			      CORE_MQTT_CONNECT_TIMEOUT_MS, &session_present, NULL, NULL);
	if (status != MQTTSuccess) {
		uint8_t pending[16];
		int32_t pending_len;

		core_mqtt_transport_get_stats(&tx_bytes, &rx_bytes);
		BK_LOGW(TAG, "MQTT_Connect fail tx=%lu rx=%lu status=%s\r\n",
			(unsigned long)tx_bytes, (unsigned long)rx_bytes,
			MQTT_Status_strerror(status));

		pending_len = core_mqtt_transport_drain(&s_net_ctx, pending, sizeof(pending));
		if (pending_len > 0) {
			BK_LOGW(TAG, "pending rx after connect fail len=%ld first=0x%02x\r\n",
				(long)pending_len, pending[0]);
		}

		goto fail;
	}

	core_mqtt_transport_set_nonblock_mode(&s_net_ctx, true);

	s_mqtt_connected = 1;
	BK_LOGI(TAG, "online (MQTT v5)\r\n");

	if (s_sub_topic && s_mqtt_mutex) {
		rtos_lock_mutex(&s_mqtt_mutex);
		(void)core_mqtt_do_subscribe();
		rtos_unlock_mutex(&s_mqtt_mutex);
	}

	return 0;

fail:
	core_mqtt_cleanup_session();
	return -1;
}

static void core_mqtt_thread(void *arg)
{
	uint32_t tx_bytes = 0;
	uint32_t rx_bytes = 0;

	(void)arg;

	while (s_mqtt_running) {
		core_mqtt_wait_for_network();
		if (!s_mqtt_running)
			break;

		if (core_mqtt_establish_session() != 0) {
			if (!s_mqtt_running)
				break;

			if (s_wifi_has_ip) {
				BK_LOGW(TAG, "connect failed, retry in %us\r\n",
					(unsigned)(CORE_MQTT_RECONNECT_DELAY_MS / 1000U));
				rtos_delay_milliseconds(CORE_MQTT_RECONNECT_DELAY_MS);
			}
			continue;
		}

		while (s_mqtt_running && s_mqtt_connected) {
			if (!core_mqtt_process_loop_once(&tx_bytes, &rx_bytes))
				break;

			rtos_delay_milliseconds(CORE_MQTT_PROCESS_LOOP_DELAY_MS);
		}

		if (!s_mqtt_running)
			break;

		BK_LOGW(TAG, "offline\r\n");
		core_mqtt_cleanup_session();

		if (s_wifi_has_ip) {
			BK_LOGI(TAG, "reconnect in %us\r\n",
				(unsigned)(CORE_MQTT_RECONNECT_DELAY_MS / 1000U));
			rtos_delay_milliseconds(CORE_MQTT_RECONNECT_DELAY_MS);
		}
	}

	core_mqtt_cleanup_session();
	core_mqtt_release_config();
	core_mqtt_wifi_cb_unregister();
	if (s_mqtt_mutex) {
		rtos_deinit_mutex(&s_mqtt_mutex);
		s_mqtt_mutex = NULL;
	}
	s_mqtt_connected = 0;
	s_mqtt_running = 0;
	s_mqtt_started = 0;
	s_mqtt_thread = NULL;
	rtos_delete_thread(NULL);
}

static int core_mqtt_client_start(const char *host, const char *username,
				  const char *password)
{
	bk_err_t ret;

	if (s_mqtt_running) {
		BK_LOGW(TAG, "mqtt already running, call mqttdestroy first\r\n");
		return -1;
	}

	if (core_mqtt_parse_host_port(host) != 0)
		return -1;

	if (core_mqtt_save_string(&s_username, username) != 0)
		return -1;
	if (core_mqtt_save_password(&s_password, password) != 0) {
		core_mqtt_release_config();
		return -1;
	}

	if (rtos_init_mutex(&s_mqtt_mutex) != kNoErr) {
		core_mqtt_release_config();
		return -1;
	}

	core_mqtt_wifi_cb_register();

	s_mqtt_running = 1;
	ret = rtos_create_thread(&s_mqtt_thread, BEKEN_DEFAULT_WORKER_PRIORITY,
				 "core_mqtt", core_mqtt_thread,
				 CONFIG_COREMQTT_THREAD_STACK_SIZE, NULL);
	if (ret != kNoErr) {
		s_mqtt_running = 0;
		core_mqtt_wifi_cb_unregister();
		core_mqtt_release_config();
		rtos_deinit_mutex(&s_mqtt_mutex);
		s_mqtt_mutex = NULL;
		return -1;
	}

	s_mqtt_started = 1;
	return 0;
}

int core_mqtt_connect(const char *host, const char *username, const char *password)
{
	uint32_t waited = 0;
	int ret;

	if (!host)
		return -1;

	ret = core_mqtt_client_start(host, username, password);
	if (ret != 0) {
		BK_LOGI(TAG, "core_mqtt_connect ret=%d\r\n", ret);
		return ret;
	}

	while (!s_mqtt_connected && s_mqtt_running &&
	       waited < CORE_MQTT_CONNECT_TIMEOUT_MS) {
		rtos_delay_milliseconds(50);
		waited += 50;
	}

	if (!s_mqtt_connected) {
		BK_LOGW(TAG, "connect wait timeout\r\n");
		core_mqtt_destroy();
		ret = -1;
	} else {
		ret = 0;
	}

	BK_LOGI(TAG, "core_mqtt_connect ret=%d\r\n", ret);
	return ret;
}

int core_mqtt_subscribe(const char *topic)
{
	int ret = -1;

	if (!topic || topic[0] == '\0') {
		BK_LOGW(TAG, "subscribe topic is empty\r\n");
		return -1;
	}

	if (!s_mqtt_started || !s_mqtt_connected) {
		BK_LOGW(TAG, "mqtt not connected\r\n");
		return -1;
	}

	if (s_sub_topic) {
		os_free(s_sub_topic);
		s_sub_topic = NULL;
	}
	if (s_pub_topic) {
		os_free(s_pub_topic);
		s_pub_topic = NULL;
	}

	if (core_mqtt_save_string(&s_sub_topic, topic) != 0)
		return -1;
	if (core_mqtt_save_string(&s_pub_topic, topic) != 0) {
		os_free(s_sub_topic);
		s_sub_topic = NULL;
		return -1;
	}

	rtos_lock_mutex(&s_mqtt_mutex);
	ret = core_mqtt_do_subscribe();
	rtos_unlock_mutex(&s_mqtt_mutex);

	BK_LOGI(TAG, "core_mqtt_subscribe ret=%d\r\n", ret);
	return ret;
}

int core_mqtt_publish(const char *topic, const char *msg)
{
	MQTTPublishInfo_t publish_info = { 0 };
	const char *pub_topic;
	uint16_t packet_id;
	MQTTStatus_t status;
	int rc = -1;

	if (!s_mqtt_started || !s_mqtt_connected) {
		BK_LOGW(TAG, "mqtt not connected\r\n");
		return -1;
	}

	if (!msg)
		return -1;

	pub_topic = (topic && topic[0]) ? topic : s_pub_topic;
	if (!pub_topic) {
		BK_LOGW(TAG, "publish topic is null\r\n");
		return -1;
	}

	rtos_lock_mutex(&s_mqtt_mutex);

	publish_info.qos = MQTTQoS1;
	publish_info.retain = false;
	publish_info.pTopicName = pub_topic;
	publish_info.topicNameLength = os_strlen(pub_topic);
	publish_info.pPayload = msg;
	publish_info.payloadLength = os_strlen(msg);

	BK_LOGI(TAG, "publish topic=%s msg=%s\r\n", pub_topic, msg);

	packet_id = MQTT_GetPacketId(&s_mqtt_ctx);
	status = MQTT_Publish(&s_mqtt_ctx, &publish_info, packet_id, NULL);
	rc = (status == MQTTSuccess) ? 0 : -1;

	rtos_unlock_mutex(&s_mqtt_mutex);
	return rc;
}

int core_mqtt_destroy(void)
{
	uint32_t waited = 0;

	if (!s_mqtt_running && !s_mqtt_started)
		return 0;

	BK_LOGI(TAG, "destroy mqtt session\r\n");
	s_mqtt_running = 0;
	core_mqtt_transport_abort(&s_net_ctx);

	while (s_mqtt_started && waited < CORE_MQTT_DESTROY_WAIT_MS) {
		rtos_delay_milliseconds(20);
		waited += 20;
	}

	if (s_mqtt_started) {
		BK_LOGW(TAG, "destroy timeout\r\n");
		return -1;
	}

	BK_LOGI(TAG, "core_mqtt_destroy done\r\n");
	return 0;
}
