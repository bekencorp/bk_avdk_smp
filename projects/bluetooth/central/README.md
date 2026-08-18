# Bluetooth A2DP Source (Music Player) Example

* [中文](./README_CN.md)

## Introduction

This project implements the BK7259 board as a Bluetooth A2DP audio source (Source). Acting as an A2DP Source (the sender side), the board actively connects to a Bluetooth speaker/headset and provides the following capabilities:

- decode MP3 files from an SD card and push them to the speaker/headset over Bluetooth for playback;
- control playback (play / pause / stop / track switching) over the serial console;
- act as an AVRCP player (Target), responding to remote transport-control keys and reporting the current track, play position and playback status;
- set the peer (speaker) absolute volume via AVRCP;
- act as an **HFP AG (Audio Gateway, the "phone" side)** toward a Bluetooth headset (HF), establishing a Service Level Connection (SLC): simulate incoming/outgoing calls, two-way voice over SCO, negotiate the CVSD/mSBC voice codec, and handle the volume reported by the headset. HFP AG is gated by the Kconfig `BLUETOOTH_BTDM_COMPONENT_HFP_AG` (enabled by default in this project); HFP is set up automatically once a headset reconnects and finishes authentication.

The abbreviations below appear throughout this document:

| Abbrev. | Full name | Role in this project |
| --- | --- | --- |
| A2DP Source | Advanced Audio Distribution Profile (sender) | Encode local audio and push it to a Bluetooth speaker/headset |
| AVRCP TG | Audio/Video Remote Control Profile (Target / player side) | Respond to remote transport control and report playback status/track/position |
| AVRCP CT | Audio/Video Remote Control Profile (Controller) | Set the peer (speaker) absolute volume, receive peer volume changes |
| HFP AG | Hands-Free Profile (Audio Gateway / the "phone" side) | Establish an SLC with a Bluetooth headset (HF): incoming/outgoing calls, two-way SCO voice, CVSD/mSBC negotiation, volume handling |

> Note: this project demonstrates **Classic Bluetooth (BR/EDR)**, not BLE. The board is the **initiator**, connecting to A2DP Sink devices such as speakers/headsets.

Data flow (**one-way downlink**: board → speaker):

```text
MP3 file on SD card
    → MP3 decode (helix)                 # produce PCM
    → resample (if src rate != negotiated rate)  # e.g. 48k → 44.1k
    → SBC encode                         # produce the A2DP bitstream
    → A2DP Source send                   # over the Bluetooth link
    → Bluetooth speaker / headset (A2DP Sink) decodes and plays
```

## Quick start (5 steps)

> Prerequisites: a working BK7259 build/flash environment, a Bluetooth speaker/headset, and an SD card with MP3 files.

1. **Build the firmware**

   ```bash
   make bk7259 PROJECT=bluetooth/central
   ```

2. **Flash the firmware** to the board and insert the **SD card** that holds the MP3 files.

3. **Open a serial terminal** (to view boot logs and send commands). After boot you should see the Bluetooth and media services finish initializing.

4. **Connect** a Bluetooth speaker/headset (using the target device's MAC address):

   ```bash
   ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX
   ```

   > If the speaker's MAC is unknown, run `ap_cmd a2dp_player discover` first (10s by default, results printed on serial), then connect using the address found.

5. **Play music**: push an MP3 from the SD card to the speaker:

   ```bash
   ap_cmd a2dp_player play 1:/music.mp3   # 1:/ is the SD card root
   ap_cmd a2dp_player pause               # pause
   ap_cmd a2dp_player resume              # resume
   ```

The board now functions as a Bluetooth music player. See below for the full command set and internals.

## Typical flow

Using "connect a speaker → play music → remote control" as an example (commands are detailed in [4.2.2 Serial CLI commands](#422-serial-cli-commands)):

```text
# 1. (optional) Scan for nearby devices to get the speaker MAC (10s by default, results printed on serial)
ap_cmd a2dp_player discover
ap_cmd a2dp_player discover_cancel   # stop the scan early if needed

# 2. Connect the speaker (or let the speaker connect the board; the project does eager init)
ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX

# 3. Play an MP3 from the SD card
ap_cmd a2dp_player play 1:/music.mp3
ap_cmd a2dp_player pause
ap_cmd a2dp_player resume
ap_cmd a2dp_player next          # no real track management yet: replays the same file

# 4. Set the speaker volume (AVRCP absolute volume, 0~0x7f)
ap_cmd a2dp_player abs_vol 60

# 5. Disconnect when done
ap_cmd a2dp_player disconnect XX:XX:XX:XX:XX:XX
```

> Tip: `CMDRSP:OK` only means the command was accepted; whether the link is actually up and audio is playing depends on the profile-state and audio logs (see [4.2.3](#423-how-to-tell-success-from-failure)).

## 1. Overview

This project demonstrates Classic Bluetooth **A2DP audio source** capabilities on the Beken platform, mainly:

- A2DP Source: read an MP3 from the SD card, decode → resample → SBC encode, and push it over Bluetooth to a speaker/headset
- AVRCP Target (player): respond to remote play/pause/track passthrough keys, and report playback status, track change and play position
- AVRCP Controller: set the peer absolute volume, receive peer volume/battery changes

After boot the project initializes the media service and Bluetooth manager, **eagerly** initializes A2DP Source and AVRCP (so a speaker-initiated connection is also accepted), and registers the `a2dp_player` serial CLI for manual discovery, connection and playback control.

### 1.1 Test environment

- Hardware
  - Target chip: BK7259
  - Storage: SD card (FATFS) holding the MP3 files to play
- External device
  - A Bluetooth speaker or headset supporting A2DP/AVRCP (as the A2DP Sink)
- Output
  - Bluetooth connection and profile-state logs
  - MP3 decode / resample / encode / send audio-pipeline logs
  - AVRCP control and reporting logs

> **Note**: use the reference board for learning and validation. MP3 files must reside on the SD card, with paths starting with `1:/` (FATFS drive letter).

## 2. Directory layout

The project uses an AP-CP dual-core structure. The AP side runs the business logic (media service, Bluetooth manager, A2DP Source / AVRCP demo, CLI); the CP side only brings up the AP:

```text
central/
├── ap/
│   ├── ap_main.c                       # AP entry: bk_init → media_service_init → bt_manager_init → a2dp source demo → hfp ag demo → CLI
│   ├── a2dp_source/
│   │   ├── a2dp_source_demo.c          # A2DP Source connection mgmt + music playback (MP3 decode task, feed PCM to the component)
│   │   ├── a2dp_source_demo_cli.c      # `a2dp_player` CLI commands
│   │   ├── a2dp_source_demo_avrcp.c    # AVRCP policy: passthrough handling, playback/track/position reporting, volume
│   │   ├── a2dp_source_demo.h
│   │   └── a2dp_source_demo_avrcp.h
│   ├── hfp_ag/                          # gated by Kconfig BLUETOOTH_BTDM_COMPONENT_HFP_AG (default on)
│   │   ├── hfp_ag_demo.c               # HFP AG policy: call state machine, AT answers (+CIND/+COPS/+CLCC…), auto-connect after auth
│   │   ├── hfp_ag_demo_cli.c           # `hfp_ag` CLI commands
│   │   └── hfp_ag_demo.h
│   └── config/bk7259_ap/defconfig      # AP-side Kconfig overrides (audio / ADK / FATFS / BT)
└── cp/
    └── config/bk7259/defconfig         # CP-side Kconfig overrides (BT controller, etc.)
```

The actual A2DP Source TX/encode pipeline and AVRCP logic live in **reusable components** (`ap/components/bk_bluetooth/service/dm/`); the demo only does "policy + file read + decode":

| Component | Role |
| --- | --- |
| `service/dm/a2dp/bk_a2dp_source_service` | A2DP Source connection state machine + TX pipeline (ring buffer, encode callback, AVDTP start/suspend) |
| `service/dm/a2dp/bk_a2dp_source_pcm_service` | Standalone worker: resample + SBC encode |
| `service/dm/avrcp/bk_avrcp_tg_service` | AVRCP Target (player): passthrough, playback/track/position notifications |
| `service/dm/avrcp/bk_avrcp_ct_service` | AVRCP Controller: absolute volume, peer volume/battery |
| `service/dm/hfp/bk_hfp_ag_service` (+ `hfp_ag_audio`) | HFP AG: AG lifecycle (init/features/bt_manager registration), SLC/call/codec event plumbing to the app, and the SCO voice engine (audio_play/record, CVSD/mSBC) |
| `service/dm/bt_manager` | Single GAP callback: name/COD/discoverability/pairing/link-key storage/role switch |

## Code walkthrough (for modifying / extending)

The demo entry is `ap/ap_main.c`, initializing in a fixed order at boot:

```c
bk_init();                 // SDK base init
media_service_init();      // media service (audio playback framework)

bt_manager_init(&cfg);     // Classic BT manager: name a2dp_source_XXYYZZ / COD_PHONE / role=master
bt_a2dp_source_demo_init();// eager init of A2DP Source + AVRCP (accepts speaker-initiated connections)
cli_a2dp_source_demo_init();// register the a2dp_player serial commands
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_HFP_AG
hfp_ag_demo_init();        // eager init of HFP AG (accepts headset-initiated connections; auto-SLC after auth)
cli_hfp_ag_demo_init();    // register the hfp_ag serial commands
#endif
```

Module responsibilities and key functions:

| Module / file | Key functions | Notes |
| --- | --- | --- |
| `ap/ap_main.c` | `main` | Boot entry; builds the name from `a2dp_source_` + last 3 MAC bytes; `bt_manager_cfg_t.role=1` makes the source switch to master once linked |
| Bluetooth manager (`bt_manager`) | `bt_manager_init(&cfg)` | Name, COD, page/scan discoverability, pairing IO cap, link-key storage, role switching |
| A2DP Source (`a2dp_source/a2dp_source_demo.c`) | `bt_a2dp_source_demo_init` / `bt_a2dp_source_demo_music_play` | Connect/disconnect, start the MP3 decode task, start the AVDTP stream, feed PCM to the component |
| AVRCP (`a2dp_source/a2dp_source_demo_avrcp.c`) | `bt_avrcp_demo_init` / `bt_avrcp_demo_report_playback` / `..._report_track_change` | Handle passthrough keys from the speaker; report playback status, track and position |
| HFP AG (`hfp_ag/hfp_ag_demo.c`) | `hfp_ag_demo_init` / `hfp_ag_demo_cb` / `hfp_ag_demo_gap_cb` | Call state machine, AT answers (+CIND/+COPS/+CLCC…); auto-establishes the SLC after authentication; SCO voice is carried by the `bk_hfp_ag_service` component |
| CLI (`a2dp_source_demo_cli.c` / `hfp_ag/hfp_ag_demo_cli.c`) | `cli_a2dp_source_demo_init` / `cmd_a2dp_player_demo` / `cli_hfp_ag_demo_init` | Parse `a2dp_player` / `hfp_ag` subcommands and call the interfaces above |

Common change points:

- **Change name / discoverability / role**: `bt_manager_cfg_t` in `ap_main.c`.
- **Add a custom command**: add a branch in `cmd_a2dp_player_demo` in `a2dp_source_demo_cli.c`.

## 3. Features

### 3.1 Currently supported

- Auto-init of Classic Bluetooth and media service at boot, with eager A2DP Source / AVRCP bring-up
- Discover, connect, disconnect a speaker/headset (board-initiated or speaker-initiated)
- Play MP3 from the SD card: decode → resample (as needed) → SBC encode → push and play
- Playback control: play / pause / resume / stop / prev / next
- AVRCP player: respond to remote passthrough keys, report playback status / track change / play position
- AVRCP absolute volume control
- HFP AG (Audio Gateway): establish an SLC with a Bluetooth headset; simulate incoming/outgoing calls, two-way SCO voice; CVSD/mSBC codec negotiation; handle headset volume reports (Kconfig switch, default on)
- Serial CLI for manual control

## 4. Build and run

### 4.1 Build

```bash
make bk7259 PROJECT=bluetooth/central
```

### 4.2 Run

After flashing and inserting an SD card with MP3 files, watch the boot logs on the serial terminal and use the `a2dp_player` CLI to control connection and playback.

#### 4.2.1 Default configuration

The AP-side defaults are in `ap/config/bk7259_ap/defconfig`; key switches:

```text
CONFIG_BT=y
CONFIG_BLUETOOTH_AP=y
CONFIG_MEDIA_SERVICE=y
CONFIG_AUDIO=y
CONFIG_AUDIO_PLAY=y
CONFIG_FATFS=y
CONFIG_FATFS_SDCARD=y
CONFIG_SDCARD=y
CONFIG_ADK_SBC_ENCODER=y      # SBC encoder (used by A2DP Source)
CONFIG_BLUETOOTH_BTDM_COMPONENT_ENABLE=y
CONFIG_BLUETOOTH_BTDM_COMPONENT_HFP_AG=y   # HFP AG (Audio Gateway) component
```

#### 4.2.2 Serial CLI commands

The CLI runs on the AP side, so commands are forwarded via `ap_cmd`. `XX:XX:XX:XX:XX:XX` is the MAC of the speaker/headset.

> For argument formats, run `ap_cmd a2dp_player -h`.

**Connection management**

| Command | Description |
| --- | --- |
| `ap_cmd a2dp_player discover [sec] [report count]` | Scan for nearby Bluetooth devices; `sec` is the scan duration (seconds, default 10), `report count` caps the number of reported devices (default 0 = unlimited). Results are printed on the serial console |
| `ap_cmd a2dp_player discover_cancel` | Cancel an ongoing scan |
| `ap_cmd a2dp_player connect XX:XX:XX:XX:XX:XX` | Connect the speaker/headset at the given MAC, establishing A2DP + AVRCP |
| `ap_cmd a2dp_player disconnect XX:XX:XX:XX:XX:XX` | Disconnect the given MAC |

**Playback control**

| Command | Description |
| --- | --- |
| `ap_cmd a2dp_player play 1:/music.mp3` | Play an MP3 from the SD card (`1:/` is the SD drive) and report "playing" |
| `ap_cmd a2dp_player pause` | Pause playback and report "paused" |
| `ap_cmd a2dp_player resume` | Resume playback and report "playing" |
| `ap_cmd a2dp_player stop` | Stop playback and report "stopped" |
| `ap_cmd a2dp_player prev` | Previous track (no real track management yet; replays the current file) |
| `ap_cmd a2dp_player next` | Next track (same as above) |

**Volume control (AVRCP)**

| Command | Description |
| --- | --- |
| `ap_cmd a2dp_player abs_vol 60` | Set the peer (speaker) absolute volume, range 0~0x7f (0~127) |

**HFP AG commands (Audio Gateway, interacting with a Bluetooth headset / HF)**

> HFP AG needs no manual connect: the SLC is established automatically once a headset reconnects and finishes authentication; if the peer has no HFP HF, a connection failure is reported.

| Command | Description |
| --- | --- |
| `ap_cmd hfp_ag incoming [number]` | Simulate an incoming call (optional caller number, default 10010); rings the headset |
| `ap_cmd hfp_ag answer` | Answer the current incoming call |
| `ap_cmd hfp_ag hangup` | Hang up the current call |
| `ap_cmd hfp_ag dial <number>` | Simulate an outgoing call (dial) |
| `ap_cmd hfp_ag audio on\|off` | Turn SCO voice on/off (AG mic ↔ HF two-way intercom) |
| `ap_cmd hfp_ag codec cvsd\|msbc` | Choose the SCO voice codec (CVSD 8k / mSBC 16k) |
| `ap_cmd hfp_ag battery <0-5>` | Report the AG battery level to the headset |
| `ap_cmd hfp_ag vgs <0-15>` / `ap_cmd hfp_ag vgm <0-15>` | Set the headset speaker / mic volume |
| `ap_cmd hfp_ag cmd <at-result-code>` | Send a custom AT result code |

Commands return `CMDRSP:OK` on accept, `CMDRSP:ERROR` on failure.

#### 4.2.3 How to tell success from failure

`CMDRSP:OK` only means the CLI command was accepted; it does **not** mean the Bluetooth operation completed. Judge from the profile-state and audio-pipeline logs:

- after `connect`, you should see A2DP / AVRCP connected and the negotiated codec (SBC) and sample rate;
- after `play`, the speaker should actually make sound, with MP3 decode / encode / send logs on the serial;
- after `abs_vol`, the speaker volume should change (if it supports absolute volume).

#### 4.2.4 Integration test

A2DP Source operation requires an external Bluetooth speaker and MP3 files on the SD card, and is usually validated manually or with dedicated automation.

## 5. Notes & FAQ

1. **A speaker/headset is required**: as an audio source, this project needs an A2DP Sink peer.
2. **An SD card with MP3 files is required**: `play` paths start with `1:/` (FATFS SD drive); make sure the card is inserted and the file exists.
3. **Fixed MAC format**: MACs in the CLI must be `XX:XX:XX:XX:XX:XX`, otherwise parsing fails.
4. **Device name**: defaults to the `a2dp_source` prefix, broadcasting as `a2dp_source_XXYYZZ` (suffix = last 3 bytes of the local BT MAC); change it in `ap_main.c`.
5. **prev / next have no real track management yet**: they currently replay the same file, only to exercise the AVRCP reporting path.

**FAQ**

- **Speaker not found**: put the speaker in pairing/discoverable mode, then run `ap_cmd a2dp_player discover`; confirm the serial log shows Bluetooth initialized.
- **Connected but no sound**: verify the `play` MP3 path (`1:/...`) and that the SD card is inserted; check the decode / encode / send logs; raise the volume with `abs_vol` if needed.
- **`CMDRSP:ERROR`**: usually a malformed argument (especially the MAC or file path); verify with `ap_cmd a2dp_player -h`.
- **Stutter / noise during playback**: prefer source files at a common sample rate (e.g. 44.1kHz); 48kHz triggers resampling — if quality is off, check the resample path and whether PCM is fed fast enough.
