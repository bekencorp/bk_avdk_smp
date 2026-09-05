# Secure Boot XIP OTA Project

* [中文](./README_CN.md)

## Overview

`secureboot_xip` demonstrates the BK7259 secure-boot flow with dual executable image slots and direct-XIP OTA updates.

The generated package contains the secure boot metadata, second-stage bootloader, secure service firmware, CP application, and AP application. Matching `primary_*` and `secondary_*` partitions provide two signed application slots. Boot selection metadata is stored in `boot_param`.

## Project layout

- `ap/` and `cp/`: non-secure AP/CP application entries and configurations
- `config/bk7259/config`: enables security firmware packaging
- `config/key/`: example signing key files
- `partitions/bk7259/auto_partitions.csv`: primary/secondary executable-slot layout
- `partitions/bk7259/security.csv`: secure packaging settings
- `partitions/bk7259/pack.json`: output image composition

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=secureboot_xip
```

## Run and validation

1. Run the build command. The generated secure-boot images are located in `build/bk7259/secureboot_xip/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin` from that directory.
4. Reset the board and confirm the CP log reports that the non-secure application was reached and the AP starts.
5. Install a compatible signed OTA image into the inactive executable slot through the enabled OTA flow.
6. Reset and verify that the bootloader selects and starts the updated slot.

## Security notes

- The keys under `config/key/` are SDK example keys. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- The primary and secondary partition pairs must remain compatible. Check the secure packer and bootloader constraints before changing the layout.
