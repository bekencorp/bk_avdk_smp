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

#ifndef CORE_MQTT_TEST_H
#define CORE_MQTT_TEST_H

/* CLI placeholder: mqttconnect ... JWP  -> use JWT below */
#define CORE_MQTT_JWT_PASSWORD_PLACEHOLDER  "JWP"
#define CORE_MQTT_JWT_PASSWORD \
	"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ0ZXN0IiwiZXhwIjoxNzg3Mjk3OTc0fQ.EHJ57TKJJJse7wMkasajbykJ-_ePblHeAVlQw48qa9g"

int core_mqtt_connect(const char *host, const char *username, const char *password);
int core_mqtt_subscribe(const char *topic);
int core_mqtt_publish(const char *topic, const char *msg);
int core_mqtt_destroy(void);

#endif /* CORE_MQTT_TEST_H */
