# Audio Player Example

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates the BK7259 `bk_audio_player` component. The player
runs on the AP, supports file, network, and HLS sources, decodes MP3, WAV, AAC,
and TS audio, and uses the onboard speaker sink.

## 2. Hardware and Console

- BK7259 development board with an onboard speaker.
- An SD card is required for local playback; it is mounted at `/sd0`.
- Network connectivity is required for network or HLS sources.
- Connect the serial terminal to the CP UART at 115200 baud.
- The player CLI runs on the AP, so prefix commands entered on the CP console
  with `ap_cmd`.

## 3. Project Structure

```text
audio_player_example/
├── ap/
│   ├── ap_main.c
│   ├── audio_player_test/
│   │   └── cli_audio_player.c
│   └── config/bk7259_ap/defconfig
├── cp/
│   ├── cp_main.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
```

## 4. Build and Flash

```bash
make bk7259 PROJECT=multimedia/audio_player_example
```

The generated image is:

```text
build/bk7259/audio_player_example/package/all-app.bin
```

Flash the image using the standard BK7259 procedure.

## 5. CLI Commands

Use `ap_cmd audio_player <subcommand> [parameters]`.

| Subcommand | Parameters | Purpose |
| --- | --- | --- |
| `init` / `deinit` | None | Create or delete the player |
| `add` | `<name> <uri>` | Add an item |
| `rm` | `<uri>` | Remove an item by URI |
| `clear` / `dump` | None | Clear or print the playlist |
| `mode` | `<mode>` | Set the play mode |
| `volume` | `[value]` | Set or read the volume |
| `start` / `stop` | None | Start or stop playback |
| `pause` / `resume` | None | Pause or resume playback |
| `prev` / `next` | None | Select the previous or next item |
| `jump` | `<id>` | Jump to an item |
| `sd_mount` / `sd_unmount` | None | Mount or unmount the SD card |
| `sd_scan` | None | Scan `/sd0` and add media files |

SD card playback example:

```text
ap_cmd audio_player init
ap_cmd audio_player sd_mount
ap_cmd audio_player sd_scan
ap_cmd audio_player dump
ap_cmd audio_player start
```

Adding file and network URIs directly:

```text
ap_cmd audio_player init
ap_cmd audio_player add local /sd0/test.mp3
ap_cmd audio_player add network http://example.com/test.mp3
ap_cmd audio_player start
```

A successful command returns `CMDRSP:OK`; a failed command returns
`CMDRSP:ERROR`.

## 6. Key Configuration

The main options are in `ap/config/bk7259_ap/defconfig`:

- `CONFIG_MEDIA_SERVICE`
- `CONFIG_AUDIO_PLAYER`
- `CONFIG_SDCARD`
- `CONFIG_FATFS`
- `CONFIG_VFS`
- `CONFIG_WEBCLIENT`
