# OTP2 配置与烧录指南

本文档说明 secureboot_xxx 工程中 OTP2 区域的配置方法、客户自定义空间的使用，以及如何通过 `otp_efuse_config.json` 由 bkfil 工具烧写进硬件。

---

## 1. 概述

- OTP2 是通过 AHB 访问的一次性可编程（OTP）存储区，总容量 `0xC00`（3072 字节，768 个 32-bit 字）。
- OTP2 的条目布局由 `otp2.csv` 描述，编译时由脚本 `tools/env_tools/beken_utils/scripts/gen_otp_map.py` 生成枚举与映射表 `_otp.h` / `_otp.c`（`otp2_id_t`、`otp_map_2[]`）。
- OTP 特性：每个 bit 只能从 `0` 编程为 `1`，不可回写；未编程读回为全 `0`。

### 1.1 地址映射

| 视图 | 基址 | 说明 |
| --- | --- | --- |
| 安全 / 基址视图 | `0x42010000` | 烧录工具（bkfil）与 JSON 使用此视图 |
| 非安全视图 | `0x52010000` | 运行时 NS 代码（AP / CP 应用态）访问 |

两者指向**同一块物理 fuse**（相差 `0x10000000` 的 TrustZone 别名）。

某条目的物理地址：

```
物理地址 = 0x42010000 + 该条目在 otp2.csv 中的 offset
```

> 说明：OTP2 数据区从寄存器块偏移 `0x0` 开始（无额外数据窗偏移），因此地址即 `基址 + offset`。

---

## 2. OTP2 空间布局

| 区间 | 用途 | 安全属性 | 可否修改 |
| --- | --- | --- | --- |
| `0x000` ~ `0x600` | 平台标准区：PHY 参数、RF 校准、MAC、GADC/温度校准、器件 ID 等 | 非安全 | 不建议改动 |
| `0x600` ~ `0xC00` | **客户自定义区（绿色）** | 可安全 / 可非安全 | 客户按需分配 |

- `0x600` 是标准区与自定义区的分界，且为 256 字节对齐（`0x600 = 6 × 0x100`）。
- 自定义区可分配多个条目，可配置为非安全，也可配置为安全。

---

## 3. 操作步骤

### 步骤 1：将 otp2.csv 拷贝到工程

客户需要自定义写入内容时,以 board 目录下的标准表为基准，拷贝到工程分区目录（工程副本优先于 board 表被编译使用）：

```bash
cp code/cp/middleware/boards/bk7259/csv/otp2.csv \
   code/projects/secureboot_xip/partitions/bk7259/otp2.csv
```

之后所有自定义均在工程副本 `projects/secureboot_xip/partitions/bk7259/otp2.csv` 上进行。

### 步骤 2：在 0x600 之后分配自定义条目

#### 2.1 CSV 字段说明

```
id,   name,        size,   offset,   end,     privilege,        security,   crc
```

| 字段 | 含义 |
| --- | --- |
| `id` | 条目序号，必须从 0 连续递增（= 行号） |
| `name` | 条目名，将生成为 `otp2_id_t` 枚举成员 |
| `size` | 数据长度（字节，十进制） |
| `offset` | 起始偏移（十六进制） |
| `end` | 结束偏移（十六进制），须满足 `end - offset == size` |
| `privilege` | `OTP_READ_WRITE` / `OTP_READ_ONLY` / `OTP_NO_ACCESS` |
| `security` | `TRUE` = 安全，`FALSE` = 非安全 |
| `crc` | 是否附加 CRC，一般 `FALSE` |

#### 2.2 生成脚本的校验规则（务必满足，否则编译报错）

1. `id` 从 0 连续递增，不能跳号；
2. `offset` 不能小于上一条目的 `end`（不可重叠 / 回退）；
3. `size == end - offset`。

#### 2.3 安全属性与对齐规则

- 自定义条目可设为非安全（`FALSE`）或安全（`TRUE`）。
- **安全与非安全的分界必须按 256 字节（`0x100`）对齐**：同一 256 字节块内不能同时存在安全与非安全条目。规划时让每段安全属性相同的区域起止都落在 256 字节边界上。
- 标准区（`0x000`~`0x600`）全部为非安全，`0x600` 起可开始放置安全条目，天然满足对齐。

#### 2.4 示例：在 0x600 处新增一个非安全私钥条目

将自定义区（`0x600` 之后）改为如下（示例把原 `0x600` 起的条目替换为自定义项）：

```
24,   OTP_DEVICE_ID,       8,     0x5F8,   0x600,   OTP_READ_WRITE,   FALSE,   FALSE,
25,   OTP_PRIV_KEY,        32,    0x600,   0x620,   OTP_READ_WRITE,   FALSE,   FALSE,
26,   OTP_CUSTOM_RESERVED, 1504,  0x620,   0xC00,   OTP_READ_WRITE,   FALSE,   FALSE,
```

- `OTP_PRIV_KEY` 位于 `0x600`，32 字节，非安全；其枚举值等于其行号（此处为 `25`）。
- 剩余空间用一个保留条目占满到 `0xC00`，保证 `id` 连续、无空洞。

修改后重新编译，生成的 `otp2_id_t` 即包含该条目。

### 步骤 3：配置 otp_efuse_config.json 供 bkfil 烧写

烧录工具 bkfil 读取
`projects/secureboot_xip/build/bk7259/secureboot_xip/package/otp_efuse_config.json`
中的 `Security_Data` 数组，将每个条目按 `start_addr` 写入硬件。

在 `Security_Data` 中为自定义条目追加一段：

```json
{
    "name": "priv_key",
    "mode": "write",
    "permission": "RO",
    "start_addr": "0x42010600",
    "last_valid_addr": "0x42010620",
    "byte_len": "0x20",
    "data": "0123456789...abcdef",
    "data_type": "hex",
    "status": "true"
}
```

字段计算方法：

| 字段 | 取值 | 说明 |
| --- | --- | --- |
| `start_addr` | `0x42010000 + offset` | 例：`offset=0x600` → `0x42010600` |
| `last_valid_addr` | `start_addr + byte_len` | 例：`0x42010600 + 0x20 = 0x42010620` |
| `byte_len` | 十六进制字节数 | 例：32 字节 → `0x20` |
| `data` | 待烧写数据（hex 串） | 客户填入真实数据 |
| `mode` | `write` / `read` | `read` 时用于回读校验 |
| `permission` | `RO` | 烧写后置为只读 |

> 提示：现有 `flash_aes_key`、`bl1/bl2_rotpk_hash` 条目也遵循 `0x42010000/0x42100000 + offset` 的地址约定，可作参考。

---

## 4. 烧录与验证

### 4.1 烧录

使用 bkfil 加载上面的 `otp_efuse_config.json`，将固件与 OTP 数据一并烧入。烧录后复位。

### 4.2 CLI 回读验证

需要在配置中开启 OTP 测试命令（本工程已在 AP 侧开启）：

```
CONFIG_OTP_V1=y
CONFIG_OTP_TEST=y
```

在控制台使用（`item_id` 为枚举序号，`size` 为字节数，均为十进制）：

```
otp_ahb read  <item_id> <size>          # 读取
otp_ahb write <item_id> <size> <hex>    # 写入
otp_ahb read_permission <item_id>       # 查看权限
```

例：读取上文 `OTP_PRIV_KEY`（枚举值 25，32 字节）：

```
otp_ahb read 25 32
```

读通路会打印实际访问地址与原始值，便于核对：

```
[otp_ahb_read] item=25 off=0x600 ... phys=0x52010600
[otp_ahb_read] loc=384 addr=0x52010600 raw=0x........
```

- `phys` 为运行时（非安全别名）地址，与烧录地址 `0x42010600` 为同一位置。
- `raw` 全 0 表示该 fuse 未编程；非 0 即为已烧入数据。
- 注意字节序：读回按小端逐字显示，与烧录 hex 串对照时留意大小端。

---

## 5. 注意事项

1. OTP 为一次性：bit 只能 `0→1`，写错不可恢复，量产前务必先在样片上 `read` 回读确认。
2. `otp_efuse_config.json` 位于 build 目录. 烧录前请确认该文件包含所需条目，或单独保存一份烧录用的 JSON。
3. 自定义区规划遵循「id 连续、offset 不回退、size=end-offset、安全/非安全 256 字节对齐」四条规则。
4. `otp2.csv` 的源头优先查找自定义的工程副本 `projects/secureboot_xxx/partitions/bk7259/otp2.csv`；board 目录下的表仅作基准模板。
