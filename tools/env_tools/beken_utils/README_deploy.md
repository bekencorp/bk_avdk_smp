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
   `flash_aes_key` 字段（`flash_aes_type=FIXED`）。也可临时注入（见第 6 节）。

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

## 5. 分步离线签名（pack server / sign server）

第 4 节是"一台机器一步到底"。若要求**私钥不出签名机**，则把同一条流程拆成若干个
`pack run` 阶段：pack server 只算摘要、装回签名、打包；sign server 只做签名，是唯一
持有 `root_ec256_privkey.pem` 的一方。

### 5.1 两侧的前置条件

- 两侧各有一份 `beken_utils`，并且 `pack.json` / `partitions.csv` / `ota.csv` /
  `security.csv` **逐字节一致**（它们决定地址、`app_version`、`security_counter`，
  不一致则 hash 必然不同、签名验不过）。推荐把同一次构建的 `install/pack/` 原样拷到
  两侧。
- sign server 需要 `root_ec256_privkey.pem`；pack server **不需要**（连
  `bl1_manifest_digest` 那条路也不需要，见 5.4）。

### 5.2 阶段与命令

全部在**同一个工作目录**内就地累积（示例为 XIP；OVERWRITE 完全相同的命令序列）：

| 顺序 | 机器 | 命令 | 输入 | 产出 |
|---|---|---|---|---|
| 1 | pack | `main.py pack run --preset hash` | `install/pack/` 全部输入 | `manifest_hash.json`、`app_hash.json` |
| 2 | sign | `main.py pack run --preset sign-hash` | 上面两个 hash + `security.csv` | `manifest_sig.json`、`app_sig.json` |
| 3 | pack | `main.py pack run --preset apply-sig` | 两个 sig | `primary_manifest.bin`、`primary_all_code_signed.bin` |
| 4 | pack | `main.py pack run --preset hash-ota` | 上一步的 app 镜像 | `ota_hash.json` |
| 5 | sign | `main.py pack run --preset sign-hash-ota` | `ota_hash.json` | `ota_sig.json` |
| 6 | pack | `main.py pack run --preset apply-sig-ota` | `ota_sig.json` | `ota_signed.bin` |
| 7 | pack | `main.py pack run --preset pack` | 全部签名结果 | `all-app.bin` / `ota.bin` / `bootloader.bin` / `install/` |

实际部署时用 `--config_dir` 指向工作目录：

```sh
# pack server（全程无 root_ec256_privkey.pem）
python3 -B main.py pack run --preset hash          --config_dir ./work
#   ── 只把 manifest_hash.json / app_hash.json 送到 sign server ──
# sign server（唯一持有私钥）
python3 -B main.py pack run --preset sign-hash     --config_dir ./work
#   ── 只把 manifest_sig.json / app_sig.json 送回 pack server ──
python3 -B main.py pack run --preset apply-sig     --config_dir ./work
python3 -B main.py pack run --preset hash-ota      --config_dir ./work
#   ── 只把 ota_hash.json 送到 sign server ──
python3 -B main.py pack run --preset sign-hash-ota --config_dir ./work
#   ── 只把 ota_sig.json 送回 pack server ──
python3 -B main.py pack run --preset apply-sig-ota --config_dir ./work
python3 -B main.py pack run --preset pack          --config_dir ./work
```

跨机器的只有 `*_hash.json` / `*_sig.json`；`primary_all_code.bin`、`compress.bin`、
`primary_all_code_signed.bin` 等中间镜像始终留在 pack server，**期间不要重跑会改动它们
的动作**（否则 hash 与签名不再对应）。

也可以直接给 action 列表（调试/CI）：

```sh
python3 -B main.py pack run --steps PACK_CONTAINERS,INSERT_PK_HASH,INSTALL
```

### 5.3 结果校验

```sh
python3 -B main.py pack check            # 容器结构 + 全局 CRC32 + 子镜像 checksum + boot_param
python3 -B verify_firmware.py --bootloader bootloader.bin --all-app all-app.bin \
    --root-pubkey root_ec256_pubkey.pem --flash-key <64位hex> --otp otp_efuse_config.json
```

`pack run` **不会**重置 `pack_cmd_list.txt`（与 `pack all` 不同），因此分步执行后该文件
完整记录整条链路，可用来核对两侧执行的命令是否一致。

### 5.4 需要把 ROTPK hash 写进镜像时（opt-in）

`INSERT_PK_HASH` 把首 4K 块尾部的 `0xFC0..0x1000` 覆盖为
`bl1_rotpk_hash || bl2_rotpk_hash`（各 32B，同一把 EC256 根公钥的两种编码：
BL1 = `SHA256(未压缩点)`，BL2 = `SHA256(DER SPKI)`）。BK7259 现网的两个 hash 都从 OTP
eFuse 读取，该槽位默认保持 `0xFF`，所以它**不在任何内置预设里**。若确认 ROM 需要镜像内
副本，在工程的 `pack.json` 里声明：

```json
"pipelines": {
    "all-pk-hash": ["all", "INSERT_PK_HASH", "INSTALL", "IMAGE_CHECK"]
}
```

```sh
python3 -B main.py pack run --preset all-pk-hash --config_dir ./work
```

`INSERT_PK_HASH` 必须在 `INSTALL` **之前**，否则 `install/` 里的副本仍是旧字节（随后补跑
`INSTALL`，并再跑一次 `IMAGE_CHECK` 以"按出厂内容校验"）。它只需要公钥 PEM，pack
server 依旧无需私钥。

## 6. 临时注入 flash 加密密钥（可选）

```sh
FLASH_AES_KEY=<64或128位hex> ./deployment_main.sh ./input_dir ./output_dir
```

或把 hex 写入 `config/flash_aes_key.txt`（`gen_keys.sh` 会自动写入）。未提供时沿用
`security.csv` 中的现有值。

## 7. 产物（output_dir）

| 文件 | 用途 |
|------|------|
| `all-app.bin` | 量产烧录镜像（应用区）|
| `bootloader.bin` | 启动链镜像（bl1_control/boot_flag/manifest/bl2）|
| `ota.bin` | OTA 升级包 |
| `otp_efuse_config.json` | OTP/efuse 烧录配置（ROTPK / flash AES key / 使能位）|

## 8. OTP 烧录说明

`otp_efuse_config.json` 由 `gen_otp.py` 生成，包含：

- `bl1_rotpk_hash @ 0x42100628`、`bl2_rotpk_hash @ 0x42100648`
  （BL1 = `secure_boot_tool` 输出；BL2 = `SHA256(DER 公钥)`；均按 32bit 字序反转为小端）；
- `flash_aes_key1/2 @ 0x42100560 / 0x42100580`；
- `Security_Ctrl` 使能位：`flash_aes_enable`、`secure_boot_supported` 等。

文件末尾附有各字段的小端注释，便于产线核对。**首次量产前**需用该文件烧写 OTP，之后
设备才进入安全启动。

## 9. 注意事项

- **密钥一致性**：`config/key/root_ec256_*.pem` 必须与固件（编译进 MCUboot 的公钥）
  及 OTP ROTPK 匹配；换代需重编固件 + 重烧 OTP。
- 私钥只保存在安全服务器，禁止外传。
- 依赖：`python3`，`cryptography`、`pycryptodome`、`PyYAML`（服务器预装，或随
  `bk_py_libs` 携带；`deployment_main.sh` 会在检测到 `bk_py_libs.tgz` 时自动解压并
  设置 `PYTHONPATH`，并在开始签名打包前检查 `pyyaml` 是否可用，缺失时直接报错退出）。
  其中 `PyYAML` 用于读取 `scripts/gen_otp_config.yaml`（OTP/efuse 位定义与地址表）。
  完整依赖清单见 `requirements.txt`。
- BL1 签名依赖 `tools/sh_sec_tools/secure_boot_tool`（随包携带）。
- **`pack.json` 版本配套**：`pack.json` 里可选的顶层键 `pipelines`（声明 action 顺序，
  见第 5 节）只有较新的 `beken_utils` 认识；用旧版工具跑带 `pipelines` 的 `pack.json`
  会报 `pack.json has nonexist partition <预设名>`。因此工具目录与工程的 `pack.json`
  必须一起升级，`pipelines` 只是可选键——不放它则完全走内置默认顺序。
- `main.py pack list-steps` 可打印当前内置/工程预设（含展开后的 action 序列）与所有
  已注册 action，便于排查预设名或 action 名拼写错误。
