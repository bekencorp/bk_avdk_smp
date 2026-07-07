PBAP PCE demo

功能说明：
该 demo 用于进行 PBAP PCE API 的功能验证。
它提供了一组 CLI 命令，用于初始化 PBAP PCE、连接/断开远端设备、
处理 OBEX 认证、浏览电话本目录、获取电话本大小、下载电话本数据、
列出 vCard 条目、获取单个 vCard，以及中止当前操作。

源文件说明：
1. pbap_pce_demo.c
   实现 PBAP PCE 回调注册、初始化/去初始化、事件队列/线程，
   以及 PBAP 事件处理逻辑。

2. pbap_pce_demo_cli.c
   实现 `pbap` CLI 命令及其参数解析。

CLI 用法：
  如果运行在 SMP 分支上，需要在命令前加上 `ap_cmd` 前缀，例如：
  ap_cmd pbap init

  pbap init
  pbap deinit
  pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]
  pbap disconnect
  pbap auth <pin> [user_id]
  pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]
  pbap set_path root|parent|child [folder_name]
  pbap get_vcard_list [folder_name] [offset]
  pbap get_vcard [vcard_name]
  pbap get_size pb|mch|och|ich|cch|spd|fav
  pbap abort

命令说明：
1. pbap init
   创建 PBAP 事件队列/线程，注册 PBAP 回调，并调用 `bk_bt_pbap_pce_init()`。

2. pbap deinit
   调用 `bk_bt_pbap_pce_deinit()`，注销回调，并停止 PBAP 事件任务。

3. pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]
   连接远端 PBAP 服务端。
   如果带有 `auth` 参数，则在连接时同时携带可选的 `pin` 和 `user_id`。
   连接成功后的设备地址会被保存，供后续命令复用。

4. pbap disconnect
   断开当前已保存设备地址对应的连接。

5. pbap auth <pin> [user_id]
   向当前已保存设备地址对应的远端设备发送 OBEX 认证响应。
   当对端主动发起认证时，使用该命令回复 response。

6. pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]
   从远端设备下载一个电话本对象或一个通话记录对象。
   对象映射如下：
   - pb  -> 主电话本, telecom/pb.vcf
   - mch -> 未接来电记录, telecom/mch.vcf
   - och -> 呼出记录, telecom/och.vcf
   - ich -> 呼入记录, telecom/ich.vcf
   - cch -> 综合通话记录, telecom/cch.vcf
   - spd -> 快速拨号, telecom/spd.vcf
   - fav -> 收藏联系人, telecom/fav.vcf
   `offset` 为可选参数，默认值为 0。

7. pbap set_path root|parent|child [folder_name]
   修改远端设备当前的 PBAP 路径。
   示例：
   - pbap set_path root
   - pbap set_path child telecom
   - pbap set_path child pb
   - pbap set_path child cch
   PBAP 虚拟目录结构：
   root
   |-- telecom
   |   |-- pb
   |   |-- ich
   |   |-- och
   |   |-- mch
   |   |-- cch
   |   |-- spd
   |   |-- fav
   |-- SIM1
       |-- telecom
           |-- pb
           |-- ich
           |-- och
           |-- mch
           |-- cch

8. pbap get_vcard_list [folder_name] [offset]
   获取当前目录或指定目录名下的 vCard 列表。
   如果省略 `folder_name`，默认对象名为 `pb`。
   `offset` 为可选参数，默认值为 0。

9. pbap get_vcard [vcard_name]
   下载单个 vCard 对象。
   如果省略 `vcard_name`，默认对象名为 `0.vcf`。
   `vcard_name` 应从 `pbap get_vcard_list` 返回的 handle 中选择，例如 `0.vcf`。

10. pbap get_size pb|mch|och|ich|cch|spd|fav
    获取指定对象在远端设备上的电话本大小。

11. pbap abort
    中止当前已保存设备地址对应的 PBAP 操作。

典型测试步骤：
1. 初始化 PBAP PCE：
   pbap init

2. 连接远端设备：
   pbap connect xx:xx:xx:xx:xx:xx

3. 如果对端要求进行 OBEX 认证：
   pbap auth 123456

4. 获取主电话本：
   pbap get_phonebook pb 0

5. 获取未接来电记录：
   pbap get_phonebook mch 0

6. 浏览目录：
   pbap set_path root
   pbap set_path child telecom
   pbap set_path child pb

7. 获取 vCard 列表：
   pbap get_vcard_list pb 0

8. 获取单个 vCard：
   pbap get_vcard 0.vcf

9. 获取电话本大小：
   pbap get_size pb

10. 如有需要，中止传输：
    pbap abort

11. 断开连接并去初始化：
    pbap disconnect
    pbap deinit

预期行为：
1. `INIT/DEINIT/CONNECT/DISCONNECT/SET_PHONEBOOK/GET_SIZE/ABORT` 事件会打印状态日志。

2. `CONNECT_CFM_EVT` 会打印：
   - status
   - 远端设备地址
   - supported repositories
   - supported features

3. `GET_PHONEBOOK_CFM_EVT`：
   接收到的 body 会按 vCard 数据进行解析。
   demo 会打印联系人姓名、电话号码，以及在可用时打印通话时间。
   当传输结束时，还会打印 vCard 总数。

4. `GET_VCARD_LIST_CFM_EVT`：
   接收到的 body 会按 vCard 列表数据进行解析。
   demo 会打印卡片名称和 handle，例如：
   <card handle="0.vcf" name="Name"/>

5. `GET_VCARD_CFM_EVT`：
   接收到的 body 会按单个 vCard 进行解析并以类似方式打印。

注意事项：
1. 除 `pbap init`、`pbap deinit` 和 `pbap connect` 之外，其余命令都会使用已保存的连接设备地址。
   如果当前还没有已连接设备，CLI 会提示：
   "pbap: not connected. connect first."

2. demo 使用的默认对象名如下：
   - `get_phonebook/get_size` 在选择 `pb` 时，默认路径为 `telecom/pb.vcf`
   - `get_vcard_list` 的默认对象名为 `pb`
   - `get_vcard` 的默认对象名为 `0.vcf`
