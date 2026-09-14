# Bluetooth Headset Example

* [中文](./README_CN.md)

## Overview

This project turns a BK7258 board into a Bluetooth Classic speaker and hands-free device. A phone can stream music to the board through A2DP, while the board can control the phone player through AVRCP and handle calls as an HFP Hands-Free (HF) device.

The default project configuration enables A2DP Sink, HFP HF, and PBAP Phone Book Client Equipment (PCE) demos.

## Features

- A2DP Sink: receive and play music sent by a phone or other A2DP Source
- AVRCP Controller: control remote playback, tracks, seeking, volume, delay value, and media-attribute query
- HFP HF: voice-recognition control, dial, answer/reject calls, SCO call audio, and custom AT commands
- PBAP PCE: phone-book access demo; see `ap/pbap_pce/README.txt` and `README_CN.txt`
- Pairing, active connect/disconnect, and serial CLI control

> This is a Bluetooth Classic (BR/EDR) project, not a BLE application.

## Hardware and test environment

- BK7258 reference board
- Speaker for A2DP playback and microphone for HFP calls
- Phone supporting Bluetooth Classic A2DP, AVRCP, and HFP
- Serial terminal for logs and commands

## Directory structure

```text
headset/
├── ap/
│   ├── ap_main.c                         # AP entry and demo initialization
│   ├── bt_manager.c                      # Device name, pairing, reconnect policy
│   ├── headset_user_config.h             # Local name and audio-related defaults
│   ├── a2dp_sink/                        # A2DP Sink audio/decode handling
│   ├── a2dp_sink_demo_cli.c              # `headset` CLI
│   ├── hfp_hf/                           # HFP HF and SCO voice handling
│   ├── pbap_pce/                         # PBAP PCE demo and its documentation
│   └── config/bk7258_ap/config           # AP configuration
├── cp/                                   # CP startup and controller configuration
└── partitions/bk7258/                    # BK7258 partition definitions
```

At boot, the AP initializes the SDK, media service, Bluetooth manager, A2DP Sink, HFP HF, PBAP PCE, and the corresponding CLI commands. A2DP AAC and HFP mSBC are initialized with `0`, so SBC and CVSD are the default verification codecs.

## Build and flash

From the SDK root:

```bash
CCACHE_DISABLE=1 make bk7258 PROJECT=bluetooth/headset -j$(nproc)
```

Flash the firmware using the normal BK7258 procedure, connect the audio peripherals, and open the serial terminal.

## Quick start

1. Make the board discoverable:

   ```text
   headset pair_mode
   ```

2. On the phone, find and connect `soundbar_XXYYZZ`. The suffix is derived from the local Bluetooth MAC address.
3. Start music playback on the phone. Audio should play through the board speaker.
4. Control playback from the serial terminal if needed:

   ```text
   headset pause
   headset play
   headset next
   ```

5. For an incoming call, answer or reject it:

   ```text
   headset answer 1
   headset answer 0
   ```

## CLI commands

The following are AP CLI commands. When entering them through the default CP serial console, prepend `ap_cmd`; for example, enter `ap_cmd headset pair_mode`. When using an AP CLI directly, enter the commands exactly as listed below. The table is authoritative: the built-in `headset -h` output lists only the basic media commands and does not include every supported sub-command.

| Command | Description |
| --- | --- |
| `headset pair_mode` | Enter pairable/discoverable mode. |
| `headset connect <MAC>` / `disconnect <MAC>` | Actively connect or disconnect a remote device. |
| `headset play` / `pause` / `prev` / `next` | Send AVRCP playback controls. |
| `headset rewind [ms]` / `fast_forward [ms]` | Seek; default duration is 500 ms. |
| `headset vol_up` / `vol_down` | Send AVRCP volume controls. |
| `headset set_delay_value <value>` / `get_delay_value` | Set or read the 16-bit A2DP delay value. |
| `headset get_attr <id>` | Query an AVRCP media attribute. |
| `headset vr <0\|1>` | Disable or enable HFP voice recognition. |
| `headset dial <0\|1> [number]` | Control dialing; the default number is `112`. |
| `headset answer <0\|1>` | Answer (`1`) or reject/hang up (`0`) a call. |
| `headset hfpcmd <AT command>` | Send a custom HFP AT command. |

`<MAC>` must use `XX:XX:XX:XX:XX:XX` format.

## Verification

`CMDRSP:OK` means a command was accepted; use profile and audio logs to verify completion:

- the phone discovers and connects to `soundbar_XXYYZZ`;
- phone music plays through the local speaker after A2DP connects;
- call actions produce the expected HFP events, and microphone/speaker audio works over the SCO link;
- PBAP PCE behavior is verified according to its module README.

## Notes

- `LOCAL_NAME` in `ap/headset_user_config.h` controls the `soundbar` name prefix.
- The default configuration is in `ap/config/bk7258_ap/config`; demo switches are in `ap/Kconfig.projbuild`.
- Audio board wiring, channels, and gain must match the reference-board configuration.
