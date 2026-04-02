.. _project_eth_example:

ETH Example
=============================

Overview
-----------------------------

本示例演示如何使用 ETH 作为网络接口，实现网络通信。

Hardware Requirements
------------------------------

Configure and Build
-----------------------------

Configure the Project
****************************

如果要自己创建工程使用此功能时，需要开启如下宏配置：

CP侧需要开启如下宏配置
   CONFIG_ETH=y（启用 ETH 模块）
   CONFIG_ETH_DHCP=y（启用 DHCP 获取 IP 地址，不启用则需要手动配置 IP 地址）
   CONFIG_ETH_PIN_GROUP0=y （ETH PIN GROUP0 对应 GPIO27,29-39）
   CONFIG_ETH_CSUM_OFFLOAD=y（启用 IP/TCP/UDP/ICMP 校验和卸载）
   CONFIG_PHY_SMSC=y（使用 SMSC PHY）
   CONFIG_ETH_PM_CB_SUPPORT=y（ETH模块上电支持以及启用 PM 模块进入/退出低电压回调支持）

Build the Project
****************************

构建命令：

   make bk7258 PROJECT=eth_example

Flash
****************************

Running and Output
------------------------------

Operate
*****************************

输出信息：

.. code-block:: text

   CP检测到PHY芯片正常后，会打印如下信息：
   - netif st connected to SMSC LAN8710/LAN8720, mode rmii, phyad 1
   CP启动ETH模块成功，会打印如下信息：
   - ETH link up, speed 100M, Full-duplex
   - lwip:D(1740):eth ip start
   - lwip:D(1740):configuring iface eth (with DHCP client)
   插上网线后，每隔5秒输出一次：
   - ETH:ip=192.168.1.100, mask=255.255.255.0, gw=192.168.1.1, dns=192.168.1.1
   断开网线后，会打印如下信息：
   - lwip:D(872619):ETH link down
   - lwip:D(872619):eth ip down
   - ETH: ip=0.0.0.0, mask=0.0.0.0, gw=0.0.0.0, dns=0.0.0.0

Output
*****************************

.. important::

**内存开销**

   - 本示例内存占用主要由 ETH 模块占用，根据实际需求合理调整。
   - 如果内存较小，可能会导致网络吞吐率降低,根据可以需求合理调整。
   - AP默认不支持ETH模块，没有代码支持AP侧的ETH模块。

