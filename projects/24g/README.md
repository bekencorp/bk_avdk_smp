# Proprietary 2.4GHz (BK24G) TX/RX Example

* [中文](./README_CN.md)

## Introduction

This project turns the BK7259 board into a **proprietary 2.4GHz (BK24G)** transceiver. BK24G is Beken's proprietary point-to-point protocol on the 2.4GHz ISM band; its TX/RX scheme is similar to nRF24's Enhanced ShockBurst (multiple RX pipes, ACK payload and auto-retransmission), and it does not go through the Bluetooth or Wi-Fi stack. After boot the board automatically initializes the 2.4G controller and enters receive (RX) mode. Via the serial console you can:

- initialize / deinitialize the 2.4G controller;
- switch between transmit (TX) and receive (RX) mode;
- send data (with or without requesting an ACK);
- configure the RF channel, air data rate and ACK payload;
- reset the controller and flush the TX/RX FIFO.

> Note: this is a **proprietary 2.4G** project; the logic runs on the **CP core**. Because of a clock-resource conflict, **2.4G cannot coexist with Bluetooth**, so Bluetooth is disabled by default (`CONFIG_BLUETOOTH=n`).

How it works (point-to-point, one sender, one receiver):

```text
TX board: 2.4g_demo send  ──► air ──►  RX board: data received
                          ◄── ACK ◄──  RX board: (optional) returns ACK payload
```

## Quick start (two-board test)

> Prerequisites: two BK7259 boards (one TX, one RX), a build/flash environment, and serial terminals.

1. **Build the firmware**

   ```bash
   make bk7259 PROJECT=24g
   ```

2. **Flash the firmware** to both boards (call them A and B).

3. **Open a serial terminal** for each. After boot both boards have already initialized 2.4G and entered RX mode.

4. **Make sure the channel and data rate match** (identical by default; if you change them, set both boards to the same value):

   ```text
   2.4g_demo set_channel 10    # same channel on both boards
   2.4g_demo set_dr 0          # 1Mbps on both boards
   ```

5. **Verify TX/RX**: keep B receiving and let A send (`send` switches to TX internally):

   ```text
   2.4g_demo send 16           # board A sends 16 bytes
   ```

   Board B's serial should print the received data; board A should print the send / ACK result.

## Typical flow

Using "A sends, B receives, with ACK payload returned" as an example (commands detailed in [4.2.2 Serial CLI commands](#422-serial-cli-commands)):

```text
# Common setup on both boards: same channel and data rate
2.4g_demo set_channel 10
2.4g_demo set_dr 0

# Receiver B: enter RX and preset an ACK payload (returned to A with the ACK)
2.4g_demo txrx 1
2.4g_demo set_ack 16

# Sender A: send data (ACK requested)
2.4g_demo send 16
# For one-way send without ACK:
2.4g_demo send_no_ack 16
```

> Tip: `CMDRSP:OK` only means the command was accepted, not that the over-the-air transfer succeeded; check the TX/RX logs on both boards for the actual result.

## 1. Overview

This project demonstrates **proprietary 2.4GHz (BK24G) point-to-point** TX/RX on the Beken platform, mainly:

- init / deinit / reset of the 2.4G controller;
- TX / RX mode switching;
- data transmission (with ACK / without ACK);
- RF channel, air data rate and ACK payload configuration.

The actual TX/RX is provided by the SDK `bk_24g` component (`CONFIG_BK24G=y`); the project only registers the `2.4g_demo` serial command and performs the default initialization at boot.

### 1.1 Test environment

- Hardware
  - Target chip: BK7259 ×2 (one TX, one RX)
  - Antenna: on-board or external 2.4GHz antenna
- Output
  - 2.4G init and mode-switch logs
  - Send result (with ACK status) and received-data logs

> **Note**: use the reference boards for learning and validation. Both boards must use the **same channel and air data rate** to communicate.

## 2. Directory layout

The project uses an AP-CP dual-core structure. All 2.4G logic lives on the **CP side**; the AP side only does basic system init (no 2.4G logic):

```text
24g/
├── ap/
│   ├── ap_main.c                       # AP entry: bk_init (no 2.4G logic on AP)
│   └── config/bk7259_ap/defconfig      # AP-side Kconfig overrides
├── cp/
│   ├── cp_main.c                       # CP entry: bk_init → init bk24 and enter RX at boot → register 2.4g_demo CLI
│   ├── 2.4g/
│   │   ├── 2.4g_demo_cli.c             # `2.4g_demo` CLI implementation
│   │   └── 2.4g_demo.h
│   ├── vnd_cal.c / vnd_cal.h           # RF calibration override (compiled in when CONFIG_OVERRIDE_VND_CAL)
│   └── config/bk7259/defconfig         # CP-side Kconfig overrides (CONFIG_BK24G=y, etc.)
├── partitions/bk7259/                  # flash / ram partition tables
├── Makefile
└── CMakeLists.txt
```

The actual 2.4G TX/RX driver lives in a reusable SDK **component**:

| Component | Role |
| --- | --- |
| `components/bk_24g` (`bk_24g_api.h` / `bk_24g.h`) | 2.4G controller driver: power/clock/interrupt, TX/RX, send (with/without ACK), channel/rate/address/retransmission config, TX/RX callbacks |

## Code walkthrough (for modifying / extending)

The demo entry is `cp/cp_main.c`, which initializes 2.4G and registers the command at boot:

```c
int main(void)
{
    rtos_set_user_app_entry(user_app_main);  // user entry
    bk_init();                               // SDK base init
    cli_24g_demo_init();                     // register the 2.4g_demo serial command
    return 0;
}

static void user_app_main(void)
{
    bk_24g_os_adapter_init();                // 2.4G OS adapter init
    bk24_init();                             // init the 2.4G controller (default TX params)
    bk24_switch_to_tx_rx(1);                 // enter RX mode by default
}
```

See `components/bk24/bk_24g.h` for the full 2.4G TX/RX API.

Common change points:

- **Default boot mode**: `user_app_main` in `cp_main.c` (currently `bk24_switch_to_tx_rx(1)`, i.e. RX).
- **Add a custom command**: add a branch in `cmd_parse` in `cp/2.4g/2.4g_demo_cli.c`.
- **RF calibration**: `cp/vnd_cal.c` (overrides default calibration when `CONFIG_OVERRIDE_VND_CAL` is set).

## 3. Features

### 3.1 Currently supported

- Auto-init of the 2.4G controller and entering RX mode at boot
- TX / RX mode switching
- Data transmission: with ACK request / without ACK
- RF channel (0~127) and air data rate (1Mbps / 2Mbps) configuration
- ACK payload (preset on RX, returned with the ACK)
- Controller reset and FIFO flush
- Manual control via serial CLI

> The `bk_24g` component also provides address configuration (`bk24_set_tx_addr` / `bk24_set_rx_addr`), auto-retransmission (`bk24_set_retran`) and TX/RX callbacks; this demo's CLI does not expose all of them — extend `2.4g_demo_cli.c` as needed.

## 4. Build and run

### 4.1 Build

```bash
make bk7259 PROJECT=24g
```

### 4.2 Run

After flashing, watch the boot log on the CP main serial port and use `2.4g_demo` to control the 2.4G transceiver.

#### 4.2.1 Default configuration

The CP-side defaults are in `cp/config/bk7259/defconfig`; key switches:

```text
CONFIG_SOC_SMP=y            # AP-CP dual core
CONFIG_FREERTOS_SMP=y
CONFIG_BK24G=y             # 2.4G (bk24) driver
CONFIG_BLUETOOTH=n         # Bluetooth off (unused here)
CONFIG_MAC802154=n         # 802.15.4 off
CONFIG_AT_CMD=y
```

#### 4.2.2 Serial CLI commands

The CLI runs on the **CP side**; enter `2.4g_demo <subcommand>` directly on the CP main serial port (no `ap_cmd` forwarding).

> Unsure about the argument format? Run `2.4g_demo -h`.

**Controller & mode**

| Command | Description |
| --- | --- |
| `2.4g_demo init [0\|1]` | Init (`1`, default) / deinit (`0`) the 2.4G controller |
| `2.4g_demo txrx <0\|1>` | Switch mode: `0`=TX, `1`=RX; entering RX also presets a 16-byte ACK payload |
| `2.4g_demo reset` | Reset the controller and flush the TX/RX FIFO |

**Data transmission**

| Command | Description |
| --- | --- |
| `2.4g_demo send [len]` | Send `len` bytes (default 16, keep ≤ 32) with ACK requested; switches to TX internally |
| `2.4g_demo send_no_ack [len]` | Send `len` bytes without requesting an ACK |

**Parameter configuration**

| Command | Description |
| --- | --- |
| `2.4g_demo set_channel <0-127>` | Set the RF channel (offset from 2400MHz, 1MHz step) |
| `2.4g_demo set_dr <0\|1>` | Set air data rate: `0`=1Mbps, `1`=2Mbps |
| `2.4g_demo set_ack [len] [pipe]` | Preset the ACK payload for a pipe (0~5) in RX mode |

Commands return `CMDRSP:OK` on accept, `CMDRSP:ERROR` on failure.

#### 4.2.3 How to tell success from failure

`CMDRSP:OK` only means the CLI command was accepted; it does **not** mean the over-the-air transfer succeeded. Judge from the TX/RX logs on both boards:

- after `send`, the sender should print the send result and, when ACK is requested, whether the ACK was received;
- the receiver should print the received data and its length;
- if the receiver sees nothing, first check that both boards share the same channel and data rate, and that antenna/distance are fine.

## 5. Notes & FAQ

1. **Channel and data rate must match**: `set_channel` and `set_dr` must be identical on both boards, otherwise they cannot communicate.
2. **Sending requires TX mode**: `send` / `send_no_ack` switch to TX internally; a board cannot receive while sending.
3. **Payload length limit**: a single packet is normally ≤ 32 bytes.
4. **ACK payload is preset by the receiver**: call `set_ack` on the RX side first; its content is returned to the sender with the ACK.
5. **Commands run on the CP side**: type `2.4g_demo ...` directly, unlike AP-side projects that need `ap_cmd` forwarding.
6. **RX by default at boot**: the firmware already runs `init` and enters RX mode at startup, so no manual `init` is needed before sending.

**FAQ**

- **Receiver gets nothing**: confirm both boards share the same `set_channel` / `set_dr`; confirm the sender is in TX and the receiver in RX; check antenna and distance.
- **`send` fails or returns `CMDRSP:ERROR`**: confirm `init` was done, the length is valid (≤ 32), and the board is in TX mode.
- **ACK requested but not received**: confirm the receiver is present, in RX and has called `set_ack`; try a lower rate (`set_dr 0`) or another channel.
