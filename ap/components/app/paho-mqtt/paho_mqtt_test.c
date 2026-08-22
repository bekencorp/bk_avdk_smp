#include <string.h>
#include <stdio.h>

#include "paho_mqtt.h"
#include "paho_mqtt_udp.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/system.h>

#define TAG "paho_mqtt"
#define PAHO_MQTT_BUF_SIZE          1024
#define PAHO_MQTT_URI_SIZE          128
#define PAHO_MQTT_CLIENT_ID_SIZE    32
#define PAHO_MQTT_DEFAULT_PORT      "1883"

static MQTT_CLIENT_T s_paho_client;
static char s_mqtt_uri[PAHO_MQTT_URI_SIZE];
static char s_client_id[PAHO_MQTT_CLIENT_ID_SIZE];
static char *s_sub_topic = NULL;
static char *s_pub_topic = NULL;
static char *s_username = NULL;
static char *s_password = NULL;
static beken_mutex_t s_publish_mutex = NULL;
static int s_paho_started = 0;

static char *paho_mqtt_get_client_id(void)
{
	uint8_t mac[BK_MAC_ADDR_LEN] = {0};

	if (s_client_id[0] == '\0') {
		bk_get_mac(mac, MAC_TYPE_BASE);
		os_snprintf(s_client_id, sizeof(s_client_id), "bk7259_%02x%02x%02x%02x%02x%02x",
			    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return s_client_id;
}

static void paho_mqtt_release_config(void)
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

static int paho_mqtt_save_string(char **dst, const char *src)
{
	char *buf = NULL;
	size_t len;

	if (!src)
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

static int paho_mqtt_build_uri(const char *host, char *uri, size_t uri_len)
{
	if (!host || !uri || uri_len == 0)
		return -1;

	if (os_strchr(host, ':')) {
		os_snprintf(uri, uri_len, "tcp://%s", host);
	} else {
		os_snprintf(uri, uri_len, "tcp://%s:%s", host, PAHO_MQTT_DEFAULT_PORT);
	}

	return 0;
}

static void paho_mqtt_sub_callback(MQTT_CLIENT_T *c, MessageData *msg_data)
{
	(void)c;

	if (!msg_data || !msg_data->message || !msg_data->topicName)
		return;

	BK_LOGI(TAG, "topic=%.*s payload=%.*s\r\n",
		msg_data->topicName->lenstring.len,
		msg_data->topicName->lenstring.data,
		(int)msg_data->message->payloadlen,
		(char *)msg_data->message->payload);
}

static void paho_mqtt_connect_callback(MQTT_CLIENT_T *c)
{
	(void)c;
	BK_LOGI(TAG, "connect callback\r\n");
}

static void paho_mqtt_online_callback(MQTT_CLIENT_T *c)
{
	(void)c;
	BK_LOGI(TAG, "online callback\r\n");
}

static void paho_mqtt_offline_callback(MQTT_CLIENT_T *c)
{
	(void)c;
	BK_LOGI(TAG, "offline callback\r\n");
}

static int paho_mqtt_client_start(const char *host, const char *username,
				  const char *password, const char *topic)
{
	MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
	const char *sub_topic = topic ? topic : "/aclsemi/bk7256/cmd/changyun";

	if (s_paho_started)
		return 0;

	if (paho_mqtt_build_uri(host, s_mqtt_uri, sizeof(s_mqtt_uri)) != 0)
		return -1;

	if (paho_mqtt_save_string(&s_username, username) != 0)
		return -1;
	if (paho_mqtt_save_string(&s_password, password) != 0)
		return -1;
	if (paho_mqtt_save_string(&s_sub_topic, sub_topic) != 0)
		return -1;
	if (paho_mqtt_save_string(&s_pub_topic, sub_topic) != 0)
		return -1;

	if (rtos_init_mutex(&s_publish_mutex) != kNoErr)
		return -1;

	os_memset(&s_paho_client, 0, sizeof(s_paho_client));
	os_memcpy(&s_paho_client.condata, &condata, sizeof(condata));

	s_paho_client.uri = s_mqtt_uri;
	s_paho_client.condata.clientID.cstring = paho_mqtt_get_client_id();
	s_paho_client.condata.keepAliveInterval = 60;
	s_paho_client.condata.cleansession = 1;
	s_paho_client.condata.username.cstring = s_username;
	s_paho_client.condata.password.cstring = s_password;

	s_paho_client.buf_size = s_paho_client.readbuf_size = PAHO_MQTT_BUF_SIZE;
	s_paho_client.buf = os_malloc(s_paho_client.buf_size);
	s_paho_client.readbuf = os_malloc(s_paho_client.readbuf_size);
	if (!s_paho_client.buf || !s_paho_client.readbuf)
		goto error;

	s_paho_client.connect_callback = paho_mqtt_connect_callback;
	s_paho_client.online_callback = paho_mqtt_online_callback;
	s_paho_client.offline_callback = paho_mqtt_offline_callback;

	s_paho_client.messageHandlers[0].topicFilter = s_sub_topic;
	s_paho_client.messageHandlers[0].callback = paho_mqtt_sub_callback;
	s_paho_client.messageHandlers[0].qos = QOS1;
	s_paho_client.defaultMessageHandler = paho_mqtt_sub_callback;

	BK_LOGI(TAG, "start uri=%s client=%s topic=%s\r\n",
		s_mqtt_uri, s_client_id, s_sub_topic);

	if (paho_mqtt_start(&s_paho_client) != 0)
		goto error;

	s_paho_started = 1;
	return 0;

error:
	if (s_paho_client.buf) {
		os_free(s_paho_client.buf);
		s_paho_client.buf = NULL;
	}
	if (s_paho_client.readbuf) {
		os_free(s_paho_client.readbuf);
		s_paho_client.readbuf = NULL;
	}
	paho_mqtt_release_config();
	if (s_publish_mutex) {
		rtos_deinit_mutex(&s_publish_mutex);
		s_publish_mutex = NULL;
	}
	return -1;
}

void test_paho_mqtt_start(const char *host_name, const char *username,
			  const char *password, const char *topic)
{
	int ret;

	ret = paho_mqtt_client_start(host_name, username, password, topic);
	BK_LOGI(TAG, "test_paho_mqtt_start ret=%d\r\n", ret);
}

int paho_mqtt_cmd_msg_send(char *topic, char *msg)
{
	MQTTMessage message;
	const char *pub_topic;
	int rc = -1;

	if (!s_paho_started || !s_paho_client.is_connected) {
		BK_LOGW(TAG, "mqtt not started or not connected\r\n");
		return -1;
	}

	if (!msg)
		return -1;

	pub_topic = (topic && topic[0]) ? topic : s_pub_topic;
	if (!pub_topic) {
		BK_LOGW(TAG, "publish topic is null\r\n");
		return -1;
	}

	rtos_lock_mutex(&s_publish_mutex);

	message.qos = QOS1;
	message.retained = 0;
	message.dup = 0;
	message.payload = msg;
	message.payloadlen = os_strlen(msg);

	BK_LOGI(TAG, "publish topic=%s msg=%s\r\n", pub_topic, msg);
	rc = mqtt_publish_with_topic(&s_paho_client, pub_topic, &message);

	rtos_unlock_mutex(&s_publish_mutex);
	return rc;
}
