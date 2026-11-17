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

#include <stdio.h>
#include <string.h>

#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <components/log.h>

#include <components/netif.h>
#include <components/webclient.h>
#include <modules/wifi.h>

#include "bk_js_internal.h"

#if CONFIG_BK_JERRYSCRIPT_NET

/* Implemented by webclient.c but not exported through webclient.h. */
int webclient_handle_response(struct webclient_session *session);

#define BK_JS_NET_TAG            "js_net"

#define JS_NET_URL_MAX           (192)
#define JS_NET_METHOD_MAX        (8)
#define JS_NET_CONTENT_TYPE_MAX  (48)
#define JS_NET_BODY_MAX          (16 * 1024)
#define JS_NET_POST_MAX          (4 * 1024)
#define JS_NET_TIMEOUT_MS        (10000)
#define JS_NET_WIFI_TIMEOUT_MS   (20000)
#define JS_NET_WIFI_POLL_MS      (200)
#define JS_NET_QUEUE_LEN         (4)
#define JS_NET_PENDING_MAX       (8)
#if CONFIG_WEBCLIENT_TLS
/* TLS handshakes need a deeper stack than plain sockets. */
#define JS_NET_TASK_STACK        (20480)
#else
#define JS_NET_TASK_STACK        (8192)
#endif
#define JS_NET_TEXT_MAX          (96)

typedef enum {
	JS_NET_REQ_HTTP = 1,
	JS_NET_REQ_WIFI_CONNECT,
} js_net_req_kind_t;

typedef struct {
	uint32_t id;
	uint8_t kind;
	uint8_t is_post;
	uint8_t reserved[2];
	int32_t timeout_ms;
	char url[JS_NET_URL_MAX];
	char method[JS_NET_METHOD_MAX];
	char content_type[JS_NET_CONTENT_TYPE_MAX];
	char ssid[WIFI_SSID_STR_LEN];
	char password[WIFI_PASSWORD_LEN];
	char *post;
	uint32_t post_len;
} js_net_req_t;

typedef struct {
	uint32_t id;
	uint8_t kind;
	uint8_t reserved[3];
	int32_t status;
	int32_t value;
	int32_t length;
	char text[JS_NET_TEXT_MAX];
	char *body;
} js_net_result_t;

typedef struct {
	bool used;
	uint32_t id;
	uint8_t kind;
	jerry_value_t promise;
} js_net_pending_t;

static beken_queue_t s_req_q = NULL;
static beken_thread_t s_task = NULL;
static volatile bool s_task_running = false;
static uint32_t s_next_id = 1;
static js_net_pending_t s_pending[JS_NET_PENDING_MAX];

/* ------------------------------------------------------------------ */
/* Small helpers                                                      */
/* ------------------------------------------------------------------ */

static void js_net_free_req_post(js_net_req_t *req)
{
	if (req->post != NULL) {
		os_free(req->post);
		req->post = NULL;
	}
	req->post_len = 0;
}

void bk_js_net_result_free(void *result)
{
	js_net_result_t *res = (js_net_result_t *)result;

	if (res == NULL) {
		return;
	}
	if (res->body != NULL) {
		os_free(res->body);
	}
	os_free(res);
}

static void js_net_set_text(char *dst, uint32_t size, const char *src)
{
	if (size == 0) {
		return;
	}
	if (src == NULL) {
		src = "";
	}
	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';
}

static jerry_value_t js_net_throw(jerry_error_t type, const char *message)
{
	return jerry_throw_sz(type, message);
}

static void js_net_obj_set(jerry_value_t object, const char *key, jerry_value_t value)
{
	jerry_value_free(jerry_object_set_sz(object, key, value));
	jerry_value_free(value);
}

static void js_net_obj_set_num(jerry_value_t object, const char *key, double value)
{
	js_net_obj_set(object, key, jerry_number(value));
}

static void js_net_obj_set_bool(jerry_value_t object, const char *key, bool value)
{
	js_net_obj_set(object, key, jerry_boolean(value));
}

static void js_net_obj_set_str(jerry_value_t object, const char *key, const char *value)
{
	js_net_obj_set(object, key, jerry_string_sz(value != NULL ? value : ""));
}

static void js_net_set_fn(jerry_value_t object, const char *name, jerry_external_handler_t handler)
{
	js_net_obj_set(object, name, jerry_function_external(handler));
}

static bool js_net_value_to_string(jerry_value_t value, char *buf, uint32_t size)
{
	jerry_size_t len;

	if (!jerry_value_is_string(value) || size == 0) {
		return false;
	}

	len = jerry_string_size(value, JERRY_ENCODING_UTF8);
	if (len >= size) {
		return false;
	}

	jerry_string_to_buffer(value, JERRY_ENCODING_UTF8, (jerry_char_t *)buf, len);
	buf[len] = '\0';
	return true;
}

static char *js_net_string_dup(jerry_value_t value, uint32_t max, uint32_t *out_len)
{
	jerry_size_t len;
	char *buf;

	if (!jerry_value_is_string(value)) {
		return NULL;
	}

	len = jerry_string_size(value, JERRY_ENCODING_UTF8);
	if (len > max) {
		return NULL;
	}

	buf = (char *)os_malloc((uint32_t)len + 1);
	if (buf == NULL) {
		return NULL;
	}

	jerry_string_to_buffer(value, JERRY_ENCODING_UTF8, (jerry_char_t *)buf, len);
	buf[len] = '\0';

	if (out_len != NULL) {
		*out_len = (uint32_t)len;
	}
	return buf;
}

static void js_net_opt_string(jerry_value_t options, const char *key, char *buf, uint32_t size)
{
	jerry_value_t value = jerry_object_get_sz(options, key);

	if (jerry_value_is_string(value)) {
		if (!js_net_value_to_string(value, buf, size)) {
			BK_LOGW(BK_JS_NET_TAG, "option '%s' too long or invalid\r\n", key);
		}
	}
	jerry_value_free(value);
}

static void js_net_opt_number(jerry_value_t options, const char *key, int32_t *out)
{
	jerry_value_t value = jerry_object_get_sz(options, key);

	if (jerry_value_is_number(value)) {
		*out = (int32_t)jerry_value_as_number(value);
	}
	jerry_value_free(value);
}

/* ------------------------------------------------------------------ */
/* Pending promise bookkeeping (JavaScript task only)                 */
/* ------------------------------------------------------------------ */

static js_net_pending_t *js_net_pending_alloc(uint8_t kind, jerry_value_t promise)
{
	uint32_t i;

	for (i = 0; i < JS_NET_PENDING_MAX; i++) {
		if (s_pending[i].used) {
			continue;
		}

		s_pending[i].used = true;
		s_pending[i].id = s_next_id++;
		if (s_next_id == 0) {
			s_next_id = 1;
		}
		s_pending[i].kind = kind;
		s_pending[i].promise = jerry_value_copy(promise);
		return &s_pending[i];
	}

	return NULL;
}

static void js_net_pending_release(js_net_pending_t *slot)
{
	if (slot == NULL || !slot->used) {
		return;
	}

	jerry_value_free(slot->promise);
	slot->used = false;
}

static js_net_pending_t *js_net_pending_find(uint32_t id)
{
	uint32_t i;

	for (i = 0; i < JS_NET_PENDING_MAX; i++) {
		if (s_pending[i].used && s_pending[i].id == id) {
			return &s_pending[i];
		}
	}

	return NULL;
}

static void js_net_settle(js_net_pending_t *slot, jerry_value_t argument, bool resolved)
{
	jerry_value_t result;

	if (resolved) {
		result = jerry_promise_resolve(slot->promise, argument);
	} else {
		result = jerry_promise_reject(slot->promise, argument);
	}

	if (jerry_value_is_exception(result)) {
		bk_js_console_exception(result);
	} else {
		jerry_value_free(result);
	}

	jerry_value_free(argument);
	jerry_value_free(slot->promise);
	slot->used = false;
}

void bk_js_net_shutdown(void)
{
	uint32_t i;

	for (i = 0; i < JS_NET_PENDING_MAX; i++) {
		if (!s_pending[i].used) {
			continue;
		}

		js_net_settle(&s_pending[i], jerry_string_sz("runtime stopped"), false);
	}
}

/* ------------------------------------------------------------------ */
/* Worker task                                                        */
/* ------------------------------------------------------------------ */

static int js_net_http_run(const js_net_req_t *req, js_net_result_t *res)
{
	struct webclient_session *session;
	uint32_t want;
	uint32_t got = 0;
	int status;
	int content_len;
	char *body;

	if (req->url[0] == '\0') {
		js_net_set_text(res->text, sizeof(res->text), "empty url");
		res->status = -1;
		return -1;
	}

	session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
	if (session == NULL) {
		js_net_set_text(res->text, sizeof(res->text), "no memory for session");
		res->status = -1;
		return -1;
	}

	webclient_set_timeout(session, req->timeout_ms > 0 ? req->timeout_ms : JS_NET_TIMEOUT_MS);

	if (req->is_post) {
		if (req->content_type[0] != '\0') {
			webclient_header_fields_add(session, "Content-Type: %s\r\n", req->content_type);
		}
		/* webclient does not add Content-Length on its own. */
		webclient_header_fields_add(session, "Content-Length: %u\r\n", req->post_len);
		status = webclient_post(session, req->url, req->post, req->post_len);
		if (status == 0 && req->post_len == 0) {
			/* webclient_post() sends the header but skips the response read when
			 * the body is empty, so fetch the response here. */
			status = webclient_handle_response(session);
		}
	} else {
		status = webclient_get(session, req->url);
	}

	if (status <= 0) {
		BK_LOGW(BK_JS_NET_TAG, "request '%s' failed: %d\r\n", req->url, status);
		js_net_set_text(res->text, sizeof(res->text), "connect or request failed");
		res->status = -1;
		webclient_close(session);
		return -1;
	}

	res->status = status;

	content_len = webclient_content_length_get(session);
	if (content_len > 0 && (uint32_t)content_len < JS_NET_BODY_MAX) {
		want = (uint32_t)content_len;
	} else {
		want = JS_NET_BODY_MAX;
	}

	body = (char *)os_malloc(want + 1);
	if (body == NULL) {
		js_net_set_text(res->text, sizeof(res->text), "no memory for response");
		res->status = -1;
		webclient_close(session);
		return -1;
	}

	while (got < want) {
		int n = webclient_read(session, body + got, want - got);

		if (n <= 0) {
			break;
		}
		got += (uint32_t)n;
	}
	body[got] = '\0';

	webclient_close(session);

	res->body = body;
	res->length = (int32_t)got;
	BK_LOGI(BK_JS_NET_TAG, "request '%s' -> %d, %u bytes\r\n", req->url, status, got);
	return 0;
}

static int js_net_wifi_run(const js_net_req_t *req, js_net_result_t *res)
{
	wifi_sta_config_t config;
	bk_err_t err;
	uint32_t waited = 0;

	os_memset(&config, 0, sizeof(config));
	os_strncpy(config.ssid, req->ssid, sizeof(config.ssid) - 1);
	os_strncpy(config.password, req->password, sizeof(config.password) - 1);
	config.security = WIFI_SECURITY_AUTO;

	err = bk_wifi_sta_set_config(&config);
	if (err != BK_OK) {
		snprintf(res->text, sizeof(res->text), "set config failed: %d", err);
		BK_LOGW(BK_JS_NET_TAG, "wifi set config failed: %d\r\n", err);
		return -1;
	}

	/* The station may already be running; the IP poll below is authoritative. */
	(void)bk_wifi_sta_start();

	while (waited < JS_NET_WIFI_TIMEOUT_MS) {
		netif_ip4_config_t ip4;

		os_memset(&ip4, 0, sizeof(ip4));
		if (bk_netif_get_ip4_config(NETIF_IF_STA, &ip4) == BK_OK &&
		    ip4.ip[0] != '\0' && strcmp(ip4.ip, "0.0.0.0") != 0) {
			js_net_set_text(res->text, sizeof(res->text), ip4.ip);
			res->value = 1;
			BK_LOGI(BK_JS_NET_TAG, "wifi connected, ip %s\r\n", ip4.ip);
			return 0;
		}

		rtos_delay_milliseconds(JS_NET_WIFI_POLL_MS);
		waited += JS_NET_WIFI_POLL_MS;
	}

	js_net_set_text(res->text, sizeof(res->text), "no ip address (timeout)");
	BK_LOGW(BK_JS_NET_TAG, "wifi connect timeout\r\n");
	return -1;
}

static void js_net_task_entry(beken_thread_arg_t arg)
{
	js_net_req_t req;

	(void)arg;

	for (;;) {
		if (rtos_pop_from_queue(&s_req_q, &req, BEKEN_WAIT_FOREVER) != kNoErr) {
			continue;
		}

		js_net_result_t *res = (js_net_result_t *)os_malloc(sizeof(js_net_result_t));

		if (res != NULL) {
			os_memset(res, 0, sizeof(*res));
			res->id = req.id;
			res->kind = req.kind;

			if (req.kind == JS_NET_REQ_WIFI_CONNECT) {
				js_net_wifi_run(&req, res);
			} else {
				js_net_http_run(&req, res);
			}

			if (!bk_js_post_net_result(res)) {
				bk_js_net_result_free(res);
			}
		}

		js_net_free_req_post(&req);
	}
}

static bool js_net_ensure_task(void)
{
	if (s_task_running) {
		return true;
	}

	if (s_req_q == NULL) {
		if (rtos_init_queue(&s_req_q, "js_net_q", sizeof(js_net_req_t), JS_NET_QUEUE_LEN) != kNoErr) {
			s_req_q = NULL;
			return false;
		}
	}

	if (rtos_create_thread(&s_task, BEKEN_DEFAULT_WORKER_PRIORITY, "js_net",
	                       js_net_task_entry, JS_NET_TASK_STACK, NULL) != kNoErr) {
		rtos_deinit_queue(&s_req_q);
		s_req_q = NULL;
		return false;
	}

	s_task_running = true;
	return true;
}

/* ------------------------------------------------------------------ */
/* Request submission                                                 */
/* ------------------------------------------------------------------ */

static jerry_value_t js_net_submit(js_net_req_t *req)
{
	js_net_pending_t *slot;
	jerry_value_t promise;

	if (!js_net_ensure_task()) {
		return js_net_throw(JERRY_ERROR_COMMON, "network task unavailable");
	}

	promise = jerry_promise();
	if (jerry_value_is_exception(promise)) {
		return promise;
	}

	slot = js_net_pending_alloc(req->kind, promise);
	if (slot == NULL) {
		jerry_value_free(promise);
		return js_net_throw(JERRY_ERROR_COMMON, "too many pending network requests");
	}

	req->id = slot->id;
	if (rtos_push_to_queue(&s_req_q, req, 0) != kNoErr) {
		js_net_pending_release(slot);
		jerry_value_free(promise);
		return js_net_throw(JERRY_ERROR_COMMON, "network queue full");
	}

	return promise;
}

static jerry_value_t js_net_http_submit(const char *url, bool post, char *body, uint32_t body_len,
                                        const char *content_type, int32_t timeout_ms)
{
	js_net_req_t req;
	jerry_value_t promise;

#ifndef WEBCLIENT_USING_MBED_TLS
	if (strncmp(url, "https://", 8) == 0) {
		if (body != NULL) {
			os_free(body);
		}
		return js_net_throw(JERRY_ERROR_COMMON, "https is not enabled in this firmware");
	}
#endif

	os_memset(&req, 0, sizeof(req));
	req.kind = JS_NET_REQ_HTTP;
	req.is_post = post ? 1 : 0;
	req.timeout_ms = timeout_ms;
	js_net_set_text(req.url, sizeof(req.url), url);
	js_net_set_text(req.method, sizeof(req.method), post ? "POST" : "GET");
	js_net_set_text(req.content_type, sizeof(req.content_type), content_type);
	req.post = body;
	req.post_len = body_len;

	promise = js_net_submit(&req);
	if (body != NULL && jerry_value_is_exception(promise)) {
		/* The request never entered the queue, the caller still owns it. */
		os_free(body);
	}
	return promise;
}

/* ------------------------------------------------------------------ */
/* JavaScript bindings                                                */
/* ------------------------------------------------------------------ */

static jerry_value_t js_m_wifi_connect(const jerry_call_info_t *call_info_p,
                                       const jerry_value_t args_p[], const jerry_length_t args_count)
{
	js_net_req_t req;

	(void)call_info_p;

	os_memset(&req, 0, sizeof(req));
	req.kind = JS_NET_REQ_WIFI_CONNECT;
	req.timeout_ms = JS_NET_WIFI_TIMEOUT_MS;

	if (args_count < 1 || !js_net_value_to_string(args_p[0], req.ssid, sizeof(req.ssid))) {
		return js_net_throw(JERRY_ERROR_TYPE, "wifi.connect(ssid, password)");
	}

	if (args_count >= 2 && jerry_value_is_string(args_p[1])) {
		if (!js_net_value_to_string(args_p[1], req.password, sizeof(req.password))) {
			return js_net_throw(JERRY_ERROR_RANGE, "password too long");
		}
	}

	return js_net_submit(&req);
}

static jerry_value_t js_m_wifi_status(const jerry_call_info_t *call_info_p,
                                      const jerry_value_t args_p[], const jerry_length_t args_count)
{
	wifi_link_status_t link;
	netif_ip4_config_t ip4;
	jerry_value_t object;
	bool connected;

	(void)call_info_p;
	(void)args_p;
	(void)args_count;

	os_memset(&link, 0, sizeof(link));
	os_memset(&ip4, 0, sizeof(ip4));

	(void)bk_wifi_sta_get_link_status(&link);
	(void)bk_netif_get_ip4_config(NETIF_IF_STA, &ip4);

	connected = (link.state == WIFI_LINKSTATE_STA_CONNECTED ||
	             link.state == WIFI_LINKSTATE_STA_GOT_IP) &&
	            ip4.ip[0] != '\0' && strcmp(ip4.ip, "0.0.0.0") != 0;

	object = jerry_object();
	js_net_obj_set_bool(object, "connected", connected);
	js_net_obj_set_num(object, "state", (double)link.state);
	js_net_obj_set_str(object, "ssid", link.ssid);
	js_net_obj_set_num(object, "rssi", (double)link.rssi);
	js_net_obj_set_str(object, "ip", ip4.ip);
	js_net_obj_set_str(object, "gateway", ip4.gateway);
	js_net_obj_set_str(object, "dns", ip4.dns);
	return object;
}

static jerry_value_t js_m_http_get(const jerry_call_info_t *call_info_p,
                                   const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char url[JS_NET_URL_MAX];

	(void)call_info_p;

	if (args_count < 1 || !js_net_value_to_string(args_p[0], url, sizeof(url))) {
		return js_net_throw(JERRY_ERROR_TYPE, "http.get(url)");
	}

	return js_net_http_submit(url, false, NULL, 0, NULL, JS_NET_TIMEOUT_MS);
}

static jerry_value_t js_m_http_post(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char url[JS_NET_URL_MAX];
	char content_type[JS_NET_CONTENT_TYPE_MAX] = "application/octet-stream";
	char *body = NULL;
	uint32_t body_len = 0;
	int32_t timeout = JS_NET_TIMEOUT_MS;

	(void)call_info_p;

	if (args_count < 1 || !js_net_value_to_string(args_p[0], url, sizeof(url))) {
		return js_net_throw(JERRY_ERROR_TYPE, "http.post(url, body)");
	}

	if (args_count >= 2 && jerry_value_is_string(args_p[1])) {
		body = js_net_string_dup(args_p[1], JS_NET_POST_MAX, &body_len);
		if (body == NULL) {
			return js_net_throw(JERRY_ERROR_RANGE, "body too long or out of memory");
		}
		/* An empty body is treated as no body by webclient_post(). */
		if (body_len == 0) {
			os_free(body);
			body = NULL;
		}
	}

	if (args_count >= 3 && jerry_value_is_object(args_p[2])) {
		js_net_opt_string(args_p[2], "contentType", content_type, sizeof(content_type));
		js_net_opt_number(args_p[2], "timeout", &timeout);
	}

	return js_net_http_submit(url, true, body, body_len, content_type, timeout);
}

static jerry_value_t js_m_http_request(const jerry_call_info_t *call_info_p,
                                       const jerry_value_t args_p[], const jerry_length_t args_count)
{
	char url[JS_NET_URL_MAX];
	char method[JS_NET_METHOD_MAX] = "GET";
	char content_type[JS_NET_CONTENT_TYPE_MAX] = "";
	char *body = NULL;
	uint32_t body_len = 0;
	int32_t timeout = JS_NET_TIMEOUT_MS;
	bool post = false;
	jerry_value_t body_value;

	(void)call_info_p;

	if (args_count < 1 || !js_net_value_to_string(args_p[0], url, sizeof(url))) {
		return js_net_throw(JERRY_ERROR_TYPE, "http.request(url, options)");
	}

	if (args_count >= 2 && jerry_value_is_object(args_p[1])) {
		js_net_opt_string(args_p[1], "method", method, sizeof(method));
		js_net_opt_string(args_p[1], "contentType", content_type, sizeof(content_type));
		js_net_opt_number(args_p[1], "timeout", &timeout);

		body_value = jerry_object_get_sz(args_p[1], "body");
		if (jerry_value_is_string(body_value)) {
			body = js_net_string_dup(body_value, JS_NET_POST_MAX, &body_len);
			if (body == NULL) {
				jerry_value_free(body_value);
				return js_net_throw(JERRY_ERROR_RANGE, "body too long or out of memory");
			}
			/* An empty body is treated as no body by webclient_post(). */
			if (body_len == 0) {
				os_free(body);
				body = NULL;
			}
		}
		jerry_value_free(body_value);
	}

	/* webclient exposes GET and POST only. */
	if (strcmp(method, "GET") == 0) {
		if (body != NULL) {
			os_free(body);
			return js_net_throw(JERRY_ERROR_TYPE, "GET cannot carry a body");
		}
		post = false;
	} else if (strcmp(method, "POST") == 0) {
		post = true;
	} else {
		if (body != NULL) {
			os_free(body);
		}
		return js_net_throw(JERRY_ERROR_TYPE, "only GET and POST are supported");
	}

	return js_net_http_submit(url, post, body, body_len, content_type, timeout);
}

/* ------------------------------------------------------------------ */
/* Result delivery (JavaScript task)                                  */
/* ------------------------------------------------------------------ */

void bk_js_net_deliver(void *payload)
{
	js_net_result_t *res = (js_net_result_t *)payload;
	js_net_pending_t *slot;

	if (res == NULL) {
		return;
	}

	slot = js_net_pending_find(res->id);
	if (slot == NULL) {
		/* The runtime was torn down or the request was cancelled. */
		bk_js_net_result_free(res);
		return;
	}

	if (res->kind == JS_NET_REQ_WIFI_CONNECT) {
		jerry_value_t object = jerry_object();

		js_net_obj_set_bool(object, "connected", res->value != 0);
		if (res->value != 0) {
			js_net_obj_set_str(object, "ip", res->text);
		} else {
			js_net_obj_set_str(object, "error", res->text);
		}
		js_net_settle(slot, object, res->value != 0);
	} else if (res->status > 0) {
		jerry_value_t object = jerry_object();
		jerry_value_t body;

		body = jerry_string((const jerry_char_t *)(res->body != NULL ? res->body : ""),
		                    (jerry_size_t)(res->body != NULL ? res->length : 0),
		                    JERRY_ENCODING_UTF8);

		js_net_obj_set_num(object, "status", (double)res->status);
		js_net_obj_set_num(object, "length", (double)res->length);
		js_net_obj_set(object, "body", body);
		js_net_settle(slot, object, true);
	} else {
		js_net_settle(slot, jerry_string_sz(res->text[0] != '\0' ? res->text : "http request failed"),
		              false);
	}

	bk_js_net_result_free(res);
}

void bk_js_net_bind(jerry_value_t global)
{
	jerry_value_t wifi = jerry_object();
	jerry_value_t http = jerry_object();

	js_net_set_fn(wifi, "connect", js_m_wifi_connect);
	js_net_set_fn(wifi, "status", js_m_wifi_status);
	js_net_obj_set(global, "wifi", wifi);

	js_net_set_fn(http, "get", js_m_http_get);
	js_net_set_fn(http, "post", js_m_http_post);
	js_net_set_fn(http, "request", js_m_http_request);
	js_net_obj_set(global, "http", http);

	BK_LOGI(BK_JS_NET_TAG, "network bindings registered\r\n");
}

#endif /* CONFIG_BK_JERRYSCRIPT_NET */
