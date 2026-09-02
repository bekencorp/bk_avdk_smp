AP Fast Boot PM Example
=======================

Overview
--------

This project derives from ``projects/pm/pm`` and enables AP FreeRTOS
execution-point retention by default. The public AP vote API remains unchanged.
Projects without ``CONFIG_PM_AP_FAST_BOOT_ENABLE`` keep the original
cold-boot behavior. Cold boot starts CPU2 and CPU3 together
(``CONFIG_CPU_HOTPLUG_BOOT_OFFLINE`` stays disabled). Before AP OFF, the PM
thread hotplugs CPU3 offline; after fast resume, AP0 brings CPU3 back online.
The retained execution context belongs only to CPU2.

Build
-----

From the SDK root::

    make bk7259 PROJECT=pm/ap_fast_boot

Test
----

Use the CP UART0 console::

    ap_fast_boot off
    ap_fast_boot on

or perform one OFF/ON cycle with a one-second off interval::

    ap_fast_boot cycle

The AP prints ``RESUME_PROOF`` every two seconds. After a successful cycle,
``seq`` continues increasing, ``main_entries`` remains 1, and the stack/heap
canaries retain their values. AP SRAM remains powered during the AP power
cycle. AP DTCM is backed up into a 64-KiB ``.noinit`` buffer in retained AP
SRAM before WFI and restored before interrupts are re-enabled. PSRAM
application data is preserved in place by PSRAM data retention; no AP memory
is staged through PSRAM.

If AP does not publish a saved CPU context, or PSRAM retention fails, the
fast-resume flag is cleared and the original AP cold-boot path is used.
