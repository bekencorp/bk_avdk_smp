# KVS AWS Sample Project

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates Amazon Kinesis Video Streams (KVS) WebRTC P2P on
BK7259. After the board connects to a Wi-Fi router with Internet access, it can
join a signaling channel as either **Master** or **Viewer**.

The project provides:

- AP-side CLI: `ap_cmd kvs master|viewer [channel]`
- Default channel name: `kvs_aws_channel`
- Runtime role selection; the same firmware can switch Master / Viewer without
  rebuild
- Master: send H.264 / H.265 / Opus from static sample frames on the SD card
- Viewer: receive peer media and handle it in `Common.c` callbacks (logging by
  default)
- AWS credentials configurable two ways: compile-time default in
  `ap/ap_main.c`, or the runtime CLI `ap_cmd kvs cred <ak> <sk> [region]`
- Verification with a second board as Viewer, or the AWS KVS WebRTC Test Page
  against Master

## 2. Test Environment

- Board: BK7259 family or compatible platform
- SD card: FAT filesystem with sample media directories at the root (see 5.1)
- Network: board connected to a router with Internet access
- AWS account: Kinesis Video Streams WebRTC enabled, with permission to access
  signaling channels
- Serial console: AP console for logs and CLI commands
- (Optional) Second board or PC browser as the peer Viewer

## 3. Project Layout

```text
kvs_aws_sample/
├── ap/
│   ├── ap_main.c                       # AP entry: mount SD, AWS key, CLI init
│   ├── kvs_cli.c                       # kvs master|viewer|cred CLI
│   ├── Common.c / Samples.h            # Shared KVS logic and config
│   ├── StaticMedia.c                   # Read static sample frames from SD
│   ├── kvsWebRTCClientMaster.c         # Master entry
│   ├── kvsWebRTCClientViewer.c         # Viewer entry
│   ├── h264SampleFrames/               # H.264 samples (copy to SD card)
│   ├── opusSampleFrames/               # Opus samples (copy to SD card)
│   ├── certs/cert.pem                  # TLS CA (or embed via CONFIG_KVS_GET_CA_FROM_ARRAY)
│   └── config/bk7259_ap/defconfig      # AP config
├── cp/
├── partitions/
├── CMakeLists.txt
└── Makefile
```

## 4. Build and Flash

Run from the SDK root:

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=kvs_aws_sample -j$(nproc)
```

Flash the generated firmware with the usual flashing tool. After boot, open the
AP serial console. The commands below are entered from the AP console (via
`ap_cmd` when needed).

## 5. Usage

### 5.1 Prepare Sample Media on the SD Card

Master reads static frames from the SD mount point `/sdcard`. Copy the sample
directories to the SD card root:

```text
/sdcard/h264SampleFrames/frame-XXXX.h264
/sdcard/h265SampleFrames/frame-XXXX.h265   # if using H.265
/sdcard/opusSampleFrames/sample-XXX.opus
```

Partial samples are under `ap/h264SampleFrames/` and `ap/opusSampleFrames/` in
the source tree; copy them to the SD card as needed. At boot, `ap_main.c`
auto-mounts the SD card at `/sdcard`.

### 5.2 Configure AWS Credentials

Two methods are supported and can coexist: a **compile-time default** (edit the
source, applied automatically at boot) and **runtime CLI input** (overrides at
any time).

#### Method A: Compile-time default (edit `ap_main.c`)

Open `ap/ap_main.c` and replace the placeholders in
`kvs_set_aws_credentials_env()` with the actual AWS Access Key and Secret Key:

```c
setenv("AWS_ACCESS_KEY_ID", "YOUR_ACCESS_KEY_ID", 1);
setenv("AWS_SECRET_ACCESS_KEY", "YOUR_SECRET_ACCESS_KEY", 1);
/* Optional: setenv("AWS_DEFAULT_REGION", "us-west-2", 1); */
```

If the signaling channel is not in the default region, also enable and update
`AWS_DEFAULT_REGION`. The IAM key needs Kinesis Video Streams and signaling
channel permissions.

Rebuild and flash after changing the keys.

> Note: this method stores the plaintext key in source; be careful not to leak
> this file when committing code.

#### Method B: Runtime CLI input (recommended, no secrets in source)

No source change needed. Set the key from the AP serial console with the
`kvs cred` command (kept in RAM only, not persisted across reboot), then start
master / viewer:

```text
# Set AWS credentials (region optional)
ap_cmd kvs cred <access_key> <secret_key> [region]

# Show current credentials (Secret shown as length only, not echoed)
ap_cmd kvs cred show

# Clear credentials
ap_cmd kvs cred clear
```

Example:

```text
ap_cmd kvs cred AWS_ACCESS_KEY_ID  AWS_SECRET_ACCESS_KEY  AWS_DEFAULT_REGION
ap_cmd kvs master
```

Notes:

- The command uses `setenv`, so it **overrides** the Method A compile-time
  default; use it to switch to another key on the fly.
- Master / viewer read the credentials when `ap_cmd kvs master|viewer` starts,
  so always run **`cred` before `master` / `viewer`**.
- Since values live in RAM only, **re-run `ap_cmd kvs cred ...` after a
  reboot** (or rely on the Method A compile-time default).

For the more secure IoT Core credential flow, see `kvs_aws_sample.rst`.

### 5.3 Connect to the Router

After the board boots, connect it to the Wi-Fi router from the AP console:

```text
ap_cmd sta <ssid> <password>
```

The logs should show STA connected and IP acquired. You can also check Wi-Fi
state:

```text
ap_cmd state
```

Make sure the router has Internet access, otherwise AWS signaling, STUN/TURN
and NTP may fail.

### 5.4 Sync NTP Time

KVS WebRTC authentication requires valid system time. After network connection,
run:

```text
ap_cmd uptime
```

With `CONFIG_NTP_SYNC_RTC` enabled, this command triggers NTP sync and prints
logs such as `Get local time from NTP server` and `NTP Time`. If no valid time
is obtained, check Internet access, DNS and NTP reachability first.

### 5.5 Start Master or Viewer

Start Master with the default channel (send sample A/V):

```text
ap_cmd kvs master
```

Or start Viewer (receive peer stream):

```text
ap_cmd kvs viewer
```

Use a custom channel name (Master and Viewer must match):

```text
ap_cmd kvs master <channel_name>
ap_cmd kvs viewer <channel_name>
```

The default channel is defined in `ap/kvs_cli.c`:

```text
kvs_aws_channel
```

After startup, check serial logs for `kvs_cli`, `KVS Master` / `KVS Viewer`,
`Signaling` and `ICE` to confirm signaling and WebRTC connection state.

> Note: after `kvs master` or `kvs viewer`, the device enters the KVS main loop;
> switching role usually requires a reboot before entering a new command.

### 5.6 Verify Connectivity

**Option A: Two boards**

1. Board A: `ap_cmd kvs master` (or the same channel name)
2. Board B: `ap_cmd kvs viewer` (same channel)
3. Viewer serial logs should show receive-side activity
   (`sampleVideoFrameHandler` / `sampleAudioFrameHandler`)

**Option B: Web Viewer against Master**

Open the AWS KVS WebRTC Test Page in a PC browser:

<https://awslabs.github.io/amazon-kinesis-video-streams-webrtc-sdk-js/examples/index.html>

Configure the web page with the same settings as the device:

1. Enter the same `Access Key ID`, `Secret Access Key` and `Region`.
2. Set `Channel Name` to the device channel, for example `kvs_aws_channel`.
3. Select the `Viewer` role.
4. Click `Start Viewer`.

After a successful connection, the browser should display the sample audio/video
stream from the board.

## 6. Diagnostics

- `AWS_ACCESS_KEY_ID must be set` or `AWS_SECRET_ACCESS_KEY must be set`: check
  that the placeholders in `ap/ap_main.c` were replaced (then rebuild and flash),
  or set them before starting master / viewer with
  `ap_cmd kvs cred <ak> <sk> [region]`; use `ap_cmd kvs cred show` to confirm
  the current values.
- Authentication failed or AWS request denied: check key, region, IAM
  permissions and RTC/NTP time.
- Frame read / open file failed: confirm the SD card is inserted and directories
  such as `/sdcard/h264SampleFrames` exist.
- Web or peer Viewer cannot connect: make sure key, region and channel match
  exactly on both sides.
- ICE connection failed: make sure the router allows Internet access and does
  not block UDP, STUN/TURN or WebSocket connections.
- No media from device: confirm `ap_cmd kvs master` started successfully, then
  check `KVS Master`, `Signaling` and `ICE` logs.

## 7. Quick Command Reference

```text
# Connect to router
ap_cmd sta <ssid> <password>

# Sync and print NTP/RTC time
ap_cmd uptime

# Set AWS credentials at runtime (optional, overrides compile-time default; region optional)
ap_cmd kvs cred <access_key> <secret_key> [region]

# Show / clear the configured credentials
ap_cmd kvs cred show
ap_cmd kvs cred clear

# Start Master / Viewer with default channel
ap_cmd kvs master
ap_cmd kvs viewer

# Start with a custom channel (must match on both peers)
ap_cmd kvs master <channel_name>
ap_cmd kvs viewer <channel_name>
```
