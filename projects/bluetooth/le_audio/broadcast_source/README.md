# BK7259 LE Audio Broadcast Source

* [中文](./README_CN.md)

## Supported Targets

| Target | Status | LE Audio role | Host / Controller split |
| --- | --- | --- | --- |
| BK7259 | Supported | Broadcast Source (BIS TX) | Host on AP, Controller on CP |

A **Broadcast Source** (Auracast transmitter). It builds a BIS, encodes a locally
generated sine tone to LC3, and transmits it connectionless. Any number of
[broadcast_sink](../broadcast_sink/) devices can sync and play it.

## How it works

The demo runs an FSM thread; `le_audio start` kicks off the chain and each GA
callback advances it automatically:

```text
start ──▶ alloc session ──▶ configure ──▶ SEP register ──▶ setup announcement
                                                              │ (announcement done)
                                                              ▼
                                          create BIG ──▶ (BIG started) ──▶ TX tone  ──▶ STREAMING
stop  ──▶ suspend BIG ──▶ end announcement ──▶ free session ──▶ IDLE
```

- The periodic advertising (PA) carries the BASE (codec/stream layout) so sinks
  know how to decode; the extended advertising carries the Broadcast Audio
  Announcement with the Broadcast ID.
- Audio comes from `../common/audio_tone.c` (sine tone) → `audio_tx.c`
  (LC3 encode + `bk_dm_bap_broadcast_data_send`).

## Build

```bash
make bk7259 PROJECT=bluetooth/le_audio/broadcast_source
```

## CLI

All commands run on the AP core; from the CP console prefix with `ap_cmd`.

| Command | Action |
| --- | --- |
| `le_audio preset list` / `le_audio preset <name>` | List / select LC3 preset (before `start`) |
| `le_audio start` | Build the broadcast and start streaming the tone |
| `le_audio stop` | Tear the broadcast down (suspend → end → free) |
| `le_audio broadcast_code <hex32>` / `clear` | Set / clear the 16-byte Broadcast Code (encrypted broadcast) |

Typical session:

```text
ap_cmd le_audio preset list
ap_cmd le_audio start
# ... broadcast_sink syncs and plays ...
ap_cmd le_audio stop
```

Expected log on start:

```text
setup announcement pending session=0 sep=0
create BIG session=0 ret=0
broadcast started big=0 bis_handle=0x....
tx started; stop with ap_cmd le_audio stop
```

## Notes

- Single BIS / single stream by design.
- For an encrypted broadcast, set the same Broadcast Code on the sink side.
- LC3 stream parameters come from `ap/le_audio_user_config.h` (default 48 kHz /
  10 ms / 120 bytes) and the shared preset table in `../common/audio_codec.c`.
