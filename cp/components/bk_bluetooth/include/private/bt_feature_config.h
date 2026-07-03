#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RF_MODE_POLAR 0
#define RF_MODE_IQ_HIGH_PLL 1
#define RF_MODE_IQ_LOW_PLL 2

typedef struct
{
    uint8_t _is_gatt_discovery_auto;                //only valid when use single ble host.
    uint8_t _ignore_smp_key_distr_all_zero;         //only valid when use single ble host.
    uint8_t _strict_smp_key_distr_check_except_all_zero;
    uint8_t _ignore_smp_already_pair;               //only valid when use single ble host.
    uint8_t _send_peripheral_feature_req_auto;
    uint8_t _stop_smp_when_pair_err;
    uint8_t _enable_smp_sec_req_evt;

    uint8_t _support_lpo_rosc;
    uint8_t _support_lowpower_sleep;
    uint8_t _rf_mode;
} bt_feature_struct_t;
