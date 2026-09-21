# CP Audio KWS Example Project

* [中文](./README_CN.md)

## 1. Overview

This project validates the BK7259 CP-side audio path, prompt playback, and KWS/ASR service entry points. It covers:

- CP-side `voice_service` mic-to-speaker loopback
- Integration build for CP audio pipeline, audio driver/HAL, and codec components
- Runtime `audio_obs` statistics for mic/aec/eq/enc/dec/spk/kws processing cost and event counters
- CP-side `player_service` playback and prompt-tone path validation
- CP-side ASR/KWS stub flow

Note: the current CP KWS/ASR algorithm library may still be stubbed. The `audio asr_turn_on` command validates the service path and data flow, not real wake-word accuracy.

## 2. Test Environment

- Recommended board: BK7259 family or compatible platform
- Serial console: CP default log/CLI port
- Audio hardware: onboard microphone and onboard speaker/amplifier output
- If BLE/Wi-Fi interferes with audio tests, disable BLE before testing

## 3. Project Layout

```
cp_audio_kws_example/
├── ap/
│   ├── ap_main.c
│   └── config/
├── cp/
│   ├── cp_main.c
│   ├── cli_voice_service.c      # voice_service and audio_obs CLI
│   ├── cli_player_service.c     # player_service CLI
│   ├── cp_asr_cli.c             # audio asr_turn_on/off CLI
│   ├── cp_kws_cli.c             # KWS stub helper, not registered as CLI currently
│   ├── cp_prompt_cli.c          # Prompt helper, not registered as CLI currently
│   ├── audio_param/
│   └── config/
└── CMakeLists.txt
```

## 4. Build and Firmware

Run from the SDK root:

```
make clean
CCACHE_DISABLE=1 make bk7259 PROJECT=multimedia/cp_audio_kws_example
```

After a successful build, the full firmware image is:

```
build/bk7259/cp_audio_kws_example/package/all-app.bin
```

The OTA image is:

```
build/bk7259/cp_audio_kws_example/package/app_pack.rbl
```

## 5. Flash and Boot Check

Flash `all-app.bin` with the normal flashing tool. After boot, check the CP serial log for:

```
cp_audio_kws_example CP init done
cp_audio_kws_example CP main running
```

To reduce wireless-side interference during audio validation, optionally run:

```
AT+BLEPOWER=0
```

## 6. CLI Test Examples

### 6.1 Enable audio_obs Statistics

`audio_obs` selects the audio components to observe and configures the statistics window. It does not start the audio path.

```
audio_obs [bitmap|eq,spk,mic,aec,enc,dec,kws,all,off] [interval_ms]
```

Common examples:

```
audio_obs all 5000
audio_obs eq,spk 5000
audio_obs off
```

The logs include process count, err/timeout/short/zero/done counters, and processing cost min/max/avg/p95/p99.

### 6.2 Mic-to-Speaker Loopback

Start 16 kHz, no AEC, PCM encode/decode, onboard mic to onboard speaker, mono output:

```
voice_service start onboard 16000 0 pcm pcm onboard 16000 1 call
```

Expected result:

- CLI returns `CMDRSP:OK`
- Onboard speaker plays mic loopback audio
- If `audio_obs` is enabled, mic/eq/spk statistics are printed periodically
- There should be no obvious noise, audio discontinuity, or continuous short/timeout/error counters

Stop:

```
voice_service stop
```

`voice_service` argument format:

```
voice_service start <mic_type> <mic_sample_rate> <aec_en> <enc_type> <dec_type> <spk_type> <spk_sample_rate> <spk_chl> [dac_source]
```

Common arguments:

- `mic_type`: `onboard`
- `mic_sample_rate`: `8000` or `16000`
- `aec_en`: `0` to disable AEC, `1` to enable software AEC
- `enc_type`/`dec_type`: `pcm`, `g711a`, `g711u`, `aac`, `g722`
- `spk_type`: `onboard`
- `spk_chl`: `1` mono, `2` stereo
- `dac_source`: optional, `call`, `a2dp`, `hint`, or `prompt`; only `prompt` enables multi-source prompt mixing

### 6.3 Standalone Player/Prompt Playback

Prompt audio can be played without `voice_service`. Play a built-in array source:

```
player_service playback start array 1
```

Stop playback:

```
player_service playback stop
```

### 6.4 Multi-Source Prompt-Tone Mixing

The `player_service prompt_tone` subcommand validates multi-source audio mixing into an existing speaker path; it is not the normal standalone playback path. This path depends on the speaker element created by `voice_service`. Run the commands in this order:

1. Start mic-to-speaker loopback first:

```
voice_service start onboard 16000 0 pcm pcm onboard 16000 1 prompt
```

2. Start prompt-tone mixing. In low-memory scenarios, prefer `array 0`, which is a 16 kHz/16-bit/mono PCM prompt and does not require an extra MP3/WAV decoder:

```
player_service prompt_tone start 1 array 0
```

3. Stop prompt tone and release player/port resources. Run stop even if `PLAYER_EVENT_FINISH` has already been printed:

```
player_service prompt_tone stop 1
```

4. Stop loopback:

```
voice_service stop
```

If `voice_service` has not been started first, the `prompt_tone` multi-source mixing path fails because `gl_voice_service_handle` is NULL. For standalone prompt playback, use `player_service playback start array <id>` from the previous section.

Note: for 16 kHz multi-source testing, `prompt` mode uses A2DP as the main path and CALL as the prompt source. This matches the Bluetooth music use case, but the A2DP 16 kHz path may be resampled to 48 kHz and therefore has higher memory pressure than CALL/HINT-only testing.

Note: `array 1` is MP3 and `array 2` is WAV. When `voice_service` has already enabled AEC/codec/EQ, mixing an MP3/WAV prompt may fail because the decoder needs extra memory. The log looks like `mp3_decoder_init: Memory exhausted`.

### 6.5 ASR/KWS Stub Path

Start the CP ASR/KWS stub service path:

```
audio asr_turn_on 0 0 16000 1
```

Argument format:

```
audio asr_turn_on <aec> <uac> <sample_rate> [asr_en]
```

- `aec`: `0` to disable AEC, `1` to enable AEC
- `uac`: CP example does not support UAC mic, use `0`
- `sample_rate`: `8000` or `16000`
- `asr_en`: `0` or `1`

Expected log:

```
cp_asr turn_on done: service pipeline running
cp_asr frame=... bytes=... kws=stub
cp_asr kws_stub result=success
cp_asr result handler: recognition success
```

Stop:

```
audio asr_turn_off
```

## 7. Diagnostics and Notes

- If `voice_service start` returns an error, check the argument count and whether the codec combination is supported.
- For loopback noise, enable `audio_obs all 5000` and check whether mic, eq, or spk has continuous short/timeout/error counters or abnormal p95/p99 cost.
- For playback or prompt discontinuity, focus on `ONBOARD_SPK` and decoder/raw stream statistics.
- If ASR/KWS logs include `kws=stub` or `lib=not_linked`, the algorithm library is not linked. This is expected for the stub build.
- `cp_prompt` and `cp_kws` are not registered as CLI commands currently; use `player_service` and `audio asr_turn_on/off` for this firmware.
- If wireless activity affects audio validation, run `AT+BLEPOWER=0` before testing.

