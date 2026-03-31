# Player Service Test Project

* [中文](./README_CN.md)

## 1. Project Overview
This project is a player service test module designed to test the functionality of the bk_player_service component on the Beken platform. This module provides comprehensive audio playback link testing, including audio data sources, encoding/decoding, and audio output functions, and is configured and controlled through a command-line interface (CLI).

* For player service API reference, please see:

  - [Player Service API Reference](../../../api-reference/audio/player_service/index.html)

* For player service development guide, please see:

  - [Player Service Development Guide](../../../developer-guide/audio/player_service/index.html)

## 2. Directory Structure
The project adopts an AP-CP dual-core architecture, with the main source code located in the AP directory. The project structure is as follows:
```
player_service_example/
├── .ci                             # CI configuration directory
├── .gitignore                      # Git ignore file configuration
├── CMakeLists.txt                  # Project-level CMake build file
├── Makefile                        # Make build file
├── README.md                       # English README
├── README_CN.md                    # Chinese README
├── ap/                             # AP core code directory
│   ├── CMakeLists.txt              # AP CMake build file
│   ├── ap_main.c                   # Main entry file, implementing CLI commands and test functions
│   ├── config/                     # AP configuration directory
│   │   └── bk7258_ap/              # BK7258 AP configuration
│   └── player_service_test/        # Player service test module directory
│       ├── cli_player_service.c    # Player service CLI command implementation
│       ├── cli_player_service.h    # Player service CLI command header file
│       └── prompt_tone_test.h      # Prompt tone test data header file
│   └── voice_service_test/         # Voice service test module directory
│       ├── cli_voice_service.c     # Voice service CLI command implementation
│       └── cli_voice_service.h     # Voice service CLI command header file
├── cp/                             # CP core code directory
│   ├── CMakeLists.txt              # CP CMake build file
│   ├── cp_main.c                   # CP main file
│   ├── config/                     # CP configuration directory
│   │   └── bk7258/                 # BK7258 CP configuration
│   └── customer_msg/               # Customer message processing module
│       ├── customer_msg.c          # Customer message processing implementation
│       └── customer_msg.h          # Customer message processing header file
├── partitions/                     # Partition configuration directory
│   └── bk7258/                     # BK7258 partition configuration
│       ├── auto_partitions.csv     # Automatic partition configuration
│       └── ram_regions.csv         # RAM region configuration
└── pj_config.mk                    # Project configuration file
```

## 3. Function Description
### 3.1 Main Functions
   - **Audio Playback**: Default support for onboard speaker (onboard SPK) and UAC speaker
   - **Audio Source Support**: Support for two types of audio data sources: array and file system (vfs)
   - **Codec Support**: Multiple audio formats including MP3 and WAV

### 3.3 Configuration Parameter Description

The player service supports two types of commands:
   - ``playback`` (Audio Playback): Test independent audio playback functionality
   - ``prompt_tone`` (Prompt Tone Playback): Test prompt tone insertion functionality during multiple audio playback

#### 3.3.1 Playback Command Parameters

1. **Command Format**:
   ```
   player_service playback [cmd] [source_type] [info]
   ```

2. **Parameter Description**:

   - ``[cmd]`` ：Command type
      - ``start`` ：Start playback
      - ``stop`` ：Stop playback

   - ``[source_type]`` ：Audio source type
      - ``array`` ：Array audio source, using built-in prompt tone data
      - ``vfs`` ：File system audio source, reading audio files from the file system

   - ``[info]`` ：Audio information
      - When ``source_type`` is ``array`` ， ``info`` is the array ID：
         - ``0`` ：ASR wake-up prompt tone (PCM format)
         - ``1`` ：Network configuration prompt tone (MP3 format)
         - ``2`` ：Low voltage prompt tone (WAV format)
      - When ``source_type`` is ``vfs`` ， ``info`` is the file path, for example ``/data/test.wav``

#### 3.3.2 Prompt Tone Command Parameters

1. **Command Format**:
   ```
   player_service prompt_tone [cmd] [tone_id] [source_type] [info]
   ```

2. **Parameter Description**:

   - ``[cmd]`` ：Command type
      - ``start`` ：Start playback
      - ``stop`` ：Stop playback

   - ``[tone_id]`` ：Prompt tone ID
      - ``0`` ：First prompt tone (ASR wake-up prompt tone)
      - ``1`` ：Second prompt tone (network configuration prompt tone)
      - ``2`` ：Third prompt tone (low voltage prompt tone)

   - ``[source_type]`` ：Audio source type
      - ``array`` ：Array audio source, using built-in prompt tone data
      - ``vfs`` ：File system audio source, reading audio files from the file system

   - ``[info]`` ：Audio information
      - When ``source_type`` is ``array`` ， ``info`` is the array ID：
         - ``0`` ：ASR wake-up prompt tone (PCM format)
         - ``1`` ：Network configuration prompt tone (MP3 format)
         - ``2`` ：Low voltage prompt tone (WAV format)
      - When ``source_type`` is ``vfs`` ， ``info`` is the file path, for example ``/data/test.wav``

## 4. Compilation and Execution
### 4.1 Compilation Method
Use the following command to compile the project:

.. code-block:: bash

   make bk7258 PROJECT=player_service_example

### 4.2 Execution Method
After compilation, burn the generated firmware to the development board, and then use the following commands through the serial terminal to test the player service functions:

#### 4.2.1 Playback Command (Audio Playback)
**Basic Format**:

.. code-block:: bash

   player_service playback [cmd] [source_type] [info]

**Examples**:

.. code-block:: bash

   # Start playing ASR wake-up prompt tone
   player_service playback start array 0

   # Start playing an audio file from the file system
   player_service playback start vfs /sd0/test.wav

   # Stop playback
   player_service playback stop

#### 4.2.2 Prompt Tone Command (Prompt Tone Playback)
**Basic Format**:

.. code-block:: bash

   player_service prompt_tone [cmd] [tone_id] [source_type] [info]

**Examples**:

.. code-block:: bash

   # Start playing the first prompt tone (ASR wake-up prompt tone)
   player_service prompt_tone start 1 array 0

   # Start playing the second prompt tone (network configuration prompt tone)
   player_service prompt_tone start 2 array 1

   # Start playing an audio file from the file system as a prompt tone
   player_service prompt_tone start 1 vfs /sd0/test.wav

   # Stop prompt tone playback
   player_service prompt_tone stop 1

## 5. Test Plan
Use speakers to test audio playback. The test procedure is as follows:

1. Select an appropriate playback command (playback or prompt_tone)
2. Configure the audio source type and information
3. Listen to the speaker to check if the played audio can be heard
4. Send a stop command to end playback

## 6. Test Examples
1. Playback command test - array audio playback

.. code-block:: bash

   Send the following test commands in sequence:
   # Start playing ASR wake-up prompt tone
   player_service playback start array 0

   # Stop playback
   player_service playback stop

2. Playback command test - file audio playback

.. code-block:: bash

   Send the following test commands in sequence:
   # Start playing an audio file from the file system
   player_service playback start vfs /data/test.wav

   # Stop playback
   player_service playback stop

3. Prompt Tone command test - interrupt array prompt tone

.. code-block:: bash

   Send the following test commands in sequence:
   # Start voice call function
   ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

   # Start interrupting prompt tone
   player_service prompt_tone start 1 array 0

   # Stop prompt tone playback
   player_service prompt_tone stop

   # Close voice call function
   ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0

4. Prompt Tone command test - interrupt file prompt tone

.. code-block:: bash

   Send the following test commands in sequence:
   # Start voice call function
   ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

   # Start playing an audio file from the file system as a prompt tone
   player_service prompt_tone start 1 vfs /sd0/test.wav

   # Stop playback
   player_service prompt_tone stop 1

   # Close voice call function
   ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0

Successful command execution returns: ``CMDRSP:OK``
Failed command execution returns: ``CMDRSP:ERROR``

.. important::

   For information on using voice call function commands, please refer to: `Voice Service Example Project <../../../examples/projects/voice_service_example/index.html>`_

