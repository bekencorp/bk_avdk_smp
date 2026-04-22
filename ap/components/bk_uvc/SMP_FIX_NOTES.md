# BK UVC SMP 最小修复说明

## 1. 文档目的

本文档用于说明 `bk_uvc` 模块本次面向 SMP 场景的最小化修复内容，便于代码评审、联调和后续提测。

本次修改遵循以下原则：

1. 不修改对外接口。
2. 不重构现有线程模型。
3. 只修复当前最明显、最容易在 SMP 下暴露的并发风险。

本次文档仅覆盖实际已合入的代码修改，不包含之前讨论过但本次未采纳的编译期配置保护项。

## 2. 修改范围

本次修改涉及以下文件：

- `ap/components/bk_uvc/include/uvc_urb_list.h`
- `ap/components/bk_uvc/include/bk_uvc_common.h`
- `ap/components/bk_uvc/src/uvc_urb_list.c`
- `ap/components/bk_uvc/src/bk_uvc_stream.c`

本次未修改以下对外接口的函数签名：

- `bk_uvc_camera_stream_start()`
- `bk_uvc_camera_stream_stop()`
- `bk_uvc_camera_stream_suspend()`
- `bk_uvc_camera_stream_resume()`

## 3. 修改背景

在原实现中，`bk_uvc` 模块在 SMP 系统下存在几类典型风险：

1. URB 空闲链表和就绪链表的保护方式不统一，任务上下文与中断上下文走的是两套保护路径，跨核时存在并发访问风险。
2. URB list 在 deinit 过程中，可能与 `malloc/free/push/pop` 并发交叉，存在访问已释放资源的风险。
3. `bk_uvc_camera_stream_start()` 使用了 `handle->lock`，但 `bk_uvc_camera_stream_stop()` 没有，导致公开控制面串行化不完整。
4. `bk_uvc_camera_stream_suspend()` 和 `bk_uvc_camera_stream_resume()` 直接修改 `stream_state`，容易与完成回调、stream task 并发冲突。

## 4. 详细修改说明

### 4.1 统一 URB list 的并发保护模型

涉及文件：

- `ap/components/bk_uvc/src/uvc_urb_list.c`

本次新增：

- `uvc_urb_list_enter_critical()`
- `uvc_urb_list_exit_critical()`

调整点：

- `uvc_camera_urb_malloc()`
- `uvc_camera_urb_free()`
- `uvc_camera_urb_push()`
- `uvc_camera_urb_pop()`
- `uvc_camera_urb_list_clear()`
- `uvc_camera_urb_list_deinit()`

修改内容：

- 原先任务上下文走 `mutex`、中断上下文走本核关中断的分裂保护方式被收敛。
- 所有共享链表操作统一走同一套临界区入口和出口。

修改目的：

- 避免不同 CPU 以不同同步方式同时修改 `free/ready` 链表。
- 降低 SMP 压力下链表损坏、重复回收或非法摘链的风险。

### 4.2 为 URB list deinit 增加封口保护

涉及文件：

- `ap/components/bk_uvc/include/uvc_urb_list.h`
- `ap/components/bk_uvc/src/uvc_urb_list.c`

本次新增：

- `uvc_urb_list_t.deiniting`

修改内容：

- `uvc_camera_urb_list_deinit()` 在真正释放资源之前，先将 list 标记为 `deiniting`，并关闭新的进入路径。
- `free` 和 `ready` 两条链表先在受保护区内摘出，再在锁外释放对应节点。
- `buffer` 先从全局对象中摘掉，再做释放。
- `uvc_camera_urb_malloc()`、`uvc_camera_urb_free()`、`uvc_camera_urb_push()`、`uvc_camera_urb_pop()` 在 `enable == false` 或 `deiniting == true` 时直接拒绝继续执行。
- `uvc_camera_urb_pop()` 在等待 semaphore 前增加了一次状态复检，避免 deinit 过程中继续阻塞等待。

修改目的：

- 防止 deinit 过程中仍有新 URB 操作进入。
- 降低访问已释放 `buffer`、semaphore、链表节点的风险。

### 4.3 统一 stream 控制面的串行化

涉及文件：

- `ap/components/bk_uvc/src/bk_uvc_stream.c`

修改内容：

- `bk_uvc_camera_stream_stop()` 现在和 `bk_uvc_camera_stream_start()` 一样，使用 `handle->lock` 做公开控制面的串行化。
- `bk_uvc_camera_stream_suspend()` 和 `bk_uvc_camera_stream_resume()` 也纳入 `handle->lock` 的保护范围。

修改目的：

- 防止同一个 `handle` 在不同 CPU 上并发执行 `start/stop/suspend/resume`。
- 降低回调注册注销、状态检查、事件等待之间的乱序和竞态。

### 4.4 将 suspend/resume 改为 stream task 驱动

涉及文件：

- `ap/components/bk_uvc/include/bk_uvc_common.h`
- `ap/components/bk_uvc/src/bk_uvc_stream.c`

本次新增 event bit：

- `UVC_STREAM_SUSPEND_BIT`
- `UVC_STREAM_RESUME_BIT`

本次新增消息类型：

- `UVC_STREAM_SUSPEND_IND`
- `UVC_STREAM_RESUME_IND`

本次新增处理函数：

- `uvc_camera_stream_suspend_handle()`
- `uvc_camera_stream_resume_handle()`

修改内容：

- `bk_uvc_camera_stream_suspend()` 不再直接改写 `stream_state`，而是向 stream task 发送 suspend 消息并等待完成。
- `uvc_camera_stream_suspend_handle()` 会先将状态切到 `CLOSING`，在存在 in-flight URB 时等待其自然收尾，然后清空当前半帧长度，并将状态切回 `CONNECTED`。
- `bk_uvc_camera_stream_resume()` 同样改为向 stream task 发送 resume 消息并等待完成。
- `uvc_camera_stream_resume_handle()` 在必要时重新申请 URB，恢复 `STREAMING` 状态，并重新发起数据请求。
- `uvc_camera_stream_task_main()` 新增了 suspend/resume 事件分发逻辑。

修改目的：

- 将 suspend/resume 的核心状态切换放到和其他 stream 操作一致的执行上下文中。
- 降低 API 调用线程与 USB 完成回调、stream task 之间的竞争。
- 让 suspend/resume 的行为更接近现有的 start/stop 控制模型。

## 5. 行为变化说明

本次修改后，预期会有以下行为变化：

1. `start/stop/suspend/resume` 的控制路径比以前更加串行。
2. `suspend()` 不再是简单写状态立即返回，可能会等待当前 in-flight URB 收尾。
3. `resume()` 会在 stream task 内重新补齐 URB 和 request_data 流程，如果上下文不完整会返回失败。

兼容性说明：

- 不修改公开接口定义。
- 不修改 callback 接口。
- 不引入新的用户配置项。

## 6. 设计取舍

本次修复是“最小修复”，不是完整重构。

本次解决的问题：

- 修正当前最主要的 SMP 并发窗口。
- 尽量复用已有锁、event 和 stream task 机制。

本次未解决的问题：

- 未对整个 stream 状态机做彻底梳理。
- 未引入更严格的 URB 生命周期引用计数机制。
- 未统一所有 USB 错误场景的恢复策略。
- 未增加编译期 SMP 配置一致性校验。

## 7. 建议验证项

建议做以下验证：

1. 同一 port 在不同任务中反复并发执行 `start/stop`。
2. 正常拉流过程中循环执行 `suspend/resume`。
3. 拉流过程中做设备热插拔。
4. 在后台仍有 URB 活动时执行 deinit。
5. 做长时间 SMP 压测，观察是否出现死锁、卡住、链表损坏或异常超时。

重点观察项：

- `free/ready` 链表是否仍会损坏。
- `handle->lock` 和 event wait 是否会出现死锁。
- `suspend()` 后是否仍持续提交旧传输。
- `resume()` 后是否能继续稳定收流。

## 8. 当前状态

当前状态说明：

- 代码修改已经完成。
- 由于当前工作区的 IDE 头文件解析环境不完整，静态诊断结果不能作为最终编译依据。
- 完整编译结果和板级运行结果仍建议单独确认。

## 9. 关键改动索引

URB list 相关：

- `uvc_urb_list_t.deiniting`
- `uvc_urb_list_enter_critical()`
- `uvc_urb_list_exit_critical()`
- `uvc_camera_urb_list_deinit()`
- `uvc_camera_urb_malloc()`
- `uvc_camera_urb_free()`
- `uvc_camera_urb_push()`
- `uvc_camera_urb_pop()`

stream 控制相关：

- `UVC_STREAM_SUSPEND_BIT`
- `UVC_STREAM_RESUME_BIT`
- `UVC_STREAM_SUSPEND_IND`
- `UVC_STREAM_RESUME_IND`
- `uvc_camera_stream_suspend_handle()`
- `uvc_camera_stream_resume_handle()`
- `bk_uvc_camera_stream_suspend()`
- `bk_uvc_camera_stream_resume()`
- `bk_uvc_camera_stream_stop()`
