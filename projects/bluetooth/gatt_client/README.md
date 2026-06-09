# GATT Client Demo (BK7259)

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates a Bluetooth GATT client on the BK7259 SMP platform. The BLE host runs on AP and the controller runs on CP.

The demo provides:

- BLE scan start/stop control
- Connection to an advertising peripheral
- Automatic GATT service discovery after connection
- ATT read/write, notify CCC control, manual discovery, bonding, and white-list commands
- Integration-test entries in `.it.csv`

Source code: `projects/bluetooth/gatt_client/ap/gatt_client_demo.c`

### 1.1 Test Environment

- Hardware: BK7259 development board
- Peer device: another board running `projects/bluetooth/gatt_server`
- UART: UART0 for flashing, logging, and CLI
- Firmware output: `build/bk7259/gatt_client/package/all-app.bin`

## 2. Directory Structure

```text
gatt_client/
├── README.md
├── README_CN.md
├── .ci                         # CI build command
├── .it.csv                     # Integration test cases
├── ap/
│   ├── ap_main.c
│   ├── gatt_client_demo.c
│   ├── gatt_client_demo.h
│   └── CMakeLists.txt          # Copies .it.csv to build directory
├── cp/
├── partitions/
└── config files
```

## 3. Features

- CLI command group: `ap_cmd ble_gattc`
- Start/stop BLE scan
- Connect/disconnect by peer address
- Automatic SDP discovery after connection
- Read/write by handle
- Enable/disable notify CCC
- Manual service, characteristic, and descriptor discovery
- Bonding and white-list commands

## 4. Build And Run

### 4.1 Build

From the SDK root:

```bash
make bk7259 PROJECT=bluetooth/gatt_client
```

Docker build:

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_client
```

The project `.ci` file contains the docker build command used by CI:

```text
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_client
```

### 4.2 Flash

Flash the generated image with BKFIL:

```text
build/bk7259/gatt_client/package/all-app.bin
```

### 4.3 CLI Commands

On BK7259 SMP, AP-side commands must use the `ap_cmd` prefix. Without `ap_cmd`, the UART shell reports `cmd NOT found: ble_gattc`.

Common commands:

```text
ap_cmd ble_gattc help
ap_cmd ble_gattc scan 1
ap_cmd ble_gattc scan 0
ap_cmd ble_gattc conn <addr> [addr_type]
ap_cmd ble_gattc disconn
ap_cmd ble_gattc read <val_handle_hex>
ap_cmd ble_gattc write <val_handle_hex> <data>
ap_cmd ble_gattc notifyindcate_en <0|1> <desc_handle_hex>
ap_cmd ble_gattc discover_service <sh> <eh> <uuid> <uuid_len>
ap_cmd ble_gattc discover_char <sh> <eh> <uuid> <uuid_len>
ap_cmd ble_gattc discover_desc <sh> <eh>
ap_cmd ble_gattc bond
ap_cmd ble_gattc scan_filter <0|1>
```

Successful command submission returns:

```text
BLE GATTC RSP:OK
```

Failed command submission returns:

```text
BLE GATTC RSP:ERROR
```

### 4.4 How To Judge Pass Or Fail

After reboot, the client is ready when the log contains:

```text
gatt_client_demo_init success
```

For scan tests, `ap_cmd ble_gattc scan 1` should return `BLE GATTC RSP:OK`; if there are advertising BLE devices nearby, logs such as `ADV_IND` or `ADV_NONCONN_IND` may follow.

For connection tests with `gatt_server`, wait for automatic discovery logs:

```text
APPC_SERVICE_CONNECTED
==>Get GATT Service UUID:0xFA00
==>Get GATT Characteristic UUID:0xEA01
==>Get GATT Characteristic Description UUID:0x2902
```

Always use the handles printed by the current connection log. Typical handles with the matching `gatt_server` demo are:

```text
0xEA01 notify value handle: 0x12
0xEA01 CCC descriptor handle: 0x13
0xEA05 read/write value handle: 0x17
0xEA06 read/write value handle: 0x19
0xEA07 read/write value handle: 0x1B
```

### 4.5 Integration Test Commands

`.it.csv` is the runtime integration-test entry file. The test platform reads each row, sends the test command to the selected device UART, waits for the expected result string, and marks the case as passed if the string is found before timeout.

Current cases:

```text
ap_cmd ble_gattc help
ap_cmd ble_gattc scan 1
ap_cmd ble_gattc scan 0
```

Expected strings:

```text
BLE GATTC RSP:OK
```

The current static CSV covers stable single-board smoke tests. Full connect/read/write/notify verification requires a second board and a dynamic peer address from scan logs, so it is documented as a manual two-board flow instead of being hard-coded in `.it.csv`.

## 5. Test With GATT Server

1. Flash `projects/bluetooth/gatt_server` on board A.
2. Flash `projects/bluetooth/gatt_client` on board B.
3. Board A: confirm `gatt_server_demo_init success` and `start adv success`.
4. Board B: run `ap_cmd ble_gattc scan 1`.
5. Board B: find the server `adv_addr` from scan logs.
6. Board B: run `ap_cmd ble_gattc scan 0`.
7. Board B: run `ap_cmd ble_gattc conn <adv_addr>`.
8. Board B: wait for `APPC_SERVICE_CONNECTED` and service `0xFA00` discovery logs.
9. Board B: run `ap_cmd ble_gattc write 17 ssid_ab`, then `ap_cmd ble_gattc read 17` if the discovered handle is `0x17`.
10. Board B: run `ap_cmd ble_gattc notifyindcate_en 1 13` if the discovered CCC handle is `0x13`.
11. Board A: run `ap_cmd ble_gatts notify`.
12. Board B: run `ap_cmd ble_gattc disconn`.
13. Board A: run `ap_cmd ble_gatts adv_en 1` before the next connection.

## 6. Peer MAC Address

The recommended way is to use the `adv_addr` printed by `ap_cmd ble_gattc scan 1`.

If you calculate the address manually, the GATT server BLE public address is usually the flash base MAC with the last octet plus 1. For example, base MAC `c8:47:8c:b5:09:61` maps to BLE address `c8:47:8c:b5:09:62`.

## 7. Notes

1. AP logs may be prefixed with `ap0:` or `ap1:`.
2. GATT discovery runs automatically after connect.
3. Use handles from the current connection log; do not assume handle values are fixed across all firmware versions.
4. After disconnect, the server demo needs `ap_cmd ble_gatts adv_en 1` before reconnecting.
