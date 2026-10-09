# BK7259 LE Audio Unicast Server

* [中文](./README_CN.md)

## Supported Targets

| Target | Status | LE Audio role | Host / Controller split |
| --- | --- | --- | --- |
| BK7259 | Supported | Unicast Server (CIS RX) | Host on AP, Controller on CP |

A **Unicast Server** (CIS acceptor) — the earbud/speaker side of unicast LE Audio.
It exposes PACS + ASCS, advertises connectable, accepts the CIS a
[unicast_client](../unicast_client/) sets up, then LC3-decodes the incoming ISO and
plays it on the local speaker.

## How it works

The server is reactive. It advertises at boot; the client drives all ASCS
configuration (codec/QoS/enable), and the demo FSM only reacts to the isochronous
events:

```text
boot ──▶ advertise (connectable)
     ──▶ (peer connects, client configures ASE over ASCS)
     ──▶ CIS request ──▶ CIS established ──▶ ISO path ready
                                                 │  auto
                                                 ▼
                                        receiver start ready ──▶ playback
peer disconnects ──▶ stop playback ──▶ re-advertise
```

- `receiver start ready` is issued **automatically** when the ISO data path is
  ready, so audio starts without any CLI step.
- Advertising is idempotent and self-healing: it is enabled once at boot, the
  controller auto-terminates it on connection, and it is re-enabled after a
  disconnect. Re-issuing `adv on` while already advertising is a safe no-op.

## Build

```bash
make bk7259 PROJECT=bluetooth/le_audio/unicast_server
```

## CLI

All commands run on the AP core; from the CP console prefix with `ap_cmd`.
In the common case you do not need to type anything — just connect from the client.

| Command | Action |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | List / select LC3 preset |
| `le_audio adv on` / `off` | Manually enable/disable advertising (guarded, idempotent) |
| `le_audio rx_ready <ase>` | Manually issue Receiver Start Ready (normally automatic) |
| `le_audio release <ase>` | Release an ASE and stop playback |

### Volume control (VCP renderer, `CONFIG_LE_AUDIO_VCP`, on by default)

This side is the Volume Renderer, so a Volume Controller on the client side is
reported through a callback. The commands below change the volume locally and
notify every subscribed controller.

| Command | Action |
| --- | --- |
| `le_audio volume <0-255>` | Set the absolute volume |
| `le_audio mute on` / `off` | Mute / unmute |

### Media control (MCP, `CONFIG_LE_AUDIO_MCP`, on by default)

The player state lives on the peer; this side acts as Media Control Client and
issues control point operations.

| Command | Action |
| --- | --- |
| `le_audio play` / `pause` / `stop` | Play / pause / stop |
| `le_audio next` / `prev` | Next / previous track |
| `le_audio mcp_notify on` / `off` | Subscribe / unsubscribe from player state notifications |
| `le_audio mcp_read` | Read the current player state |

Expected log when a client connects and streams:

```text
cis_request ase=0x01 ...
cis_established ase=0x01 cis_handle=0x....
iso_path_ready ase=0x01 ... -> auto rx_ready
receiver start ready ase=1
```

## Notes

- Stereo over two streams by default: two sink ASEs are registered, PACS exposes
  front left plus front right, the streams are carried by **two CIS of the same
  CIG**, and both are decoded and mixed into one stereo playback. The stream count
  comes from `CONFIG_LE_AUDIO_STREAM_COUNT`.
- PACS deliberately reports `supported_channel_counts` of 1: each stream carries a
  single channel. Reporting 2 lets a peer set up one CIS instead and pack left and
  right into the same SDU.
- The QoS in the ASE is only a preference; the client decides the final parameters.
- The client chooses the codec/QoS; this side just accepts and plays.
