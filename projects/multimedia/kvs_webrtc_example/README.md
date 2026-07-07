# KVS WebRTC Example Project

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates BK7259 running as an Amazon Kinesis Video Streams
WebRTC Master. After the board connects to a Wi-Fi router, it uses an AWS KVS
WebRTC signaling channel and streams local camera audio/video to a web Viewer.

The project provides:

- AP-side KVS WebRTC Master CLI: `ap_cmd kvs_wb master [channel]`
- Default channel name: `kvs_doorbell_channel`
- AWS credentials configured through environment variables in `ap/ap_main.c`
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
  that the placeholders in `ap/ap_main.c` were replaced, then rebuild and flash.
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

# Start KVS WebRTC Master with default channel
ap_cmd kvs_wb master

# Start KVS WebRTC Master with a custom channel
ap_cmd kvs_wb master <channel_name>
```
