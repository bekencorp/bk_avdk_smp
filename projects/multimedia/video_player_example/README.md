# Video Recorder and Player Sample Project

* [中文](./README_CN.md)

This project demonstrates the end-to-end multimedia file pipeline on BK7259:

- Record camera video and microphone audio to SD card files.
- Play AVI/MP4 files from SD card.
- Display video on the LCD panel.
- Output audio through the onboard speaker.

## Hardware Requirements

- **SoC/board**: BK7259 series.
- **Storage**: SD card with FATFS/FAT32, mounted at `/sd0`.
- **Display**: MIPI LCD panel configured by the board profile.
- **Camera**: MIPI camera configured by the board profile.
- **Audio**: onboard microphone and onboard speaker.

The exact sensor, panel, resolution, and frame rate come from the project board configuration.

## Build

Build the example:

```bash
make bk7259 PROJECT=multimedia/video_player_example
```

After flashing, send CLI commands through UART:

```bash
ap_cmd <command>
```

Successful commands return `CMDRSP:OK`; failed commands return `CMDRSP:ERROR`.

## CLI Overview

This project provides three CLI command groups:

- `video_record`: record camera and audio to a file.
- `video_play_engine`: play one file through the player engine layer.
- `video_play_playlist`: play files through the playlist layer.

## Recording

Entry:

```bash
ap_cmd video_record ...
```

Usage:

```bash
ap_cmd video_record start [file_path] [width] [height] [mjpeg|h264] [avi|mp4] [pcm|aac|g711a|g711u|g722]
ap_cmd video_record stop
```

Parameters:

- `file_path`: output file path. Default is `/sd0/record.avi`.
- `width height`: recording resolution. Default is `480 320`.
- `mjpeg`: record MJPEG video. This is the default video format.
- `h264`: record H.264 video.
- `avi`: force AVI container.
- `mp4`: force MP4 container.
- `pcm`: record PCM audio. This is the default audio format.
- `aac`: record AAC audio.
- `g711a`, `g711u`, `g722`: record the selected compressed audio format.

Container selection rules:

- MJPEG defaults to AVI unless `mp4` is specified.
- H.264 defaults to MP4. If the output path ends in `.avi`, the suffix is changed to `.mp4`.
- H.264 can be forced into AVI by passing `avi`, but MP4 is the recommended container.

Examples:

```bash
# MJPEG + AVI + PCM
ap_cmd video_record start /sd0/recordmjpeg.avi 640 480 mjpeg

# MJPEG + AVI + G.711 A-law
ap_cmd video_record start /sd0/recordmjpeg.avi 640 480 mjpeg g711a

# MJPEG + MP4 + AAC
ap_cmd video_record start /sd0/recordmjpeg.mp4 640 480 mjpeg mp4 aac

# H.264 + MP4 + PCM, 1080p
ap_cmd video_record start /sd0/record1080.mp4 1920 1080 h264

# Stop recording
ap_cmd video_record stop
```

Recording notes:

- The CLI does not currently expose a separate FPS argument.
- `record_framerate` is taken from the configured camera sensor FPS.
- The recorder has internal frame-rate limiting based on `record_framerate`.
- Recording automatically stops after the configured maximum duration, currently 5 minutes.
- For H.264, the recorder waits for the first IDR frame before starting the recording timeline.
- For H.264, invalid AnnexB access units are dropped before writing, so incomplete or malformed frames are not written into the container.
- The recorder tries to mount the SD card before starting. Make sure the card is inserted and writable.
- Recording uses camera, encoder, SD card, audio, and display resources. Stop conflicting services before recording.

## Playback

The player supports AVI/MP4 containers, MJPEG video, H.264 video, and supported audio formats based on the enabled decoders.

### Engine CLI

Entry:

```bash
ap_cmd video_play_engine ...
```

Usage:

```bash
ap_cmd video_play_engine start [file_path] [frame|gpu|flexa] [norotate|rotate90|rotate270]
ap_cmd video_play_engine stop
ap_cmd video_play_engine pause
ap_cmd video_play_engine resume
ap_cmd video_play_engine seek <time_ms>
ap_cmd video_play_engine ff <time_ms>
ap_cmd video_play_engine rewind <time_ms>
ap_cmd video_play_engine avsync <offset_ms>
ap_cmd video_play_engine volume <0-100>
ap_cmd video_play_engine vol_up <step>
ap_cmd video_play_engine vol_down <step>
ap_cmd video_play_engine mute <on|off>
ap_cmd video_play_engine status
ap_cmd video_play_engine info [file_path]
```

H.264 decoder mode:

- `flexa` or `gpu`: H.264 Flexa/GPU bonded path. This is the default.
- `frame`: H.264 frame decoder path.

Rotation:

- `norotate` or `rotate0`: no rotation.
- `rotate90`, `rot90`, or `r90`: rotate 90 degrees.
- `rotate270`, `rot270`, or `r270`: rotate 270 degrees.

Examples:

```bash
# Play MP4/H.264 with default Flexa GPU decoder
ap_cmd video_play_engine start /sd0/record1080.mp4

# Play MP4/H.264 with Flexa GPU decoder and rotate 90 degrees
ap_cmd video_play_engine start /sd0/record1080.mp4 flexa rotate90

# Play MP4/H.264 with frame decoder and rotate 90 degrees
ap_cmd video_play_engine start /sd0/record1080.mp4 frame rotate90

# Play MJPEG AVI and rotate 90 degrees
ap_cmd video_play_engine start /sd0/recordmjpeg1080.avi rotate90

# Query file information
ap_cmd video_play_engine info /sd0/record1080.mp4

# Playback controls
ap_cmd video_play_engine pause
ap_cmd video_play_engine resume
ap_cmd video_play_engine seek 5000
ap_cmd video_play_engine ff 3000
ap_cmd video_play_engine rewind 3000
ap_cmd video_play_engine avsync -200
ap_cmd video_play_engine volume 80
ap_cmd video_play_engine mute on
ap_cmd video_play_engine mute off
ap_cmd video_play_engine stop
```

Playback notes:

- The Flexa/GPU path outputs compressed ARGB8888 for display.
- The H.264 frame path outputs NV12 from the decoder. When rotate90 or rotate270 is requested, the example applies full-frame GPU post-processing before display.
- MJPEG rotate90/rotate270 also uses the common full-frame GPU post-processing path.
- If video decoding is slower than the audio clock, the player may drop video frames to keep A/V sync.
- H.264 catch-up waits for the next IDR frame before resuming decode, which avoids resuming from a broken P-frame reference chain.

### Playlist CLI

Entry:

```bash
ap_cmd video_play_playlist ...
```

Usage:

```bash
ap_cmd video_play_playlist start [file_path]
ap_cmd video_play_playlist stop
ap_cmd video_play_playlist pause
ap_cmd video_play_playlist resume
ap_cmd video_play_playlist add <file_path>
ap_cmd video_play_playlist remove <file_path>
ap_cmd video_play_playlist clear
ap_cmd video_play_playlist next
ap_cmd video_play_playlist prev
ap_cmd video_play_playlist play <file_path|index>
ap_cmd video_play_playlist list
ap_cmd video_play_playlist status
ap_cmd video_play_playlist seek <time_ms>
ap_cmd video_play_playlist ff <time_ms>
ap_cmd video_play_playlist rewind <time_ms>
ap_cmd video_play_playlist avsync <offset_ms>
ap_cmd video_play_playlist info [file_path]
ap_cmd video_play_playlist play_mode <stop|repeat|loop>
ap_cmd video_play_playlist volume <0-100>
ap_cmd video_play_playlist vol_up <step>
ap_cmd video_play_playlist vol_down <step>
ap_cmd video_play_playlist mute <on|off>
```

Examples:

```bash
ap_cmd video_play_playlist start /sd0/record1080.mp4
ap_cmd video_play_playlist add /sd0/recordmjpeg.avi
ap_cmd video_play_playlist play_mode loop
ap_cmd video_play_playlist next
ap_cmd video_play_playlist prev
ap_cmd video_play_playlist stop
```

Playlist notes:

- Call `start` before `add`, `remove`, `next`, `prev`, or `play`.
- If `add` is called before `start`, the command reports `Video player playlist not started`.
- Track switching resets audio output in the example to avoid stale buffered audio.

## Common Workflows

Record H.264 1080p and play it back:

```bash
ap_cmd video_record start /sd0/record1080.mp4 1920 1080 h264
ap_cmd video_record stop
ap_cmd video_play_engine start /sd0/record1080.mp4 flexa rotate90
```

Record MJPEG 1080p AVI and play it back:

```bash
ap_cmd video_record start /sd0/recordmjpeg1080.avi 1920 1080 mjpeg avi
ap_cmd video_record stop
ap_cmd video_play_engine start /sd0/recordmjpeg1080.avi rotate90
```

## Troubleshooting

- `CMDRSP:ERROR`: check the detailed UART log before the response.
- SD mount failure: confirm the SD card is inserted, formatted, and mounted as `/sd0`.
- Recording stops early: check the auto-stop timer and SD card write errors.
- Playback is choppy: 1080p H.264/MJPEG with rotation is expensive; the player may drop frames to keep audio synchronized.
- Playlist command reports `Video player playlist not started`: run `video_play_playlist start` first.

## Related Documentation

- [Video File Recording](../../../developer-guide/multimedia/video_file_recording.html)
- [Video File Playback](../../../developer-guide/multimedia/video_file_playback.html)
- [Video Recorder API](../../../api-reference/multimedia/bk_video_record.html)
- [Video Player API](../../../api-reference/multimedia/bk_video_player.html)