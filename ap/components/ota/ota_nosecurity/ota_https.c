#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include "components/system.h"
#include "components/log.h"
#include "driver/flash.h"
#include "modules/ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"
#include "bk_https.h"
#include "bk_private/bk_ota_private.h"

#if CONFIG_PSA_MBEDTLS
#include "psa/crypto.h"
#endif


#define TAG "HTTPS_OTA"

extern UINT8  ota_flag ;

#define HTTPS_INPUT_SIZE   (5120)

/* TLS trust anchor: DigiCert Global Root G2, the root of dl.bekencorp.com's
 * chain. Needs a real clock (CONFIG_NTP_SYNC_RTC=y) for the cert validity check. */
const char ca_crt_rsa[] = {
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
"MrY=\r\n"
"-----END CERTIFICATE-----\r\n"
};

bk_http_client_handle_t bk_https_client_flash_init(bk_http_input_t config)
{
	bk_http_client_handle_t client = NULL;

	ota_flag = 1;
#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(1);
#endif
#if HTTP_WR_TO_FLASH
	http_flash_init();
#endif
	client = bk_http_client_init(&config);

	return client;

}

bk_err_t bk_https_client_flash_deinit(bk_http_client_handle_t client)
{
	int err;

	ota_flag = 0;
#if CONFIG_SYSTEM_CTRL
	//bk_wifi_ota_dtim(0);
#endif
#if HTTP_WR_TO_FLASH
	http_flash_deinit();
#endif
	if(!client)
		err = bk_http_client_cleanup(client);
	else
		return BK_FAIL;

	return err;

}

bk_err_t https_ota_event_cb(bk_http_client_event_t *evt)
{
    if(!evt)
    {
        return BK_FAIL;
    }

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
	BK_LOGE(TAG, "HTTPS_EVENT_ERROR\r\n");
	break;
    case HTTP_EVENT_ON_CONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_CONNECTED\r\n");
#ifdef CONFIG_HTTP_OTA_WITH_BLE
#if CONFIG_BLUETOOTH
        bk_ble_register_sleep_state_callback(ble_sleep_cb);
#endif
#endif
	break;
    case HTTP_EVENT_HEADER_SENT:
	BK_LOGE(TAG, "HTTPS_EVENT_HEADER_SENT\r\n");
	break;
    case HTTP_EVENT_ON_HEADER:
	BK_LOGE(TAG, "HTTPS_EVENT_ON_HEADER\r\n");
	break;
    case HTTP_EVENT_ON_DATA:
	//do something: evt->data, evt->data_len
	if (bk_ota_process_data((char *)evt->data, evt->data_len, evt->data_len, evt->client->response->content_length) != 0) {
		BK_LOGE(TAG, "ota data process failed, abort download\r\n");
		return BK_FAIL;
	}
	//BK_LOGD(TAG, "HTTP_EVENT_ON_DATA, length:%d , content_length:0x%x \r\n", evt->data_len , evt->client->response->content_length);
	break;
    case HTTP_EVENT_ON_FINISH:
	//bk_ota_process_data((char *)evt->data, evt->data_len, evt->data_len, evt->client->response->content_length);
	bk_https_client_flash_deinit(evt->client);
	BK_LOGI(TAG, "HTTPS_EVENT_ON_FINISH\r\n");
	break;
    case HTTP_EVENT_DISCONNECTED:
	BK_LOGE(TAG, "HTTPS_EVENT_DISCONNECTED\r\n");
	break;

    }
    return BK_OK;
}


int bk_https_ota_download(const char *url)
{
	int err;

	if(!url)
	{
		err = BK_FAIL;
		BK_LOGI(TAG, "url is NULL\r\n");

		return err;
	}

	err = ota_get_init_status();
	if(err == 1) //has already init
	{
		BK_LOGI(TAG, "has already do ota init \r\n");
	}
	else //do ota init
	{
		if(ota_do_init_operation() == BK_FAIL)
		{
			BK_LOGE(TAG, "do ota init fail\r\n");
			ota_do_deinit_operation();
			return BK_FAIL;
		}
	}

	bk_http_input_t config = {
		.url = url,
		.cert_pem = ca_crt_rsa,
		.event_handler = https_ota_event_cb,
		.buffer_size = HTTPS_INPUT_SIZE,
		.timeout_ms = 15000
	};

	bk_http_client_handle_t client = bk_https_client_flash_init(config);
	if (client == NULL) {
		BK_LOGI(TAG, "client is NULL\r\n");
		err = BK_FAIL;
		ota_do_deinit_operation();
		return err;
	}
	/* Disable STA power-save during download; PS sleep drops MAC timer IRQs and
	 * stalls the transfer. Restored right after, before any branch. */
	bk_wifi_sta_pm_disable();
	err = bk_http_client_perform(client);
	bk_wifi_sta_pm_enable();
	if(err == BK_OK){
		BK_LOGI(TAG, "bk_http_client_perform ok\r\n");

#ifdef CONFIG_HTTP_AB_PARTITION
	if(ota_get_dest_id() == OTA_WR_TO_FLASH)
	{
		int ret_val = 0;
		#ifdef CONFIG_OTA_HASH_FUNCTION
		ret_val= ota_do_hash_check();
		if(ret_val != BK_OK)
		{
			BK_LOGE(TAG,"hash fail.\r\n");
			ota_do_deinit_operation();
			return  ret_val;
		}
		#endif
		ret_val = bk_ota_update_partition_flag(0);
		if(ret_val != BK_OK)
		{
			ota_do_deinit_operation();
			return ret_val;
		}
	}
	BK_LOGI(TAG, "ota_success, rebooting\r\n");
	BK_LOG_FLUSH();
	ota_do_deinit_operation();
	bk_reboot();
#else
	BK_LOGI(TAG, "ota_success, rebooting\r\n");
	BK_LOG_FLUSH();
	bk_ota_finish_and_reboot();
#endif
	}
	else{
		bk_https_client_flash_deinit(client);
		ota_do_deinit_operation();
		BK_LOGI(TAG, "bk_http_client_perform fail, err:%x\r\n", err);
	}

	return err;
}
