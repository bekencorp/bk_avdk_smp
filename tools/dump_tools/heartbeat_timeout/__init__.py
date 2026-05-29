"""Heartbeat-timeout dump analyzer.

This subpackage of `dump_tools` consolidates the offline analyzer used for
CP-side heartbeat timeout dumps of BK7259 AP. Inputs are a serial log
captured during the CP `mb_ipc_task:292` heartbeat-timeout assert plus the
matching AP `app.elf`. Outputs are either separate text files (extract,
msp_core0, msp_core1, peri) suitable for further AI inspection or a single
auto-generated Markdown review report.

Public entry points
-------------------
``analyze``                Top-level driver (see :mod:`analyze`)
``core``                   Dump parser (base64 -> bytes -> memmap)
``symbols``                Auto-discover FreeRTOS / recorder symbol addresses
``extract``                FreeRTOS globals + task/IRQ recorder dump
``msp_walk``               Per-core MSP stack scan
``peri_regs``              Peripheral register snapshot decoder
``report``                 Smoking-gun analyzer + markdown renderer
"""
