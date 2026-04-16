// Copyright 2020-2021 Beken
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


/*
 * DEFINES
 ****************************************************************************************
 */

/* CMD_RF_HOLD_BIT_SET/_CLR*/
///saradc control rf
#define RF_BY_SARADC_BIT                     (1 << 6)
///temp control rf
#define RF_BY_TEMP_BIT                       (1 << 5)
///ate control rf
#define RF_BY_ATE_BT_BIT                     (1 << 4)
#define RF_BY_ATE_WIFI_BIT                   (1 << 3)
///bkreg
#define RF_BY_BKREG_BIT                      (1 << 2)
///ble control rf
#define RF_BY_BLE_BIT                        (1 << 1)
///wifi control rf
#define RF_BY_WIFI_BIT                       (1 << 0)
/* END*/


/* CMD_PLL_HOLD_BIT_SET/_CLR*/
#define RF_WIFIPLL_HOLD_BY_BLE_BIT                   (1 << 1)
#define RF_WIFIPLL_HOLD_BY_WIFI_BIT                  (1 << 0)
/* END*/


/*
 * ENUM
 ****************************************************************************************
 */

enum {
    RF_CLOSE,
    RF_OPEN,
};

enum {
    CMD_RF_WIFIPLL_HOLD_BIT_SET,
    CMD_RF_WIFIPLL_HOLD_BIT_CLR,
};

enum MODULE_TYPE_E{
    MODULE_TYPE_BLE_BT,
    MODULE_TYPE_WIFI,
    MODULE_TYPE_THREAD,
    MODULE_TYPE_MAX,
};

enum RF_OPERATION_E{
    RF_OPERATION_INVALID,
    RF_OPERATION_FREE,
    RF_OPERATION_APPLY,
    RF_OPERATION_MAX,
};

enum RF_PATH_E{
    RF_PATH_WIFI_IQ,
    RF_PATH_BT_IQ,
    RF_PATH_BT_POLAR,
    RF_PATH_MAX,
};

enum RF_PRIORITY_E{
    RF_PRIORITY_BLE_BT_HIGH,
    RF_PRIORITY_WIFI_HIGH,
    RF_PRIORITY_BLE_BT_NORMAL,
    RF_PRIORITY_WIFI_NORMAL,
    RF_PRIORITY_MAX,
};

enum RF_PLL_E{
    RF_PLL_LOW,
    RF_PLL_HIGH,
    RF_PLL_MAX,
};

struct RF_ARBITRATION_T{
    enum MODULE_TYPE_E module_type;
    enum RF_OPERATION_E operation;
    enum RF_PATH_E rf_path;
    enum RF_PLL_E rf_pll;
    enum RF_PRIORITY_E priority;
    bool is_save_when_failed;
    bool is_need_switch_rf;
};

enum RF_ARBIT_RESULT_E{
    RF_ARBIT_RESULT_SUCCESS,
    RF_ARBIT_RESULT_CONFLICT,
    RF_ARBIT_RESULT_ERROR,
    RF_ARBIT_RESULT_MAX,
};
/*
 * FUNCTION
 ****************************************************************************************
 */
 #if 0
 UINT32 rf_pll_ctrl(UINT32 cmd, UINT32 param);
 #else
 enum RF_ARBIT_RESULT_E rf_pll_ctrl(
     enum MODULE_TYPE_E module_type, 
     enum RF_OPERATION_E operation, 
     enum RF_PATH_E rf_path, 
     enum RF_PLL_E rf_pll,
     enum RF_PRIORITY_E priority, 
     bool is_save_when_failed
     );
 #endif
void rf_module_vote_ctrl(uint8_t cmd,uint32_t module);
void phy_clk_close_handler(uint32_t module);
void phy_clk_open_handler(uint32_t module);


