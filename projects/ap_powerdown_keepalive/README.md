AP Powerdown Keepalive Example Project

* [中文](./README_CN.md)

## 1. Overview

This project demonstrates keepalive and power-management interactions between the AP (Application core) and CP (Communication core) in a dual-core system. It shows how the AP can be powered down while the CP continues to keep a heartbeat with a remote server and how wakeup flows are performed.

## 2. Test environment

- Recommended board: BK7258 family (or compatible)
- Two development boards for server/client validation
- Both boards connected to same router (LAN)
- Serial console access to AP and CP for logs and CLI

## 3. Project layout

```
ap_powerdown_keepalive/
├── ap/        # AP-side code (AP -> CP IPC calls)
├── cp/        # CP-side code (keepalive client/server and power control)
├── CMakeLists.txt
├── Makefile
└── pj_config.mk
```

## 4. Features

- AP-side APIs and CLI to start/stop CP keepalive (`ap_cmd lp start_ka` / `ap_cmd lp stop_ka`)
- CP-side `lp client` / `lp server` CLI for keepalive demo and control messages
- AP powerdown (simulated) while CP continues keepalive; remote-triggered wakeup support

## 5. Key files

- `ap/keepalive/keepalive.c` — AP IPC wrapper APIs
- `ap/lp_ipc_msg/lp_ipc_msg.c` — AP CLI to call IPC (`ap_cmd lp start_ka/stop_ka/control`)
- `cp/keepalive/keepalive_client.c` — CP client implementation and CLI
- `cp/keepalive/keepalive_server.c` — CP server implementation and CLI
- `cp/powerctrl/powerctrl.c` — power control and wakeup helpers

## 6. Build

```
make bk7258 PROJECT=ap_powerdown_keepalive
```

## 7. Run / CLI examples (AP powerdown keepalive)

Scenario: Device-A = Server, Device-B = Client. Device-B's AP starts keepalive to Device-A and then powers down; CP should maintain the keepalive.

1) Ensure both boards are on the same LAN and can ping each other.

2) Start server on Device-A (CP serial console):

```
lp server -p 8000
```

3) From Device-B AP serial console request CP to start keepalive via IPC:

```
ap_cmd lp start_ka <server_ip> 8000
```

This will make the AP request the local CP to start the keepalive toward the server. The default transmit interval is defined by `KEEPALIVE_TX_INTERVAL_SEC` in `cp/keepalive/keepalive_msg.h`.

4) Confirm keepalive activity in Device-B CP logs and Device-A server logs (look for `KEEPALIVE`, `PL` tags).

5) Simulate AP powerdown on Device-B (CP console):

```
lp client pwrdown_host
```

CP should continue sending heartbeats to Device-A; verify Device-A logs.

6) Wake AP:

- From Device-A (Server):

```
lp server -m wakeup_host
```

- Or on Device-B CP:

```
lp client wakeup_host
```

7) Stop keepalive:

```
# From AP (IPC)
ap_cmd lp stop_ka

# Or on CP
lp client stop
```

## 8. Diagnostics & notes

- Check serial logs for `KEEPALIVE`, `PL`, `LP_IPC` messages to trace commands and heartbeats.
- Network issues: ensure correct server IP/port, and that routing/firewall permits the keepalive packets.
- Want sample serial logs or an automation script? Ask and I will add them to the README.

