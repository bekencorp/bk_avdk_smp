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

#pragma once

#ifdef __cplusplus
extern"C" {
#endif

#ifndef BK_BT_BIT
#define BK_BT_BIT(n)    (1UL << (n))
#endif


#define BK_BT_DATA_FLAGS                        0x01 /**< AD flags */
#define BK_BT_DATA_UUID16_SOME                  0x02 /**< 16-bit UUID, more available */
#define BK_BT_DATA_UUID16_ALL                   0x03 /**< 16-bit UUID, all listed */
#define BK_BT_DATA_UUID32_SOME                  0x04 /**< 32-bit UUID, more available */
#define BK_BT_DATA_UUID32_ALL                   0x05 /**< 32-bit UUID, all listed */
#define BK_BT_DATA_UUID128_SOME                 0x06 /**< 128-bit UUID, more available */
#define BK_BT_DATA_UUID128_ALL                  0x07 /**< 128-bit UUID, all listed */
#define BK_BT_DATA_NAME_SHORTENED               0x08 /**< Shortened name */
#define BK_BT_DATA_NAME_COMPLETE                0x09 /**< Complete name */
#define BK_BT_DATA_TX_POWER                     0x0a /**< Tx Power */
#define BK_BT_DATA_SM_TK_VALUE                  0x10 /**< Security Manager TK Value */
#define BK_BT_DATA_SM_OOB_FLAGS                 0x11 /**< Security Manager OOB Flags */
#define BK_BT_DATA_PERIPHERAL_INT_RANGE         0x12 /**< Peripheral Connection Interval Range */
#define BK_BT_DATA_SOLICIT16                    0x14 /**< Solicit UUIDs, 16-bit */
#define BK_BT_DATA_SOLICIT128                   0x15 /**< Solicit UUIDs, 128-bit */
#define BK_BT_DATA_SVC_DATA16                   0x16 /**< Service data, 16-bit UUID */
#define BK_BT_DATA_PUB_TARGET_ADDR              0x17 /**< Public Target Address */
#define BK_BT_DATA_RAND_TARGET_ADDR             0x18 /**< Random Target Address */
#define BK_BT_DATA_GAP_APPEARANCE               0x19 /**< GAP appearance */
#define BK_BT_DATA_ADV_INT                      0x1a /**< Advertising Interval */
#define BK_BT_DATA_LE_BK_BT_DEVICE_ADDRESS      0x1b /**< LE Bluetooth Device Address */
#define BK_BT_DATA_LE_ROLE                      0x1c /**< LE Role */
#define BK_BT_DATA_SIMPLE_PAIRING_HASH          0x1d /**< Simple Pairing Hash C256 */
#define BK_BT_DATA_SIMPLE_PAIRING_RAND          0x1e /**< Simple Pairing Randomizer R256 */
#define BK_BT_DATA_SOLICIT32                    0x1f /**< Solicit UUIDs, 32-bit */
#define BK_BT_DATA_SVC_DATA32                   0x20 /**< Service data, 32-bit UUID */
#define BK_BT_DATA_SVC_DATA128                  0x21 /**< Service data, 128-bit UUID */
#define BK_BT_DATA_LE_SC_CONFIRM_VALUE          0x22 /**< LE SC Confirmation Value */
#define BK_BT_DATA_LE_SC_RANDOM_VALUE           0x23 /**< LE SC Random Value */
#define BK_BT_DATA_URI                          0x24 /**< URI */
#define BK_BT_DATA_INDOOR_POS                   0x25 /**< Indoor Positioning */
#define BK_BT_DATA_TRANS_DISCOVER_DATA          0x26 /**< Transport Discovery Data */
#define BK_BT_DATA_LE_SUPPORTED_FEATURES        0x27 /**< LE Supported Features */
#define BK_BT_DATA_CHANNEL_MAP_UPDATE_IND       0x28 /**< Channel Map Update Indication */
#define BK_BT_DATA_MESH_PROV                    0x29 /**< Mesh Provisioning PDU */
#define BK_BT_DATA_MESH_MESSAGE                 0x2a /**< Mesh Networking PDU */
#define BK_BT_DATA_MESH_BEACON                  0x2b /**< Mesh Beacon */
#define BK_BT_DATA_BIG_INFO                     0x2c /**< BIGInfo */
#define BK_BT_DATA_BROADCAST_CODE               0x2d /**< Broadcast Code */
#define BK_BT_DATA_CSIS_RSI                     0x2e /**< CSIS Random Set ID type */
#define BK_BT_DATA_ADV_INT_LONG                 0x2f /**< Advertising Interval long */
#define BK_BT_DATA_BROADCAST_NAME               0x30 /**< Broadcast Name */
#define BK_BT_DATA_ENCRYPTED_AD_DATA            0x31 /**< Encrypted Advertising Data */
#define BK_BT_DATA_3D_INFO                      0x3D /**< 3D Information Data */
#define BK_BT_DATA_MANUFACTURER_DATA            0xff /**< Manufacturer Specific Data */


#define BK_BT_UUID_GAP                              0x1800
#define BK_BT_UUID_GATT                             0x1801
#define BK_BT_UUID_IAS                              0x1802
#define BK_BT_UUID_LLS                              0x1803
#define BK_BT_UUID_TPS                              0x1804
#define BK_BT_UUID_CTS                              0x1805
#define BK_BT_UUID_RTUS                             0x1806
#define BK_BT_UUID_NDSTS                            0x1807
#define BK_BT_UUID_GS                               0x1808
#define BK_BT_UUID_HTS                              0x1809
#define BK_BT_UUID_DIS                              0x180a
#define BK_BT_UUID_NAS                              0x180b
#define BK_BT_UUID_WDS                              0x180c
#define BK_BT_UUID_HRS                              0x180d
#define BK_BT_UUID_PAS                              0x180e
#define BK_BT_UUID_BAS                              0x180f
#define BK_BT_UUID_BPS                              0x1810
#define BK_BT_UUID_ANS                              0x1811
#define BK_BT_UUID_HIDS                             0x1812
#define BK_BT_UUID_SPS                              0x1813
#define BK_BT_UUID_RSCS                             0x1814
#define BK_BT_UUID_AIOS                             0x1815
#define BK_BT_UUID_CSC                              0x1816
#define BK_BT_UUID_CPS                              0x1818
#define BK_BT_UUID_LNS                              0x1819
#define BK_BT_UUID_ESS                              0x181a
#define BK_BT_UUID_BCS                              0x181b
#define BK_BT_UUID_UDS                              0x181c
#define BK_BT_UUID_WSS                              0x181d
#define BK_BT_UUID_BMS                              0x181e
#define BK_BT_UUID_CGMS                             0x181f
#define BK_BT_UUID_IPSS                             0x1820
#define BK_BT_UUID_IPS                              0x1821
#define BK_BT_UUID_POS                              0x1822
#define BK_BT_UUID_HPS                              0x1823
#define BK_BT_UUID_TDS                              0x1824
#define BK_BT_UUID_OTS                              0x1825
#define BK_BT_UUID_FMS                              0x1826
#define BK_BT_UUID_MESH_PROV                        0x1827
#define BK_BT_UUID_MESH_PROXY                       0x1828
#define BK_BT_UUID_MESH_PROXY_SOLICITATION          0x1859
#define BK_BT_UUID_RCSRV                            0x1829
#define BK_BT_UUID_IDS                              0x183a
#define BK_BT_UUID_BSS                              0x183b
#define BK_BT_UUID_ECS                              0x183c
#define BK_BT_UUID_ACLS                             0x183d
#define BK_BT_UUID_PAMS                             0x183e
#define BK_BT_UUID_AICS                             0x1843
#define BK_BT_UUID_VCS                              0x1844
#define BK_BT_UUID_VOCS                             0x1845
#define BK_BT_UUID_CSIS                             0x1846
#define BK_BT_UUID_DTS                              0x1847
#define BK_BT_UUID_MCS                              0x1848
#define BK_BT_UUID_GMCS                             0x1849
#define BK_BT_UUID_CTES                             0x184a
#define BK_BT_UUID_TBS                              0x184b
#define BK_BT_UUID_GTBS                             0x184c
#define BK_BT_UUID_MICS                             0x184d
#define BK_BT_UUID_ASCS                             0x184e
#define BK_BT_UUID_BASS                             0x184f
#define BK_BT_UUID_PACS                             0x1850
#define BK_BT_UUID_BASIC_AUDIO                      0x1851
#define BK_BT_UUID_BROADCAST_AUDIO                  0x1852
#define BK_BT_UUID_CAS                              0x1853
#define BK_BT_UUID_HAS                              0x1854
#define BK_BT_UUID_TMAS                             0x1855
#define BK_BT_UUID_PBA                              0x1856
#define BK_BT_UUID_GATT_PRIMARY                     0x2800
#define BK_BT_UUID_GATT_SECONDARY                   0x2801
#define BK_BT_UUID_GATT_INCLUDE                     0x2802
#define BK_BT_UUID_GATT_CHRC                        0x2803
#define BK_BT_UUID_GATT_CEP                         0x2900
#define BK_BT_UUID_GATT_CUD                         0x2901
#define BK_BT_UUID_GATT_CCC                         0x2902
#define BK_BT_UUID_GATT_SCC                         0x2903
#define BK_BT_UUID_GATT_CPF                         0x2904
#define BK_BT_UUID_GATT_CAF                         0x2905
#define BK_BT_UUID_VALID_RANGE                      0x2906
#define BK_BT_UUID_HIDS_EXT_REPORT                  0x2907
#define BK_BT_UUID_HIDS_REPORT_REF                  0x2908
#define BK_BT_UUID_VAL_TRIGGER_SETTING              0x290a
#define BK_BT_UUID_ES_CONFIGURATION                 0x290b
#define BK_BT_UUID_ES_MEASUREMENT                   0x290c
#define BK_BT_UUID_ES_TRIGGER_SETTING               0x290d
#define BK_BT_UUID_TM_TRIGGER_SETTING               0x290e
#define BK_BT_UUID_GAP_DEVICE_NAME                  0x2a00
#define BK_BT_UUID_GAP_APPEARANCE                   0x2a01
#define BK_BT_UUID_GAP_PPF                          0x2a02
#define BK_BT_UUID_GAP_RA                           0x2a03
#define BK_BT_UUID_GAP_PPCP                         0x2a04
#define BK_BT_UUID_GATT_SC                          0x2a05
#define BK_BT_UUID_ALERT_LEVEL                      0x2a06
#define BK_BT_UUID_TPS_TX_POWER_LEVEL               0x2a07
#define BK_BT_UUID_GATT_DT                          0x2a08
#define BK_BT_UUID_GATT_DW                          0x2a09
#define BK_BT_UUID_GATT_DDT                         0x2a0a
#define BK_BT_UUID_GATT_ET256                       0x2a0c
#define BK_BT_UUID_GATT_DST                         0x2a0d
#define BK_BT_UUID_GATT_TZ                          0x2a0e
#define BK_BT_UUID_GATT_LTI                         0x2a0f
#define BK_BT_UUID_GATT_TDST                        0x2a11
#define BK_BT_UUID_GATT_TA                          0x2a12
#define BK_BT_UUID_GATT_TS                          0x2a13
#define BK_BT_UUID_GATT_RTI                         0x2a14
#define BK_BT_UUID_GATT_TUCP                        0x2a16
#define BK_BT_UUID_GATT_TUS                         0x2a17
#define BK_BT_UUID_GATT_GM                          0x2a18
#define BK_BT_UUID_BAS_BATTERY_LEVEL                0x2a19
#define BK_BT_UUID_BAS_BATTERY_POWER_STATE          0x2a1a
#define BK_BT_UUID_BAS_BATTERY_LEVEL_STATE          0x2a1b
#define BK_BT_UUID_HTS_MEASUREMENT                  0x2a1c
#define BK_BT_UUID_HTS_TEMP_TYP                     0x2a1d
#define BK_BT_UUID_HTS_TEMP_INT                     0x2a1e
#define BK_BT_UUID_HTS_TEMP_C                       0x2a1f
#define BK_BT_UUID_HTS_TEMP_F                       0x2a20
#define BK_BT_UUID_HTS_INTERVAL                     0x2a21
#define BK_BT_UUID_HIDS_BOOT_KB_IN_REPORT           0x2a22
#define BK_BT_UUID_DIS_SYSTEM_ID                    0x2a23
#define BK_BT_UUID_DIS_MODEL_NUMBER                 0x2a24
#define BK_BT_UUID_DIS_SERIAL_NUMBER                0x2a25
#define BK_BT_UUID_DIS_FIRMWARE_REVISION            0x2a26
#define BK_BT_UUID_DIS_HARDWARE_REVISION            0x2a27
#define BK_BT_UUID_DIS_SOFTWARE_REVISION            0x2a28
#define BK_BT_UUID_DIS_MANUFACTURER_NAME            0x2a29
#define BK_BT_UUID_GATT_IEEE_RCDL                   0x2a2a
#define BK_BT_UUID_CTS_CURRENT_TIME                 0x2a2b
#define BK_BT_UUID_MAGN_DECLINATION                 0x2a2c
#define BK_BT_UUID_GATT_LLAT                        0x2a2d
#define BK_BT_UUID_GATT_LLON                        0x2a2e
#define BK_BT_UUID_GATT_POS_2D                      0x2a2f
#define BK_BT_UUID_GATT_POS_3D                      0x2a30
#define BK_BT_UUID_GATT_SR                          0x2a31
#define BK_BT_UUID_HIDS_BOOT_KB_OUT_REPORT          0x2a32
#define BK_BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT        0x2a33
#define BK_BT_UUID_GATT_GMC                         0x2a34
#define BK_BT_UUID_GATT_BPM                         0x2a35
#define BK_BT_UUID_GATT_ICP                         0x2a36
#define BK_BT_UUID_HRS_MEASUREMENT                  0x2a37
#define BK_BT_UUID_HRS_BODY_SENSOR                  0x2a38
#define BK_BT_UUID_HRS_CONTROL_POINT                0x2a39
#define BK_BT_UUID_GATT_REM                         0x2a3a
#define BK_BT_UUID_GATT_SRVREQ                      0x2a3b
#define BK_BT_UUID_GATT_SC_TEMP_C                   0x2a3c
#define BK_BT_UUID_GATT_STRING                      0x2a3d
#define BK_BT_UUID_GATT_NETA                        0x2a3e
#define BK_BT_UUID_GATT_ALRTS                       0x2a3f
#define BK_BT_UUID_GATT_RCP                         0x2a40
#define BK_BT_UUID_GATT_RS                          0x2a41
#define BK_BT_UUID_GATT_ALRTCID_MASK                0x2a42
#define BK_BT_UUID_GATT_ALRTCID                     0x2a43
#define BK_BT_UUID_GATT_ALRTNCP                     0x2a44
#define BK_BT_UUID_GATT_UALRTS                      0x2a45
#define BK_BT_UUID_GATT_NALRT                       0x2a46
#define BK_BT_UUID_GATT_SNALRTC                     0x2a47
#define BK_BT_UUID_GATT_SUALRTC                     0x2a48
#define BK_BT_UUID_GATT_BPF                         0x2a49
#define BK_BT_UUID_HIDS_INFO                        0x2a4a
#define BK_BT_UUID_HIDS_REPORT_MAP                  0x2a4b
#define BK_BT_UUID_HIDS_CTRL_POINT                  0x2a4c
#define BK_BT_UUID_HIDS_REPORT                      0x2a4d
#define BK_BT_UUID_HIDS_PROTOCOL_MODE               0x2a4e
#define BK_BT_UUID_GATT_SIW                         0x2a4f
#define BK_BT_UUID_DIS_PNP_ID                       0x2a50
#define BK_BT_UUID_GATT_GF                          0x2a51
#define BK_BT_UUID_RECORD_ACCESS_CONTROL_POINT      0x2a52
#define BK_BT_UUID_RSC_MEASUREMENT                  0x2a53
#define BK_BT_UUID_RSC_FEATURE                      0x2a54
#define BK_BT_UUID_SC_CONTROL_POINT                 0x2a55
#define BK_BT_UUID_GATT_DI                          0x2a56
#define BK_BT_UUID_GATT_DO                          0x2a57
#define BK_BT_UUID_GATT_AI                          0x2a58
#define BK_BT_UUID_GATT_AO                          0x2a59
#define BK_BT_UUID_GATT_AGGR                        0x2a5a
#define BK_BT_UUID_CSC_MEASUREMENT                  0x2a5b
#define BK_BT_UUID_CSC_FEATURE                      0x2a5c
#define BK_BT_UUID_SENSOR_LOCATION                  0x2a5d
#define BK_BT_UUID_GATT_PLX_SCM                     0x2a5e
#define BK_BT_UUID_GATT_PLX_CM                      0x2a5f
#define BK_BT_UUID_GATT_PLX_F                       0x2a60
#define BK_BT_UUID_GATT_POPE                        0x2a61
#define BK_BT_UUID_GATT_POCP                        0x2a62
#define BK_BT_UUID_GATT_CPS_CPM                     0x2a63
#define BK_BT_UUID_GATT_CPS_CPV                     0x2a64
#define BK_BT_UUID_GATT_CPS_CPF                     0x2a65
#define BK_BT_UUID_GATT_CPS_CPCP                    0x2a66
#define BK_BT_UUID_GATT_LOC_SPD                     0x2a67
#define BK_BT_UUID_GATT_NAV                         0x2a68
#define BK_BT_UUID_GATT_PQ                          0x2a69
#define BK_BT_UUID_GATT_LNF                         0x2a6a
#define BK_BT_UUID_GATT_LNCP                        0x2a6b
#define BK_BT_UUID_ELEVATION                        0x2a6c
#define BK_BT_UUID_PRESSURE                         0x2a6d
#define BK_BT_UUID_TEMPERATURE                      0x2a6e
#define BK_BT_UUID_HUMIDITY                         0x2a6f
#define BK_BT_UUID_TRUE_WIND_SPEED                  0x2a70
#define BK_BT_UUID_TRUE_WIND_DIR                    0x2a71
#define BK_BT_UUID_APPARENT_WIND_SPEED              0x2a72
#define BK_BT_UUID_APPARENT_WIND_DIR                0x2a73
#define BK_BT_UUID_GUST_FACTOR                      0x2a74
#define BK_BT_UUID_POLLEN_CONCENTRATION             0x2a75
#define BK_BT_UUID_UV_INDEX                         0x2a76
#define BK_BT_UUID_IRRADIANCE                       0x2a77
#define BK_BT_UUID_RAINFALL                         0x2a78
#define BK_BT_UUID_WIND_CHILL                       0x2a79
#define BK_BT_UUID_HEAT_INDEX                       0x2a7a
#define BK_BT_UUID_DEW_POINT                        0x2a7b
#define BK_BT_UUID_GATT_TREND                       0x2a7c
#define BK_BT_UUID_DESC_VALUE_CHANGED               0x2a7d
#define BK_BT_UUID_GATT_AEHRLL                      0x2a7e
#define BK_BT_UUID_GATT_AETHR                       0x2a7f
#define BK_BT_UUID_GATT_AGE                         0x2a80
#define BK_BT_UUID_GATT_ANHRLL                      0x2a81
#define BK_BT_UUID_GATT_ANHRUL                      0x2a82
#define BK_BT_UUID_GATT_ANTHR                       0x2a83
#define BK_BT_UUID_GATT_AEHRUL                      0x2a84
#define BK_BT_UUID_GATT_DATE_BIRTH                  0x2a85
#define BK_BT_UUID_GATT_DATE_THRASS                 0x2a86
#define BK_BT_UUID_GATT_EMAIL                       0x2a87
#define BK_BT_UUID_GATT_FBHRLL                      0x2a88
#define BK_BT_UUID_GATT_FBHRUL                      0x2a89
#define BK_BT_UUID_GATT_FIRST_NAME                  0x2a8a
#define BK_BT_UUID_GATT_5ZHRL                       0x2a8b
#define BK_BT_UUID_GATT_GENDER                      0x2a8c
#define BK_BT_UUID_GATT_HR_MAX                      0x2a8d
#define BK_BT_UUID_GATT_HEIGHT                      0x2a8e
#define BK_BT_UUID_GATT_HC                          0x2a8f
#define BK_BT_UUID_GATT_LAST_NAME                   0x2a90
#define BK_BT_UUID_GATT_MRHR                        0x2a91
#define BK_BT_UUID_GATT_RHR                         0x2a92
#define BK_BT_UUID_GATT_AEANTHR                     0x2a93
#define BK_BT_UUID_GATT_3ZHRL                       0x2a94
#define BK_BT_UUID_GATT_2ZHRL                       0x2a95
#define BK_BT_UUID_GATT_VO2_MAX                     0x2a96
#define BK_BT_UUID_GATT_WC                          0x2a97
#define BK_BT_UUID_GATT_WEIGHT                      0x2a98
#define BK_BT_UUID_GATT_DBCHINC                     0x2a99
#define BK_BT_UUID_GATT_USRIDX                      0x2a9a
#define BK_BT_UUID_GATT_BCF                         0x2a9b
#define BK_BT_UUID_GATT_BCM                         0x2a9c
#define BK_BT_UUID_GATT_WM                          0x2a9d
#define BK_BT_UUID_GATT_WSF                         0x2a9e
#define BK_BT_UUID_GATT_USRCP                       0x2a9f
#define BK_BT_UUID_MAGN_FLUX_DENSITY_2D             0x2aa0
#define BK_BT_UUID_MAGN_FLUX_DENSITY_3D             0x2aa1
#define BK_BT_UUID_GATT_LANG                        0x2aa2
#define BK_BT_UUID_BAR_PRESSURE_TREND               0x2aa3
#define BK_BT_UUID_BMS_CONTROL_POINT                0x2aa4
#define BK_BT_UUID_BMS_FEATURE                      0x2aa5
#define BK_BT_UUID_CENTRAL_ADDR_RES                 0x2aa6
#define BK_BT_UUID_CGM_MEASUREMENT                  0x2aa7
#define BK_BT_UUID_CGM_FEATURE                      0x2aa8
#define BK_BT_UUID_CGM_STATUS                       0x2aa9
#define BK_BT_UUID_CGM_SESSION_START_TIME           0x2aaa
#define BK_BT_UUID_CGM_SESSION_RUN_TIME             0x2aab
#define BK_BT_UUID_CGM_SPECIFIC_OPS_CONTROL_POINT   0x2aac
#define BK_BT_UUID_GATT_IPC                         0x2aad
#define BK_BT_UUID_GATT_LAT                         0x2aae
#define BK_BT_UUID_GATT_LON                         0x2aaf
#define BK_BT_UUID_GATT_LNCOORD                     0x2ab0
#define BK_BT_UUID_GATT_LECOORD                     0x2ab1
#define BK_BT_UUID_GATT_FN                          0x2ab2
#define BK_BT_UUID_GATT_ALT                         0x2ab3
#define BK_BT_UUID_GATT_UNCERTAINTY                 0x2ab4
#define BK_BT_UUID_GATT_LOC_NAME                    0x2ab5
#define BK_BT_UUID_URI                              0x2ab6
#define BK_BT_UUID_HTTP_HEADERS                     0x2ab7
#define BK_BT_UUID_HTTP_STATUS_CODE                 0x2ab8
#define BK_BT_UUID_HTTP_ENTITY_BODY                 0x2ab9
#define BK_BT_UUID_HTTP_CONTROL_POINT               0x2aba
#define BK_BT_UUID_HTTPS_SECURITY                   0x2abb
#define BK_BT_UUID_GATT_TDS_CP                      0x2abc
#define BK_BT_UUID_OTS_FEATURE                      0x2abd
#define BK_BT_UUID_OTS_NAME                         0x2abe
#define BK_BT_UUID_OTS_TYPE                         0x2abf
#define BK_BT_UUID_OTS_SIZE                         0x2ac0
#define BK_BT_UUID_OTS_FIRST_CREATED                0x2ac1
#define BK_BT_UUID_OTS_LAST_MODIFIED                0x2ac2
#define BK_BT_UUID_OTS_ID                           0x2ac3
#define BK_BT_UUID_OTS_PROPERTIES                   0x2ac4
#define BK_BT_UUID_OTS_ACTION_CP                    0x2ac5
#define BK_BT_UUID_OTS_LIST_CP                      0x2ac6
#define BK_BT_UUID_OTS_LIST_FILTER                  0x2ac7
#define BK_BT_UUID_OTS_CHANGED                      0x2ac8
#define BK_BT_UUID_GATT_RPAO                        0x2ac9
#define BK_BT_UUID_OTS_TYPE_UNSPECIFIED             0x2aca
#define BK_BT_UUID_OTS_DIRECTORY_LISTING            0x2acb
#define BK_BT_UUID_GATT_FMF                         0x2acc
#define BK_BT_UUID_GATT_TD                          0x2acd
#define BK_BT_UUID_GATT_CTD                         0x2ace
#define BK_BT_UUID_GATT_STPCD                       0x2acf
#define BK_BT_UUID_GATT_STRCD                       0x2ad0
#define BK_BT_UUID_GATT_RD                          0x2ad1
#define BK_BT_UUID_GATT_IBD                         0x2ad2
#define BK_BT_UUID_GATT_TRSTAT                      0x2ad3
#define BK_BT_UUID_GATT_SSR                         0x2ad4
#define BK_BT_UUID_GATT_SIR                         0x2ad5
#define BK_BT_UUID_GATT_SRLR                        0x2ad6
#define BK_BT_UUID_GATT_SHRR                        0x2ad7
#define BK_BT_UUID_GATT_SPR                         0x2ad8
#define BK_BT_UUID_GATT_FMCP                        0x2ad9
#define BK_BT_UUID_GATT_FMS                         0x2ada
#define BK_BT_UUID_MESH_PROV_DATA_IN                0x2adb
#define BK_BT_UUID_MESH_PROV_DATA_OUT               0x2adc
#define BK_BT_UUID_MESH_PROXY_DATA_IN               0x2add
#define BK_BT_UUID_MESH_PROXY_DATA_OUT              0x2ade
#define BK_BT_UUID_GATT_NNN                         0x2adf
#define BK_BT_UUID_GATT_AC                          0x2ae0
#define BK_BT_UUID_GATT_AV                          0x2ae1
#define BK_BT_UUID_GATT_BOOLEAN                     0x2ae2
#define BK_BT_UUID_GATT_CRDFP                       0x2ae3
#define BK_BT_UUID_GATT_CRCOORDS                    0x2ae4
#define BK_BT_UUID_GATT_CRCCT                       0x2ae5
#define BK_BT_UUID_GATT_CRT                         0x2ae6
#define BK_BT_UUID_GATT_CIEIDX                      0x2ae7
#define BK_BT_UUID_GATT_COEFFICIENT                 0x2ae8
#define BK_BT_UUID_GATT_CCTEMP                      0x2ae9
#define BK_BT_UUID_GATT_COUNT16                     0x2aea
#define BK_BT_UUID_GATT_COUNT24                     0x2aeb
#define BK_BT_UUID_GATT_CNTRCODE                    0x2aec
#define BK_BT_UUID_GATT_DATEUTC                     0x2aed
#define BK_BT_UUID_GATT_EC                          0x2aee
#define BK_BT_UUID_GATT_ECR                         0x2aef
#define BK_BT_UUID_GATT_ECSPEC                      0x2af0
#define BK_BT_UUID_GATT_ECSTAT                      0x2af1
#define BK_BT_UUID_GATT_ENERGY                      0x2af2
#define BK_BT_UUID_GATT_EPOD                        0x2af3
#define BK_BT_UUID_GATT_EVTSTAT                     0x2af4
#define BK_BT_UUID_GATT_FSTR16                      0x2af5
#define BK_BT_UUID_GATT_FSTR24                      0x2af6
#define BK_BT_UUID_GATT_FSTR36                      0x2af7
#define BK_BT_UUID_GATT_FSTR8                       0x2af8
#define BK_BT_UUID_GATT_GENLVL                      0x2af9
#define BK_BT_UUID_GATT_GTIN                        0x2afa
#define BK_BT_UUID_GATT_ILLUM                       0x2afb
#define BK_BT_UUID_GATT_LUMEFF                      0x2afc
#define BK_BT_UUID_GATT_LUMNRG                      0x2afd
#define BK_BT_UUID_GATT_LUMEXP                      0x2afe
#define BK_BT_UUID_GATT_LUMFLX                      0x2aff
#define BK_BT_UUID_GATT_LUMFLXR                     0x2b00
#define BK_BT_UUID_GATT_LUMINT                      0x2b01
#define BK_BT_UUID_GATT_MASSFLOW                    0x2b02
#define BK_BT_UUID_GATT_PERLGHT                     0x2b03
#define BK_BT_UUID_GATT_PER8                        0x2b04
#define BK_BT_UUID_GATT_PWR                         0x2b05
#define BK_BT_UUID_GATT_PWRSPEC                     0x2b06
#define BK_BT_UUID_GATT_RRICR                       0x2b07
#define BK_BT_UUID_GATT_RRIGLR                      0x2b08
#define BK_BT_UUID_GATT_RVIVR                       0x2b09
#define BK_BT_UUID_GATT_RVIIR                       0x2b0a
#define BK_BT_UUID_GATT_RVIPOD                      0x2b0b
#define BK_BT_UUID_GATT_RVITR                       0x2b0c
#define BK_BT_UUID_GATT_TEMP8                       0x2b0d
#define BK_BT_UUID_GATT_TEMP8_IPOD                  0x2b0e
#define BK_BT_UUID_GATT_TEMP8_STAT                  0x2b0f
#define BK_BT_UUID_GATT_TEMP_RNG                    0x2b10
#define BK_BT_UUID_GATT_TEMP_STAT                   0x2b11
#define BK_BT_UUID_GATT_TIM_DC8                     0x2b12
#define BK_BT_UUID_GATT_TIM_EXP8                    0x2b13
#define BK_BT_UUID_GATT_TIM_H24                     0x2b14
#define BK_BT_UUID_GATT_TIM_MS24                    0x2b15
#define BK_BT_UUID_GATT_TIM_S16                     0x2b16
#define BK_BT_UUID_GATT_TIM_S8                      0x2b17
#define BK_BT_UUID_GATT_V                           0x2b18
#define BK_BT_UUID_GATT_V_SPEC                      0x2b19
#define BK_BT_UUID_GATT_V_STAT                      0x2b1a
#define BK_BT_UUID_GATT_VOLF                        0x2b1b
#define BK_BT_UUID_GATT_CRCOORD                     0x2b1c
#define BK_BT_UUID_GATT_RCF                         0x2b1d
#define BK_BT_UUID_GATT_RCSET                       0x2b1e
#define BK_BT_UUID_GATT_RCCP                        0x2b1f
#define BK_BT_UUID_GATT_IDD_SC                      0x2b20
#define BK_BT_UUID_GATT_IDD_S                       0x2b21
#define BK_BT_UUID_GATT_IDD_AS                      0x2b22
#define BK_BT_UUID_GATT_IDD_F                       0x2b23
#define BK_BT_UUID_GATT_IDD_SRCP                    0x2b24
#define BK_BT_UUID_GATT_IDD_CCP                     0x2b25
#define BK_BT_UUID_GATT_IDD_CD                      0x2b26
#define BK_BT_UUID_GATT_IDD_RACP                    0x2b27
#define BK_BT_UUID_GATT_IDD_HD                      0x2b28
#define BK_BT_UUID_GATT_CLIENT_FEATURES             0x2b29
#define BK_BT_UUID_GATT_DB_HASH                     0x2b2a
#define BK_BT_UUID_GATT_BSS_CP                      0x2b2b
#define BK_BT_UUID_GATT_BSS_R                       0x2b2c
#define BK_BT_UUID_GATT_EMG_ID                      0x2b2d
#define BK_BT_UUID_GATT_EMG_TXT                     0x2b2e
#define BK_BT_UUID_GATT_ACS_S                       0x2b2f
#define BK_BT_UUID_GATT_ACS_DI                      0x2b30
#define BK_BT_UUID_GATT_ACS_DON                     0x2b31
#define BK_BT_UUID_GATT_ACS_DOI                     0x2b32
#define BK_BT_UUID_GATT_ACS_CP                      0x2b33
#define BK_BT_UUID_GATT_EBPM                        0x2b34
#define BK_BT_UUID_GATT_EICP                        0x2b35
#define BK_BT_UUID_GATT_BPR                         0x2b36
#define BK_BT_UUID_GATT_RU                          0x2b37
#define BK_BT_UUID_GATT_BR_EDR_HD                   0x2b38
#define BK_BT_UUID_GATT_BK_BT_SIG_D                 0x2b39
#define BK_BT_UUID_GATT_SERVER_FEATURES             0x2b3a
#define BK_BT_UUID_GATT_PHY_AMF                     0x2b3b
#define BK_BT_UUID_GATT_GEN_AID                     0x2b3c
#define BK_BT_UUID_GATT_GEN_ASD                     0x2b3d
#define BK_BT_UUID_GATT_CR_AID                      0x2b3e
#define BK_BT_UUID_GATT_CR_ASD                      0x2b3f
#define BK_BT_UUID_GATT_SC_ASD                      0x2b40
#define BK_BT_UUID_GATT_SLP_AID                     0x2b41
#define BK_BT_UUID_GATT_SLP_ASD                     0x2b42
#define BK_BT_UUID_GATT_PHY_AMCP                    0x2b43
#define BK_BT_UUID_GATT_ACS                         0x2b44
#define BK_BT_UUID_GATT_PHY_ASDESC                  0x2b45
#define BK_BT_UUID_GATT_PREF_U                      0x2b46
#define BK_BT_UUID_GATT_HRES_H                      0x2b47
#define BK_BT_UUID_GATT_MID_NAME                    0x2b48
#define BK_BT_UUID_GATT_STRDLEN                     0x2b49
#define BK_BT_UUID_GATT_HANDEDNESS                  0x2b4a
#define BK_BT_UUID_GATT_DEVICE_WP                   0x2b4b
#define BK_BT_UUID_GATT_4ZHRL                       0x2b4c
#define BK_BT_UUID_GATT_HIET                        0x2b4d
#define BK_BT_UUID_GATT_AG                          0x2b4e
#define BK_BT_UUID_GATT_SIN                         0x2b4f
#define BK_BT_UUID_GATT_CI                          0x2b50
#define BK_BT_UUID_GATT_TMAPR                       0x2b51
#define BK_BT_UUID_AICS_STATE                       0x2b77
#define BK_BT_UUID_AICS_GAIN_SETTINGS               0x2b78
#define BK_BT_UUID_AICS_INPUT_TYPE                  0x2b79
#define BK_BT_UUID_AICS_INPUT_STATUS                0x2b7a
#define BK_BT_UUID_AICS_CONTROL                     0x2b7b
#define BK_BT_UUID_AICS_DESCRIPTION                 0x2b7c
#define BK_BT_UUID_VCS_STATE                        0x2b7d
#define BK_BT_UUID_VCS_CONTROL                      0x2b7e
#define BK_BT_UUID_VCS_FLAGS                        0x2b7f
#define BK_BT_UUID_VOCS_STATE                       0x2b80
#define BK_BT_UUID_VOCS_LOCATION                    0x2b81
#define BK_BT_UUID_VOCS_CONTROL                     0x2b82
#define BK_BT_UUID_VOCS_DESCRIPTION                 0x2b83
#define BK_BT_UUID_CSIS_SET_SIRK                    0x2b84
#define BK_BT_UUID_CSIS_SET_SIZE                    0x2b85
#define BK_BT_UUID_CSIS_SET_LOCK                    0x2b86
#define BK_BT_UUID_CSIS_RANK                        0x2b87
#define BK_BT_UUID_GATT_EDKM                        0x2b88
#define BK_BT_UUID_GATT_AE32                        0x2b89
#define BK_BT_UUID_GATT_AP                          0x2b8a
#define BK_BT_UUID_GATT_CO2CONC                     0x2b8c
#define BK_BT_UUID_GATT_COS                         0x2b8d
#define BK_BT_UUID_GATT_DEVTF                       0x2b8e
#define BK_BT_UUID_GATT_DEVTP                       0x2b8f
#define BK_BT_UUID_GATT_DEVT                        0x2b90
#define BK_BT_UUID_GATT_DEVTCP                      0x2b91
#define BK_BT_UUID_GATT_TCLD                        0x2b92
#define BK_BT_UUID_MCS_PLAYER_NAME                  0x2b93
#define BK_BT_UUID_MCS_ICON_OBJ_ID                  0x2b94
#define BK_BT_UUID_MCS_ICON_URL                     0x2b95
#define BK_BT_UUID_MCS_TRACK_CHANGED                0x2b96
#define BK_BT_UUID_MCS_TRACK_TITLE                  0x2b97
#define BK_BT_UUID_MCS_TRACK_DURATION               0x2b98
#define BK_BT_UUID_MCS_TRACK_POSITION               0x2b99
#define BK_BT_UUID_MCS_PLAYBACK_SPEED               0x2b9a
#define BK_BT_UUID_MCS_SEEKING_SPEED                0x2b9b
#define BK_BT_UUID_MCS_TRACK_SEGMENTS_OBJ_ID        0x2b9c
#define BK_BT_UUID_MCS_CURRENT_TRACK_OBJ_ID         0x2b9d
#define BK_BT_UUID_MCS_NEXT_TRACK_OBJ_ID            0x2b9e
#define BK_BT_UUID_MCS_PARENT_GROUP_OBJ_ID          0x2b9f
#define BK_BT_UUID_MCS_CURRENT_GROUP_OBJ_ID         0x2ba0
#define BK_BT_UUID_MCS_PLAYING_ORDER                0x2ba1
#define BK_BT_UUID_MCS_PLAYING_ORDERS               0x2ba2
#define BK_BT_UUID_MCS_MEDIA_STATE                  0x2ba3
#define BK_BT_UUID_MCS_MEDIA_CONTROL_POINT          0x2ba4
#define BK_BT_UUID_MCS_MEDIA_CONTROL_OPCODES        0x2ba5
#define BK_BT_UUID_MCS_SEARCH_RESULTS_OBJ_ID        0x2ba6
#define BK_BT_UUID_MCS_SEARCH_CONTROL_POINT         0x2ba7
#define BK_BT_UUID_GATT_E32                         0x2ba8
#define BK_BT_UUID_OTS_TYPE_MPL_ICON                0x2ba9
#define BK_BT_UUID_OTS_TYPE_TRACK_SEGMENT           0x2baa
#define BK_BT_UUID_OTS_TYPE_TRACK                   0x2bab
#define BK_BT_UUID_OTS_TYPE_GROUP                   0x2bac
#define BK_BT_UUID_GATT_CTEE                        0x2bad
#define BK_BT_UUID_GATT_ACTEML                      0x2bae
#define BK_BT_UUID_GATT_ACTEMTC                     0x2baf
#define BK_BT_UUID_GATT_ACTETD                      0x2bb0
#define BK_BT_UUID_GATT_ACTEI                       0x2bb1
#define BK_BT_UUID_GATT_ACTEP                       0x2bb2
#define BK_BT_UUID_TBS_PROVIDER_NAME                0x2bb3
#define BK_BT_UUID_TBS_UCI                          0x2bb4
#define BK_BT_UUID_TBS_TECHNOLOGY                   0x2bb5
#define BK_BT_UUID_TBS_URI_LIST                     0x2bb6
#define BK_BT_UUID_TBS_SIGNAL_STRENGTH              0x2bb7
#define BK_BT_UUID_TBS_SIGNAL_INTERVAL              0x2bb8
#define BK_BT_UUID_TBS_LIST_CURRENT_CALLS           0x2bb9
#define BK_BT_UUID_CCID                             0x2bba
#define BK_BT_UUID_TBS_STATUS_FLAGS                 0x2bbb
#define BK_BT_UUID_TBS_INCOMING_URI                 0x2bbc
#define BK_BT_UUID_TBS_CALL_STATE                   0x2bbd
#define BK_BT_UUID_TBS_CALL_CONTROL_POINT           0x2bbe
#define BK_BT_UUID_TBS_OPTIONAL_OPCODES             0x2bbf
#define BK_BT_UUID_TBS_TERMINATE_REASON             0x2bc0
#define BK_BT_UUID_TBS_INCOMING_CALL                0x2bc1
#define BK_BT_UUID_TBS_FRIENDLY_NAME                0x2bc2
#define BK_BT_UUID_MICS_MUTE                        0x2bc3
#define BK_BT_UUID_ASCS_ASE_SNK                     0x2bc4
#define BK_BT_UUID_ASCS_ASE_SRC                     0x2bc5
#define BK_BT_UUID_ASCS_ASE_CP                      0x2bc6
#define BK_BT_UUID_BASS_CONTROL_POINT               0x2bc7
#define BK_BT_UUID_BASS_RECV_STATE                  0x2bc8
#define BK_BT_UUID_PACS_SNK                         0x2bc9
#define BK_BT_UUID_PACS_SNK_LOC                     0x2bca
#define BK_BT_UUID_PACS_SRC                         0x2bcb
#define BK_BT_UUID_PACS_SRC_LOC                     0x2bcc
#define BK_BT_UUID_PACS_AVAILABLE_CONTEXT           0x2bcd
#define BK_BT_UUID_PACS_SUPPORTED_CONTEXT           0x2bce
#define BK_BT_UUID_GATT_NH4CONC                     0x2bcf
#define BK_BT_UUID_GATT_COCONC                      0x2bd0
#define BK_BT_UUID_GATT_CH4CONC                     0x2bd1
#define BK_BT_UUID_GATT_NO2CONC                     0x2bd2
#define BK_BT_UUID_GATT_NONCH4CONC                  0x2bd3
#define BK_BT_UUID_GATT_O3CONC                      0x2bd4
#define BK_BT_UUID_GATT_PM1CONC                     0x2bd5
#define BK_BT_UUID_GATT_PM25CONC                    0x2bd6
#define BK_BT_UUID_GATT_PM10CONC                    0x2bd7
#define BK_BT_UUID_GATT_SO2CONC                     0x2bd8
#define BK_BT_UUID_GATT_SF6CONC                     0x2bd9
#define BK_BT_UUID_HAS_HEARING_AID_FEATURES         0x2bda
#define BK_BT_UUID_HAS_PRESET_CONTROL_POINT         0x2bdb
#define BK_BT_UUID_HAS_ACTIVE_PRESET_INDEX          0x2bdc
#define BK_BT_UUID_GATT_FSTR64                      0x2bde
#define BK_BT_UUID_GATT_HITEMP                      0x2bdf
#define BK_BT_UUID_GATT_HV                          0x2be0
#define BK_BT_UUID_GATT_LD                          0x2be1
#define BK_BT_UUID_GATT_LO                          0x2be2
#define BK_BT_UUID_GATT_LST                         0x2be3
#define BK_BT_UUID_GATT_NOISE                       0x2be4
#define BK_BT_UUID_GATT_RRCCTP                      0x2be5
#define BK_BT_UUID_GATT_TIM_S32                     0x2be6
#define BK_BT_UUID_GATT_VOCCONC                     0x2be7
#define BK_BT_UUID_GATT_VF                          0x2be8
#define BK_BT_UUID_BAS_BATTERY_CRIT_STATUS          0x2be9
#define BK_BT_UUID_BAS_BATTERY_HEALTH_STATUS        0x2bea
#define BK_BT_UUID_BAS_BATTERY_HEALTH_INF           0x2beb
#define BK_BT_UUID_BAS_BATTERY_INF                  0x2bec
#define BK_BT_UUID_BAS_BATTERY_LEVEL_STATUS         0x2bed
#define BK_BT_UUID_BAS_BATTERY_TIME_STATUS          0x2bee
#define BK_BT_UUID_GATT_ESD                         0x2bef
#define BK_BT_UUID_BAS_BATTERY_ENERGY_STATUS        0x2bf0
#define BK_BT_UUID_GATT_SL                          0x2bf5
#define BK_BT_UUID_GMAS                             0x1858
#define BK_BT_UUID_GMAP_ROLE                        0x2C00
#define BK_BT_UUID_GMAP_UGG_FEAT                    0x2C01
#define BK_BT_UUID_GMAP_UGT_FEAT                    0x2C02
#define BK_BT_UUID_GMAP_BGS_FEAT                    0x2C03
#define BK_BT_UUID_GMAP_BGR_FEAT                    0x2C04
#define BK_BT_UUID_SDP                              0x0001
#define BK_BT_UUID_UDP                              0x0002
#define BK_BT_UUID_RFCOMM                           0x0003
#define BK_BT_UUID_TCP                              0x0004
#define BK_BT_UUID_TCS_BIN                          0x0005
#define BK_BT_UUID_TCS_AT                           0x0006
#define BK_BT_UUID_ATT                              0x0007
#define BK_BT_UUID_OBEX                             0x0008
#define BK_BT_UUID_IP                               0x0009
#define BK_BT_UUID_FTP                              0x000a
#define BK_BT_UUID_HTTP                             0x000c
#define BK_BT_UUID_WSP                              0x000e
#define BK_BT_UUID_BNEP                             0x000f
#define BK_BT_UUID_UPNP                             0x0010
#define BK_BT_UUID_HIDP                             0x0011
#define BK_BT_UUID_HCRP_CTRL                        0x0012
#define BK_BT_UUID_HCRP_DATA                        0x0014
#define BK_BT_UUID_HCRP_NOTE                        0x0016
#define BK_BT_UUID_AVCTP                            0x0017
#define BK_BT_UUID_AVDTP                            0x0019
#define BK_BT_UUID_CMTP                             0x001b
#define BK_BT_UUID_UDI                              0x001d
#define BK_BT_UUID_MCAP_CTRL                        0x001e
#define BK_BT_UUID_MCAP_DATA                        0x001f
#define BK_BT_UUID_L2CAP                            0x0100


#define BK_BT_AUDIO_LOCATION_MONO_AUDIO                 (0)
#define BK_BT_AUDIO_LOCATION_FRONT_LEFT                 BK_BT_BIT(0)
#define BK_BT_AUDIO_LOCATION_FRONT_RIGHT                BK_BT_BIT(1)
#define BK_BT_AUDIO_LOCATION_FRONT_CENTER               BK_BT_BIT(2)
#define BK_BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1         BK_BT_BIT(3)
#define BK_BT_AUDIO_LOCATION_BACK_LEFT                  BK_BT_BIT(4)
#define BK_BT_AUDIO_LOCATION_BACK_RIGHT                 BK_BT_BIT(5)
#define BK_BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER       BK_BT_BIT(6)
#define BK_BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER      BK_BT_BIT(7)
#define BK_BT_AUDIO_LOCATION_BACK_CENTER                BK_BT_BIT(8)
#define BK_BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2         BK_BT_BIT(9)
#define BK_BT_AUDIO_LOCATION_SIDE_LEFT                  BK_BT_BIT(10)
#define BK_BT_AUDIO_LOCATION_SIDE_RIGHT                 BK_BT_BIT(11)
#define BK_BT_AUDIO_LOCATION_TOP_FRONT_LEFT             BK_BT_BIT(12)
#define BK_BT_AUDIO_LOCATION_TOP_FRONT_RIGHT            BK_BT_BIT(13)
#define BK_BT_AUDIO_LOCATION_TOP_FRONT_CENTER           BK_BT_BIT(14)
#define BK_BT_AUDIO_LOCATION_TOP_CENTER                 BK_BT_BIT(15)
#define BK_BT_AUDIO_LOCATION_TOP_BACK_LEFT              BK_BT_BIT(16)
#define BK_BT_AUDIO_LOCATION_TOP_BACK_RIGHT             BK_BT_BIT(17)
#define BK_BT_AUDIO_LOCATION_TOP_SIDE_LEFT              BK_BT_BIT(18)
#define BK_BT_AUDIO_LOCATION_TOP_SIDE_RIGHT             BK_BT_BIT(19)
#define BK_BT_AUDIO_LOCATION_TOP_BACK_CENTER            BK_BT_BIT(20)
#define BK_BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER        BK_BT_BIT(21)
#define BK_BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT          BK_BT_BIT(22)
#define BK_BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT         BK_BT_BIT(23)
#define BK_BT_AUDIO_LOCATION_FRONT_LEFT_WIDE            BK_BT_BIT(24)
#define BK_BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE           BK_BT_BIT(25)
#define BK_BT_AUDIO_LOCATION_LEFT_SURROUND              BK_BT_BIT(26)
#define BK_BT_AUDIO_LOCATION_RIGHT_SURROUND             BK_BT_BIT(27)

/* coding format assigned numbers, used for codec IDs */
#define BK_BT_CODEC_ID_ULAW_LOG           0x00
#define BK_BT_CODEC_ID_ALAW_LOG           0x01
#define BK_BT_CODEC_ID_CVSD               0x02
#define BK_BT_CODEC_ID_TRANSPARENT        0x03
#define BK_BT_CODEC_ID_LINEAR_PCM         0x04
#define BK_BT_CODEC_ID_MSBC               0x05
#define BK_BT_CODEC_ID_LC3                0x06
#define BK_BT_CODEC_ID_G729A              0x07
#define BK_BT_CODEC_ID_VS                 0xFF

#define BK_BT_CODEC_FRAME_DURATION_7500US       0x00
#define BK_BT_CODEC_FRAME_DURATION_10000US      0x01

#define BK_BT_CODEC_CFG_FREQ_8KHZ         0x01 /** 8 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_11KHZ        0x02 /** 11.025 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_16KHZ        0x03 /** 16 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_22KHZ        0x04 /** 22.05 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_24KHZ        0x05 /** 24 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_32KHZ        0x06 /** 32 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_44KHZ        0x07 /** 44.1 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_48KHZ        0x08 /** 48 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_88KHZ        0x09 /** 88.2 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_96KHZ        0x0a /** 96 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_176KHZ       0x0b /** 176.4 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_192KHZ       0x0c /** 192 Khz codec sampling frequency */
#define BK_BT_CODEC_CFG_FREQ_384KHZ       0x0d /** 384 Khz codec sampling frequency */

#define BK_BT_AUDIO_SAMPLING_FREQUENCY              0x01
#define BK_BT_AUDIO_FRAME_DURATION                  0x02
#define BK_BT_AUDIO_CHANNEL_ALLOCATION              0x03
#define BK_BT_AUDIO_OCTETS_PER_CODEC_FRAME          0x04
#define BK_BT_AUDIO_CODEC_FRAME_BLOCKS_PER_SDU      0x05

#ifdef __cplusplus
}
#endif

