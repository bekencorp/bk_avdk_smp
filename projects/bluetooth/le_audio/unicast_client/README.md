# BK7259 LE Audio Unicast Client

* [中文](./README_CN.md)

## Supported Targets

| Target | Status | LE Audio role | Host / Controller split |
| --- | --- | --- | --- |
| BK7259 | Supported | Unicast Client + Broadcast Assistant (CIS TX) | Host on AP, Controller on CP |

This project provides two roles:

- **Unicast Client** (CIS initiator) — the phone/central side. It scans for
  connectable LE Audio servers, connects, drives the full ASCS setup, and streams a
  local tone to a [unicast_server](../unicast_server/) over a CIS.
- **Broadcast Assistant** (BASS client) — it scans Broadcast Sources for a Scan
  Delegator ([broadcast_sink](../broadcast_sink/)), then remote-controls it with
  BASS Add Source and transfers the periodic-adv sync via PAST.

## How it works

Both roles are fully automatic once started; the demo FSM thread advances each step
from the matching GA/BASS callback.

Unicast Client — one command (`connect`) triggers the whole chain:

```text
connect <peer> ──▶ ACL connected ──▶ setup ──▶ get caps ──▶ discover ASE
   ──▶ config codec ──▶ set CIG ──▶ QoS ──▶ enable ──▶ create CIS
   ──▶ ISO path ready ──▶ TX tone ──▶ STREAMING
```

Broadcast Assistant — you drive it with a few high-level commands; PAST is handled
automatically:

```text
assistant_scan on ──▶ (source found, PA-synced now while scanning)
assistant_connect <deleg> ──▶ ACL to delegator
assistant_discover <deleg> ──▶ BASS discovered
assistant_add <src> <bis> ──▶ Add Source ──▶ (delegator requests PAST)
                                          ──▶ PAST -> delegator
```

Key detail: the Assistant **PA-syncs the source during scanning**, not at
`assistant_add`. PA-create-sync only succeeds while the ext scan is active on this
controller, so the sync handle is acquired up front and is ready when the delegator
requests PAST (if it is still pending, PAST is sent from the associate callback).

## Build

```bash
make bk7259 PROJECT=bluetooth/le_audio/unicast_client
```

## CLI

All commands run on the AP core; from the CP console prefix with `ap_cmd`.

### Unicast Client

| Command | Action |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | List / select LC3 preset (before connect) |
| `le_audio scan on` / `off` | Scan for connectable LE Audio servers |
| `le_audio peers` | Print scanned peers with their index |
| `le_audio connect <peer_index>` | Connect by index and auto-run the full setup |
| `le_audio connect <addr> [type]` | Connect by MAC address (`type` 0=public,1=random) |
| `le_audio tone stop` | Stop the outgoing tone |

```text
ap_cmd le_audio scan on
ap_cmd le_audio peers          # [0] addr=.. name=..
ap_cmd le_audio connect 0
# auto: setup -> caps -> discover -> config -> cig -> qos -> enable -> cis -> tone
```

### Broadcast Assistant

| Command | Action |
| --- | --- |
| `le_audio assistant_scan on` / `off` | Scan Broadcast Sources (PA-syncs the first one) |
| `le_audio assistant_sources` | Print scanned broadcast sources |
| `le_audio assistant_connect <deleg_addr> [type]` | Connect to the Scan Delegator |
| `le_audio assistant_discover <deleg_addr> [type]` | Discover its BASS |
| `le_audio assistant_add <src_index> [bis_mask]` | Add Source; triggers PAST (`bis_mask` default `0x1`) |
| `le_audio assistant_remove <source_id>` | Remove a source from the delegator |
| `le_audio assistant_stop` | Stop assistant activity and reset |
| `le_audio broadcast_code <hex32>` / `clear` | Set / clear the Broadcast Code pushed over BASS |

```text
ap_cmd le_audio assistant_scan on
# log: source[0] ... ; PA-sync source[0] for PAST ret=0
ap_cmd le_audio assistant_scan off
ap_cmd le_audio assistant_connect <deleg_addr> 0
ap_cmd le_audio assistant_discover <deleg_addr> 0
ap_cmd le_audio assistant_add 0 1
# log: PAST -> delegator sync=0x.... ret=0
```

## Notes

- Single stream by design. The ASCS chain is FSM-driven, so there are no manual
  `setup/caps/discover/config/cig/qos/enable/cis` commands — `connect` does it all.
- `bis_mask` is a bitmask (BIS 1 = `0x1`); `type` is the peer address type.
