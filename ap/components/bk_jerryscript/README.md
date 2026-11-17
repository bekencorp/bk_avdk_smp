# bk_jerryscript

板级 JavaScript 运行时，基于 JerryScript，提供 `bk_js_init()` / `bk_js_eval()` /
`bk_js_app_start()` 等接口。脚本既可以编译进固件（见 `tools/README.md` 的 js2c.py），
也可以在运行期从文件系统加载。

## 0. 引擎源码

JerryScript 引擎内核的源码位于本组件的 `jerryscript/jerry-core`（上游原样），
随组件一起编译，不需要额外的工程配置。

只依赖 libc、libm 和本组件实现的 `jerry_port_*` 钩子（`src/bk_js_port.c`）。


## 1. 文件系统应用


开启下列配置后，组件会在初始化时把应用文件系统挂载到 `CONFIG_BK_JERRYSCRIPT_APPS_ROOT`
（默认 `/apps`）：

```text
CONFIG_LITTLEFS=y
CONFIG_VFS=y
CONFIG_BK_JERRYSCRIPT_FS=y
CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT=y
```

挂载失败只打印警告，不会阻塞启动；此时只有文件系统应用不可用，内嵌脚本（`bk_js_eval()`）仍然能跑。

### 1.1 目录布局

镜像挂载点是 `/apps`，所以镜像根目录下直接放应用目录，**不要再套一层 `apps/`**：

```text
/                                  <- littlefs 镜像根，挂载为 /apps
└── launcher/
    ├── manifest.json
    ├── main.js
    └── lib/*.js                   <- 可选，供 require('lib/foo.js') 使用
```

`manifest.json` 字段：

```json
{
  "id": "launcher",
  "name": "Launcher",
  "version": "1.0.0",
  "entry": "main.js"
}
```

`bk_js_app_start("launcher")` 会读取 `/apps/launcher/manifest.json`，再执行 `entry`
指定的入口脚本。

## 2. 制作 littlefs bin

工具为 `ap/components/littlefs/mkimg/mklittlefs`，参数说明见同目录 README。
组件的 littlefs 适配层固定使用 4096 字节 block、256 字节 page，打包参数必须一致：

```text
-b 4096  -p 256  -s <分区大小，字节>
```

`-s` 必须等于 `usr_config` 分区大小（见编译输出的 `partitions.txt`），否则挂载后
block_count 不匹配。

与具体工程无关的通用步骤、参数推导和排查表见
[docs/pack_fs_image.md](docs/pack_fs_image.md)。下面以 `launcher_v9` 为例。

### 2.1 步骤

以 `projects/lvgl/launcher_v9` 为例，在**工程目录**下执行：

```bash
# 1. 准备镜像根目录，目录名就是应用 id
mkdir -p fs/launcher

# 2. 合并 JS（顺序即依赖顺序，公共库在前）
cat ap/js/ui_common.js ap/js/launcher.js ap/js/game_2048.js \
    ap/js/juice_rush.js ap/js/main.js > fs/launcher/main.js

# 3. 写清单，entry 与 main.js 对应
cat > fs/launcher/manifest.json <<'EOF'
{
  "id": "launcher",
  "name": "Launcher",
  "version": "1.0.0",
  "entry": "main.js"
}
EOF

# 4. 打包（1MB，与 usr_config 分区一致）
../../../ap/components/littlefs/mkimg/mklittlefs \
    -c fs -b 4096 -p 256 -s 1048576 littlefs_js.bin
```

产物 `littlefs_js.bin` 固定为 `-s` 指定的大小，不足部分自动补齐。

### 2.2 烧录

写到 `usr_config` 分区起始地址：

| 工程 | 分区 | 地址 | 大小 |
| --- | --- | --- | --- |
| launcher_v9 (bk7259) | `usr_config` | `0x5C9000` | 1024KB |

地址由分区表自动排版决定，扩容 AP 应用分区后会变化，务必以
`build/<chip>/<project>/partitions/partitions.txt` 为准。

### 2.3 验证

```text
ap0:js:I(xxx):mounted /apps on usr_config (0x5c9000, 1048576 bytes)
ap0:js:D(xxx):  launcher
ap0:js:D(xxx):read /apps/launcher/main.js: 65461 bytes
ap0:js:I(xxx):js app 'launcher' running from fs
```

如果出现 `cannot open /apps/launcher/main.js`，先确认镜像根目录是否多套了一层
`apps/`；如果出现 `no memory for ...`，说明脚本缓冲分配失败。

## 3. 网络绑定（可选）

打开 `CONFIG_BK_JERRYSCRIPT_NET` 后，引擎向 JS 暴露 `wifi` 与 `http` 两个全局对象。
所有网络操作在独立的 worker 线程执行，不阻塞 JS / LVGL 任务。

| 开关 | 作用 |
| --- | --- |
| `CONFIG_BK_JERRYSCRIPT_NET` | 打开本组件的网络绑定 |
| `CONFIG_LWIP` / `CONFIG_BK_NETIF` / `CONFIG_WEBCLIENT` | TCP/IP 协议栈与 HTTP 客户端 |
| `CONFIG_WIFI_VNET_CONTROLLER`（AP 核） | WiFi 控制通道 |
| `CONFIG_WEBCLIENT_TLS` + `CONFIG_PSA_MBEDTLS` | 支持 `https://` |

### 3.1 API

| API | 返回 | 说明 |
| --- | --- | --- |
| `wifi.status()` | 对象（同步） | `{connected, state, ssid, rssi, ip, gateway, dns}` |
| `wifi.connect(ssid, password?)` | Promise | 成功 resolve `{connected:true, ip}`；失败 reject `{connected:false, error}` |
| `http.get(url)` | Promise | resolve `{status, length, body}` |
| `http.post(url, body?, options?)` | Promise | 同上，`options = {contentType, timeout}` |
| `http.request(url, options)` | Promise | `options = {method, body, contentType, timeout}`，method 仅支持 `GET` / `POST` |

`http.*` 失败时 reject 字符串，例如 `empty url`、`connect or request failed`、
`no ip address (timeout)`、`https is not enabled in this firmware`。

### 3.2 限制

| 项目 | 上限 / 行为 |
| --- | --- |
| URL 长度 | 191 字节，超出抛 TypeError |
| 请求 body | 4096 字节，超出抛 RangeError |
| 响应 body | 16384 字节，超出截断；`length` 为实际返回长度 |
| 请求超时 | 默认 10000ms，可用 `options.timeout` 覆盖（仅覆盖 HTTP 阶段） |
| WiFi 连接超时 | 20000ms，固定 |
| 待执行队列 | 4，满后抛 `network queue full` |
| 未完成请求 | 8，超出抛 `too many pending network requests` |
| 执行模型 | 单 worker 线程串行执行；首次请求时惰性创建（TLS 下 20KB 栈） |

### 3.3 使用示例

```js
http.get('http://192.168.1.10:8000/hello')
    .then(function (r) {
        console.log('status=' + r.status + ' body=' + r.body);
    })
    .catch(function (err) {
        console.log('http failed: ' + err);
    });

http.post('http://192.168.1.10:8000/echo', 'hello', { contentType: 'text/plain' })
    .then(function (r) {
        console.log('echo: ' + r.body);
    });
```

串口调试：

```text
js eval typeof http
js eval http.get('http://192.168.1.10:8000/').then(function(r){console.log(r.status,r.body)},function(e){console.log('err '+e)})
js run /apps/net_demo.js
```

- `js eval` 的输入缓冲只有 160 字节，长用例请写成脚本文件后用 `js run`。
- Promise 回调是异步的，命令先返回，`console.log` 的输出稍后才出现在日志里。
- 打开 `CONFIG_LAUNCHER_JS_FROM_FS` 时 UI 脚本占用引擎，`js eval` / `js run` 会返回 busy。

### 3.4 已知限制

- `https://` 默认不校验证书。
- HTTPS 请求收到 301/302 时存在 use-after-free 风险（SDK `webclient` 侧问题），
  请只访问不重定向的 HTTPS 地址。
- TLS 连接/读取没有独立超时，异常网络下 worker 线程可能长时间阻塞。
- HTTPS + chunked 响应存在重复关闭同一 socket 的问题（SDK `webclient` 侧问题）。

## 4. 注意事项

- 修改 JS 后只需重新打包镜像并烧录，**无需重新编译固件**。
- 镜像缺失或损坏不影响内嵌脚本方式启动，只会打出
  `file system not mounted ... fs applications unavailable`。
- 运行期读取脚本和 `require()` 的缓冲都走 PSRAM，不占用 SRAM 堆。
- 当前版本的 mklittlefs 在 `-l` / `-u` 读取路径上可能挂起，校验镜像建议直接启动板子看日志。
- 文件系统方式需要工程打开 `CONFIG_BK_JERRYSCRIPT_FS`；是否需要自动挂载由
  `CONFIG_BK_JERRYSCRIPT_FS_AUTO_MOUNT` 控制。
