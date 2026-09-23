#include <string.h>
#include <common/sys_config.h>
#include "bk_uart.h"
#if CONFIG_LWIP
#include <../../lwip_intf_v2_1/lwip-2.1.2/port/net.h>
#endif
#include "lwip/ip4.h"
#if CONFIG_IPV6
#include "lwip/ip6_addr.h"
#endif
#include "bk_private/bk_wifi.h"
#include "bk_wifi_private.h"
#include "bk_cli.h"
#include "cli.h"
#include <components/event.h>
#include <components/netif.h>
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include "wifi_api.h"
#include "soc_debug.h"

#include "ftp/ftpd.h"
#include <stdio.h>
#if CONFIG_ENABLE_WIFI_USER_FCI
#include <driver/flash_partition.h>
#endif

#define TAG "wifi_cli"
//#define CMD_WLAN_MAX_BSS_CNT	50
//beken_semaphore_t wifi_cmd_sema = NULL;
//int wifi_cmd_status = 0;

#if CONFIG_ENABLE_WIFI_USER_FCI
/*
 * Tuya-style user fast-connect: host self-manages FCI.
 * Persist to sys_net partition (not easy_flash), after MAC bytes.
 * WPA2: psk; WPA3: pmk/pmkid/akmp/pmf/tk — same wifi_sta_config_t fields.
 * TK required only when pmf == MGMT_FRAME_PROTECTION_REQUIRED (2).
 *
 * Layout in BK_PARTITION_SYS_NET (4KB):
 *   [0, 6)  WiFi base MAC (bk_system/mac.c) — must not overwrite
 *   [6, ..) user FCI blob (this CLI)
 * Write path uses partition read/erase/write RMW so MAC is preserved.
 * (bk_spec_flash_write_bytes is CP-only; not linked on AP.)
 */
#define CLI_WIFI_USER_FCI_MAGIC         0x55534632 /* 'USF2' */
#define CLI_WIFI_USER_FCI_FLASH_OFFSET  0x6        /* NET_INFO_FAST_CONNECT_OFFSET */

typedef struct {
	uint32_t magic;
	char ssid[WIFI_SSID_STR_LEN];
	uint8_t bssid[WIFI_BSSID_LEN];
	char password[WIFI_PASSWORD_LEN];
	uint8_t psk[WIFI_PASSWORD_LEN]; /* 64-char hex PSK string, same as CP fci.psk */
	uint8_t ip_addr[4];
	uint8_t netmask[4];
	uint8_t gw[4];
	uint8_t dns1[4];
	uint8_t tk[16];
	uint8_t channel;
	uint8_t pmf;
	wifi_security_t security;
	uint8_t pmk_len;
	uint8_t pmk[64];
	uint8_t pmkid[16];
	int akmp;
} cli_wifi_user_fci_t;

static cli_wifi_user_fci_t s_cli_wifi_user_fci;
static bool s_cli_wifi_user_fci_valid;
#endif /* CONFIG_ENABLE_WIFI_USER_FCI */


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
	CLI_RAW_LOGI("  Show the state of station, softap");
#if CONFIG_P2P
	CLI_RAW_LOGI(", p2p go/gc");
#endif
#if CONFIG_BRIDGE
	CLI_RAW_LOGI(" and bridge");
#endif
	CLI_RAW_LOGI(".\n");
	CLI_RAW_LOGI("  -no param\n");
	CLI_RAW_LOGI("  example1: state\n");
}

#if CONFIG_BRIDGE
static const char *cli_wifi_bridge_state_str(bk_bridge_state_t st)
{
	switch (st) {
	case BRIDGE_STATE_DISABLED:
		return "disabled";
	case BRIDGE_STATE_DISABLING:
		return "disabling";
	case BRIDGE_STATE_ENABLING:
		return "enabling";
	case BRIDGE_STATE_ENABLED:
		return "enabled";
	default:
		return "unknown";
	}
}
#endif

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
	CLI_RAW_LOGI("\r\nbridge {open|close} <sta_ssid> [key] [bridge_ssid] [keep_sta]\n");
	CLI_RAW_LOGI("  Control WiFi bridge. \n");
	CLI_RAW_LOGI("  -sta_ssid <string><mandatory>: external STA SSID to connect. \n");
	CLI_RAW_LOGI("  -key <string><optional>: password of external STA. Set 0 to skip. \n");
	CLI_RAW_LOGI("  -bridge_ssid <string><optional>: bridge softap SSID. Default: <sta_ssid>_brr \n");
	CLI_RAW_LOGI("  -keep_sta <0|1><optional>: 1=keep STA up after close, 0=stop STA on close (default) \n");
	CLI_RAW_LOGI("  example1: bridge open ext_ap 12345678 \n");
	CLI_RAW_LOGI("  example2: bridge open ext_ap 12345678 my_bridge \n");
	CLI_RAW_LOGI("  example3: bridge open ext_ap 0 my_bridge \n");
	CLI_RAW_LOGI("  example4: bridge open ext_ap 12345678 my_bridge 1 \n");
	CLI_RAW_LOGI("  example5: bridge close \n");
}
#endif

#if CONFIG_P2P
void cli_wifi_p2p_help(void)
{
	CLI_RAW_LOGI("\r\np2p {enable|find|listen|stop_find|connect|cancel}\n");
	CLI_RAW_LOGI("  Control WiFi P2P operations. \n");
	CLI_RAW_LOGI("  -enable [ssid] [intent]: enable P2P with optional device name and GO Intent (0-15). \n");
	CLI_RAW_LOGI("                           intent: 0=GC, 15=GO, 1-14=preference, -1=keep default. \n");
	CLI_RAW_LOGI("  -find: start peer discovery. \n");
	CLI_RAW_LOGI("  -listen: enter listen state. \n");
	CLI_RAW_LOGI("  -stop_find: stop peer discovery. \n");
	CLI_RAW_LOGI("  -noa <0|1>: disable or enable host P2P GO NoA (MCC concurrent NoA unaffected). \n");
	CLI_RAW_LOGI("  -connect <mac> <method> <intent>: connect to peer; mac is 12 hex digits (':' optional, same as sta bssid). \n");
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

/* Strip ':' so cli_hexstr2bin can parse MAC from log (%pm) or compact form. */
static int cli_mac_str_to_bin(const char *mac_str, u8 *mac)
{
	char compact[13];
	size_t di = 0, si = 0;

	if (!mac_str || !mac)
		return -1;

	while (mac_str[si] && di < sizeof(compact) - 1) {
		if (mac_str[si] != ':')
			compact[di++] = mac_str[si];
		si++;
	}
	if (di != 12)
		return -1;

	compact[12] = '\0';
	return cli_hexstr2bin(compact, mac, 6);
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

#if CONFIG_SOFTAP_WPA3
void cli_wifi_wpa3_ap_help(void)
{
	CLI_RAW_LOGI("\r\nap_wpa3 {ssid} [password] [channel] \n");
	CLI_RAW_LOGI("  Start a WPA3-SAE softap. \n");
	CLI_RAW_LOGI("  -ssid <string><mandatory>: SSID of AP. \n");
	CLI_RAW_LOGI("  -password <string><mandatory>: password of AP, 8~63 bytes (WPA3 passphrase). \n");
	CLI_RAW_LOGI("  -channel <int><optional>: channel 1~14. Omit to use default channel. \n");
	CLI_RAW_LOGI("  Note: with 3 args, a token of 1~2 chars is treated as channel, otherwise as password. \n");
	CLI_RAW_LOGI("  example1: ap_wpa3 myap 12345678 \n");
	CLI_RAW_LOGI("  example2: ap_wpa3 myap 12345678 6 \n");
}

void cli_wifi_wpa3_ap_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	wifi_ap_config_t ap_config = WIFI_DEFAULT_AP_CONFIG();
	netif_ip4_config_t ip4_config = {0};
	char *ap_ssid = NULL;
	char *ap_key = "";
	char *ap_channel = NULL;
	int len, key_len;
	int ret = BK_OK;
	char *msg = NULL;

	if ((argc < 2) || (argc > 4)) {
		CLI_LOGE("Invalid parameters\n");
		cli_wifi_wpa3_ap_help();
		goto error;
	}

	if ((argc == 2) && (os_strcmp(argv[1], "help") == 0)) {
		cli_wifi_wpa3_ap_help();
		goto succeed;
	}

	if (argc == 2)
		ap_ssid = argv[1];
	else if (argc == 3) {
		ap_ssid = argv[1];
		/* A purely numeric token of 1~2 chars is treated as channel */
		if (os_strlen(argv[2]) <= 2 && argv[2][0] >= '0' && argv[2][0] <= '9')
			ap_channel = argv[2];
		else
			ap_key = argv[2];
	} else {
		ap_ssid = argv[1];
		ap_key = argv[2];
		ap_channel = argv[3];
	}

	len = os_strlen(ap_ssid);
	if (len == 0 || len > 32) {
		CLI_LOGE("ssid name must be 1~32 Bytes\r\n");
		goto error;
	}

	key_len = os_strlen(ap_key);
	if (key_len < 8) {
		CLI_LOGE("WPA3 requires password >= 8 bytes\r\n");
		goto error;
	}
	if (key_len > 63) {
		CLI_LOGE("WPA3 passphrase must be <= 63 bytes\r\n");
		goto error;
	}

	os_strlcpy(ap_config.ssid, ap_ssid, sizeof(ap_config.ssid));
	os_strlcpy(ap_config.password, ap_key, sizeof(ap_config.password));

	if (ap_channel) {
		int channel;
		char *end;

		channel = strtol(ap_channel, &end, 0);
		if (*end || channel < 1 || channel > 14) {
			CLI_LOGE("Invalid channel '%s', valid range 1~14\r\n", ap_channel);
			goto error;
		}
		ap_config.channel = channel;
	}

	ap_config.security = WIFI_SECURITY_WPA3_SAE;

	CLI_LOGI("Start WPA3 softap. ssid:%s chan:%d\r\n",
		 ap_config.ssid, ap_config.channel);

	os_strcpy(ip4_config.ip, WLAN_DEFAULT_IP);
	os_strcpy(ip4_config.mask, WLAN_DEFAULT_MASK);
	os_strcpy(ip4_config.gateway, WLAN_DEFAULT_GW);
	os_strcpy(ip4_config.dns, WLAN_DEFAULT_GW);
	ret = bk_netif_set_ip4_config(NETIF_IF_AP, &ip4_config);
	if (ret != BK_OK)
		goto error;

	ret = bk_wifi_ap_set_config(&ap_config);
	if (ret != BK_OK)
		goto error;

	ret = bk_wifi_ap_start();
	if (ret != BK_OK)
		goto error;

succeed:
	msg = WIFI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

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


#if CONFIG_ENABLE_WIFI_USER_FCI
static void cli_wifi_ip_str_to_bytes(const char *str, uint8_t out[4])
{
	unsigned a = 0, b = 0, c = 0, d = 0;

	os_memset(out, 0, 4);
	if (!str || !str[0])
		return;
	if (sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
		out[0] = (uint8_t)a;
		out[1] = (uint8_t)b;
		out[2] = (uint8_t)c;
		out[3] = (uint8_t)d;
	}
}

static void cli_wifi_user_fci_dump(const cli_wifi_user_fci_t *fci)
{
	if (!fci)
		return;

	CLI_LOGI("user_fci magic=0x%x ssid=%s bssid=%02x:%02x:%02x:%02x:%02x:%02x "
		 "chan=%d sec=%d pmf=%d pwd_len=%d psk_len=%d pmk_len=%d akmp=0x%x\n",
		 fci->magic, fci->ssid,
		 fci->bssid[0], fci->bssid[1], fci->bssid[2],
		 fci->bssid[3], fci->bssid[4], fci->bssid[5],
		 fci->channel, fci->security, fci->pmf,
		 (int)os_strlen(fci->password), (int)os_strlen((char *)fci->psk),
		 fci->pmk_len, fci->akmp);
	CLI_LOGI("user_fci ip=%d.%d.%d.%d mask=%d.%d.%d.%d gw=%d.%d.%d.%d dns=%d.%d.%d.%d\n",
		 fci->ip_addr[0], fci->ip_addr[1], fci->ip_addr[2], fci->ip_addr[3],
		 fci->netmask[0], fci->netmask[1], fci->netmask[2], fci->netmask[3],
		 fci->gw[0], fci->gw[1], fci->gw[2], fci->gw[3],
		 fci->dns1[0], fci->dns1[1], fci->dns1[2], fci->dns1[3]);
}

static int cli_wifi_user_fci_collect(cli_wifi_user_fci_t *fci)
{
	wifi_sta_config_t cfg = {0};
	wifi_link_status_t link = {0};
	netif_ip4_config_t ip4 = {0};
	int ret;

	if (!fci)
		return BK_ERR_NULL_PARAM;

	os_memset(fci, 0, sizeof(*fci));
	fci->magic = CLI_WIFI_USER_FCI_MAGIC;

	ret = bk_wifi_sta_get_config(&cfg);
	if (ret != BK_OK) {
		CLI_LOGE("sta_get_config failed %d\n", ret);
		return ret;
	}

	ret = bk_wifi_sta_get_link_status(&link);
	if (ret != BK_OK) {
		CLI_LOGE("sta_get_link_status failed %d\n", ret);
		return ret;
	}

	ret = bk_netif_get_ip4_config(NETIF_IF_STA, &ip4);
	if (ret != BK_OK) {
		CLI_LOGW("get_ip4_config failed %d, save without IP\n", ret);
	}

	os_strlcpy(fci->ssid, link.ssid[0] ? link.ssid : cfg.ssid, sizeof(fci->ssid));
	os_memcpy(fci->bssid, link.bssid, WIFI_BSSID_LEN);
	os_strlcpy(fci->password, cfg.password, sizeof(fci->password));
	fci->channel = link.channel ? link.channel : cfg.channel;
	fci->security = link.security;
	fci->pmf = cfg.pmf;
	os_memcpy(fci->tk, cfg.tk, sizeof(fci->tk));
	fci->pmk_len = cfg.pmk_len;
	if (cfg.pmk_len > 0 && cfg.pmk_len <= sizeof(fci->pmk)) {
		os_memcpy(fci->pmk, cfg.pmk, cfg.pmk_len);
		os_memcpy(fci->pmkid, cfg.pmkid, sizeof(fci->pmkid));
		fci->akmp = cfg.akmp;
	}

	cli_wifi_ip_str_to_bytes(ip4.ip, fci->ip_addr);
	cli_wifi_ip_str_to_bytes(ip4.mask, fci->netmask);
	cli_wifi_ip_str_to_bytes(ip4.gateway, fci->gw);
	cli_wifi_ip_str_to_bytes(ip4.dns, fci->dns1);

	/* Hex PSK from STA config (computed on CP); required for WPA2 fast connect. */
	if (os_strlen((char *)cfg.psk) >= 64)
		os_memcpy(fci->psk, cfg.psk, WIFI_PASSWORD_LEN);
	else
		CLI_LOGW("no hex psk in sta_get_config, leave psk empty\n");

	if (fci->pmk_len == 0)
		CLI_LOGW("no SAE PMK in sta_get_config yet (connect once so CP stores FCI)\n");

	return BK_OK;
}

static int cli_wifi_user_fci_flash_write(const void *buf, uint32_t size)
{
	bk_logic_partition_t *pt;
	uint8_t *sector = NULL;
	bk_err_t ret = BK_FAIL;

	if (!buf || size == 0)
		return BK_ERR_PARAM;

	pt = bk_flash_partition_get_info(BK_PARTITION_SYS_NET);
	if (!pt) {
		CLI_LOGE("sys_net partition not found\n");
		return BK_FAIL;
	}
	if (CLI_WIFI_USER_FCI_FLASH_OFFSET + size > pt->partition_length) {
		CLI_LOGE("user_fci too large for sys_net (off=%u size=%u len=%u)\n",
			 (unsigned)CLI_WIFI_USER_FCI_FLASH_OFFSET, (unsigned)size,
			 (unsigned)pt->partition_length);
		return BK_ERR_PARAM;
	}

	sector = os_malloc(pt->partition_length);
	if (!sector) {
		CLI_LOGE("sys_net RMW malloc fail\n");
		return BK_ERR_NO_MEM;
	}

	/* RMW whole partition: preserves MAC at [0, 6) */
	ret = bk_flash_partition_read(BK_PARTITION_SYS_NET, sector, 0,
				      pt->partition_length);
	if (ret != BK_OK)
		goto out;

	os_memcpy(sector + CLI_WIFI_USER_FCI_FLASH_OFFSET, buf, size);

	ret = bk_flash_partition_erase(BK_PARTITION_SYS_NET, 0, pt->partition_length);
	if (ret != BK_OK)
		goto out;

	ret = bk_flash_partition_write(BK_PARTITION_SYS_NET, sector, 0,
				       pt->partition_length);

out:
	os_free(sector);
	if (ret != BK_OK) {
		CLI_LOGE("sys_net write user_fci failed %d\n", ret);
		return BK_FAIL;
	}
	return BK_OK;
}

static int cli_wifi_user_fci_flash_read(void *buf, uint32_t size)
{
	bk_err_t ret;

	if (!buf || size == 0)
		return BK_ERR_PARAM;

	ret = bk_flash_partition_read(BK_PARTITION_SYS_NET, buf,
				     CLI_WIFI_USER_FCI_FLASH_OFFSET, size);
	if (ret != BK_OK) {
		CLI_LOGE("sys_net read user_fci failed %d\n", ret);
		return BK_FAIL;
	}
	return BK_OK;
}

static int cli_wifi_user_fci_save(const cli_wifi_user_fci_t *fci)
{
	int ret;

	if (!fci || fci->magic != CLI_WIFI_USER_FCI_MAGIC)
		return BK_ERR_PARAM;

	os_memcpy(&s_cli_wifi_user_fci, fci, sizeof(*fci));
	s_cli_wifi_user_fci_valid = true;

	ret = cli_wifi_user_fci_flash_write(fci, sizeof(*fci));
	if (ret != BK_OK)
		return ret;

	CLI_LOGI("saved user_fci to sys_net off=0x%x size=%d (MAC untouched)\n",
		 CLI_WIFI_USER_FCI_FLASH_OFFSET, (int)sizeof(*fci));
	return BK_OK;
}

static int cli_wifi_user_fci_load(cli_wifi_user_fci_t *fci)
{
	int ret;

	if (!fci)
		return BK_ERR_NULL_PARAM;

	os_memset(fci, 0, sizeof(*fci));

	ret = cli_wifi_user_fci_flash_read(fci, sizeof(*fci));
	if (ret == BK_OK && fci->magic == CLI_WIFI_USER_FCI_MAGIC) {
		os_memcpy(&s_cli_wifi_user_fci, fci, sizeof(*fci));
		s_cli_wifi_user_fci_valid = true;
		return BK_OK;
	}
	CLI_LOGW("sys_net user_fci miss/invalid (ret=%d magic=0x%x), try RAM\n",
		 ret, fci->magic);

	if (s_cli_wifi_user_fci_valid &&
	    s_cli_wifi_user_fci.magic == CLI_WIFI_USER_FCI_MAGIC) {
		os_memcpy(fci, &s_cli_wifi_user_fci, sizeof(*fci));
		return BK_OK;
	}

	return BK_ERR_NOT_FOUND;
}

static int cli_wifi_user_fci_erase(void)
{
	cli_wifi_user_fci_t empty = {0};
	int ret;

	os_memset(&s_cli_wifi_user_fci, 0, sizeof(s_cli_wifi_user_fci));
	s_cli_wifi_user_fci_valid = false;

	ret = cli_wifi_user_fci_flash_write(&empty, sizeof(empty));
	if (ret != BK_OK)
		return ret;

	CLI_LOGI("user_fci erased from sys_net (MAC untouched)\n");
	return BK_OK;
}

static int cli_wifi_user_fci_connect(const cli_wifi_user_fci_t *fci)
{
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();
	int ret;
	int i;
	bool tk_valid = false;

	if (!fci || fci->magic != CLI_WIFI_USER_FCI_MAGIC)
		return BK_ERR_PARAM;

	for (i = 0; i < 16; i++) {
		if (fci->tk[i] != 0) {
			tk_valid = true;
			break;
		}
	}
	/* Match CP: TK only mandatory for PMF required (value 2). WPA2 may omit TK. */
	if (fci->pmf == 2 /* MGMT_FRAME_PROTECTION_REQUIRED */ && !tk_valid) {
		CLI_LOGE("user_fast_connect: host TK is empty (PMF required), abort\n");
		return BK_ERR_PARAM;
	}

	os_memset(&sta_config, 0, sizeof(sta_config));
	os_strlcpy(sta_config.ssid, fci->ssid, sizeof(sta_config.ssid));
	os_memcpy(sta_config.bssid, fci->bssid, WIFI_BSSID_LEN);
	os_strlcpy(sta_config.password, fci->password, sizeof(sta_config.password));
	os_memcpy(sta_config.psk, fci->psk, WIFI_PASSWORD_LEN);
	os_memcpy(sta_config.ip_addr, fci->ip_addr, 4);
	os_memcpy(sta_config.netmask, fci->netmask, 4);
	os_memcpy(sta_config.gw, fci->gw, 4);
	os_memcpy(sta_config.dns1, fci->dns1, 4);
	os_memcpy(sta_config.tk, fci->tk, 16);
	sta_config.channel = fci->channel;
	sta_config.security = fci->security;
	sta_config.pmf = fci->pmf;
	sta_config.pmk_len = fci->pmk_len;
	if (fci->pmk_len > 0 && fci->pmk_len <= sizeof(sta_config.pmk)) {
		os_memcpy(sta_config.pmk, fci->pmk, fci->pmk_len);
		os_memcpy(sta_config.pmkid, fci->pmkid, sizeof(sta_config.pmkid));
		sta_config.akmp = fci->akmp;
	}
	/* Host self-managed FCI (WPA2 psk / WPA3 pmk), no CP flash on reconnect */
	sta_config.is_user_fast_connect = 1;

	cli_wifi_user_fci_dump(fci);

	bk_wifi_sta_stop();
	ret = bk_wifi_sta_set_config(&sta_config);
	if (ret != BK_OK) {
		CLI_LOGE("sta_set_config failed %d\n", ret);
		return ret;
	}
	ret = bk_wifi_sta_start();
	if (ret != BK_OK)
		CLI_LOGE("sta_start failed %d\n", ret);
	return ret;
}

void cli_wifi_sta_fc_help(void)
{
	CLI_RAW_LOGI("\r\nsta_fc {save|connect|show|erase}\n");
	CLI_RAW_LOGI("  Host self-managed fast-connect (persist to sys_net, MAC kept).\n");
	CLI_RAW_LOGI("  save   : get_config FCI (WPA2 psk / WPA3 pmk/pmf/tk) -> sys_net\n");
	CLI_RAW_LOGI("  connect: load FCI, set is_user_fast_connect=1 (no CP flash)\n");
	CLI_RAW_LOGI("  show   : dump saved FCI\n");
	CLI_RAW_LOGI("  erase  : clear FCI in sys_net only (does not touch MAC)\n");
	CLI_RAW_LOGI("  note   : TK required only if pmf=required; WPA2 may omit TK\n");
	CLI_RAW_LOGI("  example: sta <ssid> <pwd> -> got ip -> sta_fc save\n");
	CLI_RAW_LOGI("           reboot -> sta_fc connect\n");
}

void cli_wifi_sta_fc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = WIFI_CMD_RSP_SUCCEED;
	int ret = BK_OK;
	cli_wifi_user_fci_t fci;

	if (argc < 2 || !os_strcmp(argv[1], "help")) {
		cli_wifi_sta_fc_help();
		goto out;
	}

	if (!os_strcmp(argv[1], "save")) {
		ret = cli_wifi_user_fci_collect(&fci);
		if (ret == BK_OK) {
			cli_wifi_user_fci_dump(&fci);
			ret = cli_wifi_user_fci_save(&fci);
		}
	} else if (!os_strcmp(argv[1], "connect")) {
		ret = cli_wifi_user_fci_load(&fci);
		if (ret == BK_OK)
			ret = cli_wifi_user_fci_connect(&fci);
		else
			CLI_LOGE("no saved user_fci, run sta_fc save first\n");
	} else if (!os_strcmp(argv[1], "show")) {
		ret = cli_wifi_user_fci_load(&fci);
		if (ret == BK_OK)
			cli_wifi_user_fci_dump(&fci);
		else
			CLI_LOGE("no saved user_fci\n");
	} else if (!os_strcmp(argv[1], "erase")) {
		ret = cli_wifi_user_fci_erase();
	} else {
		cli_wifi_sta_fc_help();
		ret = BK_ERR_PARAM;
	}

out:
	if (ret != BK_OK)
		msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif /* CONFIG_ENABLE_WIFI_USER_FCI */

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

static uint8_t cli_wifi_softap_display_channel(const wifi_ap_config_t *ap_info)
{
	uint8_t ch = bk_wlan_ap_get_channel_config();

	return ch ? ch : (ap_info ? ap_info->channel : 0);
}

int cli_wifi_state_handle(void)
{
#if CONFIG_LWIP
	wifi_link_status_t link_status = {0};
	wifi_ap_config_t ap_info = {0};
	netif_ip4_config_t ip4_info = {0};
	char ssid[33] = {0};
	int sta_up = sta_ip_is_start();
	int ap_up = uap_ip_is_start();
	bool sta_link_valid = false;

	if (sta_ip_is_start()) {
		os_memset(&link_status, 0x0, sizeof(link_status));
		if (bk_wifi_sta_get_link_status(&link_status) == BK_OK) {
			sta_link_valid = true;
		}
	}

#if CONFIG_BRIDGE
	{
		bk_bridge_state_t br_st = bk_wifi_get_bridge_state();
		int br_up = bridge_ip_is_start();

		BK_LOGI(TAG, "[KW:]sta: %d, ap: %d, bridge: %d (%s) b/g/n\r\n",
				sta_up, ap_up, br_up, cli_wifi_bridge_state_str(br_st));

		if (br_st != BRIDGE_STATE_DISABLED) {
			int br_channel = sta_link_valid ? link_status.channel : 0;

			os_memset(&ap_info, 0x0, sizeof(ap_info));
			if (bk_wifi_ap_get_config(&ap_info) == BK_OK) {
				os_memcpy(ssid, ap_info.ssid, 32);
				if (br_channel == 0)
					br_channel = cli_wifi_softap_display_channel(&ap_info);
				BK_LOGI(TAG, "[KW:]bridge: ssid=%s, channel=%d, cipher_type=%s\r\n",
						ssid, br_channel,
						cli_wifi_sec_type_string(ap_info.security));
			}
		}
	}
#else
	BK_LOGI(TAG, "[KW:]sta: %d, ap: %d, b/g/n\r\n", sta_up, ap_up);
#endif

	if (sta_link_valid) {
		os_memcpy(ssid, link_status.ssid, 32);
		BK_LOGI(TAG, "[KW:]sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
				   link_status.rssi, link_status.aid, ssid, link_status.bssid,
				   link_status.channel, cli_wifi_sec_type_string(link_status.security));
	} else if (sta_ip_is_start()) {
		os_memset(&link_status, 0x0, sizeof(link_status));
		BK_RETURN_ON_ERR(bk_wifi_sta_get_link_status(&link_status));
		os_memcpy(ssid, link_status.ssid, 32);
		BK_LOGI(TAG, "[KW:]sta:rssi=%d,aid=%d,ssid=%s,bssid=%pm,channel=%d,cipher_type=%s\r\n",
				   link_status.rssi, link_status.aid, ssid, link_status.bssid,
				   link_status.channel, cli_wifi_sec_type_string(link_status.security));
	}

	if (ap_up) {
		os_memset(&ap_info, 0x0, sizeof(ap_info));
		BK_RETURN_ON_ERR(bk_wifi_ap_get_config(&ap_info));
		os_memcpy(ssid, ap_info.ssid, 32);
		BK_LOGI(TAG, "[KW:]softap: ssid=%s, channel=%d, cipher_type=%s\r\n",
				   ssid, cli_wifi_softap_display_channel(&ap_info),
				   cli_wifi_sec_type_string(ap_info.security));

		os_memset(&ip4_info, 0x0, sizeof(ip4_info));
		BK_RETURN_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_info));
		BK_LOGD(TAG, "[KW:]ip=%s,gate=%s,mask=%s,dns=%s\r\n",
				   ip4_info.ip, ip4_info.gateway, ip4_info.mask, ip4_info.dns);
	}

#if CONFIG_P2P
	{
		int p2p_role = 0;
		const char *dev_name = bk_wifi_get_p2p_dev_name();
		uint8_t p2p_mac[6] = {0};
		uint8_t p2p_ch = 0;

		bk_wifi_p2p_get_role(&p2p_role);
		if (p2p_role == 1 || p2p_role == 2) {
			p2p_ch = bk_wifi_p2p_get_operating_channel();
			bk_wifi_p2p_get_mac(p2p_mac);
			if (dev_name && dev_name[0]) {
				BK_LOGI(TAG, "[KW:]p2p: role=%s, channel=%d, dev=%s, mac=%pm\r\n",
					p2p_role == 1 ? "go" : "gc", p2p_ch, dev_name, p2p_mac);
			} else {
				BK_LOGI(TAG, "[KW:]p2p: role=%s, channel=%d, mac=%pm\r\n",
					p2p_role == 1 ? "go" : "gc", p2p_ch, p2p_mac);
			}
		}
	}
#endif
	return BK_OK;
#else
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
	const char *netif_name;

	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
		got_ip = (netif_event_got_ip4_t *)event_data;
		if (got_ip == NULL)
			break;
		switch (got_ip->netif_if) {
		case NETIF_IF_STA:
			netif_name = "BK STA";
			break;
		case NETIF_IF_AP:
			netif_name = "BK AP";
			break;
		case NETIF_IF_P2P:
			netif_name = "BK P2P";
			break;
		default:
			netif_name = "unknown netif";
			break;
		}
		CLI_LOGW("%s got ip %s\n", netif_name, got_ip->ip);
		break;
#if CONFIG_IPV6
	case EVENT_NETIF_GOT_IP6_LL:
	case EVENT_NETIF_GOT_IP6_GLOBAL: {
		netif_event_got_ip6_t *got_ip6 = (netif_event_got_ip6_t *)event_data;

		CLI_LOGI("BK STA got ipv6 %s, idx=%d, addr=%s\n",
				 event_id == EVENT_NETIF_GOT_IP6_LL ? "link-local" : "global",
				 got_ip6->addr_idx, got_ip6->ip);
		break;
	}
#endif
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

#if CONFIG_P2P
	case EVENT_WIFI_GO_CONNECTED:
		CLI_LOGW("BK P2P GO: client connected\n");
		bk_wifi_p2p_stop_find();
		break;

	case EVENT_WIFI_GO_DISCONNECTED:
		CLI_LOGW("BK P2P GO: client disconnected, GO kept up\n");
		break;

	case EVENT_WIFI_GC_CONNECTED:
		CLI_LOGW("BK P2P GC: connected to remote GO\n");
		break;

	case EVENT_WIFI_GC_DISCONNECTED:
		CLI_LOGW("BK P2P GC: disconnected\n");
		bk_wifi_p2p_find();
		break;
#endif

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
		const char *bridge_name = NULL;
		char br_ssid[WIFI_SSID_STR_LEN] = {0};
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

		if (argc >= 5 && argv[4][0] != '\0')
			bridge_name = argv[4];

		if (bridge_name) {
			if (os_strlen(bridge_name) >= WIFI_SSID_STR_LEN) {
				CLI_LOGW("bridge ssid too long (max %d)\n", WIFI_SSID_STR_LEN - 1);
				goto error;
			}
			os_strncpy(br_ssid, bridge_name, sizeof(br_ssid) - 1);
		} else {
			if (os_snprintf(br_ssid, sizeof(br_ssid), "%s_brr", ssid) >= (int)sizeof(br_ssid)) {
				CLI_LOGW("default bridge ssid too long\n");
				goto error;
			}
		}

		os_strncpy(br_config.sta_config.ssid, ssid, sizeof(br_config.sta_config.ssid) - 1);
		if (key)
			os_strncpy(br_config.sta_config.password, key,
				   sizeof(br_config.sta_config.password) - 1);
		os_strncpy(br_config.br_info.ssid, br_ssid, sizeof(br_config.br_info.ssid) - 1);
		br_config.br_info.disable_dns_server = 1;
		if (argc >= 6 && argv[5][0] != '\0')
			br_config.keep_sta_on_close = (uint8_t)os_strtoul(argv[5], NULL, 10);

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
		int intent = -1;  // Default: keep previous/default

		if (argc == 3) {
			p2p_ssid = argv[2];
			ret = bk_wifi_p2p_enable(p2p_ssid);
			if (ret != BK_OK) {
				CLI_LOGE("p2p enable failed, err=%d\n", ret);
				goto error;
			}
		}
		if (argc >= 4) {
			p2p_ssid = argv[2];
			intent = atoi(argv[3]);
			if (intent < -1 || intent > 15) {
				CLI_LOGE("invalid intent value (must be -1 or 0-15)\n");
				goto error;
			}
			ret = bk_wifi_p2p_enable_with_intent(p2p_ssid, intent);
			if (ret != BK_OK) {
				CLI_LOGE("p2p enable with intent failed, err=%d\n", ret);
				goto error;
			}
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
	} else if (!os_strcmp(argv[1], "noa")) {
		uint8_t enabled;

		if (argc < 3) {
			CLI_LOGW("invalid parameters for noa\n");
			cli_wifi_p2p_help();
			goto error;
		}

		enabled = (uint8_t)os_strtoul(argv[2], NULL, 10);
		if (enabled > 1) {
			CLI_LOGE("invalid noa value (must be 0 or 1)\n");
			goto error;
		}

		ret = bk_wifi_p2p_go_noa_set_enabled(enabled);
		if (ret != BK_OK) {
			CLI_LOGE("p2p noa failed, err=%d\n", ret);
			goto error;
		}
	} else if (!os_strcmp(argv[1], "connect")) {
		uint8_t peer_mac[6] = {0};
		int method = 0;
		int intent = 0;

		if (argc < 5) {
			CLI_LOGW("invalid parameters for connect\n");
			cli_wifi_p2p_help();
			goto error;
		}

		if (cli_mac_str_to_bin(argv[2], peer_mac) != 0) {
			CLI_LOGE("invalid peer mac (12 hex digits, ':' optional): %s\n",
				 argv[2]);
			goto error;
		}
		method = os_strtoul(argv[3], NULL, 10);
		intent = os_strtoul(argv[4], NULL, 10);
		ret = bk_wifi_p2p_connect(peer_mac, method, intent);
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

/*
 * regulatory domain command (forwarded to CP).
 *     regd get
 *     regd set <alpha2>
 */
void cli_wifi_regd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc > 1 && !os_strcmp(argv[1], "get")) {
		bk_wifi_print_regdomain();
		return;
	} else if (argc > 2 && !os_strcmp(argv[1], "set")) {
		char alpha2[3] = {0};
		os_strlcpy(alpha2, argv[2], sizeof(alpha2));
		bk_wifi_set_country_code(alpha2);
		return;
	}

	CLI_LOGE("Usage:\n");
	CLI_LOGE("\t%s get\n", argv[0]);
	CLI_LOGE("\t%s set <alpha2>\n", argv[0]);
}

#define WIFI_CMD_CNT (sizeof(s_wifi_commands) / sizeof(struct cli_command))
static const struct cli_command s_wifi_commands[] = {
	{"scan", "scan [ssid]", cli_wifi_scan_cmd},
	{"regd", "regd {get|set <alpha2>}", cli_wifi_regd_cmd},
#ifdef CONFIG_WIFI_SOFTAP
	{"ap", "ap {ssid} [password] [channel] [hidden]", cli_wifi_ap_cmd},
#endif
	{"sta", "sta {ssid} [password] [bssid] [channel] [psk]", cli_wifi_sta_cmd},
#if CONFIG_ENABLE_WIFI_USER_FCI
	{"sta_fc", "sta_fc {save|connect|show|erase}", cli_wifi_sta_fc_cmd},
#endif
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
#if CONFIG_SOFTAP_WPA3
	{"ap_wpa3", "ap_wpa3 {ssid} [password] [channel]", cli_wifi_wpa3_ap_cmd},
#endif
};

static bool s_cli_wifi_event_cbs_registered;

bk_err_t cli_wifi_register_event_cbs(void)
{
	bk_err_t ret;

	if (s_cli_wifi_event_cbs_registered)
		return BK_OK;

	ret = bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, cli_wifi_event_cb, NULL);
	if (ret != BK_OK)
		return ret;

	ret = bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, cli_netif_event_cb, NULL);
	if (ret != BK_OK)
		return ret;

	s_cli_wifi_event_cbs_registered = true;
	return BK_OK;
}

int cli_wifi_init(void)
{
	#if CONFIG_WIFI_CLI_DEBUG
	extern int cli_wifi_debug_init(void);
	cli_wifi_debug_init();
	#endif
	bk_err_t ret = cli_wifi_register_event_cbs();
	if (ret != BK_OK && ret != BK_ERR_EVENT_NOT_INIT)
		BK_LOG_ON_ERR(ret);
	int _reg_ret = cli_register_commands(s_wifi_commands, WIFI_CMD_CNT);
	return _reg_ret;
}

#endif //#if (CLI_CFG_WIFI == 1)
