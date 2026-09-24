# CP Audio KWS Example Project

* [中文](./README_CN.md)

## 1. Overview

This project validates the BK7259 CP-side audio path, prompt playback, and KWS/ASR service entry points. It covers:

- CP-side `voice_service` mic-to-speaker loopback
- Integration build for CP audio pipeline, audio driver/HAL, and codec components
- Runtime `audio_obs` statistics for mic/aec/eq/enc/dec/spk/kws processing cost and event counters
- CP-side `player_service` playback and prompt-tone path validation
- Standalone validation of the CP `audio_play` / `audio_record` components (`cp_audio_play`, `cp_audio_record`)
- CP-side `spk_service` three-source mixing (A2DP/CALL/HINT) validation (`cp_spk`)
- CP-side ASR service flow

Note: the current CP KWS/ASR algorithm library may still be stubbed. The `audio asr_turn_on` command validates the service path and data flow, not real wake-word accuracy.

Note: the speaker (DAC) and the microphone (ADC) are single-instance hardware. This project guards them with the `cp_audio_res` arbitration layer, so a conflicting command is rejected with the current owner printed instead of crashing. See section 6.9.

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
│   ├── cp_audio_res.c/.h        # Speaker/mic arbitration, used by every audio CLI
│   ├── cli_voice_service.c      # voice_service and audio_obs CLI
│   ├── cli_player_service.c     # player_service CLI
│   ├── cp_asr_cli.c             # audio asr_turn_on/off CLI
│   ├── cp_audio_play_cli.c      # cp_audio_play CLI (audio_play component)
│   ├── cp_audio_record_cli.c    # cp_audio_record CLI (audio_record component)
│   ├── cp_spk_service_cli.c     # cp_spk CLI (spk_service three-source mixing)
│   ├── cp_prompt_cli.c          # cp_prompt CLI (shortest decode + speaker self-test)
│   ├── audio_param/
│   └── config/
└── CMakeLists.txt
```

Required CP components: `cp/components/audio_play` and `cp/components/audio_record`, selected by `CONFIG_AUDIO_PLAY` / `CONFIG_AUDIO_RECORD`. Both are ported from the AP components of the same name; the only differences are the removal of the `avdk_utils` dependency, which does not exist on CP, and the CPU frequency vote lowered to `PM_CPU_FRQ_240M`, the highest value in the CP enum.

Two changes in `cp/components/bk_audio` are **deliberately CP-only and are not mirrored to AP**, because AP is the primary audio validation target and the risk there is higher. Do not overwrite them as if CP were lagging behind when upgrading or diffing against AP.

- `audio_streams/onboard_speaker_stream/v2.0/onboard_speaker_stream_v2.c`: the DAC channel count is fixed at init and `dest_data_width` is no longer rewritten at runtime, a mono main source is up-mixed to L=R on the read path, and a sample rate change now derives the input frame size from the fixed DMA output block. Fixes the crash caused by reprogramming a live REPEAT-mode DMA channel during three-source mixing.
- `audio_streams/raw_stream/raw_stream.c`: `RAW_READ_DATA_COUNT` / `RAW_WRITE_DATA_COUNT` are disabled. Their `count_util_t` is a file static while `raw_stream` has no instance limit, so opening several instances concurrently leaves an orphan timer that asserts in `timer_callback1` one interval later.

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

Flash `all-app.bin` with the normal flashing tool.

Note: `primary_cp_app` was grown from 1600K to 2048K because the added CP audio components left only about 10 KB of headroom. Everything after it (`primary_ap_app`, `ota`, `usr_config`, `easyflash`, `easyflash_ap`) shifts up by 448K, so **the whole image must be flashed, not just the app partition**. The relocated `easyflash` means previously stored key/value data is no longer found and is re-initialised as blank on first boot. `sys_rf` (0x7FE000) and `sys_net` (0x7FF000) use fixed offsets and do not move, so RF calibration and the MAC survive — but do not run a manual full-chip erase, which would wipe them.

After boot, check the CP serial log for:

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

### 6.5 ASR Service Path

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

### 6.6 audio_play / audio_record Component Validation

These two commands drive the `audio_play` / `audio_record` components directly, bypassing the voice and player services. They are the shortest path for validating the components themselves.

`cp_audio_play`:

```
cp_audio_play open [rate] [chans] [vol_db]    # defaults: 16000 1 -7
cp_audio_play tone [ms]                       # rate/16 Hz sine, default 1000ms
cp_audio_play clip                            # built-in 16k mono PCM prompt
cp_audio_play volume <db>
cp_audio_play pause|resume|mute|unmute
cp_audio_play close
cp_audio_play info
```

`clip` requires 16000 Hz mono; opening with other parameters makes it refuse with a message.

Expected:

- `cp_audio_play open ok: 16000 Hz 1 ch, frame <N> B, vol -7 dB`
- `tone` plays a `rate/16` Hz sine (1 kHz by default) on the speaker and ends with `tone done: <N>/<N> frames, ret=0`; the two frame counts must match and `ret` must be 0, while a `write short at frame` message means the speaker side is back-pressuring abnormally
- `clip` prints `clip done: <total bytes>, ret=0` and the whole wake-up prompt is audible
- After `close`, the speaker owner in `cp_aud_res` returns to `none`

`cp_audio_record`:

```
cp_audio_record open [rate] [chans] [gain_db] # defaults: 16000 1 16
cp_audio_record read [ms]                     # reports captured bytes and peak, default 1000ms
cp_audio_record gain <db>
cp_audio_record pause|resume
cp_audio_record close
cp_audio_record info
cp_audio_record loop [ms] [rate]              # mic -> spk_service CALL, defaults 3000ms 16000
```

The peak printed by `read` is the key signal that the microphone is really working: near zero when silent, clearly jumping when you speak. `loop` takes both the mic and the speaker and requires the mic to be idle, otherwise it reports `loop needs the mic, close it first`; the same applies to the speaker, so `close` `cp_audio_play` first if it is still open.

Expected:

- `cp_audio_record open ok: 16000 Hz 1 ch, frame <N> B, gain 16 dB`
- `read 1000` prints `read done: 32000 bytes, peak <P>, ret=0`; the byte count must equal `rate x chans x 2 x ms / 1000`, which is 32000 with the defaults
- `peak` jumps clearly while you speak into the mic and stays near zero when silent; a peak that is always 0 means the mic is not actually capturing
- `loop` plays your own voice back through the speaker and ends with `loop done: <N>/<N> frames, ret=0`
- After `close`, the mic owner in `cp_aud_res` returns to `none`

### 6.7 spk_service Three-Source Mixing

`cp_spk` drives `spk_service` the way the headset project does. The three sources (A2DP/CALL/HINT) are mixed by the DAC hardware, and this is the peak memory scenario of the project.

```
cp_spk init | deinit
cp_spk <a2dp|call|hint> tone [ms] [rate]      # raw PCM into the source FIFO
cp_spk <a2dp|call|hint> mp3 | wav             # decode path, same as BT music/call
cp_spk mix [ms]                               # all three at once, peak memory
cp_spk stop [a2dp|call|hint]                  # no argument stops all
cp_spk volume <db> | mute <0|1> | srcmute <a2dp|call|hint> <0|1>
cp_spk info
```

Run `memshow` before and after `cp_spk mix` to observe the PSRAM heap peak.

### 6.8 Shortest Playback Self-Test

`cp_prompt` carries a short 16 kHz mono G711A clip and builds its own `raw_stream -> g711_decoder -> onboard_speaker_stream_v2` pipeline, depending on none of voice_service, player_service, or spk_service.

```
cp_prompt play [ms]                           # default 500ms, max 3000ms
```

Use it to bisect a silent speaker: if `cp_prompt play` is audible, the DAC and the decode chain are fine and the problem is in the upper services.

### 6.9 Resource Arbitration

The speaker and the microphone are single-instance hardware. Every audio CLI registers its claim through `cp_audio_res`, and a conflicting caller is rejected with the current owner printed.

```
cp_aud_res                                    # show the current speaker/mic owners
```

Resource usage per command:

| Command | Speaker | Mic | Owner | Returned by |
|---|---|---|---|---|
| `cp_audio_play open` | yes | — | `cp_audio_play` | `cp_audio_play close` |
| `cp_audio_record open` | — | yes | `cp_audio_record` | `cp_audio_record close` |
| `cp_audio_record loop` | yes | yes | `spk_service` + `cp_audio_record` | mic automatically; speaker needs `cp_spk deinit` |
| `cp_spk init` or any `cp_spk` playback | yes | — | `spk_service` | `cp_spk deinit` |
| `voice_service start` | yes | yes | `voice_service` | `voice_service stop` |
| `player_service playback start` | yes | — | `player_service` | automatically when playback ends |
| `cp_prompt play` | yes | — | `cp_prompt` | automatically when playback ends |
| `audio asr_turn_on` | — | yes | `asr` | `audio asr_turn_off` |

`spk_service` is the one exception: it is a resident service, not a per-job resource. Everything that needs it (the `cp_spk` subcommands, `cp_audio_record loop`) only ensures it is up, the speaker token is held by `cp_spk` on their behalf, and **only `cp_spk deinit` returns it**.

That is deliberate. `SPK_SERVICE_IDLE_LINGER_MS` (default 5000) keeps the DAC alive for another 5 seconds after the last source detaches, to bridge short gaps such as track changes or call-to-music and avoid the pop of a back-to-back DAC re-init. If every job returned the token on completion, there would be a 5-second window where the token reads free while the DAC is still running; another path initialising the speaker in that window would contend for the same global state as the still-running instance. Holding the token for at least as long as the hardware is alive removes the window.

So the rule for checking cleanup is:

- Every owner other than `spk_service` must return to `none` after a test group; anything else means a path missed its release.
- Seeing `speaker owner=spk_service` is normal. Run `cp_spk deinit` to return it.

Typical mutual-exclusion cases, where being rejected with an owner printed is the expected result rather than a crash:

```
cp_spk init
cp_audio_play open          # expected: rejected, owner=spk_service
cp_spk deinit

voice_service start onboard 16000 0 pcm pcm onboard 16000 1 call
cp_spk init                 # expected: rejected, owner=voice_service
cp_audio_record open        # expected: rejected, mic held by voice_service
audio asr_turn_on 0 0 16000 1   # expected: rejected, mic conflict
voice_service stop
cp_aud_res                  # expected: everything back to none
```

After `loop` the speaker still belongs to `spk_service`. Confirm that this is returnable rather than leaked:

```
cp_audio_record loop 3000 16000
cp_aud_res                  # expected: speaker owner=spk_service refs=1, mic owner=none
cp_audio_play open          # expected: rejected, owner=spk_service
cp_spk deinit
cp_aud_res                  # expected: both speaker and mic are none
```

Running `loop` twice before `cp_spk deinit` must still show `refs=1`, not `refs=2`: the token is taken once, when the service first comes up.

Note: the `aud` CLI shipped inside the `audio_play` component does not go through `cp_audio_res`, so this project **deliberately leaves it unregistered** to prevent it from double-initialising the shared speaker alongside `cp_spk`. To validate the component on its own, call `audio_play_cli_init()` from `cp_main.c` the way `projects/bluetooth/headset` does.

## 7. Diagnostics and Notes

- If `voice_service start` returns an error, check the argument count and whether the codec combination is supported.
- For loopback noise, enable `audio_obs all 5000` and check whether mic, eq, or spk has continuous short/timeout/error counters or abnormal p95/p99 cost.
- For playback or prompt discontinuity, focus on `ONBOARD_SPK` and decoder/raw stream statistics.
- If ASR logs include `kws=stub` or `lib=not_linked`, the algorithm library is not linked, which is expected. The imported CPU KWS library is built for AP/M55 hard-float with MVE while CP/M52 is soft-float, so it cannot be linked and `CONFIG_CP_KWS_CPU_LIB` stays off. Until an M52 build exists, `audio asr_turn_on` is the only meaningful ASR validation path: it runs the full mic/AEC/asr_service chain and only the final recognition callback is a stub.
- A command rejected with `owner=xxx` is the `cp_audio_res` arbitration doing its job. Stop the current owner and retry; see section 6.9.
- If wireless activity affects audio validation, run `AT+BLEPOWER=0` before testing.

