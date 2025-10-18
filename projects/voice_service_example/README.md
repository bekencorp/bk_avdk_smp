# Voice Service Test Project

* [中文](./README_CN.md)

## 1. Project Overview
This project is a voice service test module designed to test the functionality of the bk_voice_service component on the Beken platform. This module provides a complete voice processing link test, including functions such as audio collection, encoding and decoding, Acoustic Echo Cancellation (AEC), and audio transmission. It can be configured and controlled through the Command Line Interface (CLI).

* For the API reference of the voice call service, please refer to:

  - [Voice Call Service API Reference](../../../api-reference/audio/voice_service/index.html)

* For the development guide of the voice call service, please refer to:

  - [Voice Call Service Development Guide](../../../developer-guide/audio/voice_service/index.html)

## 2. Directory Structure
The project adopts an AP-CP dual-core architecture, with main source code located in the AP directory. The project structure is as follows:
```
voice_service_example/
├── .ci                             # CI configuration directory
├── .gitignore                      # Git ignore file configuration
├── CMakeLists.txt                  # Project-level CMake build file
├── Makefile                        # Make build file
├── README.md                       # English README
├── README_CN.md                    # Chinese README
├── app.rst                         # Application description file
├── ap/                             # AP core code directory
│   ├── CMakeLists.txt              # AP CMake build file
│   ├── ap_main.c                   # Main entry file, implementing CLI commands and test functions
│   ├── config/                     # AP configuration directory
│   │   └── bk7258_ap/              # BK7258 AP configuration
│   └── voice_service_test/         # Voice service test module directory
│       ├── cli_voice_service.c     # Implementation of voice service CLI commands
│       └── cli_voice_service.h     # Header file for voice service CLI commands
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
   - **Audio Capture**: Supports onboard microphones (onboard MIC) and UAC microphones.
   - **Audio Playback**: Supports onboard speakers (onboard SPK) and UAC speakers.
   - **Codec Support**: Supports multiple audio codec formats such as PCM, G.711A, G.711U, AAC, and G.722.
   - **Echo Cancellation**: Supports the AEC echo cancellation algorithm with configurable different versions.
   - **Audio Equalization**: Supports mono and stereo audio equalization adjustment.
   - **Sample Rate Support**: Supports two sample rates: 8KHz and 16KHz.
   - **Dual-core Communication**: Adopts an AP-CP dual-core architecture and communicates through a message mechanism.

### 3.2 Voice Service Component Architecture
   The bk_voice_service component provides a complete voice processing framework:
      - **Voice Service Core**: Manages the entire voice processing lifecycle.
      - **Voice Reading Service**: Responsible for audio data collection and reading.
      - **Voice Writing Service**: Responsible for audio data playback and output.
      - **Configuration Management**: Supports flexible audio parameter configuration.

### 3.3 Configuration Parameter Description

1. **Microphone Type**:

   - `onboard`: Onboard microphone
   - `uac`: UAC microphone
   - `onboard_dual_dmic_mic`: Onboard dual-digital microphone

2. **Sample Rate**:

   - `8000`: 8KHz sample rate
   - `16000`: 16KHz sample rate

3. **Echo Cancellation**:

   - bit0:0 aec disable/1 aec enable
   - bit1:0 AEC_MODE_SOFTWARE/1 AEC_MODE_HARDWARE,
   - bit2:0 DUAL_MIC_CH_0_DEGREE/1 DUAL_MIC_CH_90_DEGREE
   - bit3:0 no mic swap/1 mic swap
   - bit4:0 no ec ooutput/1 ecoutput

4. **Codec Format**:

   - `pcm`: Raw PCM data
   - `g711a`: G.711 A-law
   - `g711u`: G.711 μ-law
   - `aac`: AAC codec
   - `g722`: G.722 codec

5. **Speaker Type**:

   - `onboard`: Onboard speaker
   - `uac`: UAC speaker

6. **Audio Equalization**:

   - `0`: Disable equalization
   - `1`: Mono equalization
   - `2`: Stereo equalization


## 4. Compilation and Execution
### 4.1 Compilation Method
Use the following command to compile the project:
```
make bk7258 PROJECT=voice_service_example
```

### 4.2 Running Method
After the compilation is complete, flash the generated firmware to the development board, and then use the following commands through the serial terminal to test the voice service functionality:

#### 4.2.1 Start the Voice Service
**Basic Format**:
```
voice_service start [mic_type] [mic_samp_rate] [aec_en] [enc_type] [dec_type] [spk_type] [spk_samp_rate] [eq_type]
```


**Example**:
```
voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

#### 4.2.2 Stop the Voice Service
**Basic Format**:
```
voice_service stop  [mic_type] [mic_samp_rate] [aec_en] [enc_type] [dec_type] [spk_type] [spk_samp_rate] [eq_type]
```

**Example**:
```
voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

## 5. Test Scheme
Use microphones and speakers for local audio self-loopback testing. The test process is as follows:
1. Start the voice service, configure the microphone and speaker as the onboard microphone and onboard speaker.
2. Play an audio file to the microphone.
3. Listen to the speaker and check if you can hear the played audio.
4. Stop the voice service.

## 6. Test Examples
1. Onboard Voice G711A 8KHz AEC Test

```
Send the following test commands in sequence:
# Start voice call service
ap_cmd voice_service start onboard 8000 1 g711a g711a onboard 8000 0

# Stop voice call service
ap_cmd voice_service stop onboard 8000 1 g711a g711a onboard 8000 0
```

Command execution success returns: CMDRSP:OK
Command execution failure returns: CMDRSP:ERROR

```
After successfully starting the voice call service, internal component work data statistics information similar to the following will be printed at fixed time intervals:
ap0:count_ut:D(36128):[ONBOARD_MIC] data_size: 63680(Bytes), 15KB/s
ap0:count_ut:D(36128):[RAW_READ] data_size: 30720(Bytes), 7KB/s 
ap0:count_ut:D(36130):[RAW_WRITE] data_size: 30400(Bytes), 7KB/s 
ap0:count_ut:D(36132):[ONBOARD_SPK] data_size: 59840(Bytes), 14KB/s 
ap0:count_ut:D(36134):[WIFI_TX] data_size: 30720(Bytes), 7KB/s 
ap0:count_ut:D(36134):[WIFI_RX] data_size: 30720(Bytes), 7KB/s 
```

.. important::

    1. Please refer to this case for the execution results of other subsequent tests, which will not be repeated one by one.


2. Onboard Voice G711A 16KHz AEC Test

```
Send the following test commands in sequence:
# Start voice call service
ap_cmd voice_service start onboard 16000 1 g711a g711a onboard 16000 0

# Stop voice call service
ap_cmd voice_service stop onboard 16000 1 g711a g711a onboard 16000 0
```

3. UAC Voice G711A 8KHz AEC Test

```
Send the following test commands in sequence:
# Start voice call service
ap_cmd voice_service start uac 8000 1 g711a g711a uac 8000 0

# Stop voice call service
ap_cmd voice_service stop uac 8000 1 g711a g711a uac 8000 0
```

4. Onboard Voice aac 8KHz AEC Test

```
Send the following test commands in sequence:
# Start voice call service
ap_cmd voice_service start onboard 8000 1 aac aac onboard 8000 0

# Stop voice call service
ap_cmd voice_service stop onboard 8000 1 aac aac onboard 8000 0
```

5. Onboard Voice g722 16KHz AEC Test

```
Send the following test commands in sequence:
# Start voice call service
ap_cmd voice_service start onboard 16000 1 g722 g722 onboard 16000 0

# Stop voice call service
ap_cmd voice_service stop onboard 16000 1 g722 g722 onboard 16000 0
```
