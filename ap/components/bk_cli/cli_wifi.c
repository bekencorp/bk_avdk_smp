#include <string.h>
#include <common/sys_config.h>
#include "bk_uart.h"
#if CONFIG_LWIP
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
#endif
#include "lwip/ip4.h"
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"
#include "bk_cli.h"
#include "cli.h"
#include <components/event.h>
#include <components/netif.h>
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"

#include "ftp/ftpd.h"

#define TAG "wifi_cli"
//#define CMD_WLAN_MAX_BSS_CNT	50
//beken_semaphore_t wifi_cmd_sema = NULL;
//int wifi_cmd_status = 0;


#if (CLI_CFG_WIFI == 1)

void cli_wifi_scan_help(void)
{
	CLI_RAW_LOGI("\r\nscan [ssid] \n");
	CLI_RAW_LOGI("  Scan APs. \n");
	CLI_RAW_LOGI("  -ssid <string><optional>: SSID of AP. No need to fill it in if not to specify ssid. \n");
	CLI_RAW_LOGI("  example1: scan \n");
	CLI_RAW_LOGI("  example2: scan Redmi_253C \n");
}

void cli_wifi_ap_help(void)
{
	CLI_RAW_LOGI("\r\nap {ssid} [password] [channel] [hidden] [disable_dns_server] \n");
	CLI_RAW_LOGI("  Start a softap. \n");
	CLI_RAW_LOGI("  -ssid <string><mandatory>: SSID of AP. \n");
	CLI_RAW_LOGI("  -password <string><optional>: password of AP. No need to fill it in if no password. Set 0 to skip this parameter. \n");
	CLI_RAW_LOGI("  -channel <int><optional>: channel of AP. No need to fill it in if use default channel. Set 0 to use default channel or skip this parameter. \n");
	CLI_RAW_LOGI("  -hidden <bool><optional>: set softap hidden. No need to fill it in if not to hide softap. Accept true/1 or false/0. \n");
	CLI_RAW_LOGI("  -disable_dns_server <bool><optional>: disable DNS server. No need to fill it in if not to disable DNS server. Accept true/1 or false/0. \n");
	CLI_RAW_LOGI("  example1: ap default_ssid  \n");
	CLI_RAW_LOGI("  example2: ap default_ssid 12345678 \n");
	CLI_RAW_LOGI("  example3: ap default_ssid 12345678 6 \n");
	CLI_RAW_LOGI("  example4: ap default_ssid 12345678 0 true \n");
	CLI_RAW_LOGI("  example5: ap default_ssid 0 0 true \n");
	CLI_RAW_LOGI("  example6: ap default_ssid 12345678 6 false true \n");
}

void cli_wifi_sta_help(void)
{
	CLI_RAW_LOGI("\r\nsta {ssid} [password] [bssid] [channel] [psk] \n");
	CLI_RAW_LOGI("  Start a station, and connect to specific AP. \n");
	CLI_RAW_LOGI("  -ssid <string><mandatory>: SSID of AP. \n");
	CLI_RAW_LOGI("  -password <string><optional>: password of AP. No need to fill it in if no password. Set 0 to skip this parameter. \n");
	CLI_RAW_LOGI("  -bssid <mac><optional>: bssid of AP without ':'. No need to fill it in if not to specify bssid. Set 0 to skip this parameter. \n");
	CLI_RAW_LOGI("  -channel <int><optional>: channel of AP. No need to fill it in if not to specify channel. Set 0 to skip this parameter. \n");
	CLI_RAW_LOGI("  -psk <string><optional>:psk of AP. No need to fill it in if not to specify psk. \n");
	CLI_RAW_LOGI("  example1: sta Redmi_253C \n");
	CLI_RAW_LOGI("  example2: sta Redmi_253C 12345678 \n");
	CLI_RAW_LOGI("  example3: sta Redmi_253C 12345678 24cf243a253e \n");
	CLI_RAW_LOGI("  example4: sta Redmi_253C 12345678 0 6 \n");
	CLI_RAW_LOGI("  example5: sta Redmi_253C 12345678 24cf243a253e 6 6be4b3f9d9c752e2bbd32ef0d4d8641af6ae220832a8349ffda4ee361f416ab6 \n");
}

void cli_wifi_stop_help(void)
{
	CLI_RAW_LOGI("\r\nstop {sta|ap} \n");
	CLI_RAW_LOGI("  Stop station or AP. \n");
	CLI_RAW_LOGI("  -sta/ap<string><mandatory>: stop station or AP \n");
	CLI_RAW_LOGI("  example1: stop sta \n");
	CLI_RAW_LOGI("  example2: stop ap \n");
}

void cli_wifi_set_interval_help(void)
{
	CLI_RAW_LOGI("\r\nset_interval {0~255} \n");
	CLI_RAW_LOGI("  Set listen interval. \n");
	CLI_RAW_LOGI("  -value<int><mandatory>: listen interval,recommend 1/3/10 \n");
	CLI_RAW_LOGI("  example1: set_interval 10 \n");
}

void cli_wifi_monitor_help(void)
{
	CLI_RAW_LOGI("\r\nmonitor {start|stop|show|chan} \n");
	CLI_RAW_LOGI("  Control a wifi monitor. \n");
	CLI_RAW_LOGI("  -start<int><mandatory>: start a monitor on specific channel \n");
	CLI_RAW_LOGI("  -stop: stop monitor \n");
	CLI_RAW_LOGI("  -show: show monitor results \n");
	CLI_RAW_LOGI("  -chan<int><mandatory>: change monitor channel \n");
	CLI_RAW_LOGI("  example1: monitor start 1 \n");
	CLI_RAW_LOGI("  example2: monitor stop \n");
	CLI_RAW_LOGI("  example3: monitor show \n");
	CLI_RAW_LOGI("  example4: monitor chan 6 \n");
}

void cli_wifi_state_help(void)
{
	CLI_RAW_LOGI("\r\nstate\n");
	CLI_RAW_LOGI("  Show the state of station and softap.\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: state\n");
}

void cli_wifi_ps_help(void)
{
	CLI_RAW_LOGI("\r\nps {open|close}\n");
	CLI_RAW_LOGI("  PS open or close. \n");
	CLI_RAW_LOGI("  -open/close:open or close power save \n");
	CLI_RAW_LOGI("  example1: ps open \n");
	CLI_RAW_LOGI("  example2: ps close \n");
}

#if CONFIG_BRIDGE
void cli_wifi_bridge_help(void)
{
	CLI_RAW_LOGI("\r\nbridge {open|close} [ssid] [key]\n");
	CLI_RAW_LOGI("  Control WiFi bridge. \n");
	CLI_RAW_LOGI("  -open <string><mandatory>: external STA SSID to connect. \n");
	CLI_RAW_LOGI("  -key <string><optional>: password of external STA. Set 0 to skip. \n");
	CLI_RAW_LOGI("  example1: bridge open ext_ap 12345678 \n");
	CLI_RAW_LOGI("  example2: bridge close \n");
}
#endif

#if CONFIG_P2P
void cli_wifi_p2p_help(void)
{
	CLI_RAW_LOGI("\r\np2p {enable|find|listen|stop_find|connect|cancel}\n");
	CLI_RAW_LOGI("  Control WiFi P2P operations. \n");
	CLI_RAW_LOGI("  -enable <string><optional>: enable P2P with optional device name. \n");
	CLI_RAW_LOGI("  -find: start peer discovery. \n");
	CLI_RAW_LOGI("  -listen: enter listen state. \n");
	CLI_RAW_LOGI("  -stop_find: stop peer discovery. \n");
	CLI_RAW_LOGI("  -connect <dev> <method> <intent>: connect to peer. \n");
	CLI_RAW_LOGI("  -cancel: cancel ongoing P2P connection. \n");
	CLI_RAW_LOGI("  -disable: disable P2P. \n");
}
#endif


static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

static int cli_hexstr2bin(const char *hex, u8 *buf, size_t len)
{
	size_t i;
	int a;
	const char *ipos = hex;
	u8 *opos = buf;

	for (i = 0; i < len; i++) {
		a = hex2byte(ipos);
		if (a < 0)
			return -1;
		*opos++ = a;
		ipos += 2;
	}
	return 0;
}

const char *cli_wifi_sec_type_string(wifi_security_t security)
{
	switch (security) {
	case WIFI_SECURITY_NONE:
		return "NONE";
	case WIFI_SECURITY_WEP:
		return "WEP";
	case WIFI_SECURITY_WPA_TKIP:
		return "WPA-TKIP";
	case WIFI_SECURITY_WPA_AES:
		return "WPA-AES";
	case WIFI_SECURITY_WPA_MIXED:
		return "WPA-MIX";
	case WIFI_SECURITY_WPA2_TKIP:
		return "WPA2-TKIP";
	case WIFI_SECURITY_WPA2_AES:
		return "WPA2-AES";
	case WIFI_SECURITY_WPA2_MIXED:
		return "WPA2-MIX";
	case WIFI_SECURITY_WPA3_SAE:
		return "WPA3-SAE";
	case WIFI_SECURITY_WPA3_WPA2_MIXED:
		return "WPA3-WPA2-MIX";
	case WIFI_SECURITY_EAP:
		return "EAP";
	case WIFI_SECURITY_OWE:
		return "OWE";
	case WIFI_SECURITY_AUTO:
		return "AUTO";
#ifdef CONFIG_WAPI_SUPPORT
	case WIFI_SECURITY_TYPE_WAPI_PSK:
		return "WAPI_PSK";
	case WIFI_SECURITY_TYPE_WAPI_CERT:
		return "WAPI_CERT";
#endif
	default:
		return "UNKNOWN";
	}
}

static int cli_wifi_scan_done_handler(void *arg, event_module_t event_module,
								  int event_id, void *_event_data)
{
	wifi_scan_result_t scan_result = {0};

	BK_LOG_ON_ERR(bk_wifi_scan_get_result(&scan_result));
	BK_LOG_ON_ERR(bk_wifi_scan_dump_result(&scan_result));
	bk_wifi_scan_free_result(&scan_result);

	return BK_OK;
}

void cli_wifi_scan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	wifi_scan_config_t scan_config = {0};
	char *ap_ssid = NULL;
	int ret = 0;
	char *msg = NULL;

	if (argc > 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_scan_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_scan_help();
		goto succeed;
	}

	if (argc == 2) {
		ap_ssid = argv[1];
		os_strcpy(scan_config.ssid, ap_ssid);
	}

	bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
							   cli_wifi_scan_done_handler, rtos_get_current_thread());


	BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));

succeed:
	if(ret == 0)
	{
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

#include "conv_utf8_pub.h"
#ifdef CONFIG_WIFI_SOFTAP
void cli_wifi_ap_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};
	int len;
	char *ap_ssid = NULL;
	char *ap_key = "";
	int channel = 0;
	bool hide_ssid = false;
	int ret = 0;
	char *msg = NULL;

	if ((argc < 2) || (argc > 6)){
		CLI_LOGW("invalid argc number\n");
		cli_wifi_ap_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_ap_help();
		goto succeed;
	}

	if (argc > 1) {
		ap_ssid = argv[1];
		len = os_strlen(ap_ssid);
		if (32 < len) {
			CLI_LOGE("ssid name more than 32 Bytes\r\n");
			goto error;
		}
	}

	if (argc > 2) {
		if ((os_strlen(argv[2]) > 1) || os_strcmp(argv[2], "0"))
			ap_key = argv[2];
	}

	if (argc > 3) {
		char *end;
		channel = strtol(argv[3], &end, 0);
		if (*end) {
			CLI_LOGE("Invalid channle number '%s'", argv[2]);
			goto error;
		}
	}

	if (argc > 4) {
		if ((!os_strncmp(argv[4], "true", 4)) || !os_strcmp(argv[4], "1"))
			hide_ssid = true;
		else if ((!os_strncmp(argv[4], "false", 5)) || !os_strcmp(argv[4], "0"))
			hide_ssid = false;
		else {
			CLI_LOGW("invalid paramter of hidden!\n");
			cli_wifi_ap_help();
			goto error;
		}
	}

	if (argc > 5) {
		if ((!os_strncmp(argv[5], "true", 4)) || !os_strcmp(argv[5], "1"))
			ap_config.disable_dns_server = true;
		else if ((!os_strncmp(argv[5], "false", 5)) || !os_strcmp(argv[5], "0"))
			ap_config.disable_dns_server = false;
		else {
			CLI_LOGW("invalid paramter of disable_dns_server!\n");
			cli_wifi_ap_help();
			goto error;
		}
	}

	os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
	os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
	os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
	os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
	ret = bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);

	os_strcpy(ap_config.ssid, ap_ssid);
	os_strcpy(ap_config.password, ap_key);

	if (channel) {
		ap_config.channel = channel;
	}

	ap_config.hidden = hide_ssid;

	CLI_LOGI("Start softap. ssid:%s key:%s chan:%d hidden:%d disable_dns_server:%d\r\n",
				ap_config.ssid, ap_config.password, ap_config.channel, ap_config.hidden, ap_config.disable_dns_server);
	ap_config.hidden = true;
	ret = bk_wifi_ap_set_config(&ap_config);
	ret = bk_wifi_ap_start();

succeed:
	if(ret == 0)
	{
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

#endif

void cli_wifi_stop_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc != 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_stop_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_stop_help();
		goto succeed;
	}

	if (os_strcmp(argv[1], "sta") == 0) {
		ret = bk_wifi_sta_stop();
	}
#ifdef CONFIG_WIFI_SOFTAP
	else if (os_strcmp(argv[1], "ap") == 0)
		ret = bk_wifi_ap_stop();
#endif
	else {
		CLI_LOGW("unknown WiFi interface\n");
		goto error;
	}

succeed:
	if (ret == 0) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

void cli_monitor_start(uint32_t primary_channel)
{
	wifi_channel_t chan = {0};

	chan.primary = primary_channel;

	//BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(cli_monitor_cb));
	BK_LOG_ON_ERR(bk_wifi_monitor_result_register());
	BK_LOG_ON_ERR(bk_wifi_monitor_start());
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
}

static bool wifi_5g_chan_usable(int chan)
{
#ifdef CONFIG_WIFI_BAND_5G
	static const int wifi_usable_5g_channel[] = {36, 40, 44, 48, 52, 56, 60, 64, 100, 104,
		108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 161, 165, 169, 173, 177, };

	for (int i = 0; i < ARRAY_SIZE(wifi_usable_5g_channel); i++)
		if (wifi_usable_5g_channel[i] == chan)
			return true;
#endif

	return false;
}

void cli_wifi_monitor_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	wifi_channel_t chan = {0};
	char *operation = "";
	uint32_t primary_channel;
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_monitor_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_monitor_help();
		goto succeed;
	}

	operation = argv[1];

	if (!os_strcmp(operation, "start")) {
		if (argc != 3) {
			CLI_LOGI("Invalid parameters\n");
			goto error;
		}
		primary_channel = os_strtoul(argv[2], NULL, 10);
		cli_monitor_start(primary_channel);
	} else if (!os_strcmp(operation, "stop")) {
		bk_wifi_monitor_stop();
	} else if (!os_strcmp(operation, "show")) {
		bk_wifi_monitor_get_result();
	} else if (!os_strcmp(operation, "chan")) {
		primary_channel = os_strtoul(argv[2], NULL, 10);
		if ((primary_channel > 0 && primary_channel < 15) || wifi_5g_chan_usable(primary_channel))
		{
			CLI_LOGI("monitor set to channel %d\r\n", primary_channel);
			chan.primary = primary_channel;
			ret = bk_wifi_monitor_set_channel(&chan);
		} else {
			CLI_LOGW("monitor channel invalid\r\n");
			goto error;
		}
	} else {
		CLI_LOGW("bad parameters\r\n");
		cli_wifi_monitor_help();
		goto error;
	}

succeed:
	if (ret == 0) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}


void cli_wifi_set_interval_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t interval = 0;
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGW("invalid argc num");
		cli_wifi_set_interval_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_set_interval_help();
		goto succeed;
	}

	interval = (uint8_t)os_strtoul(argv[1], NULL, 10);
	ret = bk_wifi_send_listen_interval_req(interval);

	CLI_LOGI("set_interval %s \n", ret? "failed": "ok");

succeed:
	if (ret == 0) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}


void cli_wifi_sta_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
	char *ssid = NULL;
	char *password = "";
	uint8_t bssid[6] = {0};
	int channel = 0;
	uint8_t *psk = 0;
	char *msg = NULL;
	int ret = 0;

	if ((argc < 2) || (argc > 6)) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_sta_help();
		goto error;
	}

	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_sta_help();
		goto succeed;
	}

	if (argc > 1) {
		ssid = argv[1];
	}

	if (argc > 2) {
		if ((os_strlen(argv[2]) > 1) || os_strcmp(argv[2], "0"))
			password = argv[2];
	}

	if (argc > 3) {
		if ((os_strlen(argv[3]) > 1) || os_strcmp(argv[3], "0"))
			cli_hexstr2bin(argv[3], bssid, 6);
	}

	if (argc > 4) {
		char *end;
		channel = strtol(argv[4], &end, 0);
		if (*end) {
			CLI_LOGE("Invalid channel number '%s'", argv[2]);
			goto error;
		}
	}

	if (argc >= 5) {
		psk = (uint8_t *)argv[5];
	}

	os_strcpy(sta_config.ssid, ssid);
	os_strcpy(sta_config.password, password);
	#ifdef CONFIG_BSSID_CONNECT
	os_memcpy(sta_config.bssid, bssid, 6);
	#endif
	sta_config.channel = channel;
	#ifdef CONFIG_CONNECT_THROUGH_PSK_OR_SAE_PASSWORD
	if (psk) {
		sta_config.psk_len = PMK_LEN * 2;
		sta_config.psk_calculated = true;
		os_strlcpy((char *)sta_config.psk, (char *)psk, sizeof(sta_config.psk));
	}
	#endif

	CLI_LOGI("Station connect. ssid:%s bssid %02x:%02x:%02x:%02x:%02x:%02x password:%s chan:%d psk:%s \r\n",
					ssid, sta_config.bssid[0], sta_config.bssid[1], sta_config.bssid[2], sta_config.bssid[3],
					sta_config.bssid[4], sta_config.bssid[5], password, channel, psk);

	ret = bk_wifi_sta_set_config(&sta_config);
	ret = bk_wifi_sta_start();

succeed:
	if(ret == 0)
	{
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

int cli_wifi_state_handle(void)
{
#if CONFIG_LWIP
	wifi_link_status_t link_status = {0};
	wifi_ap_config_t ap_info = {0};
	netif_ip4_config_t ap_ip4_info = {0};
	char ssid[33] = {0};
	wifi_linkstate_reason_t info = mhdr_get_station_status();

	BK_LOGI(TAG, "[KW:]sta: %d, ap: %d, b/g/n\r\n", !!(info.state == WIFI_LINKSTATE_STA_GOT_IP), uap_ip_is_start());

	if (sta_ip_is_start()) {
		os_memset(&link_status, 0x0, sizeof(link_status));
		BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
		os_memcpy(ssid, link_status.ssid, 32);

		BK_LOGI(TAG, "[KW:]sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
				   link_status.rssi, link_status.aid, ssid, link_status.bssid,
				   link_status.channel, cli_wifi_sec_type_string(link_status.security));
	}

	if (uap_ip_is_start()) {
		os_memset(&ap_info, 0x0, sizeof(ap_info));
		BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
		os_memcpy(ssid, ap_info.ssid, 32);
		BK_LOGI(TAG, "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
				   ssid, ap_info.channel, cli_wifi_sec_type_string(ap_info.security));

		BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &ap_ip4_info));
		BK_LOGD(TAG, "[KW:]ip=%s,gate=%s,mask=%s,dns=%s\r\n",
				   ap_ip4_info.ip, ap_ip4_info.gateway, ap_ip4_info.mask, ap_ip4_info.dns);
	}
	return BK_OK;
#endif
}

void cli_wifi_state_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;
	
	if ((argc == 2) && (!os_strncmp(argv[1], "help", 4))) {
		cli_wifi_state_help();
		goto succeed;
	}

	if (argc > 1) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_state_help();
		goto error;
	}

	ret = cli_wifi_state_handle();

succeed:
	if (ret == 0) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

int cli_netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	netif_event_got_ip4_t *got_ip;

	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
		got_ip = (netif_event_got_ip4_t *)event_data;
		CLI_LOGW("%s got ip\n", got_ip->netif_if == NETIF_IF_STA ? "BK STA" : "unknown netif");
		break;
	default:
		CLI_LOGW("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

int cli_wifi_event_cb(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;
	wifi_event_sta_connected_t *sta_connected;
	wifi_event_ap_disconnected_t *ap_disconnected;
	wifi_event_ap_connected_t *ap_connected;

	switch (event_id) {
	case EVENT_WIFI_STA_CONNECTED:
		sta_connected = (wifi_event_sta_connected_t *)event_data;
		CLI_LOGW("BK STA connected %s\n", sta_connected->ssid);
		break;

	case EVENT_WIFI_STA_DISCONNECTED:
		sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
		CLI_LOGW("BK STA disconnected, reason(%d)%s\n", sta_disconnected->disconnect_reason,
			sta_disconnected->local_generated ? ", local_generated" : "");
		break;

	case EVENT_WIFI_AP_CONNECTED:
		ap_connected = (wifi_event_ap_connected_t *)event_data;
		CLI_LOGW(BK_MAC_FORMAT" connected to BK AP\n", BK_MAC_STR(ap_connected->mac));
		break;

	case EVENT_WIFI_AP_DISCONNECTED:
		ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
		CLI_LOGD(BK_MAC_FORMAT" disconnected from BK AP\n", BK_MAC_STR(ap_disconnected->mac));
            	break;

	default:
		CLI_LOGW("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

void cli_wifi_ps_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t ps_id = 0;
	uint8_t ps_val = 0;
	uint8_t ps_val1 = 0;
	int ret = 0;
	char *msg = NULL;

	if (argc != 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_ps_help();
		goto error;
	}

	if (!os_strncmp(argv[1], "help", 4)) {
		cli_wifi_ps_help();
		goto succeed;
	}

	if (os_strcmp(argv[1], "open") == 0) {
		ps_id = 0;
	} else if (os_strcmp(argv[1], "close") == 0) {
		ps_id = 1;
	} else {
		CLI_LOGW("invalid ps paramter\n");
		cli_wifi_ps_help();
		goto error;
	}

	bk_wifi_ps_config(ps_id, ps_val, ps_val1);

succeed:
	if (ret == 0) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

#if CONFIG_BRIDGE
void cli_wifi_bridge_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = BK_OK;
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_bridge_help();
		goto error;
	}

	if (!os_strncmp(argv[1], "help", 4)) {
		cli_wifi_bridge_help();
		goto succeed;
	}

	if (!os_strcmp(argv[1], "open")) {
		const char *ssid = NULL;
		const char *key = NULL;
		char br_ssid[64] = {0};
		bk_bridge_config_t br_config = {0};

		if (argc < 3) {
			CLI_LOGW("missing ssid parameter\n");
			cli_wifi_bridge_help();
			goto error;
		}

		ssid = argv[2];
		if (!ssid || !ssid[0]) {
			CLI_LOGW("invalid ssid parameter\n");
			goto error;
		}

		if (argc >= 4 && ((os_strlen(argv[3]) > 1) || os_strcmp(argv[3], "0")))
			key = argv[3];

		br_config.ext_sta_ssid = (char *)ssid;
		br_config.key = (char *)key;
		os_snprintf(br_ssid, sizeof(br_ssid), "%s_brr", ssid);
		br_config.bridge_ssid = br_ssid;

		ret = bk_bridge_start(&br_config);
		if (ret != BK_OK) {
			CLI_LOGE("bridge open failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "close")) {
		ret = bk_bridge_stop();
		if (ret != BK_OK) {
			CLI_LOGE("bridge close failed, err=%d\n", ret);
			goto error;
		}
	} else {
		CLI_LOGW("invalid bridge command\n");
		cli_wifi_bridge_help();
		goto error;
	}

succeed:
	if (ret == BK_OK) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

#endif

#if CONFIG_P2P
void cli_wifi_p2p_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = BK_OK;
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGW("invalid argc number\n");
		cli_wifi_p2p_help();
		goto error;
	}

	if (!os_strncmp(argv[1], "help", 4)) {
		cli_wifi_p2p_help();
		goto succeed;
	}

	if (!os_strcmp(argv[1], "enable")) {
		const char *p2p_ssid = NULL;
		if (argc >= 3)
			p2p_ssid = argv[2];
		ret = bk_wifi_p2p_enable(p2p_ssid);
		if (ret != BK_OK) {
			CLI_LOGE("p2p enable failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "find")) {
		ret = bk_wifi_p2p_find();
		if (ret != BK_OK) {
			CLI_LOGE("p2p find failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "listen")) {
		ret = bk_wifi_p2p_listen();
		if (ret != BK_OK) {
			CLI_LOGE("p2p listen failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "stop_find")) {
		ret = bk_wifi_p2p_stop_find();
		if (ret != BK_OK) {
			CLI_LOGE("p2p stop_find failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "connect")) {
		uint8_t *peer = NULL;
		int method = 0;
		int intent = 0;

		if (argc < 5) {
			CLI_LOGW("invalid parameters for connect\n");
			cli_wifi_p2p_help();
			goto error;
		}

		peer = (uint8_t *)argv[2];
		method = os_strtoul(argv[3], NULL, 10);
		intent = os_strtoul(argv[4], NULL, 10);
		ret = bk_wifi_p2p_connect(peer, method, intent);
		if (ret != BK_OK) {
			CLI_LOGE("p2p connect failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "cancel")) {
		ret = bk_wifi_p2p_cancel();
		if (ret != BK_OK) {
			CLI_LOGE("p2p cancel failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "disable")) {
		ret = bk_wifi_p2p_disable();
		if (ret != BK_OK) {
			CLI_LOGE("p2p disable failed, err=%d\n", ret);
			goto error;
		}
	} else {
		CLI_LOGW("invalid p2p command\n");
		cli_wifi_p2p_help();
		goto error;
	}

succeed:
	if (ret == BK_OK) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}
#endif

#define WIFI_CMD_CNT (sizeof(s_wifi_commands) / sizeof(struct cli_command))
static const struct cli_command s_wifi_commands[] = {
	{"scan", "scan [ssid]", cli_wifi_scan_cmd},
#ifdef CONFIG_WIFI_SOFTAP
	{"ap", "ap {ssid} [password] [channel] [hidden]", cli_wifi_ap_cmd},
#endif
	{"sta", "sta {ssid} [password] [bssid] [channel] [psk]", cli_wifi_sta_cmd},
	{"stop", "stop {sta|ap}", cli_wifi_stop_cmd},
	{"set_interval", "set_interval {0~255}", cli_wifi_set_interval_cmd},
	{"monitor", "monitor {start|stop|show|chan}", cli_wifi_monitor_cmd},
	{"state", "state", cli_wifi_state_cmd},
	{"ps","ps {open|close}", cli_wifi_ps_cmd},
#if CONFIG_BRIDGE
	{"bridge", "bridge {open|close}", cli_wifi_bridge_cmd},
#endif
#if CONFIG_P2P
	{"p2p", "p2p {enable|find|listen|stop_find|connect|cancel|disable}", cli_wifi_p2p_cmd},
#endif
};

int cli_wifi_init(void)
{
	#if CONFIG_WIFI_CLI_DEBUG
	extern int cli_wifi_debug_init(void);
	cli_wifi_debug_init();
	#endif
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, cli_wifi_event_cb, NULL));
	BK_LOG_ON_ERR(bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, cli_netif_event_cb, NULL));
	return cli_register_commands(s_wifi_commands, WIFI_CMD_CNT);
}

#endif //#if (CLI_CFG_WIFI == 1)
