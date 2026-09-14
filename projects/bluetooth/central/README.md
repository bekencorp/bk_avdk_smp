# Bluetooth A2DP Source Example

* [中文](./README_CN.md)

## Overview

This project turns a BK7258 board into a Bluetooth Classic A2DP Source. It reads an MP3 file from an SD card, decodes it to PCM, encodes it as SBC, and sends it to a Bluetooth speaker or headset (A2DP Sink).

The `a2dp_player` serial CLI supports discovery, connection management, playback control, and AVRCP absolute-volume control.

> This is a Bluetooth Classic (BR/EDR) example, not a BLE example. The board initiates the connection to the speaker/headset.

## Features

- Discover, connect to, and disconnect from Bluetooth speakers/headsets
- Play an MP3 file from an SD card (`1:/` FATFS drive)
- Playback control: play, pause, resume, stop, previous, and next
- AVRCP playback-status reporting and peer absolute-volume control
- A2DP performance test command

## Hardware and test environment

- BK7258 reference board
- Bluetooth speaker or headset supporting A2DP Sink; AVRCP is required for remote-control and absolute-volume tests
- SD card formatted for FATFS and containing MP3 files
- Serial terminal for boot logs and CLI commands

## Directory structure

```text
central/
├── ap/
│   ├── ap_main.c                         # AP entry; initializes SDK/media service and registers CLI
│   ├── a2dp_source/
│   │   ├── a2dp_source_demo.c            # A2DP Source connection and MP3 playback logic
│   │   ├── a2dp_source_demo_avrcp.c      # AVRCP control/status reporting
│   │   └── a2dp_source_demo_cli.c        # `a2dp_player` CLI
│   ├── storage/                          # Bluetooth storage helpers
│   └── config/bk7258_ap/config           # AP configuration
├── cp/
│   ├── cp_main.c                         # CP entry; starts the AP system
│   └── config/bk7258/config              # CP configuration
└── partitions/bk7258/                    # BK7258 partition and RAM-region definitions
```

The AP side owns the media and profile demo logic. The CP side starts the AP and provides the Bluetooth controller configuration.

## Build and flash

Run the command from the SDK root:

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/central -j$(nproc)
```

Flash the generated firmware with the normal BK7258 flashing procedure, then insert the SD card.

## Quick start

1. Open a serial terminal after boot.
2. Discover nearby Bluetooth devices:

   ```text
   a2dp_player discover
   ```

3. Connect to the speaker/headset found in the log:

   ```text
   a2dp_player connect XX:XX:XX:XX:XX:XX
   ```

4. Play an MP3 file from the SD card:

   ```text
   a2dp_player play 1:/music.mp3
   ```

5. Stop playback and disconnect when finished:

   ```text
   a2dp_player stop
   a2dp_player disconnect XX:XX:XX:XX:XX:XX
   ```

## CLI commands

The following are AP CLI commands. When entering them through the default CP serial console, prepend `ap_cmd`; for example, enter `ap_cmd a2dp_player discover`. When using an AP CLI directly, enter the commands exactly as listed below. Use `a2dp_player -h` to print command help.

| Command | Description |
| --- | --- |
| `a2dp_player discover [sec] [count]` | Scan for nearby Bluetooth devices; default duration is 10 seconds and `count=0` means no report limit. |
| `a2dp_player discover_cancel` | Stop an ongoing discovery. |
| `a2dp_player connect <MAC>` | Connect to an A2DP Sink device. |
| `a2dp_player disconnect <MAC>` | Disconnect a remote device. |
| `a2dp_player play 1:/music.mp3` | Decode and stream an MP3 file from the SD card. |
| `a2dp_player pause` / `resume` / `stop` | Control the current playback. |
| `a2dp_player prev` / `next` | Request previous/next playback; this demo does not provide a playlist manager. |
| `a2dp_player abs_vol <0-127>` | Set the peer AVRCP absolute volume. |
| `a2dp_player test_performance [cpu_mhz] [bytes] [loops] [cpu_id]` | Run the A2DP Source performance test. |

`<MAC>` must be written as `XX:XX:XX:XX:XX:XX`.

## Verification

`CMDRSP:OK` means only that the CLI accepted the command. Confirm successful operation from the Bluetooth profile and media logs:

- after `connect`, A2DP/AVRCP connection events should be reported;
- after `play`, the remote speaker/headset should play audio and MP3 decode/SBC send logs should appear;
- after `abs_vol`, the remote volume should change if it supports AVRCP absolute volume.

## Notes

- The MP3 path must use the SD-card FATFS drive, for example `1:/music.mp3`.
- The A2DP Source sources are included whenever `CONFIG_BT` is enabled. `ap_main.c` registers `a2dp_player`; the A2DP Source service is initialized on the first `connect` command, not at boot. The project Kconfig still labels its menu as “Headset Example Configuration” and contains the unrelated `A2DP_SINK_DEMO` symbol; that symbol does not start this Source demo.
- `prev` and `next` exercise playback-control handling only; they do not select files from a playlist.
