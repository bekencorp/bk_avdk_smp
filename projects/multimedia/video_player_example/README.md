# Video Recorder and Player Sample Project (video_player_example)

* [中文](./README_CN.md)

## Hardware requirements

- **SoC/board**: BK7258 series (this project uses `bk7258` as the build/run example)
- **Storage**: SD card (FATFS/FAT32 recommended), mount point `/sd0`
- **Display**: RGB LCD (the sample uses `st7282`, 480x272 by default)
- **Camera**: DVP camera (sample logs use `gc2145` as an example; must match project configuration)
- **Audio**: onboard microphone + onboard speaker


## Related documentation

* For detailed information about video file recording/playback, please refer to:

  - [Video File Recording](../../../developer-guide/multimedia/video_file_recording.html)
  - [Video File Playback](../../../developer-guide/multimedia/video_file_playback.html)

* For API reference about video file recording/playback, please refer to:

  - [Video Recorder API](../../../api-reference/multimedia/bk_video_record.html)
  - [Video Player API](../../../api-reference/multimedia/bk_video_player.html)

## Video recorder and player

This project provides CLI test commands for both player and recorder:

- **Player**: `video_play_engine` (engine layer) and `video_play_playlist` (playlist layer with play mode)
- **Recorder**: `video_record`

### Player (video_play_engine / video_play_playlist)

This project provides two playback control layers:

- **Engine layer**: `video_play_engine`, directly controls `bk_video_player_engine` for pipeline validation.
- **Playlist layer**: `video_play_playlist`, controls `bk_video_player_playlist` (playlist + play mode) for application-level control.

It supports AVI/MP4 containers, JPEG (MJPEG) video, and PCM/AAC audio (selected by file and decoder availability).

#### Engine CLI (video_play_engine)

Entry: `ap_cmd video_play_engine ...`

- `start [file_path]`
- `stop`
- `pause` / `resume`
- `seek <time_ms>`
- `ff <time_ms>` / `rewind <time_ms>`
- `avsync <offset_ms>`: set A/V sync offset in ms (range [-5000, 5000])
- `volume <0-100>` / `vol_up <step>` / `vol_down <step>`
- `mute <on|off>`
- `status`
- `info [file_path]`: query media information (can be used before playback)

Examples:

```bash
ap_cmd video_play_engine start /sd0/record.mp4
ap_cmd video_play_engine info /sd0/record.mp4
ap_cmd video_play_engine ff 3000
ap_cmd video_play_engine rewind 3000
ap_cmd video_play_engine seek 500
ap_cmd video_play_engine avsync -200
ap_cmd video_play_engine volume 80
ap_cmd video_play_engine mute on
ap_cmd video_play_engine mute off
ap_cmd video_play_engine stop
```

#### Playlist CLI (video_play_playlist)

Entry: `ap_cmd video_play_playlist ...`

Notes:

- **Call `start` before `add`**. Otherwise you will see `Video player playlist not started`.
- `next/prev/play` switches tracks. To avoid stale buffered audio, the **playlist layer** performs an audio output reset inside the switching APIs via the external `audio_output_reset_cb` (stop + flush + start).

Common commands:

- `start [file_path]`
- `stop`
- `pause` / `resume`
- `add <file_path>` / `remove <file_path>` / `clear`
- `next` / `prev`
- `play <file_path|index>`
- `list`
- `status`
- `seek <time_ms>`
- `ff <time_ms>` / `rewind <time_ms>`
- `avsync <offset_ms>`: set A/V sync offset in ms (range [-5000, 5000])
- `info [file_path]`
- `play_mode <stop|repeat|loop>`
- `volume <0-100>` / `vol_up <step>` / `vol_down <step>`
- `mute <on|off>`

Example (playlist + loop):

```bash
ap_cmd video_play_playlist start /sd0/record.mp4
ap_cmd video_play_playlist add /sd0/record.avi
ap_cmd video_play_playlist play_mode loop
ap_cmd video_play_playlist avsync -200
ap_cmd video_play_playlist next
```

Troubleshooting:

- `Video player playlist not started`
  - Cause: `video_play_playlist add/remove/next/...` is called before `start`.
  - Fix: run `ap_cmd video_play_playlist start [file_path]` first.
- Audio tail / lag after repeatedly sending `next`
  - Symptom: `voice_write_task_main: voice write start N` grows and perceived audio lags behind video.
  - Fix: clear voice_write internal buffer during switching (stop + flush + start). This is integrated in the example project.

### Recorder (video_record)

The `video_record` CLI records **DVP camera video + microphone audio** into a file on the SD card.

- **Container**: AVI (default) / MP4 (by keyword)
- **Video encoding**: MJPEG (default) / H264 (by keyword)
- **Audio**: PCM (default) / AAC (enable by `aac`)
- **Preview**: YUV frames can be flushed to LCD during recording

Entry: `ap_cmd video_record ...`

Start recording:

```bash
ap_cmd video_record start [file_path] [width] [height] [format] [type]
```

Stop recording:

```bash
ap_cmd video_record stop
```

Start parameters:

- **file_path**: output path, default `/sd0/record.avi`
- **width/height**: resolution, default `480 320`
- **format**: video format keyword
  - `mjpeg` / `jpeg`: MJPEG
  - `h264`: H264
- **type**: container keyword
  - `mp4`: MP4 container (auto replaces `.avi` suffix with `.mp4`)
  - omitted: AVI container
- **aac**: enable AAC audio encoding

Examples:

1) AVI + MJPEG + PCM (default):

```bash
ap_cmd video_record start /sd0/record.avi 480 320 mjpeg
```

2) AVI + MJPEG + AAC:

```bash
ap_cmd video_record start /sd0/record.avi 480 320 mjpeg aac
```

3) MP4 + MJPEG + AAC:

```bash
ap_cmd video_record start /sd0/record.mp4 480 320 mjpeg mp4 aac
```

Notes:

- The recorder tries to mount the SD card before starting. Ensure the SD card and filesystem are ready.
- To avoid blocking in the DVP callback, encoded frames are pushed into a queue and consumed by the recording thread. If the queue is full, frames are dropped and statistics are printed periodically.
- Recording consumes DVP/LCD/voice resources. If other voice services are running, they may conflict; stop them before recording.

## Project Notes

This is the `video_player_example` project, which demonstrates the end-to-end pipeline of **recording (`video_record`) and playback (`video_play_engine` / `video_play_playlist`)** (SD card file + demux/decoder + LCD display + audio output).

## Build and Run

Build:

```bash
make bk7258 PROJECT=video_player_example
```

Run:

- After flashing, use UART CLI to run `ap_cmd video_record ...`, `ap_cmd video_play_engine ...`, `ap_cmd video_play_playlist ...`
- Success: `CMDRSP:OK`
- Failure: `CMDRSP:ERROR`