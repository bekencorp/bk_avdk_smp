# Secure Boot + AP Fast Resume Project (Flash AES Off)

* [中文](./README_CN.md)

## Overview

`pm_secureboot_ap_fast_boot` starts from `projects/secureboot_overwrite_no_en` and lives under `projects/pm`. It adds the AP fast-resume flow from `projects/pm/ap_fast_boot`.

Secure boot and image signature check stay enabled (`secureboot_en` / `sig_verify_en`). In `partitions/bk7259/security.csv`, `flash_aes_type` is `NONE`. The signed compressed OTA image is staged in `ota`; after reset the bootloader verifies it and overwrites `primary_all` as plaintext on the data bus.

Before AP power-down, the PM flow saves the CPU2 architectural context and copies DTCM into retained AP SRAM. PSRAM data is retained in place. When the AP powers up again, FreeRTOS continues at the saved execution point. CPU2 and CPU3 start together on cold boot. CPU3 is hotplugged offline before AP OFF and brought online after resume. The flash map, TF-M configuration, signing keys, and overwrite OTA layout stay with the secure-boot project.

## Project layout

- `ap/ap_main.c`: AP resume-continuity test task
- `cp/cp_main.c`: votes the AP on after secure boot and registers the `ap_fast_boot` CLI
- `config/bk7259/config`: enables security firmware packaging
- `config/key/`: build-time copy of the SDK example keys from `projects/secureboot_overwrite_no_en`
- `partitions/bk7259/auto_partitions.csv`: single-slot and OTA staging layout
- `partitions/bk7259/security.csv`: secure packaging settings (`flash_aes_type=NONE`)
- `partitions/bk7259/ram_regions.csv`: secure-world RAM plus the low-power retained region

## Build

Run from the SDK root:

```text
make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot
```

## Run and validation

1. Run the build command. The generated secure-boot images are in `build/bk7259/pm/pm_secureboot_ap_fast_boot/package`.
2. Use BKFIL to download `bootloader.bin` from that directory, and program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin`.
4. Reset the board. The CP log should report `CP NS world reached` and vote the AP on.
5. The AP `ap_resume_test` task prints `RESUME_PROOF`. After cold boot, `main_entries` is 1.
6. On CP UART0 (GPIO10/GPIO11), run:

```text
ap_fast_boot cycle
```

After a successful resume, `seq` keeps increasing, `stack` stays `0x12345678`, `heap_value` stays `0xA55A5AA5`, and `main_entries` stays 1.
7. Write this project's signed `ota.bin` into the `ota` staging partition through `http_ota` or `https_ota`, then reset and confirm the bootloader installs it into `primary_all`.

## Security notes

- The build reuses the SDK example keys from `projects/secureboot_overwrite_no_en`. Replace them with protected product keys before production use.
- Keep private keys outside source control and the firmware delivery package.
- Do not change partition names or sizes without checking the secure packer and bootloader constraints documented in `auto_partitions.csv`.
- Disabling Flash AES does not disable secure boot. Signatures are still required; only on-flash confidentiality is omitted.
- `CONFIG_PM_AP_FAST_BOOT_ENABLE` and `CONFIG_PSRAM_DATA_RETENTION_ENABLE` must be enabled on both the CP and the AP.
