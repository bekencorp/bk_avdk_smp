# Secure Boot Overwrite OTA Project (Flash AES Off)

* [中文](./README_CN.md)

## Overview

`secureboot_overwrite_no_en` is the same BK7259 secure-boot overwrite OTA flow as `secureboot_overwrite`, with **Flash AES disabled**.

Secure boot and image signature check stay enabled (`secureboot_en` / `sig_verify_en`). In `partitions/bk7259/security.csv`, `flash_aes_type` is `NONE` instead of `FIXED`. The signed compressed OTA image is still staged in `ota`; after reset the bootloader verifies it and overwrites `primary_all`. `ota_control` records install progress for power-loss recovery. Because Flash AES is off, the bootloader programs `primary_all` as plaintext (data bus), not through the CBUS XTS-AES path.

Use this project to bring up overwrite OTA without fusing a Flash AES key. For production with on-flash encryption, use `secureboot_overwrite`.

## Project layout

- `ap/` and `cp/`: non-secure AP/CP application entries and configurations
- `config/bk7259/config`: enables security firmware packaging
- `config/key/`: example signing key files
- `partitions/bk7259/auto_partitions.csv`: single-slot and OTA staging layout
- `partitions/bk7259/security.csv`: secure packaging settings (`flash_aes_type=NONE`)
- `partitions/bk7259/pack.json`: output image composition

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=secureboot_overwrite_no_en
```

## Run and validation

1. Run the build command. The generated secure-boot images are located in `build/bk7259/secureboot_overwrite_no_en/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin` from that directory.
4. Reset the board and confirm the CP log reports that the non-secure application was reached and the AP starts.
5. Write a compatible signed OTA image (`ota.bin` from this project) to the `ota` staging partition through the enabled OTA flow (`http_ota`).
6. Reset and verify that the bootloader installs the image into `primary_all`.

## Security notes

- The keys under `config/key/` are SDK example keys. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- Do not change partition names or sizes without checking the secure packer and bootloader constraints documented in `auto_partitions.csv`.
- Disabling Flash AES does not disable secure boot. Signatures are still required; only on-flash confidentiality is omitted.
