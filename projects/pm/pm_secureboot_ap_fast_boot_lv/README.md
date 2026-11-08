# Secure Boot + AP Fast Resume + LV Low-Power Project (Flash AES Off)

* [中文](./README_CN.md)

## 1. Overview

`pm_secureboot_ap_fast_boot_lv` is derived from `projects/pm/pm_secureboot_ap_fast_boot`. It keeps the secure-boot chain, the Overwrite OTA layout, and AP fast resume, and additionally:

1. Switches the CP low-power path from Deep LV to LV (`CONFIG_PM_LV_SLEEP_ENABLE=y`) for faster wake-up.
2. Keeps the AP RAM of the Bluetooth host across AP power-down (`CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y`) instead of losing it.
3. Enables the AP PSRAM retention self-test (`CONFIG_PM_AP_PSRAM_RETENTION_CHECK=y`) and enlarges the PSRAM data-retention section.

In short:

1. Boot chain: BootROM -> CP BL2 (MCUboot) -> CP TF-M Secure -> CP non-secure app.
2. The CP app votes the AP on through `PM_BOOT_AP_MODULE_NAME_APP` and registers the `ap_fast_boot` CLI.
3. Before AP power-off, the PM flow saves the AP CPU2 architectural context and copies power-gated DTCM into retained AP SRAM. PSRAM is retained in place. On AP power-up, FreeRTOS resumes at the saved execution point instead of cold booting.
4. CPU2 and CPU3 start together on cold boot. CPU3 is hotplugged offline before AP OFF and brought back online after resume.
5. With LV sleep, `sys_hal_enter_low_voltage()` calls `sys_hal_enter_lv()` directly and does not select Deep LV from the bootloader version. Wake-up returns in place instead of re-entering through the deep-LV reset flow.
6. `CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190` shortens the hardware-to-bootloader wake-time compensation from the 4100 us default to 1190 us, matching the LV wake path.

> LV sleeps less deeply than Deep LV, so standby current is usually higher. This project targets fast wake-up and short resume latency, not the lowest possible sleep current. Use `pm_secureboot_ap_fast_boot` when Deep LV current matters more than wake latency.

## 2. Project Layout

- `ap/ap_main.c`: AP resume-continuity test task and 1 MiB PSRAM retention self-test
- `cp/cp_main.c`: votes the AP on after secure boot and registers `ap_fast_boot on|off|cycle`
- `config/bk7259/config`: enables security firmware packaging
- `CMakeLists.txt`: copies the SDK example signing keys from `projects/secureboot_overwrite_no_en` into `config/key/` at configure time
- `partitions/bk7259/auto_partitions.csv`: single execution slot (`primary_all`) plus `ota` staging area
- `partitions/bk7259/security.csv`: secure packaging settings
- `partitions/bk7259/ota.csv`: Overwrite OTA strategy and version fields
- `partitions/bk7259/ram_regions.csv`: secure-world RAM, low-power retained region, and AP PSRAM data-retention section
- `partitions/bk7259_ap/ppc.csv`, `ppro_default.csv`: AP peripheral secure/privilege attributes and PPRO defaults
- `cp/vnd_cal.c/h`: vendor calibration override used by `bk_init()`

## 3. Build And Flash

Run from the SDK root:

```text
make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot_lv
```

The Docker command recorded in `.ci` is:

```text
./dbuild.sh make bk7259 PROJECT=pm/pm_secureboot_ap_fast_boot_lv
```

The generated images are in `build/bk7259/pm_secureboot_ap_fast_boot_lv/package`.

1. Use BKFIL to download `bootloader.bin` from that directory.
2. Program the default keys from `otp_efuse_config.json` into OTP.
3. Download `all-app.bin`.
4. Reset the board.

## 4. Run And Validation

1. The CP log should report `CP NS world reached (secure boot OK)`, `AP vote ON ret=0`, and `CLI ready: ap_fast_boot on|off|cycle`.
2. The AP logs `AP main entry count=1` and the `ap_resume_test` task prints `RESUME_PROOF` with `seq`, `stack`, `heap_value`, `psram_size`, `main_entries`, and `core`.
3. On CP UART0 (`GPIO_10` RX / `GPIO_11` TX), run:

```text
ap_fast_boot cycle
```

The command votes the AP off, waits 1 second, and votes it on again. After a successful resume:

- `main_entries` stays `1` (the AP `main()` did not run again).
- `seq` continues increasing from the previous value.
- `stack` stays `0x12345678` and `heap_value` stays `0xA55A5AA5`.
- `PSRAM_RETENTION PASS` and `PSRAM_INCREMENT_PATTERN PASS` are printed for the resumed cycle.

4. To exercise only one direction, use `ap_fast_boot off` or `ap_fast_boot on`.
5. OTA: write this project's signed `ota.bin` into the `ota` staging partition through the enabled `http_ota` / `https_ota` flow, then reset. BL2 applies the image to `primary_all`; `ota_control` keeps the power-fail resume journal.

## 5. Key Configuration

### 5.1 CP Side

Important options in `cp/config/bk7259/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable Tickless Idle |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | Retain PSRAM contents across AP power-down |
| `CONFIG_PM_LV_SLEEP_ENABLE=y` | Enter LV sleep instead of Deep LV |
| `CONFIG_DEEP_LV=y` | Keep Deep LV support code built (shared paths, AP fast boot dependency) |
| `CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190` | Wake-time compensation in microseconds (default 4100) |
| `CONFIG_PM_ONLY_CP_ENABLE=y` | Keep the CP as the resident low-power core |
| `CONFIG_PM_AP_POWERDOWN_WHEN_LV=y` | Power down the AP when the CP enters LV sleep |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | Enable AP fast resume |
| `CONFIG_PM_AP_SRAM_POWER_CTRL=y` | Control AP SRAM power per bank |
| `CONFIG_PM_AP_MEM3_POWER_DOWN` ... `MEM6_POWER_DOWN=y` | Power down unused AP SRAM banks |
| `CONFIG_PM_AP_CPU2_CACHE_POWER_DOWN=y`, `CONFIG_PM_AP_CPU3_CACHE_POWER_DOWN=y` | Power down AP CPU2/CPU3 cache |
| `CONFIG_AON_LDO_SEL=2` | Select the AON LDO voltage (level 2 = 0.7 V) |
| `CONFIG_AON_ENCP=y` | Keep the shared ENCP/OTP power domain supplied (OTP sleeps instead of powering down) so the TrustEngine keeps working |
| `CONFIG_TOUCH=y`, `CONFIG_TOUCH_TEST=y` | Enable touch and its test in the current project |
| `CONFIG_BTDM_CONTROLLER_ONLY=y` | Run the Bluetooth controller only on the CP |
| `CONFIG_BLE_LV_SUPPORT=y` | Enable BLE low-voltage sleep support |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y` | Keep the Bluetooth stack state while the AP is powered down with RAM retained |
| `CONFIG_BLUETOOTH_RF_MODE_IQ_HIGH_PLL=y` | Use IQ mode with the Wi-Fi PLL |
| `CONFIG_BLUETOOTH_SUPPORT_LPO_ROSC=y` | Support the RC-oscillator LPO clock |
| `CONFIG_GPIO_WAKEUP_SUPPORT=y`, `CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT=y` | Enable static and runtime GPIO wakeup sources |
| `# CONFIG_EASY_FLASH is not set` | Do not build EasyFlash on the CP |
| `CONFIG_PM_POWER_VOTE_RECORD=y`, `CONFIG_PM_CLOCK_VOTE_RECORD=y` | Keep PM vote records for debugging |
| `CONFIG_TFM_BL2="ON"`, `CONFIG_TFM_ISOLATION_LEVEL=2` | Build TF-M BL2 with isolation level 2 |
| `CONFIG_BL2_UPGRADE_STRATEGY="OVERWRITE_ONLY"` | Use the Overwrite OTA strategy |
| `CONFIG_TFM_AP_BOOT_NSC=y`, `CONFIG_TFM_SLEEP_RETENTION_NSC=y` | Expose AP boot and sleep-retention NSC services |
| `# CONFIG_TFM_AP_SHIM_L2_CACHE_DISABLE is not set` | Keep AP L2 (PL310) initialization in the AP boot shim |

### 5.2 AP Side

Important options in `ap/config/bk7259_ap/defconfig`:

| Option | Description |
| --- | --- |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE=2` | Enable AP Tickless Idle |
| `CONFIG_PSRAM_DATA_RETENTION_ENABLE=y` | Retain PSRAM contents across AP power-down |
| `CONFIG_PM_AP_FAST_BOOT_ENABLE=y` | Resume from the saved FreeRTOS execution point |
| `CONFIG_PM_AP_PSRAM_RETENTION_CHECK=y` | Run the 1 MiB PSRAM retention self-test after each real resume |
| `CONFIG_PM_AP_WFI_ENABLE=y` | Allow the AP to enter WFI when idle |
| `CONFIG_PM_AP_SUBPOWER_DOMAIN_DEFAULT_DISABLE=y` | Disable VIDEO_POST, H26E, ISP, and NPU domains by default |
| `CONFIG_PM_AP_CPU_FRQ_DEFAUL=y` | Vote for the default AP CPU frequency after initialization |
| `CONFIG_DEEP_LV=y` | Required by AP fast boot (the AP is powered off through the deep-LV path) |
| `# CONFIG_L2_CACHE_ENABLE is not set` | Keep the AP-side L2 cache disabled |
| `CONFIG_AP_SECURE_CACHE_NSC=y` | Use the secure cache NSC service |
| `CONFIG_BLUETOOTH_AP=y` | Run the Bluetooth host on the AP |
| `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y` | Do not re-initialize the Bluetooth stack after AP resume |
| `CONFIG_SECURE_OTA_OVERWRITE=y`, `CONFIG_OTA_HTTPS=y` | Enable secure Overwrite OTA over HTTPS |
| `CONFIG_EASY_FLASH_V4=y`, `CONFIG_VFS=y`, `CONFIG_FATFS=y`, `CONFIG_LITTLEFS=y` | Enable the filesystem stack |
| `CONFIG_SDCARD=y`, `CONFIG_FATFS_SDCARD=y` | Enable the SD card host and FATFS on SD |
| `CONFIG_AT=y`, `CONFIG_DEFAULT_AT_PORT=5` | Enable AT commands on port 5 |
| `CONFIG_PSA_MBEDTLS=y`, `CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=4096` | Enable PSA mbedTLS with 4 KB TLS records |
| `CONFIG_IPERF_TEST=y`, `CONFIG_DEMOS_IPERF=y`, `CONFIG_VFS_TEST=y` | Keep the test/demo modules enabled |
| `CONFIG_DUMP_BY_LOG_UART=y` | Dump logs through the log UART |

### 5.3 LV, Fast Resume, BLE Retention, And PSRAM Retention

- **LV instead of Deep LV**: the CP takes `sys_hal_enter_lv()` unconditionally. Deep-LV debug GPIO markers are not part of this flow, and the wake path does not restart through the bootloader. `CONFIG_DEEP_LV=y` remains for the shared deep-LV code and for `PM_AP_FAST_BOOT_ENABLE` on the AP.
- **Wake-time compensation**: `CONFIG_PM_SLEEP_WAKE_TIME_HARDWARE_OR_BOOTLOADER=1190` (us) replaces the 4100 us default so that RTC compensation matches the shorter LV wake latency.
- **AP fast resume**: before AP OFF, the AP saves the CPU2 architectural context and backs up power-gated DTCM into retained AP SRAM. On wake, it restores from the reset vector, restores DTCM, then re-enables interrupts and continues the FreeRTOS execution point.
- **BLE retention**: the CP runs the controller only; the host runs on the AP. With `BLUETOOTH_SUPPORT_AP_PWD_RETENTION`, AP RAM is retained and the host stack is not re-initialized, so an established BLE link can continue after resume.
- **PSRAM retention check**: the AP allocates a 1 MiB incrementing pattern in `.psram.bss` inside `AP_PSRAM_DATA_SECTION`, records its CRC before power-off, and verifies both the CRC and the pattern after a real resume. `PSRAM_RETENTION_COMPARE` reports whether both independent scans agree.

### 5.4 Flash And RAM Layout

Flash (8 MB, `partitions/bk7259/auto_partitions.csv`):

| Partition | Offset | Size |
| --- | --- | --- |
| `bl1_control` | 0x0 | 4 KB |
| `boot_flag` | 0x1000 | 4 KB |
| `partition` | 0x2000 | 4 KB |
| `primary_manifest` | 0x3000 | 4 KB |
| `bl2` | 0x4000 | 96 KB |
| `sys_its`, `sys_ps` | 0x1C000 / 0x1E000 | 8 KB each |
| `primary_tfm_s` | 0x20000 | 320 KB |
| `primary_cpu0_app` | 0x70000 | 1644 KB |
| `primary_ap_app` | 0x20B000 | 1428 KB |
| `ota` | 0x370000 | 3072 KB |
| `ota_control` | 0x670000 | 8 KB |
| `usr_config` | 0x6B8000 | 60 KB |
| `easyflash` / `easyflash_ap` | 0x6C7000 / 0x6C9000 | 8 KB each |
| `sys_rf`, `sys_net` | 0x7FE000 / 0x7FF000 | 4 KB each |

`primary_all` is the single execution slot: 320 KB + 1644 KB + 1428 KB = 3392 KB (53 x 64 KB compression blocks). There are no `secondary_*` partitions.

RAM (`partitions/bk7259/ram_regions.csv`):

| Region | Type | Offset | Size |
| --- | --- | --- | --- |
| `CP_SPE_RAM` | secure | `0x28000000` | 80 KB |
| `AP_SPE_RAM` | secure | `0x28100000` | 4 KB |
| `CP_RAM` | SRAM | `0x28000000` | 0x05EB00 |
| `SWAP` | SRAM | after CP_RAM | 4 KB |
| `LOWPOWER` | SRAM | `0x2805FB00` | 1 KB |
| `AP_HSRAM_HEAP` | SRAM | `0x38101000` | 0x07F000 |
| `AP_RAM` | SRAM | after heap | 384 KB |
| `CP_PSRAM_HEAP` | PSRAM | after coded slab | 128 KB |
| `AP_PSRAM_DATA_SECTION` | PSRAM | `0x74E20000` | 1.25 MB |
| `AP_PSRAM_HEAP` | PSRAM | after data section | 640 KB |
| `AP_PSRAM_CACHE_HEAP` | PSRAM | after heap | 128 KB |
| `AP_PSRAM_CODE_SECTION` | PSRAM | after cache heap | 128 KB |

## 6. Differences From pm_secureboot_ap_fast_boot

| Item | `pm_secureboot_ap_fast_boot` | `pm_secureboot_ap_fast_boot_lv` |
| --- | --- | --- |
| CP sleep mode | Deep LV | LV (`CONFIG_PM_LV_SLEEP_ENABLE=y`) |
| Wake-time compensation | 4100 us default | 1190 us |
| CP Bluetooth | BLE slave only | BTDM controller only |
| Bluetooth during AP power-down | `AP_PWD_ALL` (AP RAM lost, stack re-inits) | `AP_PWD_RETENTION` (AP RAM kept, stack resumes) |
| AP PSRAM retention self-test | Disabled | Enabled, 1 MiB pattern |
| `AP_PSRAM_DATA_SECTION` | 0x110000 (1.0625 MB) | 0x140000 (1.25 MB) |
| `AP_PSRAM_HEAP` | 0x040000 (256 KB) | 0x0A0000 (640 KB) |
| `AP_PSRAM_CODE_SECTION` | 0x080000 (512 KB) | 0x020000 (128 KB) |

## 7. Security Notes

- `partitions/bk7259/security.csv` in this project sets `secureboot_en=TRUE`, `sig_verify_en=FALSE`, `flash_aes_type=NONE`, and `crc_en=FALSE`.
- `secureboot_en=TRUE` builds the secure-boot chain and partition layout (BL2, TF-M Secure, `primary_manifest`, `bl1_control`). It does not by itself force signature verification at boot.
- `sig_verify_en=FALSE` makes the packer write the `BEKEN` magic at flash offset 0x100. BL2 then skips signature verification but still verifies the image hash. Set `sig_verify_en=TRUE` (which writes `BK.SB`) before production if image signatures must be enforced. Note that OTP `secure_boot_supported` (bit 3) also forces signature verification regardless of the magic; the generated `otp_efuse_config.json` programs the ROTPK hashes but does not burn bit 3.
- `flash_aes_type=NONE` keeps the flash plaintext. The bootloader writes `primary_all` directly through the data bus instead of the CBUS XTS-AES path.
- The build reuses the SDK example EC-P256 keys from `projects/secureboot_overwrite_no_en`; `config/key/*.pem` is generated at configure time and git-ignored. Replace these keys with protected product keys and keep private keys out of source control and firmware packages.
- OTA images are compressed and written to `ota` with `encrypt=FALSE`; `ota_control` stores the install journal. Do not change partition names or sizes without checking the packer and bootloader constraints in `auto_partitions.csv`.
- `CONFIG_PM_AP_FAST_BOOT_ENABLE` and `CONFIG_PSRAM_DATA_RETENTION_ENABLE` must be enabled on both the CP and the AP, and `sig_verify_en` must be consistent with the OTP provisioning.

## 8. FAQ

### 8.1 AP `main_entries` Becomes Greater Than 1

The AP took the cold-boot path. Check `CONFIG_PM_AP_FAST_BOOT_ENABLE` and `CONFIG_PSRAM_DATA_RETENTION_ENABLE` on both cores, and confirm that the AP was really powered off (not only clock-gated) before the resume test.

### 8.2 `PSRAM_RETENTION FAIL` Or `PSRAM_INCREMENT_PATTERN FAIL`

Check the PSRAM data-retention configuration, the `AP_PSRAM_DATA_SECTION` size, and the PSRAM power supply during AP OFF. `PSRAM_RETENTION_COMPARE FAIL` with `crc_pass != pattern_pass` indicates unstable PSRAM reads rather than a stable data corruption pattern.

### 8.3 BLE Disconnects After AP Resume

Ensure both cores use `CONFIG_BLUETOOTH_SUPPORT_AP_PWD_RETENTION=y`, not `AP_PWD_ALL`. `AP_PWD_ALL` loses AP RAM and forces the Bluetooth stack to re-initialize on resume.

### 8.4 Current Is Higher Than Expected

LV sleeps less deeply than Deep LV. Check whether `CONFIG_PM_LV_SLEEP_ENABLE` is really required for the product; otherwise compare with `pm_secureboot_ap_fast_boot`. Also disable the PSRAM retention self-test, mbedTLS tests, iperf/VFS tests, and log dumping before formal current measurements.
