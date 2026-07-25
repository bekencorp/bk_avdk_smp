Wi-Fi 项目示例
================

:link_to_translation:`en:[English]`

BK7259 SMP SDK 在 ``projects/wifi/`` 下提供 Wi-Fi 参考工程，应用逻辑均在 **AP 核** 编写；CP 核为 Wi-Fi 控制器。

.. toctree::
   :maxdepth: 1

   sta_example/index
   scan_example/index
   softap_example/index
   iperf/index
   p2p/index
   bridge/index
   ipv6/index
   rlk_demo/index

通用编译
--------

在 SDK 根目录::

   make bk7259 PROJECT=wifi/<工程名>

固件输出：``build/bk7259/wifi/<工程名>/``

相关文档
--------

- :doc:`Wi-Fi 快速入门 <../../../developer-guide/wifi/bk_wifi_get_started>`
- :doc:`Wi-Fi 开发者指南 <../../../developer-guide/wifi/index>`
- :doc:`Wi-Fi API <../../../api-reference/wifi/bk_wifi>`
