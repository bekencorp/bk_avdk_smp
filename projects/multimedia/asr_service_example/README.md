# ASR Service Example

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates the ADK-based automatic speech recognition (ASR)
service on BK7259. It uses the BEKEN KWS engine with TFLite Micro and the NPU.

- `startwithmic`: the ASR service captures audio directly from an onboard or
  UAC microphone.
- `startnomic`: Voice Service provides the audio stream to the ASR service.

## 2. Hardware and Console

- BK7259 development board.
- Onboard microphone or UAC microphone.
- Connect the serial terminal to the CP UART at 115200 baud.
- The ASR CLI runs on the AP, so prefix commands entered on the CP console with
  `ap_cmd`.

## 3. Project Structure

```text
asr_service_example/
├── ap/
│   ├── ap_main.c
│   ├── asr_service_test/
│   └── config/bk7259_ap/defconfig
├── cp/
│   ├── cp_main.c
│   └── config/bk7259/defconfig
└── partitions/bk7259/
```

## 4. Build and Flash

Run from the SDK root:

```bash
make bk7259 PROJECT=multimedia/asr_service_example
```

The generated image is:

```text
build/bk7259/asr_service_example/package/all-app.bin
```

Flash the image using the standard BK7259 procedure.

## 5. CLI Commands

Command syntax:

```text
ap_cmd asr_service {startwithmic|startnomic|stop} [onboard|uac] [8000|16000] [aec_en]
```

Examples:

```text
# Onboard microphone at 16 kHz
ap_cmd asr_service startwithmic onboard 16000

# Onboard microphone with AEC
ap_cmd asr_service startwithmic onboard 16000 1

# Obtain the audio stream from Voice Service
ap_cmd asr_service startnomic onboard 16000 1

# Stop recognition
ap_cmd asr_service stop
```

`aec_en` accepts `0`, `1`, or `aec`. AEC is not enabled for the UAC microphone
path. An 8 kHz input stream is resampled to the 16 kHz rate used by the ASR
engine.

A successful command returns `CMDRSP:OK`. Recognized keywords and their command
IDs are printed on the serial console.

## 6. Key Configuration

The main options are in `ap/config/bk7259_ap/defconfig`:

- `CONFIG_ASR_SERVICE`
- `CONFIG_BEKEN_KWS`
- `CONFIG_TFLITE_MICRO`
- `CONFIG_NPU`
- `CONFIG_VOICE_SERVICE`
