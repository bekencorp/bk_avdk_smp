.. _project_gatt_server:

gatt_server
=============================

Overview
-----------------------------

Bluetooth GATT server demo for BK7259 SMP (AP+CP). Demonstrates BLE advertising,
GATT database creation, and ATT read/write/notify handling via CLI command ``ap_cmd ble_gatts``.

BLE UUIDs
-----------------------------

Advertising ``0xFE01`` (Service Data) is visible when scanning; GATT service ``0xFA00`` and
characteristics ``0xEA01``–``0xEA07`` are discovered after connection. See ``README.md`` for the full UUID table.

Disconnect behavior
-----------------------------

After a central disconnects, advertising does not resume automatically. Use ``ap_cmd ble_gatts adv_en 1`` or reboot to advertise again.

Configure and Build
-----------------------------

Build from the SDK root directory:

::

    make bk7259 PROJECT=bluetooth/gatt_server

Or use docker build:

::

    ./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_server

Flash
****************************

Flash ``build/bk7259/gatt_server/package/all-app.bin`` to the board using BKFIL.


Running and Output
------------------------------

After reset, the device advertises automatically with name ``BK_XXYYZZ``.
Use ``ap_cmd ble_gatts help`` on the serial CLI for available commands.

Pair with ``projects/bluetooth/gatt_client`` on another board for full GATT testing.
