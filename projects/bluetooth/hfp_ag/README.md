# HFP AG Example

* [中文](./README_CN.md)

## Overview

This example demonstrates how to use the BK7258 platform as a Bluetooth HFP Audio Gateway (AG). The device emulates the phone side and can establish a Service Level Connection (SLC) and a SCO/eSCO audio link with a Bluetooth headset or hands-free device implementing the HFP Hands-Free (HF) role.

The `hfp_ag` command-line interface provides simulated call states, audio-link control, codec selection, battery reporting, and volume reporting. The example also shows how to use the public APIs in `bk_dm_hfp_ag.h` to handle AT commands received from the HF.

At startup, the firmware automatically initializes the Bluetooth manager, HFP AG, and CLI. The Bluetooth device name has the form `bk_hfp_ag_XXXXXX`, and the device enters connectable and discoverable mode.

## Command Overview

All functionality is provided through the `hfp_ag` command. See [Command-Line Interface](#command-line-interface) for full usage, or run `hfp_ag -h` on the serial console.

| Command | Description |
| --- | --- |
| `hfp_ag init` | Manual init (already run at startup; normally not needed) |
| `hfp_ag connect <xx:xx:xx:xx:xx:xx>` | Connect to the given HF device |
| `hfp_ag disconnect` | Disconnect the current HFP connection |
| `hfp_ag incoming [number]` | Simulate an incoming call |
| `hfp_ag answer` | Mark the current call as active |
| `hfp_ag hangup` | End or reject the current call and tear down SCO |
| `hfp_ag dial [number]` | Simulate an outgoing call; redial the last number when omitted |
| `hfp_ag audio on\|off` | Establish or release the SCO/eSCO audio link |
| `hfp_ag codec cvsd\|msbc` | Set the codec preference for the next audio link |
| `hfp_ag battery <0-5>` | Report the AG battery indicator value |
| `hfp_ag vgs <0-15>` | Set the HF speaker volume |
| `hfp_ag vgm <0-15>` | Set the HF microphone volume |
| `hfp_ag cmd <at-result-code>` | Send a custom command |

## Supported Features

- HFP AG RFCOMM, SLC, and SCO/eSCO link management
- CVSD (8 kHz) and mSBC (16 kHz) codec negotiation
- Bidirectional voice between the AG board microphone and the HF
- Simulated incoming calls, answering, hangup, outgoing calls, and redial
- HFP messages including `RING`, `+CLIP`, `+CIEV`, `+CIND`, `+COPS`, `+CLCC`, and `+CNUM`
- HF requests including `AT+BVRA`, `AT+VTS`, `AT+NREC`, `AT+CHLD`, and `AT+BTRH`
- AG battery, speaker-volume, and microphone-volume reporting
- Runtime configuration of SDP, BRSF, and CHLD capabilities
- `AT+CMEE` extended-error handling
- Apple `AT+XAPL` and `AT+IPHONEACCEV` extension handling

## Components

- **HFP AG Demo**: Maintains the example call state and handles HFP AG callbacks
- **HFP AG CLI**: Parses the `hfp_ag` commands
- **Bluetooth Manager**: Handles Bluetooth initialization, pairing, reconnection, and link state
- **Audio Record / Audio Play**: Captures microphone audio and plays remote audio on the board
- **HFP AG Host**: Parses HFP AT commands, maintains protocol state, and invokes application callbacks

The public HFP AG API is declared in:

```text
ap/include/components/bluetooth/bk_dm_hfp_ag.h
```

## Test Environment

### Hardware

- BK7258 development board
- Onboard or external microphone
- Onboard or external speaker
- A Bluetooth headset, hands-free device, or another development board implementing the HFP HF role
- Serial terminal

Audio GPIOs and microphone/speaker circuits vary between boards. Check the following files against the actual hardware:

```text
ap/config/bk7258_ap/usr_gpio_cfg.h
cp/config/bk7258/usr_gpio_cfg.h
```

### Software

- BK7258 toolchain and flashing environment
- Serial terminal
- The Bluetooth address of the remote HF device, or a way to put that device into pairing mode

## Project Structure

```text
hfp_ag/
├── ap/
│   ├── hfp_ag/
│   │   ├── hfp_ag_demo.c              # HFP AG state, callbacks, and audio handling
│   │   ├── hfp_ag_demo.h              # Demo public interface
│   │   ├── hfp_ag_demo_cli.c          # hfp_ag CLI commands
│   │   ├── ring_buffer_particle.c     # SCO audio buffering
│   │   └── ring_buffer_particle.h
│   ├── storage/
│   │   ├── bluetooth_storage.c        # Bluetooth pairing-data storage
│   │   └── bluetooth_storage.h
│   ├── ap_main.c                      # AP startup and automatic demo initialization
│   ├── bt_manager.c                   # Bluetooth link and pairing management
│   ├── bt_manager.h
│   ├── bluetooth_user_config.h        # Bluetooth name and connection parameters
│   ├── Kconfig.projbuild
│   └── config/bk7258_ap/config
├── cp/
│   ├── cp_main.c
│   └── config/bk7258/
├── partitions/bk7258/                 # Partition and memory-region configuration
├── README_CN.md
├── README.md
├── Makefile
└── CMakeLists.txt
```

## Build and Run

Run the following command from the SDK root directory:

```bash
make bk7258 PROJECT=bluetooth/hfp_ag
```

After the build completes, flash the generated firmware to the BK7258 board and open a serial terminal.

A successful CLI command returns:

```text
CMDRSP:OK
```

An invalid command or initialization failure returns:

```text
CMDRSP:ERROR
```

Display CLI help:

```text
hfp_ag -h
```

## Quick Start

### 1. Establish an HFP Connection

HFP AG is initialized automatically at startup, so it is normally unnecessary to run `hfp_ag init` again.

Put the remote HF device into pairing mode and connect using its Bluetooth address:

```text
hfp_ag connect 11:22:33:44:55:66
```

After a successful connection, the log should show that RFCOMM is connected followed by `SLC connected`. Call control and SCO audio commands require an established SLC.

Disconnect:

```text
hfp_ag disconnect
```

### 2. Simulate an Incoming Call

```text
hfp_ag incoming 10010
```

The AG sends the incoming-call state, `RING`, and the caller number when the HF has enabled `AT+CLIP=1`. Answer on the HF or run:

```text
hfp_ag answer
hfp_ag audio on
```

Hang up:

```text
hfp_ag hangup
```

`hangup` ends the call state and disconnects SCO audio.

### 3. Simulate an Outgoing Call

```text
hfp_ag dial 10086
```

This command reports the dialing and alerting states, then automatically requests a SCO audio connection. Run `dial` without a number to redial the previously stored number:

```text
hfp_ag dial
```

Mark the outgoing call as active:

```text
hfp_ag answer
```

### 4. Test Bidirectional Voice

```text
hfp_ag audio on
hfp_ag audio off
```

After the audio link is established:

- Audio captured by the BK7258 microphone is sent to the HF
- Audio captured by the HF microphone is played through the BK7258 speaker

## Command-Line Interface

### Initialization and Connection

```text
# Manual initialization. The project initializes automatically at startup,
# so this command is normally unnecessary.
hfp_ag init

# Connect to an HF device
hfp_ag connect <xx:xx:xx:xx:xx:xx>

# Disconnect the current HFP connection
hfp_ag disconnect
```

### Call Control

```text
# Simulate an incoming call; 10010 is used when no number is supplied
hfp_ag incoming [number]

# Mark the current call as active
hfp_ag answer

# End or reject the call and disconnect SCO
hfp_ag hangup

# Simulate an outgoing call; omit the number to redial
hfp_ag dial [number]
```

ATA, AT+CHUP, ATD, and AT+BLDN received from the HF are also handled automatically by the demo callback.

### Audio and Codec

```text
# Establish or release SCO/eSCO audio
hfp_ag audio on
hfp_ag audio off

# Select the preferred codec for the next audio connection
hfp_ag codec cvsd
hfp_ag codec msbc
```

Select mSBC only after the HF reports that capability through `AT+BAC`. If the HF does not support the selected codec, the host falls back or rejects the selection.

### Battery and Volume

```text
# Report the AG battery indicator, range 0-5
hfp_ag battery <0-5>

# Set the HF speaker volume, range 0-15
hfp_ag vgs <0-15>

# Set the HF microphone volume, range 0-15
hfp_ag vgm <0-15>
```

Battery or volume values above the valid range are clamped to the maximum. When SLC is not connected, the battery value is stored and its report is deferred.

### Debug Commands

```text
# Send a raw AT result code from the AG to the HF
hfp_ag cmd <at-result-code>

# Wrap the result code in double quotes when it contains commas or spaces
hfp_ag cmd "+CIEV: 2,1"
```

`cmd` sends an AG-to-HF result code; it does not inject an AT command into the AG receiver.

> **Note**: the CLI tokenizer treats **both spaces and commas as separators**, so when `<at-result-code>` contains a comma or space you must wrap the whole string in **double quotes** (spaces and commas inside quotes are not split); otherwise the command is truncated at the first comma/space, e.g. `hfp_ag cmd "+CIEV: 2,1"`. The same applies when injecting a raw AT command from another device, e.g. `ap_cmd headset hfpcmd "AT+IPHONEACCEV=2,1,9,2,1"`.

## Typical Test Flows

### Incoming Call Answered from the Headset

```text
hfp_ag connect 11:22:33:44:55:66
hfp_ag incoming 10010
```

Press the answer button on the HF. After receiving ATA, the demo replies with `OK` and updates the call state. If audio is not established automatically, run:

```text
hfp_ag audio on
```

Press the hangup button on the HF, or run:

```text
hfp_ag hangup
```

### mSBC Bidirectional Voice

```text
hfp_ag connect 11:22:33:44:55:66
hfp_ag codec msbc
hfp_ag audio on
```

The connection log should show mSBC. If CVSD is established instead, verify that the HF advertised mSBC in `AT+BAC`.

## Implementation Details

### Initialization and Capability Configuration

`hfp_ag_demo_init()` performs the following steps:

1. Registers the HFP AG callback
2. Initializes the HFP AG host
3. Queries and configures SDP, BRSF, and CHLD capabilities
4. Registers Bluetooth Manager callbacks
5. Registers the SCO audio-data callback

Capabilities must be configured after `bk_bt_hf_ag_init()` and before SLC establishment.

### AT Requests and Application Responses

The host parses AT commands received from the HF. Requests whose result must be decided by the application are reported through `BK_HF_AG_*_REQ_EVT` callbacks:

- CIND, COPS, CLCC, and CNUM use their matching `bk_bt_hf_ag_*_response()` APIs
- BVRA, VTS, NREC, ATA, CHUP, DIAL, and CHLD use `bk_bt_hf_ag_cmee_send()` to return `OK`, `ERROR`, or `+CME ERROR`
- BTRH first uses `bk_bt_hf_ag_btrh_response()` to report its state, followed by the final result code
- Unknown AT commands are answered by the application using `bk_bt_hf_ag_unknown_at_send()` and `bk_bt_hf_ag_cmee_send()`

`+CME ERROR:<code>` is sent only after the HF enables it with `AT+CMEE=1`. Otherwise, the host automatically falls back to plain `ERROR`.

### SCO Audio Path

CVSD uses 8 kHz audio and mSBC uses 16 kHz audio. The SCO connection event contains the negotiated codec, packet lengths, and transmission interval; the demo starts its record and playback paths using those values.

The board-level audio implementation in this demo supports CVSD and mSBC. LC3-SWB protocol support in the host does not mean that this demo implements LC3 audio-data processing.

## Notes

- The example maintains a single peer and a simplified call state; it is not a complete telephony implementation.
- CHLD, multiparty calls, and memory dialing mainly demonstrate protocol interaction. A product must connect them to its real call manager.
- SLC must be established before SCO audio.
- mSBC requires support from the remote HF.
- Audio gain, GPIO mapping, and analog front-end configuration may require changes for a different board.
- `hfp_ag dial hangup` dials the string `hangup`; the correct hangup command is `hfp_ag hangup`.
- Apple extension commands are non-standard HFP features and apply only when sent by a compatible device.
