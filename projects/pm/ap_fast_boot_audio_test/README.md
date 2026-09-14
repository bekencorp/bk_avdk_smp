# AP Fast Boot Audio Test Project

* [中文](./README_CN.md)

## 1. Overview

This project verifies audio services across **AP Fast Boot (cold)** on BK7259 AP+CP. Product code uses PM callbacks, not CLI. The CLI here only mimics that flow.

Services under test:

- **voice**: call / loopback (mic + AEC + speaker)
- **asr**: on-device KWS (`CONFIG_BEKEN_KWS` / NPU enabled in this project)
- **player**: own-speaker **playback**, or **prompt_tone** mixed into the voice speaker

Cold path: `stop` tears objects and keeps cfg; after fast boot the service `init`s again. `deinit` tears objects and clears cfg so it does not resume. Hot (register backup) is out of scope.

## 2. Hardware

- Board: BK7259
- One board with mic and speaker
- CP and AP serial consoles
- Defconfig already sets `CONFIG_PM_AP_FAST_BOOT_ENABLE` and `CONFIG_AUD_PM_FAST_COLD`

## 3. Layout

```
pm/ap_fast_boot_audio_test/
├── ap/
│   ├── audio_test/          # aud_fb entry (use this)
│   ├── voice_service_test/
│   ├── asr_service_test/
│   └── player_service_test/
├── cp/aud_fb_ipc/           # AP requests CP to vote AP OFF
└── Makefile
```

## 4. Build

From the SDK root:

```
make bk7259 PROJECT=pm/ap_fast_boot_audio_test
```

Firmware: `build/bk7259/ap_fast_boot_audio_test/package/all-app.bin`

## 5. CLI rules

### 5.1 Which UART

| Console | Command |
|---------|---------|
| CP | `ap_cmd aud_fb ...` (forwarded to AP) |
| AP | `aud_fb ...` |

Examples below use `ap_cmd`. Drop that prefix on the AP console.

Prefer `aud_fb` over raw `voice_service` / `asr_service` / `player_service`: it tracks running bits, distinguishes `stop` vs `deinit`, and asks CP for AP OFF when the last service stops.

### 5.2 stop vs deinit

| Command | Objects | cfg / want_restart | After fast boot |
|---------|---------|--------------------|-----------------|
| `stop` | torn down | **kept** | auto `init` (**main test**) |
| `deinit` | torn down | **cleared** | **no resume** |

Product idle uses `stop`. Use `deinit` only for a true off.

### 5.3 Auto AP OFF and wake

`auto_ap_off` defaults to on. After the last `stop`/`deinit`, AP requests CP to power AP off.

```
ap_cmd aud_fb service status
ap_cmd aud_fb auto_ap_off off
ap_cmd aud_fb ap off
```

Wake from the **CP** console:

```
ap_fast_boot on
```

Expect `RESUME_PROOF` again and services that were `stop`ped to come back. `main_entries` stays 1 (not a cold `main` rerun).

### 5.4 Command list

```
ap_cmd aud_fb service voice init [args]
ap_cmd aud_fb service voice stop
ap_cmd aud_fb service voice deinit
ap_cmd aud_fb service asr init [args]
ap_cmd aud_fb service asr stop
ap_cmd aud_fb service asr deinit
ap_cmd aud_fb service player init [args]
ap_cmd aud_fb service player stop
ap_cmd aud_fb service player deinit
ap_cmd aud_fb service status
ap_cmd aud_fb auto_ap_off on|off
ap_cmd aud_fb ap off
```

Defaults when `init` has no extra args:

| Service | Default `init` | Meaning |
|---------|----------------|---------|
| voice | onboard 8000 AEC g711a / onboard 8000 mono | loopback |
| asr | `startwithmic onboard 16000` | ASR **owns the mic** |
| player | `playback array 0` | player **owns the speaker** |

## 6. Single-service tests

Pattern: start → check audio/KWS → `stop` → CP `ap_fast_boot on` → check auto resume.

### 6.1 Voice only

```
ap_cmd aud_fb service voice init
# Speak; speaker should loop back. Logs: ONBOARD_MIC / RAW_READ / ONBOARD_SPK
ap_cmd aud_fb service voice stop
ap_fast_boot on
# Loopback should come back; no extra init
```

Custom args (same as `voice_service start`):

```
ap_cmd aud_fb service voice init onboard 8000 1 g711a g711a onboard 8000 1
```

No resume after wake:

```
ap_cmd aud_fb service voice deinit
ap_fast_boot on
```

### 6.2 ASR only (own mic)

```
ap_cmd aud_fb service asr init
# Wake word first, then command words
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

KWS in this project:

| Say | Log |
|-----|-----|
| 你好博通 | `nihaobotong, cmd: 1` |
| 再见博通 | `zaijianbotong, cmd: 2` |
| Play Music | `play music, cmd: 3` |
| Stop Play | `stop play, cmd: 4` |
| Next song | `next song, cmd: 5` |
| Volume Up | `volume up, cmd: 6` |
| Volume Down | `volume down, cmd: 7` |

Explicit own-mic:

```
ap_cmd aud_fb service asr init startwithmic onboard 16000
```

Do **not** also `voice init` (dual ADC / dump).

### 6.3 Player only (own speaker)

```
ap_cmd aud_fb service player init
ap_cmd aud_fb service player stop
ap_fast_boot on
```

This is `playback array 0`. Do not run it together with voice.

## 7. Combinations

### 7.1 Allowed vs not

| Combo | How | Notes |
|-------|-----|-------|
| voice + prompt_tone | voice first, then `prompt_tone` | Standard multi-stream test |
| ASR sharing voice | **only** `asr init startnomic` | nomic starts voice itself; do not `voice init` |
| voice + `startwithmic` | **forbidden** | Dual ADC |
| voice + player playback | **do not** | Two speakers; loopback dies |

### 7.2 Voice + prompt_tone (recommended)

PCM is mixed into the voice speaker. Player does not own the DAC.

```
ap_cmd aud_fb service voice init
ap_cmd aud_fb service player init prompt_tone 0 array 0
```

`prompt_tone <tone_id> array <array_id>`:

| array_id | Content |
|----------|---------|
| 0 | PCM wakeup prompt (16 kHz) |
| 1 | MP3 provisioning prompt |
| 2 | WAV low-battery prompt |

Stop **player first**, then voice:

```
ap_cmd aud_fb service player stop
ap_cmd aud_fb service voice stop
ap_fast_boot on
```

After resume: voice comes up, mix is attached, prompt plays again. Look for `mix attached` / `prompt_tone resume started`.

If the one-shot prompt must **not** replay after wake, use `player deinit` instead of `stop`.

### 7.3 ASR + voice (nomic)

```
ap_cmd aud_fb service asr init startnomic onboard 16000
# Do not voice init
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

`startnomic` calls `bk_voice_init` and ASR reads that mic. A second `voice init` double-opens capture.

nomic cannot attach to an already-running voice without a code change.

### 7.4 nomic ASR + prompt_tone

```
ap_cmd aud_fb service asr init startnomic onboard 16000
ap_cmd aud_fb service player init prompt_tone 0 array 0
ap_cmd aud_fb service player stop
ap_cmd aud_fb service asr stop
ap_fast_boot on
```

Do not add `startwithmic` on top of voice.

## 8. Suggested checklist

1. Voice only: `init` → loopback → `stop` → fast boot → loopback back
2. Voice only: `init` → `deinit` → fast boot → no voice
3. ASR only: `init` → wake word → `stop` → fast boot → still recognizes
4. Player playback only: `init` → audio → `stop` → fast boot
5. Voice + prompt_tone: section 7.2, prompt plays again after resume
6. ASR nomic: section 7.3, no extra `voice init`

## 9. Logs

| Symptom | Look for |
|---------|----------|
| Command OK | `CMDRSP:OK` |
| Loopback alive | `ONBOARD_MIC` / `RAW_READ` / `ONBOARD_SPK` |
| Prompt playing | `set_input_port_info` with a non-NULL port, then `PLAYER_EVENT_FINISH` |
| AP OFF request | `no audio service running -> request CP AP OFF` |
| Fast boot OK | `RESUME_PROOF`, `main_entries=1` |
| Prompt resume fail | Missing `mix attached` / instant `FINISH` |
| KWS hit | `nihaobotong, cmd: 1` etc. |

Do not use `aud_fb ap on`. AP cannot vote itself on. Use CP `ap_fast_boot on`.

## 10. Key files

- `ap/audio_test/cli_aud_fb_test.c` — unified CLI, stop/deinit, auto AP OFF
- `ap/voice_service_test/cli_voice_service.c`
- `ap/asr_service_test/cli_asr_service.c`
- `ap/player_service_test/cli_player_service.c` — prompt mix and resume attach
- `cp/aud_fb_ipc/` — AP → CP AP OFF request
- Services: `sdk/ap/components/bk_voice_service`, `bk_asr_service`, `bk_player_service` (PM APIs under `CONFIG_AUD_PM_FAST_COLD`)
