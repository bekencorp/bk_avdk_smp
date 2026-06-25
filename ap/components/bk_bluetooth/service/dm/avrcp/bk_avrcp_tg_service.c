#include <components/system.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_avrcp_tg_service.h"

#include "components/bluetooth/bk_dm_avrcp.h"
#include "bluetooth_storage.h"
#include "bt_manager.h"

#define TAG "bk_avrcp_tg"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define AVRCP_GAIN_MAX            (128 - 1)
#define PLATFORM_SPK_GAIN_MAX     0x3f
#define PLATFORM_SPK_GAIN_DEFAULT 0x2d

typedef struct
{
    uint8_t inited;
    uint8_t connected;
    uint8_t default_volume;
    uint8_t local_volume;
    uint16_t registered_noti;
    uint8_t remote_bda[6];
    bk_avrcp_tg_event_cb_t event_cb;
    void *event_user_data;
} avrcp_tg_ctx_t;

static avrcp_tg_ctx_t s_avrcp_tg;

static void avrcp_save_volume_for_addr(const uint8_t *bda, uint8_t vol)
{
    uint8_t addr[6] = {0};

    if (bda)
    {
        os_memcpy(addr, bda, sizeof(addr));
    }
    else if (bt_manager_get_connected_device())
    {
        os_memcpy(addr, bt_manager_get_connected_device(), sizeof(addr));
    }
    else
    {
        LOGW("%s skip save volume %d, no device address\n", __func__, vol);
        return;
    }

    LOGI("%s save volume %d %02x:%02x:%02x:%02x:%02x:%02x\n",
         __func__, vol, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    bluetooth_storage_save_volume(addr, vol);
}

static uint8_t avrcp_clamp_volume(uint8_t vol)
{
    return vol > 0x7F ? 0x7F : vol;
}

static void avrcp_tg_restore_initial_volume(void)
{
    uint8_t addr[6] = {0};

    s_avrcp_tg.local_volume = s_avrcp_tg.default_volume;
    if (bluetooth_storage_get_newest_linkkey_info(addr, NULL) >= 0)
    {
        uint8_t stored = 0;
        if (bluetooth_storage_find_volume_by_addr(addr, &stored) < 0)
        {
            s_avrcp_tg.local_volume = 1.0 * PLATFORM_SPK_GAIN_DEFAULT / PLATFORM_SPK_GAIN_MAX * AVRCP_GAIN_MAX;
        }
        else
        {
            s_avrcp_tg.local_volume = stored;
        }

        if (s_avrcp_tg.local_volume == 0)
        {
            s_avrcp_tg.local_volume = 1.0 * PLATFORM_SPK_GAIN_DEFAULT / PLATFORM_SPK_GAIN_MAX * AVRCP_GAIN_MAX;
        }

        LOGI("initial volume %d %02x:%02x:%02x:%02x:%02x:%02x\n",
             s_avrcp_tg.local_volume, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    }
    else
    {
        LOGI("%s can't find linkkey info\n", __func__);
    }
}

static void bk_avrcp_tg_emit(bk_avrcp_tg_evt_t evt, void *arg)
{
    if (s_avrcp_tg.event_cb)
    {
        s_avrcp_tg.event_cb(evt, arg, s_avrcp_tg.event_user_data);
    }
}

void bk_avrcp_tg_emit_current_volume(void)
{
    uint8_t local_volume = bk_avrcp_tg_get_local_volume_value();
    bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_VOLUME_CHANGED, &local_volume);
}

static void avrcp_tg_cb(bk_avrcp_tg_cb_event_t event, bk_avrcp_tg_cb_param_t *param)
{
    LOGI("%s event: %d\n", __func__, event);

    switch (event)
    {
    case BK_AVRCP_TG_CONNECTION_STATE_EVT:
        s_avrcp_tg.connected = param->conn_stat.connected;
        LOGI("%s avrcp tg connection state: %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
             __func__,
             s_avrcp_tg.connected,
             param->conn_stat.remote_bda[5], param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[3],
             param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[1], param->conn_stat.remote_bda[0]);
        if (s_avrcp_tg.connected)
        {
            os_memcpy(s_avrcp_tg.remote_bda, param->conn_stat.remote_bda, sizeof(s_avrcp_tg.remote_bda));
            bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_CONNECTED, s_avrcp_tg.remote_bda);
        }
        else
        {
            s_avrcp_tg.registered_noti = 0;
            bk_avrcp_tg_emit(BK_AVRCP_TG_EVT_DISCONNECTED, s_avrcp_tg.remote_bda);
            os_memset(s_avrcp_tg.remote_bda, 0, sizeof(s_avrcp_tg.remote_bda));
        }
        break;

    case BK_AVRCP_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
        LOGI("%s recv abs vol 0x%x %02x:%02x:%02x:%02x:%02x:%02x\n",
             __func__,
             param->set_abs_vol.volume,
             param->set_abs_vol.remote_bda[5], param->set_abs_vol.remote_bda[4], param->set_abs_vol.remote_bda[3],
             param->set_abs_vol.remote_bda[2], param->set_abs_vol.remote_bda[1], param->set_abs_vol.remote_bda[0]);
        bk_avrcp_tg_set_local_volume(param->set_abs_vol.volume, param->set_abs_vol.remote_bda);
        break;

    case BK_AVRCP_TG_REGISTER_NOTIFICATION_EVT:
    {
        bk_avrcp_rn_param_t cmd;
        s_avrcp_tg.registered_noti |= (1 << param->reg_ntf.event_id);
        LOGI("%s recv reg evt 0x%x param %d %02x:%02x:%02x:%02x:%02x:%02x\n",
             __func__,
             param->reg_ntf.event_id,
             param->reg_ntf.event_parameter,
             param->reg_ntf.remote_bda[5], param->reg_ntf.remote_bda[4], param->reg_ntf.remote_bda[3],
             param->reg_ntf.remote_bda[2], param->reg_ntf.remote_bda[1], param->reg_ntf.remote_bda[0]);
        if (param->reg_ntf.event_id == BK_AVRCP_RN_VOLUME_CHANGE)
        {
            os_memset(&cmd, 0, sizeof(cmd));
            cmd.volume = bk_avrcp_tg_get_local_volume_value();
            bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                       param->reg_ntf.event_id,
                                       BK_AVRCP_RN_RSP_INTERIM,
                                       &cmd);
        }
        break;
    }

    default:
        LOGW("%s unknow event 0x%x\n", __func__, event);
        break;
    }
}

int bk_avrcp_tg_register_event_cb(bk_avrcp_tg_event_cb_t cb, void *user_data)
{
    s_avrcp_tg.event_cb = cb;
    s_avrcp_tg.event_user_data = user_data;
    return BK_OK;
}

int bk_avrcp_tg_set_local_volume(uint8_t vol, const uint8_t *bda)
{
    s_avrcp_tg.local_volume = avrcp_clamp_volume(vol);
    avrcp_save_volume_for_addr(bda, s_avrcp_tg.local_volume);
    bk_avrcp_tg_emit_current_volume();
    return BK_OK;
}

uint8_t bk_avrcp_tg_get_local_volume_value(void)
{
    return s_avrcp_tg.local_volume;
}

int bk_avrcp_tg_service_init(const bk_avrcp_tg_cfg_t *cfg)
{
    bk_avrcp_rn_evt_cap_mask_t tmp_cap = {0};
    bk_avrcp_rn_evt_cap_mask_t final_cap = {
        .bits = (1 << BK_AVRCP_RN_VOLUME_CHANGE),
    };
    int ret;

    LOGI("%s\n", __func__);

    if (s_avrcp_tg.inited)
    {
        LOGE("%s already init\n", __func__);
        return BK_OK;
    }

    s_avrcp_tg.default_volume = cfg ? avrcp_clamp_volume(cfg->default_volume) : 0x5A;
    if (s_avrcp_tg.default_volume == 0)
    {
        s_avrcp_tg.default_volume = 0x5A;
    }

    avrcp_tg_restore_initial_volume();

    bk_bt_avrcp_tg_init();
    bk_bt_avrcp_tg_get_rn_evt_cap(BK_AVRCP_RN_CAP_API_METHOD_ALLOWED, &tmp_cap);
    final_cap.bits &= tmp_cap.bits;
    LOGI("%s set rn cap 0x%x\n", __func__, final_cap.bits);
    ret = bk_bt_avrcp_tg_set_rn_evt_cap(&final_cap);
    if (ret != BK_OK)
    {
        LOGE("%s set rn cap err %d\n", __func__, ret);
        bk_bt_avrcp_tg_deinit();
        return ret;
    }

    bk_bt_avrcp_tg_register_callback(avrcp_tg_cb);

    s_avrcp_tg.inited = 1;
    LOGI("%s end\n", __func__);
    return BK_OK;
}

int bk_avrcp_tg_service_deinit(void)
{
    LOGI("%s\n", __func__);

    bk_bt_avrcp_tg_register_callback(NULL);
    bk_bt_avrcp_tg_deinit();
    os_memset(&s_avrcp_tg, 0, sizeof(s_avrcp_tg));

    LOGI("%s end\n", __func__);
    return BK_OK;
}

int bk_avrcp_tg_disconnect(const uint8_t bda[6])
{
    (void)bda;
    LOGW("%s disconnect API not exposed by current SDK\n", __func__);
    return BK_FAIL;
}

int bk_avrcp_tg_is_connected(void)
{
    return s_avrcp_tg.connected;
}

int bk_avrcp_tg_notify_volume_change(uint8_t vol_0_7f)
{
    bk_avrcp_rn_param_t cmd = {0};

    if (!(s_avrcp_tg.registered_noti & (1 << BK_AVRCP_RN_VOLUME_CHANGE)))
    {
        return BK_FAIL;
    }

    cmd.volume = vol_0_7f > 0x7F ? 0x7F : vol_0_7f;
    return bk_bt_avrcp_tg_send_rn_rsp(s_avrcp_tg.remote_bda,
                                      BK_AVRCP_RN_VOLUME_CHANGE,
                                      BK_AVRCP_RN_RSP_CHANGED,
                                      &cmd);
}
