# Automatic Speech Recognition Service Example Project

* [中文](./README_CN.md)

## 1. Project Overview
This project is an ASR service test module designed to test the functionality of the bk_asr_service component on the Beken platform. This module provides a complete Automatic Speech Recognition (ASR) processing link test, including audio collection, resampling, and ASR, and can be configured and controlled through the Command Line Interface (CLI).

* For the API reference of the ASR service, please refer to:

  - [ASR Service API Reference](../../../api-reference/audio/asr_service/index.html)

* For the development guide of the ASR service, please refer to:

  - [ASR Service Development Guide](../../../developer-guide/audio/asr_service/index.html)

## 2. Directory Structure
The project adopts an AP-CP dual-core architecture, with main source code located in the AP directory. The project structure is as follows:
```
asr_service_example/
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
│   └── asr_service_test/           # ASR service test module directory
│       ├── cli_asr_service.c       # Implementation of ASR service CLI commands
│       └── cli_asr_service.h       # Header file for ASR service CLI commands
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
   - **Speech Recognition**: Supports Wanson ASR engine (users can replace it with other ASR engines as needed).
   - **Resampling Support**: When the input sampling rate does not match the ASR required sampling rate, resampling is required (currently the Wanson ASR used requires 16K sampling rate).
   - **Flexible Integration**: Supports multiple working modes such as direct use of microphones or integration through voice services.
   - **Sample Rate Support**: Supports two sample rates: 8KHz and 16KHz.
   - **Dual-Core Communication**: AP-CP dual-core architecture, communication through message mechanism.

### 3.2 ASR Service Component Architecture
   The bk_asr_service component provides a complete ASR processing framework:
      - **ASR Service Core**: Manages the entire ASR processing lifecycle.
      - **Voice Reading Service**: Responsible for audio data collection and reading.
      - **Configuration Management**: Supports flexible audio parameter configuration.

### 3.3 Configuration Parameter Description

1. **Microphone Type**:

   - `onboard`: Onboard microphone
   - `uac`: UAC microphone

2. **Sample Rate**:

   - `8000`: 8KHz sample rate
   - `16000`: 16KHz sample rate


## 4. Compilation and Execution
### 4.1 Compilation Method
Use the following command to compile the project:

```
make bk7258 PROJECT=asr_service_example
```

### 4.2 Running Method
After the compilation is complete, flash the generated firmware to the development board, and then use the following commands through the serial terminal to test the ASR service functionality:

#### 4.2.1 Start the ASR Service with Microphone
**Basic Format**:

```
asr_service startwithmic [mic_type] [mic_samp_rate]
```

**Example**:

```
asr_service startwithmic onboard 8000
```

#### 4.2.2 Start the ASR Service through Voice Service
**Basic Format**:

```
asr_service startnomic [mic_type] [mic_samp_rate] [aec_en]
```

**Parameter Description**:

- `mic_type`: Microphone type (onboard/uac)

- `mic_samp_rate`: Microphone sampling rate (8000/16000)

- `aec_en`: Whether to enable echo cancellation (aec/empty)

**Example**:

```
asr_service startnomic onboard 8000 aec
```

#### 4.2.3 Stop the ASR Service
**Basic Format**:

```
asr_service stop [mic_type] [mic_samp_rate]
```

**Example**:

```
asr_service stop onboard 8000
```

## 5. Test Scheme
### 5.1 Direct Microphone Test Process
1. Start the ASR service, configure the microphone type and sampling rate.
2. Observe the serial port output to check if the wake-up words are successfully recognized.
3. Stop the ASR service.

### 5.2 Test Process through Voice Service
1. Start the voice service + ASR service, configure the microphone type, sampling rate, and whether to enable echo cancellation.
2. Observe the serial port output to check if the wake-up words are successfully recognized.
3. Stop the ASR service and voice service.

## 6. Test Examples
1. UAC Voice 16KHz ASR Test (Direct Use of Microphone)

```
Send the following test commands in sequence:
# Start ASR service
ap_cmd asr_service startwithmic uac 16000

# Stop ASR service
ap_cmd asr_service stop uac 16000
```

Command execution success returns: CMDRSP:OK

Command execution failure returns: CMDRSP:ERROR

.. important::

    1. Please refer to this case for the execution results of other subsequent tests, which will not be repeated one by one.

2. Onboard Voice 16KHz ASR Test (Direct Use of Microphone)

```
Send the following test commands in sequence:

# Start ASR service
ap_cmd asr_service startwithmic onboard 16000

# Stop ASR service
ap_cmd asr_service stop onboard 16000
```

3. UAC Voice 8KHz Resampling ASR Test (Direct Use of Microphone)

```
Send the following test commands in sequence:

# Start ASR service
ap_cmd asr_service startwithmic uac 8000

# Stop ASR service
ap_cmd asr_service stop uac 8000
```

4. Onboard Voice 8KHz Resampling ASR Test (Direct Use of Microphone)

```
Send the following test commands in sequence:

# Start ASR service
ap_cmd asr_service startwithmic onboard 8000

# Stop ASR service
ap_cmd asr_service stop onboard 8000
```

5. Onboard Voice 8KHz ASR Test (with AEC, through Voice Service)

```
Send the following test commands in sequence:

# Start ASR service (through voice service)
ap_cmd asr_service startnomic onboard 8000 aec

# Stop ASR service
ap_cmd asr_service stop onboard 8000
```

6. UAC Voice 16KHz ASR Test (through Voice Service)

```
Send the following test commands in sequence:

# Start ASR service (through voice service)
ap_cmd asr_service startnomic uac 16000

# Stop ASR service
ap_cmd asr_service stop uac 16000
```
