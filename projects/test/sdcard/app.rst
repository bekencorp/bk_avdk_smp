.. _project_sdcard:

sdcard
=============================

Overview
-----------------------------
BK7259 SD card test project. It provides one ``sdcard`` CLI command covering
FATFS mount/unmount, read/write integrity tests, throughput benchmarks,
random I/O, soak, and a background PSRAM/CPU/bus load generator for testing
SDIO under load.

See ``README.md`` (English) and ``README_CN.md`` (Chinese) for hardware
requirements, build instructions, command details and test flows.

Hardware Requirements
------------------------------
- BK7259 board with SDIO0 one-line (GPIO2/GPIO3/GPIO4) or SDIO1 four-line
  (GPIO14~GPIO19).
- A FAT-formatted SD card.
- Serial console at 115200 baud.

Configure and Build
-----------------------------
.. code-block:: sh

   make clean PROJECT=test/sdcard
   make bk7259 PROJECT=test/sdcard

Flash
-----------------------------
Flash ``build/bk7259/sdcard/package/all-app.bin`` with the Beken flashing
tool.

Running and Output
-----------------------------
Run ``ap_cmd sdcard`` on the AP CLI to print the command list. All results are
printed with the ``[SDTEST]`` prefix.
