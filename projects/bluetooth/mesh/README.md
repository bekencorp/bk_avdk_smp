# Bluetooth Mesh Example

* [中文](./README_CN.md)

This project runs the Zephyr BLE Host and Bluetooth Mesh stack on the AP core,
with the BK7259 BLE controller running on the CP core through HCI over IPC. It is
ported from the BK7258 Mesh demo and keeps the `provisioner` / `provisionee`
roles and the legacy `ble_mesh` CLI.

## Features

- Join a phone/nRF Mesh network as a `provisionee`.
- Create a Mesh network as a `provisioner` and provision nearby devices.
- Generic OnOff Server/Client, Health Server/Client, Config Server/Client.
- Beken vendor count model for two-board traffic tests.
- Tmall Genie light demo entry points are kept under `ble_mesh tmall ...`.
- Host stack: Zephyr v2.7.6 Bluetooth Host.

## Source Layout

- Project entry: `projects/bluetooth/mesh/`
- AP demo: `projects/bluetooth/mesh/ap/ap_main.c`
- AP Zephyr Host/Mesh: `ap/components/bk_bluetooth/service/mesh/`
- AP HCI over IPC driver: `ap/components/bk_bluetooth/service/mesh/hci_ipc_driver.c`
- AP Mesh CLI: `ap/components/bk_bluetooth/service/mesh/mesh_cli.c`
- CP controller-only side: `cp/components/bk_bluetooth/` and controller libraries

## Key Configuration

AP: `projects/bluetooth/mesh/ap/config/bk7259_ap/defconfig`

```text
CONFIG_BLUETOOTH_AP=y
CONFIG_BLUETOOTH_HOST_ONLY=y
CONFIG_BLE=y
# CONFIG_BT is not set
CONFIG_BLE_MESH_ZEPHYR=y
CONFIG_BT_MESH_PB_ADV=y
CONFIG_BT_MESH_PB_GATT=y
CONFIG_BT_MESH_GATT_PROXY=y
CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL=3
```

CP: `projects/bluetooth/mesh/cp/config/bk7259/defconfig`

```text
CONFIG_BTDM_CONTROLLER_ONLY=y
CONFIG_BLE=y
# CONFIG_BT is not set
```

## Build and Flash

Build from the SDK root:

```bash
make bk7259 PROJECT=bluetooth/mesh
```

Firmware output:

```text
build/bk7259/mesh/package/all-app.bin
```

## CLI

Send commands through the AP CLI:

```text
ap_cmd ble_mesh provision init provisioner
ap_cmd ble_mesh provision init provisionee
ap_cmd ble_mesh provision deprovision <MAC>
ap_cmd ble_mesh provision send_count [interval_ms]
ap_cmd ble_mesh tmall init <product_id> <device_secret>
ap_cmd ble_mesh tmall click
ap_cmd ble_mesh led init [type]
ap_cmd ble_mesh led click
ap_cmd ble_mesh led send_count [interval_ms]
```

## Phone / nRF Mesh Provisioning

1. Flash `all-app.bin`, reboot the board, and wait for:

   ```text
   ble_mesh cli ready
   ```

2. Start the board as a provisionee:

   ```text
   ap_cmd ble_mesh provision init provisionee
   ```

3. Scan with nRF Mesh or another Mesh Provisioner.
   - PB-GATT advertises the Mesh Provisioning Service UUID `0x1827`.
   - PB-ADV uses Mesh Beacon advertising and may show up as unknown in generic BLE scan tools.
   - The default device name is `zephyr-mesh` when name advertising is enabled; otherwise filter by `0x1827`.

4. Provision the node, add an AppKey, bind the Generic OnOff Server, and control
   OnOff from the app.

## Two-Board Test

1. Flash this project to two boards and reboot both.
2. Start board A as the provisioner:

   ```text
   ap_cmd ble_mesh provision init provisioner
   ```

3. Start board B as the provisionee:

   ```text
   ap_cmd ble_mesh provision init provisionee
   ```

4. After provisioning and binding succeed, send vendor count messages from board A:

   ```text
   ap_cmd ble_mesh provision send_count 1500
   ```

   Stop periodic sending:

   ```text
   ap_cmd ble_mesh provision send_count 0
   ```

## Troubleshooting

- **The phone cannot find the device name**:
  PB-ADV does not carry a normal BLE local name. For PB-GATT, scan or filter by
  Mesh Provisioning Service UUID `0x1827`. Name advertising is controlled by
  `CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME`.

- **No OnOff or AppKey binding logs**:
  `BT_INFO` logs require `CONFIG_ACLSEMI_BT_MESH_DEBUG_LEVEL >= 3`. This project
  sets it to 3 by default.

- **Composition Data fails with `Message too big`**:
  This is a Mesh transport segmentation limit, not an ATT MTU issue. This project
  sets `CONFIG_BT_MESH_TX_SEG_MAX` / `RX_SEG_MAX` to 4 for the current demo
  composition data.

- **`Advertising failed: err -12` after PB-GATT connects**:
  `-12` means no free connection object. With `CONFIG_BLE_CONN_NUM=1`, this can
  happen after the phone is already connected. It is usually harmless; increase
  the connection count if continuous connectable advertising is required.
