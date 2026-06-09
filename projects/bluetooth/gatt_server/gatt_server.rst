Bluetooth GATT Server (BK7259)
=======================================

1. Project Overview
--------------------------

This project demonstrates Bluetooth GATT server on BK7259 SMP (AP+CP):

- Configure BLE advertising parameters and data, start/stop advertising.
- Create a GATT service database and handle ATT read/write/notify requests.

Source code: ``projects/bluetooth/gatt_server/ap/gatt_server_demo.c``

Note: GATT Authorization (``ap_cmd ble_gatts author``) from the BK7258 demo is not supported on BK7259.

1.1 BLE UUIDs
***********************************

Advertising data and the GATT database use different UUIDs:

+------------------+---------+--------------------------------------------------+
| Phase            | UUID    | Role                                             |
+==================+=========+==================================================+
| Advertising      | 0xFE01  | 16-bit Service Data in adv payload (scan only)   |
+------------------+---------+--------------------------------------------------+
| GATT (connected) | 0xFA00  | Primary service (discover after connection)      |
+------------------+---------+--------------------------------------------------+

GATT characteristics under service ``0xFA00``:

+------------------+-------------+-----------------------------------------------+
| UUID             | Property    | Description                                   |
+==================+=============+===============================================+
| 0xEA01           | Notify      | Demo notify characteristic (CCCD enabled)     |
+------------------+-------------+-----------------------------------------------+
| 0xEA02           | Write       | N1 write-only (no demo handler yet)           |
+------------------+-------------+-----------------------------------------------+
| 0xEA05           | Read/Write  | N2, stored string                             |
+------------------+-------------+-----------------------------------------------+
| 0xEA06           | Read/Write  | N3, stored string                             |
+------------------+-------------+-----------------------------------------------+
| 0xEA07           | Read/Write  | N4, stored string                             |
+------------------+-------------+-----------------------------------------------+

Manufacturer data in advertising uses Beken Company ID ``0x05F0``.

The device name in advertising is ``BK_XXYYZZ``, where ``XXYYZZ`` are the first three
bytes of the BLE MAC address returned by ``bk_bluetooth_get_address()``.

1.2 Disconnect and Advertising
***********************************

When a central device connects, the stack stops advertising automatically. After
disconnect, this demo **does not** restart advertising by itself. To make the
device discoverable again, run ``ap_cmd ble_gatts adv_en 1`` on the serial CLI,
or reset the board.

2. Hardware Requirements
----------------------------

Beken BK7259 development board.


3. Build
----------------------------

From the SDK root directory:

::

    make bk7259 PROJECT=bluetooth/gatt_server

Or use docker build:

::

    ./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server


4. Connect UART
----------------------------

UART0 is used for:

- BKFIL to download images
- Logging and CLI commands

On BK7259 SMP, AP-side CLI commands must use the ``ap_cmd`` prefix (for example
``ap_cmd ble_gatts help``). CP-side logs appear without a core prefix; AP-side
logs are tagged ``ap0:`` or ``ap1:``.


5. Flash all-app.bin
----------------------------

Flash ``build/bk7259/gatt_server/package/all-app.bin`` to the board using BKFIL.


6. Play and Output
----------------------------------

After reset, the device advertises automatically with name ``BK_XXYYZZ`` (derived from BLE MAC).
Use another board running ``gatt_client`` or a phone BLE tool (for example nRF Connect) to connect.

6.1 Boot and Advertising Log
***********************************

Key log lines after power-on (CP starts first, then AP; timestamps omitted):

::

    ble:D(377):ble mac: c8:47:8c:ab:de:f9
    bluetoot:D(404):bk_bluetooth_init ok
    D(544):cp start ap system
    D(640):ap system started
    ap0:ble:D(100):gapm_cmp_evt:BLE_STACK_OK
    ap0:bluetoot:D(100):bk_bluetooth_init ok
    ap0:BLE-GATT:I(600):cd_ind:prf_id:10, status:0
    ap0:BLE-GATT:I(600):create gatt db success
    ap0:BLE-GATT:I(600):gatt_server_demo_init, dev_name:BK_F9DEAB, ret:9
    ap0:BLE-GATT:I(600):adv data length :22
    ap1:BLE-GATT:I(606):set adv paramters success
    ap1:BLE-GATT:I(613):set adv data success
    ap1:BLE-GATT:I(619):start adv success
    ap1:BLE-GATT:I(619):gatt_server_demo_init success

At this point the device is connectable and scannable. nRF Connect should show
``BK_XXYYZZ`` with Service Data UUID ``0xFE01``.

6.2 Connected Log
***********************************

Example log when a phone connects and exercises GATT (notify CCCD, read characteristics):

::

    ap1:ble:W(421024):[gapc_connection_req_ind_handler]con_peer_addr:88:0c:d3:4e:1f:7f
    ap0:BLE-GATT:I(421025):BLE_5_BOND_INFO_REQ_EVENT
    ap0:BLE-GATT:I(421025):c_ind:conn_idx:0, addr_type:1, peer_addr:88:0c:d3:4e:1f:7f
    ap1:BLE-GATT:I(421518):ble_gatts_notice_cb m_ind:conn_idx:0, mtu_size:527
    ap1:BLE-GATT:I(535686):write_cb:conn_idx:0, prf_id:10, att_idx:3, len:2, data[0]:0x01
    ap1:BLE-GATT:I(535686):write notify: 01 00, length: 2
    ap1:BLE-GATT:I(547356):read_cb:conn_idx:0, prf_id:10, att_idx:7
    ap1:BLE-GATT:I(547356):read N2: (null), length: 0
    ap1:BLE-GATT:I(554166):d_ind:conn_idx:0,reason:19

``att_idx:3`` is the CCCD for characteristic ``0xEA01``; ``01 00`` enables notify.
``att_idx:7/9/11`` map to N2/N3/N4 value attributes; reads return empty until the
central has written data first. ``reason:19`` (``0x13``) means the remote user
terminated the connection.


7. Work Flow Chart
----------------------------------

GATT server demo workflow:

::

    Power on
      |
      v
    CP: BT Controller init  -->  AP: bk_init + BLE Host init
      |
      v
    gatt_server_demo_init()
      |-- bk_ble_create_db()          (service 0xFA00)
      |-- set adv data (name, 0xFE01) 
      |-- bk_ble_start_advertising()
      |
      v
    [Advertising]  <---- ap_cmd ble_gatts adv_en 1 (after disconnect)
      |
      |  central connects
      v
    [Connected]  ATT read / write / notify (CCCD on 0xEA01)
      |
      |  central disconnects (or ap_cmd ble_gatts adv_en 0 while idle)
      v
    [Advertising stopped]  -->  manual adv_en 1 or reboot to advertise again

Pair with ``projects/bluetooth/gatt_client`` on another board: board B scans,
connects to board A's MAC, discovers service ``0xFA00``, then read/write/notify.


8. CLI Commands
----------------------------------

AP-side commands on UART0 (``ap_cmd`` prefix required):

| ``ap_cmd ble_gatts help`` : list available commands.
| ``ap_cmd ble_gatts notify`` : send ATT notify on characteristic ``0xEA01`` after connected.
| ``ap_cmd ble_gatts adv_en 1`` : start advertising (use after disconnect to become visible again).
| ``ap_cmd ble_gatts adv_en 0`` : stop advertising.
| ``ap_cmd ble_gatts bond`` : start bonding with the connected device.

8.1 Command Log
***********************************

- ``ap_cmd ble_gatts help``

::

    ap_cmd ble_gatts help
    ap0:BLE-GATT:I(...):ble_gatts help
    ap0:BLE-GATT:I(...):ble_gatts notify
    ap0:BLE-GATT:I(...):ble_gatts adv_en [1|0]
    BLE GATTS RSP:OK

- ``ap_cmd ble_gatts notify`` (requires an active connection)

::

    ap_cmd ble_gatts notify
    BLE GATTS RSP:OK

- ``ap_cmd ble_gatts adv_en 1`` (restart advertising after disconnect)

::

    ap_cmd ble_gatts adv_en 1
    ap0:BLE-GATT:I(...):start or stop adv success
    BLE GATTS RSP:OK


9. API Reference
----------------------------------

Typical initialization sequence in ``gatt_server_demo_init()``:

1. ``bk_ble_set_notice_cb(ble_gatts_notice_cb)`` — register GAP/GATT event callback.
2. ``bk_ble_create_db(&ble_db_cfg)`` — create GATT database (service ``0xFA00``); wait for ``BLE_5_CREATE_DB``.
3. Build advertising payload (Flags, local name ``BK_XXYYZZ``, Service Data ``0xFE01``, manufacturer ``0x05F0``).
4. ``bk_ble_create_advertising()`` — configure legacy connectable/scannable advertising.
5. ``bk_ble_set_adv_data()`` — set the 22-byte advertising data.
6. ``bk_ble_start_advertising()`` — start broadcasting.

After connection, ``ble_gatts_notice_cb()`` handles ``BLE_5_WRITE_EVENT`` /
``BLE_5_READ_EVENT`` for characteristic values and CCCD, and
``gatts_demo_event_notify()`` calls ``bk_ble_send_noti_value()`` for CLI notify tests.

For header definitions see ``ap/include/components/bluetooth/bk_ble.h``.
