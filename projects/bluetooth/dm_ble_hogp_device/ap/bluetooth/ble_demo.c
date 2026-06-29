#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>

#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "components/bluetooth/bk_dm_gatt_types.h"

#include "dm_gatt.h"

#include "ble_demo.h"

#define TAG "ble_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define ADV_HANDLE              0
#define BEKEN_COMPANY_ID        (0x05F0)
#define ADV_NAME_PREFIX         "BK_HOGPD"
#define SYNC_CMD_TIMEOUT_MS     4000

static beken_semaphore_t s_ble_demo_sema = NULL;
static uint8_t s_ble_demo_cb_registered = 0;

/*
 * Secondary GAP callback (added via bk_dm_prf_gap_add_gap_callback). The dispatcher
 * invokes every registered callback, so the dm_gatts callback still runs as
 * well; here we only mirror the adv-related completion events onto our own
 * semaphore so the synchronous adv sequence below can wait on them.
 */
static int32_t ble_demo_gap_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case BK_BLE_GAP_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT:
    case BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT:
    case BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT:
    case BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    case BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT:
        if (s_ble_demo_sema != NULL)
        {
            rtos_set_semaphore(&s_ble_demo_sema);
        }

        return DM_BLE_GAP_APP_CB_RET_PROCESSED;

    default:
        break;
    }

    return DM_BLE_GAP_APP_CB_RET_NO_INTERESTING;
}

static int ble_demo_wait_complete(const char *step)
{
    int ret = rtos_get_semaphore(&s_ble_demo_sema, SYNC_CMD_TIMEOUT_MS);

    if (ret != BK_OK)
    {
        LOGE("wait %s timeout %d", step, ret);
    }

    return ret;
}

static int ble_demo_adv_start(void)
{
    int ret;
    bk_bd_addr_t identity_addr = {0};
    bk_bd_addr_t rand_addr = {0};
    char adv_name[32] = {0};
    uint8_t company_id[2] = {BEKEN_COMPANY_ID & 0xFF, BEKEN_COMPANY_ID >> 8};

    bk_dm_prf_gap_get_identity_addr(identity_addr);

    snprintf(adv_name, sizeof(adv_name) - 1, "%s-%02X%02X%02X",
             ADV_NAME_PREFIX, identity_addr[2], identity_addr[1], identity_addr[0]);

    LOGI("adv name %s", adv_name);

    ret = bk_ble_gap_set_device_name(adv_name);

    if (ret)
    {
        LOGE("set device name err %d", ret);
        return ret;
    }

    bk_ble_gap_ext_adv_params_t adv_param =
    {
        .type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
        .interval_min = 120,
        .interval_max = 160,
        .channel_map = BK_ADV_CHNL_ALL,
        .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        .primary_phy = BK_BLE_GAP_PRI_PHY_1M,
        .secondary_phy = BK_BLE_GAP_PHY_1M,
        .sid = 0,
        .scan_req_notif = 0,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    };

    ret = bk_ble_gap_set_adv_params(ADV_HANDLE, &adv_param);

    if (ret)
    {
        LOGE("set adv params err %d", ret);
        return ret;
    }

    if (ble_demo_wait_complete("adv params") != BK_OK)
    {
        return -1;
    }

    if(adv_param.own_addr_type != BLE_ADDR_TYPE_PUBLIC)
    {
        /* static random address: [47:46] must be 0b11 */
        os_memcpy(rand_addr, identity_addr, sizeof(identity_addr));
        rand_addr[0]++;
        rand_addr[5] |= 0xc0;

        ret = bk_ble_gap_set_adv_rand_addr(ADV_HANDLE, rand_addr);

        if (ret)
        {
            LOGE("set adv rand addr err %d", ret);
            return ret;
        }

        if (ble_demo_wait_complete("adv rand addr") != BK_OK)
        {
            return -1;
        }
    }

    const uint8_t hogp_service_uuid[16] =
    {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00,
        BK_GATT_UUID_HID_SVC & 0xff, (BK_GATT_UUID_HID_SVC >> 8) & 0xff, 0x00, 0x00
    };

    bk_ble_adv_data_t adv_data =
    {
        .set_scan_rsp = 0,
        .include_name = 1,
        .appearance = 0,
        .manufacturer_len = sizeof(company_id),
        .p_manufacturer_data = company_id,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = sizeof(hogp_service_uuid),
        .p_service_uuid = (void *)hogp_service_uuid,
        .flag = 2,
    };

    ret = bk_ble_gap_set_adv_data(&adv_data);

    if (ret)
    {
        LOGE("set adv data err %d", ret);
        return ret;
    }

    if (ble_demo_wait_complete("adv data") != BK_OK)
    {
        return -1;
    }

    adv_data.set_scan_rsp = 1;

    ret = bk_ble_gap_set_adv_data(&adv_data);

    if (ret)
    {
        LOGE("set scan rsp err %d", ret);
        return ret;
    }

    if (ble_demo_wait_complete("scan rsp") != BK_OK)
    {
        return -1;
    }

    const bk_ble_gap_ext_adv_t ext_adv =
    {
        .instance = 0,
        .duration = 0,
        .max_events = 0,
    };

    ret = bk_ble_gap_adv_start(1, &ext_adv);

    if (ret)
    {
        LOGE("adv start err %d", ret);
        return ret;
    }

    if (ble_demo_wait_complete("adv start") != BK_OK)
    {
        return -1;
    }

    LOGI("adv started");
    return 0;
}

static int ble_demo_adv_stop(void)
{
    const uint8_t inst[] = {0};
    int ret = bk_ble_gap_adv_stop(sizeof(inst) / sizeof(inst[0]), (uint8_t *)inst);

    if (ret)
    {
        LOGE("adv stop err %d", ret);
        return ret;
    }

    LOGI("adv stopped");
    return 0;
}

int ble_demo_adv_enable(uint8_t enable)
{
    if (s_ble_demo_sema == NULL)
    {
        LOGE("not init");
        return -1;
    }

    return enable ? ble_demo_adv_start() : ble_demo_adv_stop();
}

void ble_demo_init(void)
{
    if (s_ble_demo_sema == NULL)
    {
        if (rtos_init_semaphore(&s_ble_demo_sema, 1) != BK_OK)
        {
            LOGE("init sema err");
            return;
        }
    }

    if (!s_ble_demo_cb_registered)
    {
        if (bk_dm_prf_gap_add_gap_callback(ble_demo_gap_cb) != 0)
        {
            LOGE("add gap callback err");
            return;
        }

        s_ble_demo_cb_registered = 1;
    }
}
