#ifndef __PAHO_MQTT_UDP_H__
#define __PAHO_MQTT_UDP_H__

#include "paho_mqtt.h"

#define  debug_printf           os_printf("[MQTT] ");os_printf

int mqtt_publish_with_topic(MQTT_CLIENT_T *c, const char *topicName, MQTTMessage *message);

#endif // __PAHO_MQTT_UDP_H__
// eof
