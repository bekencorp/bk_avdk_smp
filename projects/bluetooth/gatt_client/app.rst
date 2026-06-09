.. _project_gatt_client:

gatt_client
=============================

Overview
-----------------------------

Bluetooth GATT client demo for BK7259 SMP (AP+CP). Demonstrates BLE scanning,
connection, GATT service discovery, and read/write/notify via CLI command ``ble_gattc``.

Configure and Build
-----------------------------

Build from the SDK root directory:

::

    make bk7259 PROJECT=bluetooth/gatt_client

Or use docker build:

::

    ./dbuild.sh make bk7259 PROJECT=bluetooth/gatt_client

Flash
****************************

Flash ``build/bk7259/gatt_client/package/all-app.bin`` using BKFIL on UART0.

Running and Output
------------------------------

After reset, use ``ap_cmd ble_gattc help`` on the serial CLI.
Typical flow: ``ap_cmd ble_gattc scan`` -> ``ap_cmd ble_gattc conn <addr>`` -> discover/read/write/notify.

Pair with ``projects/bluetooth/gatt_server`` on another board for full GATT testing.
