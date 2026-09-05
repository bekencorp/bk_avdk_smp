# Secure Boot Overwrite OTA Project

* [中文](./README_CN.md)

## Overview

`secureboot_overwrite` demonstrates the BK7259 secure-boot flow with a single executable application slot and overwrite-style OTA updates.

The generated package contains the secure boot metadata, second-stage bootloader, secure service firmware, CP application, and AP application. During an update, the signed compressed image is written to the `ota` staging partition. After reset, the bootloader verifies the image and overwrites the executable `primary_all` slot. The `ota_control` partition records update progress for power-loss recovery.

## Project layout

- `ap/` and `cp/`: non-secure AP/CP application entries and configurations
- `config/bk7259/config`: enables security firmware packaging
- `config/key/`: example signing key files
- `partitions/bk7259/auto_partitions.csv`: single-slot and OTA staging layout
- `partitions/bk7259/security.csv`: secure packaging settings
- `partitions/bk7259/pack.json`: output image composition

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=secureboot_overwrite
```

## Run and validation

1. Run the build command. The generated secure-boot images are located in `build/bk7259/secureboot_overwrite/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin` from that directory.
4. Reset the board and confirm the CP log reports that the non-secure application was reached and the AP starts.
5. Write a compatible signed OTA image to the `ota` staging partition through the enabled OTA flow.
6. Reset and verify that the bootloader installs the image into `primary_all`.

## Security notes

- The keys under `config/key/` are SDK example keys. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- Do not change partition names or sizes without checking the secure packer and bootloader constraints documented in `auto_partitions.csv`.
