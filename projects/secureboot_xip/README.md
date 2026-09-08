# Secure Boot XIP OTA Project

* [中文](./README_CN.md)

## Overview

`secureboot_xip` demonstrates the BK7258 secure-boot flow with dual executable image slots and direct-XIP OTA updates. Flash AES is enabled (`flash_aes_type=FIXED`).

Secure boot and image signature check stay enabled (`secureboot_en` / `sig_verify_en`). Matching `primary_*` and `secondary_*` partitions provide two signed application slots. Boot selection metadata is stored in `boot_param`. With Flash AES on, the bootloader and cores access the executable slots through the CBUS XTS-AES path.

The generated package contains the secure boot metadata, second-stage bootloader, CP application, and AP application.

To bring up Direct-XIP OTA without fusing a Flash AES key, use `secureboot_xip_no_encrypt`.

## Project layout

- `ap/` and `cp/`: non-secure AP/CP application entries and configurations
- `config/bk7258/config`: enables security firmware packaging
- `config/key/`: example signing key files
- `partitions/bk7258/auto_partitions.csv`: primary/secondary executable-slot layout
- `partitions/bk7258/security.csv`: secure packaging settings (`flash_aes_type=FIXED`)
- `partitions/bk7258/pack.json`: output image composition

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=secureboot_xip
```

## Run and validation

1. Run the build command. The generated secure-boot images are located in `build/bk7258/secureboot_xip/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP (including the Flash AES key).
3. Download `all-app.bin` from that directory.
4. Reset the board and confirm the CP log reports that the non-secure application was reached and the AP starts.
5. Install a compatible signed OTA image (`ota.bin` from this project) into the inactive executable slot through the enabled OTA flow (`http_ota`).
6. Reset and verify that the bootloader selects and starts the updated slot.

## Security notes

- The keys under `config/key/` are SDK example keys. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- The primary and secondary partition pairs must remain compatible. Check the secure packer and bootloader constraints in `auto_partitions.csv` before changing the layout.
- The Flash AES key in `security.csv` / `otp_efuse_config.json` must match the key fused in OTP. A mismatch prevents the cores from executing the XIP image.
