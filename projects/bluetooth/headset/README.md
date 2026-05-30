# Bluetooth Headset Example Project

* [中文](./README_CN.md)

## 1. Project Overview

This project demonstrates a classic Bluetooth headset/speaker style application on the Beken platform. It mainly includes:

- A2DP Sink for receiving and playing music from a phone or another Bluetooth audio source
- AVRCP Controller for remote playback control, including play, pause, previous, next, and volume control
- HFP HF for hands-free call audio, dialing, answering, voice recognition control, and custom AT commands

At boot, the project initializes the Bluetooth manager, media service, A2DP Sink, HFP HF, and registers the `headset` serial CLI for manual connection, media control, and call-control operations.

### 1.1 Test Environment

- Hardware
  - Target chip: BK7259
  - Audio output: onboard or external speaker
  - Audio input: onboard or external microphone
- External device
  - Phone or other classic Bluetooth device with A2DP/AVRCP/HFP support
- Output
  - Bluetooth connection and profile state logs
  - A2DP audio playback logs
  - AVRCP control logs
  - HFP call and voice path logs

.. warning::

    Please use the reference board and audio peripherals when evaluating this demo. If the peripheral specification is different, audio channel, gain, and board-level configuration changes may be required.

## 2. Directory Structure

The project uses an AP-CP dual-core layout. AP carries the application logic (media service, Bluetooth manager, A2DP Sink / HFP HF demos, CLI), and CP only boots the AP system:

```text
headset/
├── ap/
│   ├── ap_main.c                       # AP entry: bk_init → media_service_init → bt_manager_init → demos → CLI
│   ├── Kconfig.projbuild               # A2DP_SINK_DEMO / HFP_HF_DEMO toggles
│   ├── headset_user_config.h           # LOCAL_NAME, page/scan, reconnect policy, channel count
│   ├── a2dp_sink_demo_cli.c            # `headset` CLI command implementation
│   ├── a2dp_sink/
│   │   ├── a2dp_sink_demo.c            # A2DP Sink + AVRCP control logic
│   │   └── a2dp_sink_audio.c           # Playback / mix / volume / SBC|AAC decode pipeline
│   ├── hfp_hf/hfp_hf_demo.c            # HFP HF call control and SCO audio path
│   └── config/bk7259_ap/defconfig      # AP-side Kconfig overrides (audio / ADK / BT)
└── cp/
    ├── cp_main.c                       # CP entry: forwards to bk_start_ap_system unless in ATE mode
    └── config/bk7259/defconfig         # CP-side Kconfig overrides (BT controller / PWM / LP)
```


## 3. Features

### 3.1 Currently Implemented Features

- Boot-time initialization of classic Bluetooth and media service
- A2DP Sink audio reception and local playback
- AVRCP media control: play, pause, previous, next, rewind, fast-forward, and volume control
- A2DP delay value set/get
- HFP HF call control: voice recognition, dialing, answer/reject, and custom AT commands
- Serial CLI for manually controlling the headset demo

## 4. Build And Run

### 4.1 Build

```bash
make bk7259 PROJECT=bluetooth/headset
```

### 4.2 Run

After flashing the firmware, use the serial console to observe boot logs and run the `headset` CLI to control Bluetooth connection and audio behavior.

#### 4.2.1 Default Configuration

The AP-side defconfig is located at `ap/config/bk7259_ap/defconfig`; key options are:

```text
CONFIG_BT=y
CONFIG_BLUETOOTH_AP=y
CONFIG_MEDIA_SERVICE=y
CONFIG_AUDIO=y
CONFIG_AUDIO_PLAY=y
CONFIG_AUDIO_RECORD=y
CONFIG_A2DP_SINK_DEMO=y
CONFIG_HFP_HF_DEMO=y
```

The current boot code uses:

```text
a2dp_sink_demo_init(0, 1)   // aac_supported=0, auto_accept_conn=1
hfp_hf_demo_init(0)         // msbc_supported=0
```

So A2DP AAC support and HFP mSBC support are disabled by default. Use SBC music playback and CVSD call audio as the primary validation paths.

#### 4.2.2 Serial CLI Commands

The CLI command runs on the AP side, so serial commands must use the `ap_cmd` prefix to forward the command to AP. In the commands below, `XX:XX:XX:XX:XX:XX` is the MAC address of the phone or other Bluetooth device.

Help command:

- `ap_cmd headset -h`: Print `headset` command help and show supported subcommands and parameter formats.

Connection management commands:

- `ap_cmd headset pair_mode`: Put the device into pairing/discoverable mode so a phone or other Bluetooth device can find it and start pairing.
- `ap_cmd headset connect XX:XX:XX:XX:XX:XX`: Actively connect to the remote Bluetooth device with the specified MAC address. This is typically used to connect a paired phone and establish A2DP/AVRCP/HFP links.
- `ap_cmd headset disconnect XX:XX:XX:XX:XX:XX`: Disconnect the remote Bluetooth device with the specified MAC address. Use it to stop the current Bluetooth scenario or switch test devices.

Media playback control commands:

- `ap_cmd headset play`: Send an AVRCP play command to the remote player. Use it after A2DP is connected.
- `ap_cmd headset pause`: Send an AVRCP pause command to the remote player.
- `ap_cmd headset next`: Switch to the next media item through AVRCP.
- `ap_cmd headset prev`: Switch to the previous media item through AVRCP.
- `ap_cmd headset rewind 500`: Trigger AVRCP rewind. The parameter is the duration in milliseconds; `500` means rewind for 500 ms.
- `ap_cmd headset fast_forward 500`: Trigger AVRCP fast-forward. The parameter is the duration in milliseconds; `500` means fast-forward for 500 ms.
- `ap_cmd headset vol_up`: Increase the remote or local playback volume, depending on the current AVRCP volume synchronization and audio playback state.
- `ap_cmd headset vol_down`: Decrease the remote or local playback volume, depending on the current AVRCP volume synchronization and audio playback state.

A2DP delay control commands:

- `ap_cmd headset set_delay_value 100`: Set the A2DP Sink audio delay value reported to the remote device. The parameter is a 16-bit value; `100` sets the delay value to 100.
- `ap_cmd headset get_delay_value`: Read the current A2DP Sink delay value and confirm whether the delay configuration is active.

AVRCP attribute query command:

- `ap_cmd headset get_attr 1`: Query a media attribute from the remote player. The parameter is the attribute ID; `1` queries attribute ID 1. See `bk_avrcp_media_attr_id_t` for the attribute definition.

HFP HF call control commands:

- `ap_cmd headset dial 1 10086`: Start an HFP dialing flow. The first parameter controls the dial action, and the second parameter is the phone number; this example dials `10086`.
- `ap_cmd headset answer 1`: Answer the current incoming call or call request.
- `ap_cmd headset answer 0`: Reject or hang up the current incoming call or active call. The actual behavior depends on the current HFP call state.
- `ap_cmd headset hfpcmd AT+BRSF`: Send a custom HFP AT command to the remote device for HFP protocol debugging; this example sends `AT+BRSF`.

Successful command submission returns:

```text
CMDRSP:OK
```

Failed command submission returns:

```text
CMDRSP:ERROR
```

#### 4.2.3 How To Judge Pass Or Fail

`CMDRSP:OK` only means the CLI command was accepted. It does not mean the Bluetooth operation has already completed. Judge by combining Bluetooth profile state, audio playback, and call-path logs.

For A2DP playback validation, check that:

- The phone connects to the device successfully. The advertised device name is `soundbar_XXYYZZ`, where the suffix is the last three bytes of the local BT MAC;
- The local speaker outputs music from the remote device

For HFP call validation, check that:

- The phone establishes a hands-free connection
- HFP state logs match the expected behavior after dialing, answering, or rejecting a call
- The local microphone and speaker paths work during the call

#### 4.2.4 Integration Test Commands

`.it.csv` currently only covers the basic device reboot check:

```text
AT+RST
```

The expected result string is `wakeup`. A2DP/HFP behavior requires an external Bluetooth device and is usually validated manually or through dedicated automation.

## 5. Notes

1. This project depends on an external classic Bluetooth device. Before testing, confirm that the phone or audio source supports A2DP, AVRCP, and HFP.
2. The MAC address passed to CLI commands must use the fixed `XX:XX:XX:XX:XX:XX` format.
3. The default local device name prefix is `soundbar`; adjust it in `headset_user_config.h` or the HFP demo if needed.
4. A2DP and HFP both use the audio playback path, so check the current profile state before switching scenarios.
5. To validate AAC or mSBC, update the initialization parameters, Kconfig, and remote-device capability accordingly.
