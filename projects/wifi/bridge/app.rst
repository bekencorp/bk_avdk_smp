.. _project_wifi_bridge:

WiFi Bridge Demo
================

Overview
-----------------------------

Demonstrates Beken WiFi bridge mode (STA uplink + softAP downstream, lwIP ``br0``).
Use the ``bridge`` CLI on the AP core after flash.

Hardware Requirements
------------------------------

- BK7259 board (same platform layout as ``projects/wifi/iperf``)
- UART0 for CLI

Configure and Build
-----------------------------

Configure the Project
****************************

- AP: ``projects/wifi/bridge/ap/config/bk7259_ap/defconfig`` - ``CONFIG_BRIDGE=y``
- CP: ``projects/wifi/bridge/cp/config/bk7259/defconfig`` - ``CONFIG_BRIDGE=y`` (must match AP)

Build the Project
****************************

::

    make bk7259 PROJECT=wifi/bridge

Flash
****************************

Flash AP/CP images per SDK flash tool documentation.

Running and Output
------------------------------

Operate
*****************************

1. Connect serial CLI to AP core.
2. Start bridge (STA joins upstream AP, softAP SSID defaults to ``<sta_ssid>_brr``)::

       bridge open <upstream_ssid> <password>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid>
       bridge open <upstream_ssid> <password> <bridge_softap_ssid> 1

3. Check status::

       state

4. Stop bridge::

       bridge close

Output
*****************************

- ``state`` shows bridge up/down and softAP SSID/channel when enabled.
- Logs from ``bk_bridge_fsm`` / bring-up thread on success or rollback.
