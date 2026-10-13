
#include "bk_a2dp_private.h"

bt_err_t bk_bt_a2dp_register_callback(bk_bt_a2dp_cb_t callback)
{
    return bt_a2dp_register_callback_internal(callback);
}

bt_err_t bk_bt_a2dp_sink_init(uint8_t aac_supported)
{
    return bt_a2dp_sink_init_internal(aac_supported);
}

bt_err_t bk_bt_a2dp_sink_deinit(void)
{
    return bt_a2dp_sink_deinit_internal();
}

bt_err_t bk_bt_a2dp_sink_register_data_callback(bk_bt_sink_data_cb_t callback)
{
    return bt_a2dp_sink_register_data_callback_internal(callback);
}

bt_err_t bk_bt_a2dp_sink_connect(uint8_t *remote_bda)
{
    return bt_a2dp_sink_connect_internal(remote_bda);
}

bt_err_t bk_bt_a2dp_sink_disconnect(uint8_t *remote_bda)
{
    return bt_a2dp_sink_disconnect_internal(remote_bda);
}

bt_err_t bk_bt_a2dp_sink_set_delay_value(uint16_t delay_value)
{
    return bt_a2dp_sink_set_delay_value_internal(delay_value);
}

bt_err_t bk_bt_a2dp_sink_get_delay_value(void)
{
    return bt_a2dp_sink_get_delay_value_internal();
}

bt_err_t bk_bt_a2dp_set_cap(uint8_t source_or_sink, bk_a2dp_codec_cap_t *cap)
{
    return bk_bt_a2dp_set_cap_internal(source_or_sink, cap);
}

bt_err_t bk_a2dp_sink_offload_set_enabled(bool en)
{
#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK
    return bt_a2dp_sink_offload_set_enabled_internal(en);
#else
    (void)en;
    return BK_ERR_BT_FAIL;
#endif
}

bool bk_a2dp_sink_offload_get_enabled(void)
{
#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK
    return bt_a2dp_sink_offload_get_enabled_internal();
#else
    return false;
#endif
}

bt_err_t bk_a2dp_sink_offload_set_volume(uint8_t vol)
{
#if CONFIG_BLUETOOTH_A2DP_OFFLOAD_SINK
    return bt_a2dp_sink_offload_set_volume_internal(vol);
#else
    (void)vol;
    return BK_ERR_BT_FAIL;
#endif
}


