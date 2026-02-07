# LVGL Music Project Overview

This project is based on the official LVGL music player demo and integrates the `bk_audio_player` component to provide a local music player experience on BK7258.

## 1. Directory Layout
```
music/
├── CMakeLists.txt                 # Top-level CMake entry
├── Makefile                       # Top-level Make entry
├── pj_config.mk                   # Project configuration switches
├── ap/                            # AP-core application sources
│   ├── ap_main.c                  # LVGL init, LCD bring-up, touch configuration
│   ├── lv_demo_music*.c/.h        # LVGL music UI, playback control, list management
│   ├── cli_lv_demo_music.*        # lv_music CLI commands and stress logic
│   ├── assets/                    # Spectrum textures, logos, and other UI assets
│   └── config/                    # AP-side BK7258 configuration
├── cp/                            # CP-core helper that boots CP1
│   ├── cp_main.c
│   └── config/
├── partitions/                    # Flash and RAM layout
│   └── bk7258/
└── .ci/.gitignore/.it.csv         # CI scripts and ignore lists
```

## 2. Features

### 2.1 Graphical Interface
- Custom layout defined in `lv_demo_music_main.c`, including album artwork, spectrum animation, progress slider, loop/random buttons, and a scrollable playlist.
- Supports the 480x854 ST7701S RGB LCD (configured via `lcd_device_st7701s` in `ap_main.c`) and optional touch panel through `CONFIG_TP`.
- Uses the `lv_vendor` framework to enable multi-frame buffering and partial refresh for smooth animations.

### 2.2 Audio Pipeline
- When `CONFIG_AUDIO_PLAYER` is enabled, `lv_demo_music.c` mounts `/sd0` (FATFS), recursively scans the SD card, and enqueues discovered tracks to `bk_audio_player`.
- Metadata such as title, artist, genre, and duration are retrieved via `bk_audio_metadata_get_from_file`; filenames are parsed as a fallback (`lv_demo_music_list_mgr.c`).
- Supported audio formats are controlled by Kconfig (MP3, WAV, AAC, M4A, FLAC, OGG, OPUS, AMR, TS, etc.); only enabled decoders are added to the playlist.
- UI and audio player stay in sync through event callbacks for progress updates, track switching, pause/resume, and seek operations.
- If no SD card is inserted or no audio files are found, the UI still renders while playback gracefully falls back to placeholder content.

## 3. Supported Interactions

### 3.1 Touch UI
- **Play/Pause**: Tap the center button to trigger `_lv_demo_music_play` / `_lv_demo_music_pause`, which controls `bk_audio_player`.
- **Previous/Next**: Use the left/right arrows to call `_lv_demo_music_album_next`; in addition to sequential looping, random mode is also supported.
- **Seek**: Drag the slider to invoke `_lv_demo_music_request_seek`; LVGL finishes the animation before the audio thread executes the actual `bk_audio_player_seek`.
- **Loop Mode**: The loop button keeps sequential looping enabled, while the random button toggles `AUDIO_PLAYER_MODE_RANDOM` to minimize repeats.
- **Playlist Selection**: Scroll the list to any track and tap to play it; when `LV_DEMO_MUSIC_AUTO_PLAY` is enabled, the UI cycles automatically.

## 4. Prerequisites
- **Hardware**: BK7258 development board + ST7701S RGB LCD + optional touch panel + SD card module + speaker/amplifier.
- **Storage**: FAT/FAT32 SD card mounted at `/sd0`; place supported audio files in the root or subdirectories for automatic scanning.
- **Software Configuration**:
  - Enable `CONFIG_LVGL`, `CONFIG_LV_USE_DEMO_MUSIC`, `CONFIG_AUDIO_PLAYER`, and the required decoders via menuconfig.
  - If touch input is required, enable `CONFIG_TP` and the corresponding device driver.
  - Adjust GPIO assignments for LCD/touch as needed inside `bk_display_rgb_ctlr_config_t` and the backlight helper in `ap_main.c`.

## 5. Build & Flash
```
make bk7258 PROJECT=lvgl/music
```
Flash the generated firmware to the board. After power-up, the LVGL UI starts automatically. Verify the AP UART baud rate so that logs and prompts remain readable.

## 6. Runtime Flow
1. Power off the board, insert a prepared SD card, and connect LCD, touch, and audio peripherals.
2. Power on and watch the console for `Mounted SD card to /sd0` and `Audio player initialized successfully`.
3. Once the LVGL intro completes, interact with the UI directly via touch.
4. After testing, remove the SD card or run `reboot` in the console to restart the demo.

## 7. Troubleshooting
- **No audio / Empty playlist**: Check the SD card filesystem and ensure the desired formats are enabled in Kconfig; use `ls /sd0` to confirm mounting if you see `No music files found`.
- **Touch not working**: Make sure `CONFIG_TP` is enabled and the I2C wiring is correct; adjust resolution or mirror parameters in `drv_tp_open` if needed.
- **Seek stutter**: If rapid fast-forward/rewind operations cause glitches, reduce the seek frequency or increase the interval between drags to avoid overloading the audio thread.

