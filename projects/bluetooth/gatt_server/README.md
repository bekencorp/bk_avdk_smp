# GATT Server Demo (BK7259)

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates a Bluetooth GATT server on the BK7259 SMP platform. The BLE host runs on AP and the controller runs on CP.

The demo provides:

- BLE advertising setup and start/stop control
- A GATT database with service `0xFA00`
- ATT read/write handling for demo characteristics
- ATT notify through CLI after a client enables CCCD
- Integration-test entries in `.it.csv`

Source code: `projects/bluetooth/gatt_server/ap/gatt_server_demo.c`

### 1.1 Test Environment

- Hardware: BK7259 development board
- Peer device: another board running `projects/bluetooth/gatt_client`, or a phone BLE tool such as nRF Connect
- UART: UART0 for flashing, logging, and CLI
- Firmware output: `build/bk7259/gatt_server/package/all-app.bin`

### 1.2 UUIDs

Advertising data and the connected GATT database use different UUIDs:

- Advertising Service Data UUID: `0xFE01`
- GATT primary service UUID: `0xFA00`
- Notify characteristic UUID: `0xEA01`, CCCD enabled
- Read/write characteristic UUIDs: `0xEA05`, `0xEA06`, `0xEA07`
- Write-only characteristic UUID: `0xEA02`

The advertising name is `BK_XXYYZZ`, derived from the BLE MAC address.

## 2. Directory Structure

```text
gatt_server/
├── README.md
├── README_CN.md
├── .ci                         # CI build command
├── .it.csv                     # Integration test cases
├── ap/
│   ├── ap_main.c
│   ├── gatt_server_demo.c
│   ├── gatt_server_demo.h
│   └── CMakeLists.txt          # Copies .it.csv to build directory
├── cp/
├── partitions/
└── config files
```

## 3. Features

- Auto-start advertising after boot
- CLI command group: `ap_cmd ble_gatts`
- Manual advertising control: `adv_en 1` / `adv_en 0`
- Notify test command: `notify`
- Bonding command: `bond`
- GATT authorization from the BK7258 demo is not supported on BK7259 and is not included

After a central device connects, advertising stops automatically. This demo does not restart advertising automatically after disconnect. Run `ap_cmd ble_gatts adv_en 1` or reboot the board before the next connection.

## 4. Build And Run

### 4.1 Build

From the SDK root:

```bash
make bk7259 PROJECT=bluetooth/gatt_server
```

Docker build:

```bash
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server
```

The project `.ci` file contains the docker build command used by CI:

```text
./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server
```

### 4.2 Flash

Flash the generated image with BKFIL:

```text
build/bk7259/gatt_server/package/all-app.bin
```

### 4.3 CLI Commands

On BK7259 SMP, AP-side commands must use the `ap_cmd` prefix:

```text
ap_cmd ble_gatts help
ap_cmd ble_gatts adv_en 1
ap_cmd ble_gatts adv_en 0
ap_cmd ble_gatts notify
ap_cmd ble_gatts bond
```

Successful command submission returns:

```text
BLE GATTS RSP:OK
```

Failed command submission returns:

```text
BLE GATTS RSP:ERROR
```

### 4.4 How To Judge Pass Or Fail

After reboot, the server is ready when the log contains:

```text
gatt_server_demo_init success
```

Typical advertising success logs include:

```text
create gatt db success
set adv paramters success
set adv data success
start adv success
```

For manual CLI tests, `BLE GATTS RSP:OK` means the command was accepted. For notify and bond, a valid active connection is required; otherwise the command returns `BLE GATTS RSP:ERROR`.

### 4.5 Integration Test Commands

`.it.csv` is the runtime integration-test entry file. The test platform reads each row, sends the test command to the selected device UART, waits for the expected result string, and marks the case as passed if the string is found before timeout.

Current cases:

```text
ap_cmd ble_gatts help
ap_cmd ble_gatts adv_en 0
ap_cmd ble_gatts adv_en 1
```

Expected strings:

```text
BLE GATTS RSP:OK
```

The current static CSV covers stable single-board smoke tests. Full GATT read/write/notify verification requires a second board and a dynamic peer address from scan logs, so it is documented as a manual two-board flow instead of being hard-coded in `.it.csv`.

## 5. Test With GATT Client

1. Flash `projects/bluetooth/gatt_server` on board A.
2. Flash `projects/bluetooth/gatt_client` on board B.
3. Board A: confirm `gatt_server_demo_init success` and `start adv success`.
4. Board B: run `ap_cmd ble_gattc scan 1`.
5. Board B: use the discovered `adv_addr` to run `ap_cmd ble_gattc conn <adv_addr>`.
6. Board B: wait for service discovery logs for service `0xFA00`.
7. Board B: enable notify with `ap_cmd ble_gattc notifyindcate_en 1 <ccc_handle>`.
8. Board A: run `ap_cmd ble_gatts notify`.

## 6. Notes

1. AP logs may be prefixed with `ap0:` or `ap1:`.
2. CP logs may appear without an AP prefix.
3. Use `ap_cmd` for all AP-side Bluetooth CLI commands on BK7259 SMP.
4. If the client disconnects, restart server advertising with `ap_cmd ble_gatts adv_en 1` before reconnecting.
