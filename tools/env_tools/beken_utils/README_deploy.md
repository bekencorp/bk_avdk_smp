# BK7259 安全固件独立生成（安全服务器部署手册）

本目录 (`beken_utils`) 可整体拷贝/打包 (zip) 部署到安全服务器，独立完成 BK7259
安全固件的签名、flash 加密与打包，无需完整 SDK 编译环境。签名私钥只保存在安全
服务器上。

## 1. 目录结构

```
beken_utils/
├── deployment_main.sh     # 一键生成入口
├── gen_keys.sh            # 一次性密钥生成（首次/换代）
├── main.py                # 打包 CLI
├── scripts/               # 签名/加密/打包脚本
├── tools/                 # secure_boot_tool / mcuboot_tools / packager_tools
├── config/
│   ├── key/               # 固定 EC256 签名密钥对（安全服务器持有）
│   │   ├── root_ec256_privkey.pem
│   │   └── root_ec256_pubkey.pem
│   └── flash_aes_key.txt  # （可选）flash 加密密钥，供注入 security.csv
├── input_dir/             # 放入原始固件 + 配置（构建产物 install/pack）
└── output_dir/            # 生成的安全固件
```

## 2. 输入约定（input_dir）

安全构建（`CONFIG_SECURITY_FIRMWARE=y`）在构建服务器产出
`{build}/bk7259/install/pack/`，将其内容整体拷入本目录的 `input_dir/`：

必需文件：

| 文件 | 说明 |
|------|------|
| `bl2.bin` | MCUboot BL2（BootROM 校验）|
| `tfm_s.bin` | 安全世界 TF-M（AP 侧）|
| `cpu0_app.bin` | 非安全应用（cp 侧）|
| `ap_app.bin` | AP 固件 |
| `partitions.csv` | 分区表 |
| `security.csv` | 安全配置（含 `flash_aes_key`）|
| `ota.csv` | OTA 策略 |
| `pack.json` | 打包清单 |
| `ppc.csv` / `gpio_dev.csv` | 外设权限配置 |
| `ppc_config.bin` / `ppc_config_ap.bin` | （可选）预编译 PPC 数据 |

## 3. 密钥配置（仅两类）

1. **EC256 签名密钥对**：位于 `config/key/`，BL1 + BL2 共用。其公钥必须与固件构建
   时编译进 MCUboot 的公钥一致，且其 hash 已烧入 OTP ROTPK，否则安全校验失败。
   `deployment_main.sh` 运行时会自动把该密钥对拷入 `input_dir/`，由安全服务器掌控
   签名私钥。
2. **flash 加密密钥**：256bit（或 128bit）hex，写在 `input_dir/security.csv` 的
   `flash_aes_key` 字段（`flash_aes_type=FIXED`）。也可临时注入（见第 5 节）。

首次/换代生成密钥：

```sh
./gen_keys.sh            # 生成 EC256 密钥对 + flash AES 密钥
./gen_keys.sh --force    # 覆盖已存在密钥
```

## 4. 生成安全固件

```sh
./deployment_main.sh ./input_dir ./output_dir
```

执行流程：

1. 校验参数、清空 `output_dir`（保留 `.gitignore`）；
2. 将 `config/key/` 的 EC256 密钥对拷入 `input_dir/`；
3. （可选）注入 flash 加密密钥到 `input_dir/security.csv`；
4. `main.py pack all --config_dir input_dir`：
   - 生成 `otp_efuse_config.json`（ROTPK hash + flash AES key，明文，供烧 OTP）；
   - BL1 签名（`secure_boot_tool`）→ `primary_manifest.bin`；
   - BL2 签名（MCUboot imgtool, EC-P256）→ `primary_all_code_signed.bin`；
   - 按 `security.csv` 对各 code 分区做 XTS-AES 加密；
   - 按 `pack.json` 合并；
5. 收集产物到 `output_dir`。

## 5. 临时注入 flash 加密密钥（可选）

```sh
FLASH_AES_KEY=<64或128位hex> ./deployment_main.sh ./input_dir ./output_dir
```

或把 hex 写入 `config/flash_aes_key.txt`（`gen_keys.sh` 会自动写入）。未提供时沿用
`security.csv` 中的现有值。

## 6. 产物（output_dir）

| 文件 | 用途 |
|------|------|
| `all-app.bin` | 量产烧录镜像（应用区）|
| `bootloader.bin` | 启动链镜像（bl1_control/boot_flag/manifest/bl2）|
| `ota.bin` | OTA 升级包 |
| `otp_efuse_config.json` | OTP/efuse 烧录配置（ROTPK / flash AES key / 使能位）|

## 7. OTP 烧录说明

`otp_efuse_config.json` 由 `gen_otp.py` 生成，包含：

- `bl1_rotpk_hash @ 0x42100628`、`bl2_rotpk_hash @ 0x42100648`
  （BL1 = `secure_boot_tool` 输出；BL2 = `SHA256(DER 公钥)`；均按 32bit 字序反转为小端）；
- `flash_aes_key1/2 @ 0x42100560 / 0x42100580`；
- `Security_Ctrl` 使能位：`flash_aes_enable`、`secure_boot_supported` 等。

文件末尾附有各字段的小端注释，便于产线核对。**首次量产前**需用该文件烧写 OTP，之后
设备才进入安全启动。

## 8. 注意事项

- **密钥一致性**：`config/key/root_ec256_*.pem` 必须与固件（编译进 MCUboot 的公钥）
  及 OTP ROTPK 匹配；换代需重编固件 + 重烧 OTP。
- 私钥只保存在安全服务器，禁止外传。
- 依赖：`python3`，`cryptography`、`pycryptodome`（服务器预装，或随 `bk_py_libs`
  携带；`deployment_main.sh` 会在检测到 `bk_py_libs.tgz` 时自动解压并设置 `PYTHONPATH`）。
- BL1 签名依赖 `tools/sh_sec_tools/secure_boot_tool`（随包携带）。
