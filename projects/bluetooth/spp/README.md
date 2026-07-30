# SPP Demo

* [中文](./README_CN.md)

## Overview

This project demonstrates how to use the Bluetooth SPP (Serial Port Profile)
protocol to communicate between two Beken development boards.

The project is based on the `bluetooth/headset` AP/CP project template, but the
AP application only initializes Bluetooth and registers the `spp` CLI demo.

## Hardware Requirements

- Two Beken development boards
- UART0 connection for image download, log output, and CLI commands

## Build And Flash

Build this project from the Armino SDK root with the project path set to
`bluetooth/spp`.

The generated image is the SDK build output for the `spp` project. Flash the
`all-app.bin` image to the board with BKFIL or the standard project flashing
tool.

## Workflow

Use one board as the SPP server and the other board as the SPP client:

1. On both boards, run `ap_cmd spp init`.
2. On the server board, run `ap_cmd spp start_server`.
3. Note the server channel and handle printed by the server log.
4. On the client board, run `ap_cmd spp conn <server_addr>`.
5. Note the connection handle printed by the client log.
6. Use `ap_cmd spp write <handle> <data>` on either side to send data.
7. Use `ap_cmd spp rate <handle> <hex_length>` to run a throughput test.
8. Use `ap_cmd spp disconn <remote_addr> [handle]` or `ap_cmd spp stop_server <local_channel>`
   to close the connection/server.

## CLI Commands

The project supports the following UART0 CLI commands:

```text
ap_cmd spp help
ap_cmd spp init
ap_cmd spp deinit
ap_cmd spp start_server
ap_cmd spp stop_server <local_channel>
ap_cmd spp conn <remote_addr>
ap_cmd spp conn1 <remote_addr> <remote_channel>
ap_cmd spp disconn <remote_addr> [spp_handle]
ap_cmd spp write <handle> <data>
ap_cmd spp rate <handle> <hex_length>
ap_cmd spp status <handle>
```

### Command Notes

- `ap_cmd spp init`: initializes the SPP protocol.
- `ap_cmd spp deinit`: deinitializes the SPP protocol.
- `ap_cmd spp start_server`: starts the local device as an SPP server and registers it
  in the SDP database.
- `ap_cmd spp stop_server <local_channel>`: stops the SPP server for the channel
  printed by `ap_cmd spp start_server`.
- `ap_cmd spp conn <remote_addr>`: discovers the remote SPP server channel and connects
  to it. Address format is `xx:xx:xx:xx:xx:xx`.
- `ap_cmd spp conn1 <remote_addr> <remote_channel>`: connects directly to a known
  remote server channel.
- `ap_cmd spp disconn <remote_addr> [spp_handle]`: disconnects an active SPP link.
  If the handle is omitted, the demo disconnects the first connected SPP device
  matching the remote address.
- `ap_cmd spp write <handle> <data>`: sends data on an active SPP connection.
- `ap_cmd spp rate <handle> <hex_length>`: sends randomly generated data for throughput
  testing. The length argument is parsed as hexadecimal by the CLI.
- `ap_cmd spp status <handle>`: prints the demo-side pending state for the connection.

## Example

Server board:

```text
ap_cmd spp init
ap_cmd spp start_server
```

Expected key log:

```text
bk_cli_bt_spp_callback spp init status:0
bk_cli_bt_spp_callback, spp start_server success, chnl:1, spp_handle:0x00
```

Client board:

```text
ap_cmd spp init
ap_cmd spp conn C8:47:8C:0B:DC:08
```

Expected key log:

```text
bk_cli_bt_spp_callback, spp discover success, chnl0:1, cnt:1 !!
bk_cli_bt_spp_callback, spp conn success to 0x08:0xdc:0x0b:0x8c:0x47:0xc8
HANDLE: 0x00
```

Send data:

```text
ap_cmd spp write 00 111122221111
```

Expected key log on the peer:

```text
===========DATA IND===========
bk_cli_bt_spp_callback, spp data ind, handle:0x00, len:12
111122221111
==============================
```

Throughput test:

```text
ap_cmd spp rate 00 7ffff
```

Expected key logs:

```text
========spp tx start total_length: 524287 ========
spp tx length: 524287, speed: <speed>KB/s
========spp tx finish tx_length: 524287, crc:<crc> ========
======== spp rx start ========
========spp rx finish tx_length: 524287, speed: <speed>KB/s, crc:<crc>========
```
