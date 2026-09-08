# Secure Boot XIP OTA Project (Flash AES Off)

* [中文](./README_CN.md)

## Overview

`secureboot_xip_no_encrypt` is the same BK7258 secure-boot Direct-XIP OTA flow as `secureboot_xip`, with **Flash AES disabled**.

Secure boot and image signature check stay enabled (`secureboot_en` / `sig_verify_en`). In `partitions/bk7258/security.csv`, `flash_aes_type` is `NONE` instead of `FIXED`. Matching `primary_*` and `secondary_*` partitions provide two signed application slots. Boot selection metadata is stored in `boot_param`. Because Flash AES is off, the bootloader and cores read/write the executable slots as plaintext (data bus), not through the CBUS XTS-AES path.

The generated package contains the secure boot metadata, second-stage bootloader, CP application, and AP application.

Use this project to bring up Direct-XIP OTA without fusing a Flash AES key. For production with on-flash encryption, use `secureboot_xip`.

## Project layout

- `ap/` and `cp/`: non-secure AP/CP application entries and configurations
- `config/bk7258/config`: enables security firmware packaging
- `config/key/`: example signing key files
- `partitions/bk7258/auto_partitions.csv`: primary/secondary executable-slot layout
- `partitions/bk7258/security.csv`: secure packaging settings (`flash_aes_type=NONE`)
- `partitions/bk7258/pack.json`: output image composition

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=secureboot_xip_no_encrypt
```

## Run and validation

1. Run the build command. The generated secure-boot images are located in `build/bk7258/secureboot_xip_no_encrypt/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin` from that directory.
4. Reset the board and confirm the CP log reports that the non-secure application was reached and the AP starts.
5. Install a compatible signed OTA image (`ota.bin` from this project) into the inactive executable slot through the enabled OTA flow (`http_ota`).
6. Reset and verify that the bootloader selects and starts the updated slot.

## Security notes

- The keys under `config/key/` are SDK example keys. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- The primary and secondary partition pairs must remain compatible. Check the secure packer and bootloader constraints in `auto_partitions.csv` before changing the layout.
- Disabling Flash AES does not disable secure boot. Signatures are still required; only on-flash confidentiality is omitted.
