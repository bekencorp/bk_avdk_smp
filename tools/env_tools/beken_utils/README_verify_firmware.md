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
| `--aes256` | 关闭 | 使用 256-bit XTS 密钥（保留全部 128 个十六进制字符） |

说明：
- 加密可在编译期关闭，因此加密开关是独立参数。加密开启时必须提供正确的 Flash 密钥；关闭时脚本跳过解密直接校验。
- Flash 密钥为 128 个十六进制字符时，默认取后 64 个字符用于 AES-128-XTS（与打包器一致）；如为 AES-256-XTS 请加 `--aes256`。
- 脚本假设 Flash CRC 交织为关闭状态（等距映射）。

## 校验项

bootloader.bin：
- 识别并报告签名策略（`BK.SB` 要求验签 / `BEKEN` 仅校验哈希）
- Manifest 签名用根公钥验签
- Manifest 内嵌公钥与根公钥一致
- Manifest 中的 BL2 摘要与（解密后的）BL2 镜像实际摘要一致
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

## 常见失败排查

- 全部解密相关项失败：确认加密开关（`--encrypted`/`--plaintext`）与实际固件一致，或 Flash 密钥是否正确。
- 提示定位不到应用镜像：多为密钥错误或加密开关设置错误导致解密结果异常。
- ROTPK 比对失败：确认 `--root-pubkey` 与 `--otp` 是否属于同一套密钥。
