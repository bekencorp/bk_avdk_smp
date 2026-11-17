# BK7259 BEKEN KWS Library User Manual

This document describes how to use the BK7259 KWS prebuilt libraries and the
TensorFlow Lite model files .

## Library Authorization

The KWS library is licensed per device. Wake-up and command recognition do not work until the board is authorized.

Check the following configuration in the project to ensure the AP command line
is available and the board can access the network for online authorization:

```text
CONFIG_CLI=y
CONFIG_WEBCLIENT=y
CONFIG_WEBCLIENT_TLS=y
CONFIG_PSA_MBEDTLS=y
```

If the project uses the mbedTLS port instead of PSA, `CONFIG_MBEDTLS=y` is also
accepted.

1. Enter the ASR service once, so that the `beken_kws` commands are registered.
   The entry method depends on the project (for example a key, a UI page, or a
   wake-word flow).
2. Write the 16-character license ID and 32-character key assigned per device by
   BEKEN:

   ```text
   ap_cmd beken_kws set_auth <id> <key>
   ```

   Expected output: `success`.
3. Reset the board. With HTTPS networking available, the board authorizes
   automatically during startup and prints:

   ```text
   [KAUTH] auth success
   ```

   After the next reset, the boot log prints `[KAUTH] authorized ok`.

The board must be connected to the network and able to reach the license server.
If authorization fails, the log prints `[KAUTH] auth fail` and the board retries
on the next reset. OTP is one-time programmable: a wrong ID or key cannot be
corrected and the board must be replaced.

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
