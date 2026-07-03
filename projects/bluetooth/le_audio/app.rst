.. _project_le_audio:

LE Audio / Auracast Demo
========================

Overview
--------

This project demonstrates LE Audio Broadcast and Unicast basics on BK7259
AP/CP. The default application entry is the ``le_audio`` protocol demo enabled
by ``CONFIG_LE_AUDIO_PROTOCOL_DEMO``. The BTA-style ``auracast`` CLI is kept
as a reference application-layer implementation.

The demo can be used to test BAP Broadcast Source/Sink over BIS, BAP Unicast
Source/Sink over CIS, PACS capability discovery, ASCS ASE discovery and
configuration, LC3 encode/decode, and sink speaker playback.

Configure and Build
-------------------

Build the project with:

.. code-block:: bash

    make bk7259 PROJECT=bluetooth/le_audio

Running
-------

Use AP-side CLI through ``ap_cmd``.

Protocol Broadcast:

.. code-block:: text

    ap_cmd le_audio role source
    ap_cmd le_audio broadcast source start
    ap_cmd le_audio role sink
    ap_cmd le_audio broadcast sink scan
    ap_cmd le_audio broadcast sink sync <broadcast_id>

Protocol Unicast:

.. code-block:: text

    ap_cmd le_audio role sink
    ap_cmd le_audio adv on
    ap_cmd le_audio role source
    ap_cmd le_audio connect <sink_addr> 0 ext
    ap_cmd le_audio unicast source setup
    ap_cmd le_audio unicast source caps sink
    ap_cmd le_audio unicast source discover

Auracast reference CLI:

.. code-block:: text

    ap_cmd auracast client scan_start
    ap_cmd auracast client associate
    ap_cmd auracast client enable
    ap_cmd auracast server announcement
    ap_cmd auracast server start

In the protocol demo, files are grouped by device role first:
``source/broadcast``, ``source/unicast``, ``sink/broadcast`` and
``sink/unicast``. Both sink scenarios feed the shared ``sink/common`` media
path for ISO reception, LC3 decode, and onboard speaker playback.

Broadcast sync flow:

.. code-block:: text

    Extended Advertising -> Periodic Advertising(BASE) -> PA sync
    -> BIGInfo -> BIG create sync -> BIS ISO -> LC3 decode -> playback
