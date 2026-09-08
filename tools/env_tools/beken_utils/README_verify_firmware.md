# 安全固件验证脚本 verify_firmware.py

离线校验一套安全启动固件（`bootloader.bin` + `all-app.bin`）是否由已知密钥合法生成。

## 依赖

- Python 3
- `cryptography`（`pip install cryptography`）

## 快速使用

在 `tools/env_tools/beken_utils/` 目录下执行：

```bash
# 使用默认路径：config/ 下密钥 + output_dir/ 下固件（默认按加密固件校验）
python3 verify_firmware.py

# 非加密固件
python3 verify_firmware.py --plaintext

# 指定固件与密钥路径
python3 verify_firmware.py \
    --bootloader /path/bootloader.bin \
    --all-app    /path/all-app.bin \
    --root-pubkey /path/root_ec256_pubkey.pem \
    --flash-key   /path/flash_aes_key.txt
```

退出码：全部通过为 `0`，存在失败项为 `1`（便于脚本/CI 集成）。

## 参数说明

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `--bootloader` | `output_dir/bootloader.bin` | bootloader 固件路径 |
| `--all-app` | `output_dir/all-app.bin` | 应用固件路径 |
| `--root-pubkey` | `config/key/root_ec256_pubkey.pem` | 根公钥（PEM） |
| `--flash-key` | `config/flash_aes_key.txt` | Flash AES 密钥，可为文件路径或十六进制字符串 |
| `--otp` | `output_dir/otp_efuse_config.json` | 可选，用于 ROTPK 交叉比对 |
| `--encrypted` / `--plaintext` | `--encrypted` | 固件是否启用 Flash 加密 |

说明：
- 加密可在编译期关闭，因此加密开关是独立参数。加密开启时必须提供正确的 Flash 密钥；关闭时脚本跳过解密直接校验。
- 本芯片 Flash 加密固定为 AES-128-XTS，密钥为 64 个十六进制字符（32 字节 k1||k2）。
- 脚本已适配本芯片的 Flash CRC 交织（每 32 字节数据 + 2 字节 CRC = 34 字节物理单元），会自动去交织后再校验。

## 校验项

bootloader.bin：
- 识别并报告 BootROM 安全启动 magic（`BK7236`）
- Manifest 签名用根公钥验签
- Manifest 内嵌公钥与根公钥一致
- Manifest 中的 BL2 摘要与（去交织并解密后的）BL2 镜像实际摘要一致
- 提供 `--otp` 时，OTP 中的 ROTPK 与根公钥一致

all-app.bin：
- 解析下载容器并定位应用镜像
- 应用镜像哈希与其签名负载一致
- 应用镜像签名用根公钥验签
- 应用内嵌公钥与根公钥一致
- 提供 `--otp` 时，OTP 中的 ROTPK 与应用公钥一致

## 输出说明

- `PASS`：校验通过
- `FAIL`：校验失败（固件不合法、密钥不匹配或加密开关设置错误）
- `INFO`：仅信息展示（签名策略、地址、计数器等），不计入通过/失败

## 实现说明

脚本已完整适配本芯片的 Flash 物理布局，加密固件也能完成全部校验：

- Flash CRC 交织：读取 code 区后按 34 字节物理单元去交织，还原 CPU 可见的虚拟镜像；
- Flash 加密：AES-128-XTS，以数据单元的虚拟字节地址为 tweak，单元内按 32bit 字做字节序调整；
- BootROM magic：识别位于 `0x110` 的 `BK7236`；
- BL2/应用镜像定位：BL2 依据 manifest 记录的地址定位，应用镜像在下载容器中定位后去交织解密。

## 常见失败排查

- 全部解密相关项失败：确认加密开关（`--encrypted`/`--plaintext`）与实际固件一致，或 Flash 密钥是否正确。
- 提示定位不到应用镜像：多为密钥错误或加密开关设置错误导致解密结果异常。
- ROTPK 比对失败：确认 `--root-pubkey` 与 `--otp` 是否属于同一套密钥。
