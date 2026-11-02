Secure Boot AP Fast Boot
========================

Overview
--------

This project derives from ``projects/secureboot_overwrite_no_en`` and enables
AP FreeRTOS execution-point retention on the secure-boot overwrite image
(Flash AES off). The secure boot chain is unchanged:

BootROM -> CP BL2 -> CP TF-M Secure -> CP non-secure application.

The CP non-secure application votes the AP on through
``bk_pm_module_vote_boot_ap_ctrl()``. TF-M only performs the secure AP
prepare. Cold boot starts CPU2 and CPU3 together. Before AP OFF, the PM
thread hotplugs CPU3 offline; after fast resume, AP0 brings CPU3 back online.
The retained execution context belongs only to CPU2.

Build
-----

From the SDK root::

    make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot

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
application data is preserved in place by PSRAM data retention.

Flash signing, the single execution slot, and overwrite OTA stay as documented
in ``README.md``. Do not replace this project's partition table with the
non-secure ``pm/ap_fast_boot`` layout.
