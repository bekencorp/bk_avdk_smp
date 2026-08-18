# KVS WebRTC Example Project

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates BK7259 running as an Amazon Kinesis Video Streams
WebRTC Master. After the board connects to a Wi-Fi router, it uses an AWS KVS
WebRTC signaling channel and streams local camera audio/video to a web Viewer.

The project provides:

- AP-side KVS WebRTC Master CLI: `ap_cmd kvs_wb master [channel]`
- Default channel name: `kvs_doorbell_channel`
- AWS credentials configurable two ways: compile-time default in
  `ap/ap_main.c`, or the runtime CLI `ap_cmd kvs_wb cred <ak> <sk> [region]`
- Verification with the AWS KVS WebRTC Test Page

## 2. Test Environment

- Board: BK7259 family or compatible platform
- Network: board connected to a router with Internet access
- AWS account: Kinesis Video Streams WebRTC enabled, with permission to access
  signaling channels
- Browser: PC browser that can open the AWS KVS WebRTC Test Page
- Serial console: AP console for logs and CLI commands

## 3. Project Layout

```text
kvs_webrtc_example/
├── ap/
│   ├── ap_main.c                       # AP entry, AWS key setup and KVS init
│   ├── config/bk7259_ap/defconfig      # AP config
│   └── kvs_webrtc/
│       ├── kvs_webrtc_cli.c            # kvs_wb CLI
│       ├── kvs_webrtc_master.c         # KVS WebRTC Master
│       └── kvs_common.*                # Common KVS logic
├── cp/
├── partitions/
├── CMakeLists.txt
└── Makefile
```

## 4. Build and Flash

Run from the SDK root:

```bash
cd <SDK_ROOT>
make bk7259 PROJECT=multimedia/kvs_webrtc_example -j$(nproc)
```

Flash the generated firmware with the usual flashing tool. After boot, open the
AP serial console. The commands below are entered from the AP console.

## 5. Usage

### 5.1 Configure AWS Credentials

Two methods are supported and can coexist: a **compile-time default** (edit the
source, applied automatically at boot) and **runtime CLI input** (overrides at
any time).

#### Method A: Compile-time default (edit `ap_main.c`)

Open `ap/ap_main.c` and replace the placeholders in
`kvs_set_aws_credentials_env()` with the actual AWS Access Key and Secret Key:

```c
setenv("AWS_ACCESS_KEY_ID", "YOUR_ACCESS_KEY_ID", 1);
setenv("AWS_SECRET_ACCESS_KEY", "YOUR_SECRET_ACCESS_KEY", 1);
//setenv("AWS_DEFAULT_REGION", "us-west-2", 1);
```

If the signaling channel is not in the default region, also enable and update
`AWS_DEFAULT_REGION`. The IAM key needs Kinesis Video Streams and signaling
channel permissions.

> Note: this method stores the plaintext key in source; be careful not to leak
> this file when committing code.

#### Method B: Runtime CLI input (recommended, no secrets in source)

No source change needed. Set the key from the AP serial console with the
`kvs_wb cred` command (kept in RAM only, not persisted across reboot), then
start the master:

```text
# Set AWS credentials (region optional)
ap_cmd kvs_wb cred <access_key> <secret_key> [region]

# Show current credentials (Secret shown as length only, not echoed)
ap_cmd kvs_wb cred show

# Clear credentials
ap_cmd kvs_wb cred clear
```

Example:

```text
ap_cmd kvs_wb cred AWS_ACCESS_KEY_ID  AWS_SECRET_ACCESS_KEY  AWS_DEFAULT_REGION
ap_cmd kvs_wb master
```

Notes:

- The command uses `setenv`, so it **overrides** the Method A compile-time
  default; use it to switch to another key on the fly.
- The master thread reads the credentials when `ap_cmd kvs_wb master` starts, so
  always run **`cred` before `master`**.
- Since values live in RAM only, **re-run `ap_cmd kvs_wb cred ...` after a
  reboot** (or rely on the Method A compile-time default).

### 5.2 Connect to the Router

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

### 5.3 Sync NTP Time

KVS WebRTC authentication requires valid system time. After network connection,
run:

```text
ap_cmd uptime
```

With `CONFIG_NTP_SYNC_RTC` enabled, this command triggers NTP sync and prints
logs such as `Get local time from NTP server` and `NTP Time`. If no valid time
is obtained, check Internet access, DNS and NTP reachability first.

### 5.4 Start KVS WebRTC Master

Start with the default channel:

```text
ap_cmd kvs_wb master
```

Or specify a custom channel:

```text
ap_cmd kvs_wb master <channel_name>
```

The default channel is defined in `ap/kvs_webrtc/kvs_webrtc_cli.c`:

```text
kvs_doorbell_channel
```

After startup, check serial logs for `kvs_db_cli`, `KVS Master`, `Signaling`
and `ICE` to confirm signaling and WebRTC connection state.

### 5.5 Watch from the Web Viewer

Open the AWS KVS WebRTC Test Page in a PC browser:

<https://awslabs.github.io/amazon-kinesis-video-streams-webrtc-sdk-js/examples/index.html>

Configure the web page with the same settings as the device:

1. Enter the same `Access Key ID`, `Secret Access Key` and `Region`.
2. Set `Channel Name` to the device channel, for example `kvs_doorbell_channel`.
3. Select the `Viewer` role.
4. Click `Start Viewer`.

After a successful connection, the browser should display the audio/video stream
from the board. If the channel does not exist, channel create/describe behavior
depends on the permissions of the AWS key being used.

## 6. Diagnostics

- `AWS_ACCESS_KEY_ID must be set` or `AWS_SECRET_ACCESS_KEY must be set`: check
  that the placeholders in `ap/ap_main.c` were replaced (then rebuild and flash),
  or set them before starting the master with
  `ap_cmd kvs_wb cred <ak> <sk> [region]`; use `ap_cmd kvs_wb cred show` to
  confirm the current values.
- Authentication failed or AWS request denied: check key, region, IAM
  permissions and RTC/NTP time.
- Web Viewer cannot connect: make sure key, region and channel match exactly on
  both web page and device.
- ICE connection failed: make sure the router allows Internet access and does
  not block UDP, STUN/TURN or WebSocket connections.
- No media from device: confirm `ap_cmd kvs_wb master` started successfully,
  then check `KVS Master`, `Signaling` and `ICE` logs.

## 7. Quick Command Reference

```text
# Connect to router
ap_cmd sta <ssid> <password>

# Sync and print NTP/RTC time
ap_cmd uptime

# Set AWS credentials at runtime (optional, overrides compile-time default; region optional)
ap_cmd kvs_wb cred <access_key> <secret_key> [region]

# Show / clear the configured credentials
ap_cmd kvs_wb cred show
ap_cmd kvs_wb cred clear

# Start KVS WebRTC Master with default channel
ap_cmd kvs_wb master

# Start KVS WebRTC Master with a custom channel
ap_cmd kvs_wb master <channel_name>
```
