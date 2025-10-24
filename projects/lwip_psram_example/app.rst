.. _project_lwip_psram_example:

LWIP PSRAM Example
=============================

Overview
-----------------------------

本示例演示如何使用 PSRAM 作为 LWIP 协议栈的内存缓冲区，以扩大网络数据的存储空间。

Hardware Requirements
------------------------------

Configure and Build
-----------------------------

Configure the Project
****************************

如果要自己创建工程使用此功能时，需要额外开启如下宏配置：

AP侧需要开启如下宏配置
   CONFIG_LWIP_MEM_LIBC_MALLOC_USE_PSRAM=y
   CONFIG_CONTROLLER_AP_BUFFER_COPY=y

CP侧需要开启如下配置
    CONFIG_CONTROLLER_AP_BUFFER_COPY=y


Build the Project
****************************

构建命令：

   make bk7258 PROJECT=lwip_psram_example

Flash
****************************

Running and Output
------------------------------

Operate
*****************************

Output
*****************************

.. important::

**CP 侧内存开销**

- 相比 APP 工程，当前 AP 侧数据量比较大，所 CP 侧内存也需要适量增大；
- 如果 CP 侧内存较小，可能会导致网络吞吐率降低,根据可以需求合理调整。
