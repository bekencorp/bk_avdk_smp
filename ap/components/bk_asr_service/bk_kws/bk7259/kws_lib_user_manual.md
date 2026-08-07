# BK7259 BEKEN KWS Library User Manual

This document describes how to use the BK7259 KWS prebuilt libraries and the
TensorFlow Lite model files in this directory.

## Library Selection

The SDK selects the library automatically in
`sdk/ap/components/bk_asr_service/CMakeLists.txt`.

- `CONFIG_BEKEN_KWS_MODEL_FROM_UDISK=n`: links `libkws_model.a`.
- `CONFIG_BEKEN_KWS_MODEL_FROM_UDISK=y`: links `libkws_model_vfs.a`.

### Built-In Mode

Built-in mode uses `libkws_model.a`.

In this mode the KWS model arrays are compiled into the static library. No
external `.tflite` file is needed at runtime.

Recommended use:

```text
CONFIG_ASR_SERVICE=y
CONFIG_TFLITE_MICRO=y
CONFIG_BEKEN_KWS=y
CONFIG_BEKEN_KWS_MODEL_FROM_UDISK=n
CONFIG_NPU=y
CONFIG_NPU_CACHE=y
```

### VFS Mode

VFS mode uses `libkws_model_vfs.a`.

In this mode the built-in model arrays are disabled to save flash. The runtime
loads the `.tflite` model files from VFS by calling
`bk_tflite_asr_set_model_from_file()`.

Recommended use:

```text
CONFIG_ASR_SERVICE=y
CONFIG_TFLITE_MICRO=y
CONFIG_BEKEN_KWS=y
CONFIG_BEKEN_KWS_MODEL_FROM_UDISK=y
CONFIG_BEKEN_KWS_MODEL_BUFFER_USE_PSRAM=y
CONFIG_NPU=y
CONFIG_NPU_CACHE=y
CONFIG_VFS=y
CONFIG_FATFS=y
```

If the board uses SD-NAND or SD card, also enable the board-specific SDIO/FATFS
configuration required by the project.

## VFS Model File Names

The runtime file names are fixed by `audio_engine.c`:

```text
1:/kws_model/bk_kws_wakeup.tflite
1:/kws_model/bk_kws_commands.tflite
```

When preparing a U-disk, SD card, or SD-NAND partition, create this directory:

```text
1:/kws_model/
```

Then copy the two model files to it:

```text
bk7259/bk_kws_wakeup.tflite   -> 1:/kws_model/bk_kws_wakeup.tflite
bk7259/bk_kws_commands.tflite -> 1:/kws_model/bk_kws_commands.tflite
```

Do not change these runtime file names unless the application code path in
`audio_engine.c` is changed at the same time.

## Common Issues

### Build Links The Wrong Library

Check `CONFIG_BEKEN_KWS_MODEL_FROM_UDISK`.

- `n` links `libkws_model.a`.
- `y` links `libkws_model_vfs.a`.

### VFS Model Open Failed

Check the storage mount path and exact file names. The code expects:

```text
1:/kws_model/bk_kws_wakeup.tflite
1:/kws_model/bk_kws_commands.tflite
```

### NPU Or KWS Init Failed

Check:

- `CONFIG_NPU=y`
- `CONFIG_NPU_CACHE=y`
- arena and scratch buffers are 32-byte aligned
- PSRAM/HSRAM heap has enough free memory
