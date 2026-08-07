#ifndef __BLE_PROVISIONING_H__
#define __BLE_PROVISIONING_H__

typedef void (*ble_provisioning_op_cb_t)(uint16_t opcode, uint16_t length, uint8_t *data);
typedef struct
{
    char *ssid_value;
    char *password_value;
    ble_provisioning_op_cb_t cb;
    uint8_t boarding_notify[2];
    uint16_t ssid_length;
    uint16_t password_length;
} ble_provisioning_info_t;

typedef struct
{
    ble_provisioning_info_t ble_prov_info;
    uint16_t channel;
} bk_ble_provisioning_info_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
    uint16_t length;
} ble_prov_msg_t;

typedef void (*ble_msg_handle_cb_t)(ble_prov_msg_t *msg);

bk_ble_provisioning_info_t * bk_ble_provisioning_get_boarding_info(void);
int bk_ble_provisioning_init(void);
int bk_ble_provisioning_deinit(void);

/**
 * @brief Configure the BLE provisioning advertised device name.
 *
 * When a non-empty name is set, wifi_boarding_adv_start() advertises this exact
 * string (the name the phone app scans for) instead of deriving one from the
 * MAC address. This lets the application own the device-name rule (single source
 * of truth) and display the very same name in its UI, so the advertised name and
 * the UI can never drift apart.
 *
 * @param name  NUL-terminated device name; pass NULL or "" to restore the
 *              default MAC-derived name.
 */
void bk_ble_provisioning_set_adv_name(const char *name);

/**
 * @brief Set the manufacturer-specific data payload that FOLLOWS the company ID.
 *
 * The SDK always emits the 2-byte Beken company identifier (0x05F0) as the first
 * two octets of the Manufacturer Specific Data AD structure. This function
 * supplies the application-defined bytes that come AFTER it (per the adv spec,
 * the 5-byte core header proto_ver+device_type+fw; multi-byte values
 * little-endian per CSS v10 1.4). The resulting Manufacturer AD structure is
 * advertised inside the ADV packet so a passive scan can read it. Prefer the
 * typed helper bk_ble_provisioning_set_dev_info() over passing raw bytes here.
 *
 * Must be called before provisioning/advertising starts (i.e. before
 * bk_network_provisioning_init()/wifi_boarding_adv_start()). When never called
 * (or len == 0) only the company ID is advertised (no core header) and the
 * Local Name placement is unchanged, so existing users are unaffected.
 *
 * @param data  Application payload placed after the company ID; NULL/len==0 clears.
 * @param len   Payload length in bytes (truncated to the internal max).
 */
void bk_ble_provisioning_set_manuf_data(const uint8_t *data, uint16_t len);

/**
 * @brief Advertising private-data protocol version (BLE provisioning adv spec).
 *
 * First byte of the 5-byte manufacturer core header. Bump only when the core
 * header layout changes incompatibly; the phone app reads it before parsing.
 */
#define BK_BLE_PROV_PROTO_VER 0x01

/**
 * @brief Device type codes advertised in the manufacturer core header (spec 4.2).
 *
 * Identifies the product category so the phone app can pick the right UI and
 * provisioning flow by scanning only (no connection). 0x80~0xFE are reserved for
 * customer/solution-defined types.
 */
typedef enum {
    BK_BLE_PROV_DEV_TYPE_RESERVED  = 0x00,
    BK_BLE_PROV_DEV_TYPE_DOORLOCK  = 0x01,
    BK_BLE_PROV_DEV_TYPE_DOORBELL  = 0x02,
    BK_BLE_PROV_DEV_TYPE_DASHBOARD = 0x03,
    BK_BLE_PROV_DEV_TYPE_INTERCOM  = 0x04,
    BK_BLE_PROV_DEV_TYPE_IPC       = 0x05,
    BK_BLE_PROV_DEV_TYPE_ROBOT     = 0x06,
    BK_BLE_PROV_DEV_TYPE_MESH      = 0x07,
    BK_BLE_PROV_DEV_TYPE_UNKNOWN   = 0xFF,
} bk_ble_prov_device_type_t;

/**
 * @brief Set the device type + firmware version advertised in the ADV packet.
 *
 * Builds the 5-byte manufacturer core header {proto_ver, device_type, fw_major,
 * fw_minor, fw_patch} and stores it as the manufacturer payload (the SDK
 * prepends the Beken company ID 0x05F0). wifi_boarding_adv_start() advertises
 * this core header inside the ADV Manufacturer Specific Data so a passive scan
 * reveals category and version. Thin wrapper over
 * bk_ble_provisioning_set_manuf_data(); call before advertising starts.
 *
 * @param device_type  One of bk_ble_prov_device_type_t (or a customer code).
 * @param fw_major     Firmware major version.
 * @param fw_minor     Firmware minor version.
 * @param fw_patch     Firmware patch version.
 */
void bk_ble_provisioning_set_dev_info(uint8_t device_type,
                                      uint8_t fw_major, uint8_t fw_minor, uint8_t fw_patch);

/**
 * @brief Return the canonical Local Name tag for a device type (spec 4.2/7).
 *
 * Solutions build the advertised name as "BK_<TAG>_<MAC3>" using this tag so the
 * naming is consistent across products. Unknown types map to "UNKNOWN".
 *
 * @param device_type  One of bk_ble_prov_device_type_t.
 * @return NUL-terminated uppercase tag string (never NULL).
 */
const char *bk_ble_provisioning_dev_type_tag(uint8_t device_type);

void bk_ble_provisioning_event_notify(uint16_t opcode, int status);
int wifi_boarding_notify(uint8_t *data, uint16_t length);
void bk_ble_provisioning_event_notify_with_data(uint16_t opcode, int status, char *payload, uint16_t length);

#endif
