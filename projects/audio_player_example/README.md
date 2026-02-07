# Audio Player Example Project

* [中文](./README_CN.md)

## 1. Project Overview
This project is an audio player test module designed to test the functionality of the bk_audio_player component on the Beken platform. This module provides complete audio playback functionality, including playlist management, playback control, volume adjustment, playback mode settings, and more. It can be configured and controlled through the Command Line Interface (CLI). It supports reading and playing various formats of audio files from SD card.

* For the API reference of the audio player service, please refer to:

  - [Audio Player Service API Reference](../../../api-reference/audio/audio_player/index.html)

* For the development guide of the audio player service, please refer to:

  - [Audio Player Service Development Guide](../../../developer-guide/audio/audio_player/index.html)


### 1.1 Test Environment

   * Hardware Configuration:
      * Core board, **BK7258_QFN88_9X9_V3.2**
      * SD card with mp3 music files
      * Speaker V1 board and speaker


## 2. Directory Structure
The project adopts an AP-CP dual-core architecture, with main source code located in the AP directory. The project structure is as follows:
```
audio_player_example/
├── .ci                             # CI configuration directory
├── .gitignore                      # Git ignore file configuration
├── .it.csv                         # Integration test case configuration file
├── CMakeLists.txt                  # Project-level CMake build file
├── Makefile                        # Make build file
├── README.md                       # English README
├── README_CN.md                    # Chinese README
├── app.rst                         # Application description file
├── ap/                             # AP core code directory
│   ├── CMakeLists.txt              # AP CMake build file
│   ├── ap_main.c                   # Main entry file, implementing initialization and CLI command registration
│   ├── config/                     # AP configuration directory
│   │   └── bk7258_ap/              # BK7258 AP configuration
│   └── audio_player_test/          # Audio player test module directory
│       └── cli_audio_player.c      # Implementation of audio player CLI commands
├── cp/                             # CP core code directory
│   ├── CMakeLists.txt              # CP CMake build file
│   ├── cp_main.c                   # CP-side main file
│   ├── config/                     # CP configuration directory
│   │   └── bk7258/                 # BK7258 CP configuration
│   ├── customer_msg/               # Customer message processing module
│   │   ├── customer_msg.c          # Customer message processing implementation
│   │   └── customer_msg.h          # Customer message processing header
│   └── doorbell_service/           # Doorbell service module
│       ├── doorbell_comm.h         # Doorbell communication header
│       └── doorbell_core.c         # Doorbell core implementation
├── partitions/                     # Partition configuration directory
│   └── bk7258/                     # BK7258 partition configuration
│       ├── auto_partitions.csv     # Auto partition configuration
│       └── ram_regions.csv         # RAM regions configuration
└── pj_config.mk                    # Project configuration file
```

## 3. Function Description
### 3.1 Main Functions
   - **Audio Format Support**: Supports multiple audio formats such as MP3, WAV, M4A, AMR, TS, and AAC
   - **Playback Control**: Supports basic playback controls such as play, pause, resume, and stop
   - **Playlist Management**: Supports adding, deleting, and clearing playlist
   - **Playback Mode**: Supports multiple playback modes such as sequential playback, single loop, list loop, etc.
   - **Volume Control**: Supports volume adjustment and query
   - **Playback Navigation**: Supports previous, next, and jump to specified song
   - **SD Card Support**: Supports SD card mounting, unmounting, and automatic scanning of audio files
   - **File System**: Based on VFS and FATFS file systems, supports FAT format SD cards

### 3.2 Audio Player Component Architecture
   The bk_audio_player component provides a complete audio playback framework:
      - **Player Core**: Manages the entire audio playback lifecycle
      - **Decoder**: Supports decoding of multiple audio formats
      - **Playlist Management**: Maintains and manages the playback queue
      - **Audio Output**: Responsible for audio data output to speakers
      - **Configuration Management**: Supports flexible playback parameter configuration

### 3.3 Playback Mode Description

The audio player supports multiple playback modes:

- **Sequential Playback Mode**: Plays songs in playlist order, stops after the last song
- **Single Loop Mode**: Repeats the current song
- **List Loop Mode**: Loops through the entire playlist, continues from the first song after the last one
- **Random Playback Mode**: Randomly selects songs from the playlist to play


## 4. Compilation and Execution
### 4.1 Compilation Method
Use the following command to compile the project:
```
make bk7258 PROJECT=audio_player_example
```

### 4.2 Running Method
After the compilation is complete, flash the generated firmware to the development board, and then use the following commands through the serial terminal to test the audio player functionality:

#### 4.2.1 Mount SD Card
**Command Format**:
```
audio_player sd_mount
```

**Function Description**: Mount the SD card to the /sd0 directory, enabling the audio player to access audio files on the SD card.

**Example**:
```
ap_cmd audio_player sd_mount
```

#### 4.2.2 Initialize Audio Player
**Command Format**:
```
audio_player init
```

**Function Description**: Initialize the audio player with default configuration to prepare the playback environment.

**Example**:
```
ap_cmd audio_player init
```

#### 4.2.3 Add Audio File to Playlist
**Command Format**:
```
audio_player add [id] [file_path]
```

**Parameter Description**:
- ``id`` : Unique identifier for the audio file
- ``file_path`` : Full path to the audio file (e.g., /sd0/test.mp3)

**Example**:
```
ap_cmd audio_player add 1 /sd0/test.mp3
```

#### 4.2.4 Scan SD Card Audio Files
**Command Format**:
```
audio_player sd_scan
```

**Function Description**: Automatically scan audio files in the SD card root directory and first-level subdirectories, and add them to the playlist.

**Example**:
```
ap_cmd audio_player sd_scan
```

#### 4.2.5 Playback Control

##### Start Playback
**Command Format**:
```
audio_player start
```

**Function Description**: Start playing the first song in the playlist.

**Example**:
```
ap_cmd audio_player start
```

##### Pause Playback
**Command Format**:
```
audio_player pause
```

**Function Description**: Pause the currently playing song.

**Example**:
```
ap_cmd audio_player pause
```

##### Resume Playback
**Command Format**:
```
audio_player resume
```

**Function Description**: Resume the previously paused song playback.

**Example**:
```
ap_cmd audio_player resume
```

##### Stop Playback
**Command Format**:
```
audio_player stop
```

**Function Description**: Stop the currently playing song.

**Example**:
```
ap_cmd audio_player stop
```

#### 4.2.6 Playback Navigation

##### Previous Song
**Command Format**:
```
audio_player prev
```

**Function Description**: Play the previous song in the playlist.

**Example**:
```
ap_cmd audio_player prev
```

##### Next Song
**Command Format**:
```
audio_player next
```

**Function Description**: Play the next song in the playlist.

**Example**:
```
ap_cmd audio_player next
```

##### Jump to Specified Song
**Command Format**:
```
audio_player jump [id]
```

**Parameter Description**:
- ``id`` : Identifier of the song to jump to

**Example**:
```
ap_cmd audio_player jump 3
```

#### 4.2.7 Volume Control

##### Set Volume
**Command Format**:
```
audio_player volume [level]
```

**Parameter Description**:
- ``level`` : Volume level (typically range 0-100)

**Example**:
```
ap_cmd audio_player volume 50
```

##### Query Volume
**Command Format**:
```
audio_player volume
```

**Function Description**: Query the current volume level.

**Example**:
```
ap_cmd audio_player volume
```

#### 4.2.8 Set Playback Mode
**Command Format**:
```
audio_player mode [mode_value]
```

**Parameter Description**:

- ``mode_value`` : Playback mode value

   - ``0`` : Sequential playback
   - ``1`` : Single loop
   - ``2`` : List loop
   - ``3`` : Random playback

**Example**:
```
ap_cmd audio_player mode 1
```

#### 4.2.9 Playlist Management

##### Delete Specified Audio File
**Command Format**:
```
audio_player rm [file_path]
```

**Parameter Description**:
- ``file_path`` : Path of the audio file to delete

**Example**:
```
ap_cmd audio_player rm /sd0/test.mp3
```

##### Clear Playlist
**Command Format**:
```
audio_player clear
```

**Function Description**: Clear all songs in the playlist.

**Example**:
```
ap_cmd audio_player clear
```

##### Display Playlist
**Command Format**:
```
audio_player dump
```

**Function Description**: Display information about all songs in the current playlist.

**Example**:
```
ap_cmd audio_player dump
```

#### 4.2.10 Deinitialize Audio Player
**Command Format**:
```
audio_player deinit
```

**Function Description**: Release audio player resources and clean up the playback environment.

**Example**:
```
ap_cmd audio_player deinit
```

#### 4.2.11 Unmount SD Card
**Command Format**:
```
audio_player sd_unmount
```

**Function Description**: Unmount the SD card and release file system resources.

**Example**:
```
ap_cmd audio_player sd_unmount
```


## 5. Test Scheme
Use an SD card to store audio files for playback testing. The test process is as follows:
1. Prepare an SD card and place test audio files (such as test.mp3) in the root directory or subdirectories
2. Insert the SD card into the development board's SD card slot
3. Mount the SD card to the file system
4. Initialize the audio player
5. Add audio files to the playlist
6. Start playback and test various playback control functions
7. Stop playback and release resources
8. Unmount the SD card

## 6. Test Examples

### 6.1 Basic Playback Test

Test basic audio playback functionality, including the complete process of initialization, adding files, playback, pause, resume, stop, and deinitialization.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Add audio file to playlist
ap_cmd audio_player add 1 /sd0/test.mp3

# Start playback
ap_cmd audio_player start

# Pause playback
ap_cmd audio_player pause

# Resume playback
ap_cmd audio_player resume

# Stop playback
ap_cmd audio_player stop

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```

Command execution success returns: **CMDRSP:OK**

Command execution failure returns: **CMDRSP:ERROR**


### 6.2 Automatic Scan and Play Test

Test the functionality of automatically scanning audio files on the SD card and playing them.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Automatically scan SD card audio files
ap_cmd audio_player sd_scan

# Display playlist
ap_cmd audio_player dump

# Start playback
ap_cmd audio_player start

# Stop playback
ap_cmd audio_player stop

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```


### 6.3 Playlist Navigation Test

Test song switching functionality in the playlist.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Add multiple audio files
ap_cmd audio_player add 1 /sd0/song1.mp3
ap_cmd audio_player add 2 /sd0/song2.mp3
ap_cmd audio_player add 3 /sd0/song3.mp3

# Start playing the first song
ap_cmd audio_player start

# Switch to next song
ap_cmd audio_player next

# Switch to previous song
ap_cmd audio_player prev

# Jump to the third song
ap_cmd audio_player jump 3

# Stop playback
ap_cmd audio_player stop

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```


### 6.4 Playback Mode Test

Test different playback mode functionalities.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Add audio file
ap_cmd audio_player add 1 /sd0/test.mp3

# Set single loop mode
ap_cmd audio_player mode 1

# Start playback
ap_cmd audio_player start

# (Observe the song automatically repeating after playback completes)

# Stop playback
ap_cmd audio_player stop

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```


### 6.5 Volume Control Test

Test volume adjustment functionality.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Add audio file
ap_cmd audio_player add 1 /sd0/test.mp3

# Start playback
ap_cmd audio_player start

# Set volume to 50
ap_cmd audio_player volume 50

# Query current volume
ap_cmd audio_player volume

# Set volume to 80
ap_cmd audio_player volume 80

# Stop playback
ap_cmd audio_player stop

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```


### 6.6 Playlist Management Test

Test playlist add, delete, and clear functionality.

```
Send the following test commands in sequence:
# Mount SD card
ap_cmd audio_player sd_mount

# Initialize audio player
ap_cmd audio_player init

# Add multiple audio files
ap_cmd audio_player add 1 /sd0/song1.mp3
ap_cmd audio_player add 2 /sd0/song2.mp3
ap_cmd audio_player add 3 /sd0/song3.mp3

# Display playlist
ap_cmd audio_player dump

# Delete one audio file
ap_cmd audio_player rm /sd0/song2.mp3

# Display playlist again
ap_cmd audio_player dump

# Clear playlist
ap_cmd audio_player clear

# Display playlist (should be empty)
ap_cmd audio_player dump

# Deinitialize audio player
ap_cmd audio_player deinit

# Unmount SD card
ap_cmd audio_player sd_unmount
```


## 7. Important Notes

1. **SD Card Format**: The SD card must be formatted as FAT32 or FAT16 file system, otherwise it cannot be mounted properly
2. **Audio File Format**: Ensure audio files are in supported formats (MP3, WAV, M4A, AMR, TS, AAC)
3. **File Path**: When adding audio files, the full path must be used, such as `/sd0/test.mp3`
4. **Operation Sequence**: The SD card must be mounted and the player initialized before playback operations can be performed
5. **Resource Release**: After testing, stop playback, deinitialize the player, and unmount the SD card in sequence to properly release resources
6. **Playlist**: Before starting playback, ensure that there is at least one audio file in the playlist
7. **Error Handling**: If a command execution fails, the system will automatically attempt to stop the player, deinitialize, and unmount the SD card

