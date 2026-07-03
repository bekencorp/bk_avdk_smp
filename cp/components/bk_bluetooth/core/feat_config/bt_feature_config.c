#include "bt_feature_config.h"

#include "os/os.h"
#include "os/mem.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static bt_feature_struct_t s_bt_feature_struct;

int bk_bt_feature_init(void)
{
    int ret = BK_OK;

    os_memset(&s_bt_feature_struct, 0, sizeof(s_bt_feature_struct));

#if CONFIG_BLUETOOTH_BLE_DISCOVER_AUTO
    s_bt_feature_struct._is_gatt_discovery_auto = 1;
#endif

#if CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC
    s_bt_feature_struct._support_lpo_rosc = 1;
#endif

#if CONFIG_BLE_LV_SUPPORT
    s_bt_feature_struct._support_lowpower_sleep = 1;
#endif

    extern bool ate_is_enabled(void);
    if(!ate_is_enabled())
    {
    #if CONFIG_BLUETOOTH_RF_MODE_POLAR
        s_bt_feature_struct._rf_mode = RF_MODE_POLAR;
    #elif CONFIG_BLUETOOTH_RF_MODE_IQ_HIGH_PLL
        s_bt_feature_struct._rf_mode = RF_MODE_IQ_HIGH_PLL;
    #elif CONFIG_BLUETOOTH_RF_MODE_IQ_LOW_PLL
        s_bt_feature_struct._rf_mode = RF_MODE_IQ_LOW_PLL;
    #else
        s_bt_feature_struct._rf_mode = RF_MODE_IQ_HIGH_PLL;
    #endif
    }
    else
    {
        s_bt_feature_struct._rf_mode = RF_MODE_IQ_HIGH_PLL;
    }

    extern int bt_feature_adapter_init(void *arg);

    if (bt_feature_adapter_init((void *)&s_bt_feature_struct) != 0)
    {
        return BK_FAIL;
    }

    return ret;
}

int bk_bt_feature_enable_fuzz(uint8_t enable)
{
    if (enable)
    {
        s_bt_feature_struct._ignore_smp_key_distr_all_zero = 1;
        s_bt_feature_struct._strict_smp_key_distr_check_except_all_zero = 1;
        s_bt_feature_struct._ignore_smp_already_pair = 1;
        s_bt_feature_struct._send_peripheral_feature_req_auto = 1;
        s_bt_feature_struct._stop_smp_when_pair_err = 1;
        s_bt_feature_struct._is_gatt_discovery_auto = 0;
        s_bt_feature_struct._enable_smp_sec_req_evt = 1;
    }
    else
    {
        s_bt_feature_struct._ignore_smp_key_distr_all_zero = 0;
        s_bt_feature_struct._strict_smp_key_distr_check_except_all_zero = 0;
        s_bt_feature_struct._ignore_smp_already_pair = 0;
        s_bt_feature_struct._send_peripheral_feature_req_auto = 0;
        s_bt_feature_struct._stop_smp_when_pair_err = 0;
        s_bt_feature_struct._enable_smp_sec_req_evt = 0;
#if CONFIG_BLUETOOTH_BLE_DISCOVER_AUTO
        s_bt_feature_struct._is_gatt_discovery_auto = 1;
#endif
    }

    return 0;
}
