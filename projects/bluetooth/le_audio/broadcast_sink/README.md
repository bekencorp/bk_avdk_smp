# BK7259 LE Audio Broadcast Sink

* [中文](./README_CN.md)

## Supported Targets

| Target | Status | LE Audio role | Host / Controller split |
| --- | --- | --- | --- |
| BK7259 | Supported | Broadcast Sink + Scan Delegator (BIS RX) | Host on AP, Controller on CP |

A **Broadcast Sink** (Auracast receiver) that also acts as a **BASS Scan
Delegator**. It has two ways to end up receiving a BIS:

- **Self-sync**: the device scans for Broadcast Sources itself, you pick a source
  and BIS, and it syncs and plays.
- **Assisted (Scan Delegator)**: it exposes a BASS server and stays passive; a
  Broadcast Assistant (e.g. [unicast_client](../unicast_client/)) tells it which
  broadcast to receive and transfers the periodic-advertising sync via PAST.

Received ISO is LC3-decoded and rendered by `../common/audio_rx.c` /
`audio_playback.c`.

## How it works

Both paths converge on the same "enable BIS → stream" tail, driven by the demo
FSM thread:

```text
Self-sync:
  scan on ──▶ (announcement) ──▶ sync <src> <bis>
      ──▶ PA associate ──▶ (BIGInfo) ──▶ broadcast_enable ──▶ ENABLE_CNF ──▶ STREAMING

Scan Delegator (PAST):
  delegator on ──▶ (Assistant Add Source) ──▶ await PAST
      ──▶ PA associate (via PAST) ──▶ broadcast_enable ──▶ ENABLE_CNF ──▶ STREAMING
```

Two path-specific details worth knowing:

- On self-sync, scanning is **kept running** through PA-associate → BIG-create-sync.
  Stopping the scan there would leave a scan-disable HCI command in flight and the
  BIG create-sync (same GA activity context) would be rejected with "Context exists".
- On the PAST path there is **no separate BIGInfo event** (the controller only
  attaches BIGInfo as ACAD while BIG create-sync is pending), so the demo issues
  `broadcast_enable` directly from the PA-associate callback.

## Build

```bash
make bk7259 PROJECT=bluetooth/le_audio/broadcast_sink
```

## CLI

All commands run on the AP core; from the CP console prefix with `ap_cmd`.

| Command | Action |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | List / select LC3 preset |
| `le_audio scan on` / `off` | Start/stop scanning for Broadcast Sources |
| `le_audio list` | Print the scanned sources with their index |
| `le_audio sync <src_index> <bis_index>` | Self-sync to a source's BIS (`bis_index` is 1-based) |
| `le_audio delegator on` / `off` | Advertise the BASS server for a Broadcast Assistant |
| `le_audio stop` | Stop / tear down the current session |
| `le_audio broadcast_code <hex32>` / `clear` | Set / clear the Broadcast Code |

Self-sync session:

```text
ap_cmd le_audio scan on
# log: [0] sid=1 addr=.. bcast_id=0x...
ap_cmd le_audio sync 0 1
# log: PA associated -> BIGInfo -> broadcast_enable ret=0 -> streaming
ap_cmd le_audio stop
```

Scan Delegator session (with a Broadcast Assistant):

```text
ap_cmd le_audio delegator on
# Assistant does Add Source + PAST; expected log:
#   Assistant Add Source ... pa_sync=1
#   awaiting PAST from Assistant
#   PA associated handle=0x....
#   broadcast_enable ... ret=0
#   broadcast event=... (ENABLE_CNF) -> streaming
```

## Notes

- Single BIS / single stream by design; `bis_index` is 1-based.
- For an encrypted broadcast set the Broadcast Code (or let the Assistant push it
  over BASS) before syncing.
