/*
 * Copyright 2020-2025 Beken
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 自定义 LWIP 配置文件示例
 * 
 * 使用方法：
 * 1. 在 Kconfig 中启用 CONFIG_ENABLE_LWIP_CUSTOM_CONFIG
 * 2. 将此文件重命名为 lwipopts_custom.h 并放置在项目配置目录中
 * 3. 根据需要修改以下配置项
 * 
 * 注意：此文件会完全替代默认的 lwipopts.h 配置
 * 请确保包含所有必要的配置项
 */

#pragma once

/* 基础包含文件 */
#include <common/sys_config.h>
#include <components/log.h>

/* ========== 基础配置 ========== */
#define LWIP_SOCKET_OFFSET             1

/* 回环接口配置 */
#define LWIP_NETIF_LOOPBACK             1
#define LWIP_HAVE_LOOPIF                1
#define LWIP_NETIF_LOOPBACK_MULTITHREADING       1
#define LWIP_LOOPBACK_MAX_PBUFS         8

/* IP 转发配置 */
#ifdef CONFIG_IP_FORWARD
#define IP_FORWARD                      1
#else
#define IP_FORWARD                      0
#endif

#ifdef CONFIG_IP_FORWARD_ALLOW_TX_ON_RX_NETIF
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF    1
#else
#define IP_FORWARD_ALLOW_TX_ON_RX_NETIF    0
#endif

/* DHCP 释放时间配置 */
#if CONFIG_BK_DHCP_RELEASE_TIME
#define BK_DHCP_RELEASE_TIME CONFIG_BK_DHCP_RELEASE_TIME
#endif

/* NAPT 配置 */
#ifdef CONFIG_IP_NAPT
#define IP_NAPT                         1
#else
#define IP_NAPT                         0
#endif

/* 网络缓冲区接收信息配置 */
#ifdef CONFIG_LWIP_NETBUF_RECVINFO
#define LWIP_NETBUF_RECVINFO		1
#else
#define LWIP_NETBUF_RECVINFO		0
#endif

/* ========== 线程配置 ========== */
#define TCPIP_THREAD_NAME               "tcp/ip"

#if defined(CONFIG_KEYVALUE) || defined(CONFIG_FTP_SERVER)
#define TCPIP_THREAD_STACKSIZE          1024
#elif defined CONFIG_LWIP_TCPIP_THREAD_STACKSIZE
#define TCPIP_THREAD_STACKSIZE          CONFIG_LWIP_TCPIP_THREAD_STACKSIZE
#endif

#if CONFIG_LITEOS_M
#define TCPIP_THREAD_PRIO               7
#else
#define TCPIP_THREAD_PRIO               2
#endif

#define DEFAULT_THREAD_STACKSIZE        200
#if CONFIG_LITEOS_M
#define DEFAULT_THREAD_PRIO             1
#else
#define DEFAULT_THREAD_PRIO             8
#endif

/* ========== 调试配置 ========== */
/* 禁用 lwIP 断言 */
#define LWIP_NOASSERT			        1

#if CONFIG_AGORA_IOT_SDK
#define LWIP_DEBUG                      1
#else
#define LWIP_DEBUG                      0
#endif

/* 关闭所有调试输出 */
#define LWIP_DEBUG_TRACE                0
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define INET_DEBUG                      LWIP_DBG_OFF
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#define RAW_DEBUG                       LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define SYS_DEBUG                       LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define PPP_DEBUG                       LWIP_DBG_OFF
#define SLIP_DEBUG                      LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#define SNMP_MSG_DEBUG                  LWIP_DBG_OFF
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF
#define DNS_DEBUG                       LWIP_DBG_OFF
#define IP6_DEBUG                       LWIP_DBG_OFF
#define MDNS_DEBUG                      LWIP_DBG_OFF

/* ========== 系统保护配置 ========== */
#define SYS_LIGHTWEIGHT_PROT            1
#define BK_LWIP                         1

/* ========== 内存配置 ========== */
#define MEM_ALIGNMENT                   4
#define MEM_LIBC_MALLOC                  CONFIG_LWIP_MEM_LIBC_MALLOC
#define MEMP_MEM_MALLOC                  CONFIG_LWIP_MEMP_MEM_MALLOC
#define MEM_TRX_DYNAMIC_EN               CONFIG_LWIP_MEM_TRX_DYNAMIC_EN
#define MEMP_STATS                       CONFIG_LWIP_MEMP_STATS
#define MEM_STATS                        CONFIG_LWIP_MEM_STATS

/* 套接字数量配置 */
#define MAX_SOCKETS_TCP 8
#define MAX_LISTENING_SOCKETS_TCP 4
#define MAX_SOCKETS_UDP 8
#define TCP_SND_BUF_COUNT 5

/* 内存大小配置 */
#define MEM_SIZE                        CONFIG_LWIP_MEM_SIZE
#define MEM_MAX_TX_SIZE                 CONFIG_LWIP_MEM_MAX_TX_SIZE
#define MEM_MAX_RX_SIZE                 CONFIG_LWIP_MEM_MAX_RX_SIZE

/* ========== 内存池配置 ========== */
#define MEMP_NUM_PBUF                   10
#define MEMP_NUM_TCP_PCB                MAX_SOCKETS_TCP
#define MEMP_NUM_TCP_PCB_LISTEN         MAX_LISTENING_SOCKETS_TCP
#define MEMP_NUM_TCPIP_MSG_INPKT        32
#define MEMP_NUM_SYS_TIMEOUT            12
#define MEMP_NUM_NETBUF                 CONFIG_LWIP_MEMP_NUM_NETBUF
#define MEMP_NUM_NETCONN	(MAX_SOCKETS_TCP + \
	MAX_LISTENING_SOCKETS_TCP + MAX_SOCKETS_UDP)
#define PBUF_POOL_SIZE                  CONFIG_LWIP_PBUF_POOL_SIZE

/* PBUF 配置 */
#if 1
#define PBUF_LINK_ENCAPSULATION_HLEN    CONFIG_MSDU_RESV_HEAD_LENGTH + CONFIG_MSDU_RESV_DESC_LENGTH
#if CONFIG_LWIP_RESV_TLEN_ENABLE
#define PBUF_LINK_ENCAPSULATION_TLEN    CONFIG_MSDU_RESV_TAIL_LENGTH
#endif
#define PBUF_POOL_BUFSIZE               (1580 + PBUF_LINK_ENCAPSULATION_HLEN)
#else
#define PBUF_POOL_BUFSIZE               1580
#endif

/* ========== 协议配置 ========== */
#define LWIP_RAW                        1
#ifdef CONFIG_IPV6
#define LWIP_IPV6                        1
#endif

/* Auto IP 配置 */
#ifdef CONFIG_AUTOIP
#define LWIP_AUTOIP                     1
#define LWIP_DHCP_AUTOIP_COOP           1
#define LWIP_DHCP_AUTOIP_COOP_TRIES		5
#endif

/* MDNS 配置 */
#ifdef CONFIG_MDNS
#define LWIP_MDNS_RESPONDER             1
#define LWIP_NUM_NETIF_CLIENT_DATA      (LWIP_MDNS_RESPONDER)
#endif

#if CONFIG_BRIDGE
#if !defined LWIP_NUM_NETIF_CLIENT_DATA
#define LWIP_NUM_NETIF_CLIENT_DATA      1
#endif
#endif

/* ========== Socket 配置 ========== */
#define LWIP_SOCKET                     1
#define LWIP_NETIF_API			1
#define LWIP_RECV_CB                1
#define SO_REUSE                        1
#define SO_REUSE_RXTOALL 				1
#define LWIP_TCP_KEEPALIVE              1

/* ========== 统计配置 ========== */
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1

/* ========== DHCP 配置 ========== */
#define LWIP_DHCP                       1
#define LWIP_NETIF_STATUS_CALLBACK      1

/* ========== DNS 配置 ========== */
#define LWIP_DNS                        1
#define DNS_TABLE_SIZE                  CONFIG_DNS_TABLE_SIZE
#define DNS_MAX_SERVERS                 2
#define DNS_DOES_NAME_CHECK             1
#define DNS_MSG_SIZE                    512
#define MDNS_MSG_SIZE                   512
#define MDNS_TABLE_SIZE                 1
#define MDNS_MAX_SERVERS                1
#define MEMP_NUM_UDP_PCB		(MAX_SOCKETS_UDP + 2)

/* ========== IGMP 配置 ========== */
#define LWIP_IGMP                       1

/* ========== Socket 超时配置 ========== */
#define LWIP_SO_CONTIMEO                1
#define LWIP_SO_SNDTIMEO                1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_LINGER				1

/* ========== TCP 配置 ========== */
#if CONFIG_AGORA_IOT_SDK
//#define TCP_LISTEN_BACKLOG		        1
#else
#define TCP_LISTEN_BACKLOG		        1
#endif
#define LWIP_PROVIDE_ERRNO		        1

#if CONFIG_AGORA_IOT_SDK
#include "sys/errno.h"
#else
#include <errno.h>
#define ERRNO				            1
#endif

/* ========== 网络接口配置 ========== */
#define LWIP_NETIF_HOSTNAME             1

/* ========== 校验和配置 ========== */
#ifdef CHECKSUM_BY_HARDWARE
  #define CHECKSUM_GEN_IP                 0
  #define CHECKSUM_GEN_UDP                0
  #define CHECKSUM_GEN_TCP                0
  #define CHECKSUM_CHECK_IP               0
  #define CHECKSUM_CHECK_UDP              0
  #define CHECKSUM_CHECK_TCP              0
#else
  #define CHECKSUM_GEN_IP                 1
  #define CHECKSUM_GEN_UDP                1
  #define CHECKSUM_GEN_TCP                1
  #define CHECKSUM_CHECK_IP               1
  #define CHECKSUM_CHECK_UDP              1
  #define CHECKSUM_CHECK_TCP              1
#endif

#if CONFIG_ETH
#define LWIP_CHECKSUM_CTRL_PER_NETIF      1
#endif

/* ========== TCP 高级配置 ========== */
#define TCP_RESOURCE_FAIL_RETRY_LIMIT     50
#define LWIP_TCP_SACK_OUT               CONFIG_LWIP_TCP_SACK_OUT

/* TCP 参数配置 */
#define TCP_MSS                 CONFIG_LWIP_TCP_MSS
#define TCP_WND                 CONFIG_LWIP_TCP_WND
#define TCP_SND_BUF             CONFIG_LWIP_TCP_SND_BUF
#define TCP_SND_QUEUELEN        CONFIG_LWIP_TCP_SND_QUEUELEN

/* DHCP ARP 检查 */
#define DHCP_DOES_ARP_CHECK            (0)

#define TCP_MAX_ACCEPT_CONN 5
#define MEMP_NUM_TCP_SEG               CONFIG_LWIP_MEMP_NUM_TCP_SEG
#define DEFAULT_UDP_RECVMBOX_SIZE       CONFIG_LWIP_UDP_RECVMBOX_SIZE

/* TCP 重传配置 */
#define TCP_SYNMAXRTX                   CONFIG_LWIP_TCP_SYNMAXRTX
#define LWIP_TCP_RTO_TIMEOUT            CONFIG_LWIP_TCP_RTO_TIMEOUT
#define TCP_QUEUE_OOSEQ                 CONFIG_LWIP_TCP_QUEUE_OOSEQ
#define TCP_MSL (TCP_TMR_INTERVAL)

/* ICMP 错误限制 */
#define ICMP_ERR_RATELIMIT                 CONFIG_ICMP_ERR_RATELIMIT
#define ICMP_ERR_RATE_THRESHOLD            CONFIG_ICMP_ERR_RATE_THRESHOLD

/* ========== 兼容性配置 ========== */
#define LWIP_COMPAT_MUTEX_ALLOWED       (1)
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS
#define ETHARP_SUPPORT_STATIC_ENTRIES   1
#define LWIP_RIPPLE20                   1

/* ========== Beken 特定配置 ========== */
#define BK_DHCP                         1
#define BK_DNS			1
#define LWIP_SIOCOUTQ                  1

/* 日志配置 */
#define LWIP_TAG "lwip"
#define LWIP_LOGI(...) BK_LOGI(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGW(...) BK_LOGW(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGE(...) BK_LOGE(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGD(...) BK_LOGD(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGV(...) BK_LOGV(LWIP_TAG, ##__VA_ARGS__)

#ifdef CONFIG_RIO
#define LWIP_HOOK_FILENAME              "lwip_hooks.h"
#endif

#define BK_IP4_ROUTE                    1
#define BK_DHCPS_DNS                    1

#if CONFIG_FREERTOS
#define LWIP_NETCONN_SEM_PER_THREAD 1
#define LWIP_NETCONN_FULLDUPLEX 1
#define BK_LWIP_DEBUG 1
#endif

/* HTTP 配置 */
#define HTTP_IS_DATA_VOLATILE(hs) \
({ \
  extern uint32_t _stext, _etext; \
  u8_t __api_flags = (hs->file >= (const char *)(&_stext) && \
    hs->file <= (const char *)(&_etext)) ? TCP_WRITE_FLAG_COPY : 0; \
  __api_flags; \
})

#ifdef CONFIG_LWIP_HW_IP_CHECKSUM
uint16_t hw_ipcksum_standard_chksum(const void *dataptr, int len);
#define LWIP_CHKSUM hw_ipcksum_standard_chksum
#define LWIP_CHKSUM_ALGORITHM 2
#endif

/* ========== PPP 配置 ========== */
#ifdef CONFIG_LWIP_PPP_SUPPORT
#define PPP_SUPPORT                     1
#define PPP_IPV6_SUPPORT                               0
#define PPP_NOTIFY_PHASE                1
#define PAP_SUPPORT                     1
#define PPP_MAXIDLEFLAG                 0
#endif  /* CONFIG_LWIP_PPP_SUPPORT */

/* ========== 自定义 PBUF 支持 ========== */
#define LWIP_SUPPORT_CUSTOM_PBUF        1

/* ========== 自定义配置示例 ========== */
/*
 * 以下是一些自定义配置的示例，可以根据项目需求进行调整：
 */

/* 示例1：增加 TCP 连接数 */
// #define MAX_SOCKETS_TCP 16
// #define MEMP_NUM_TCP_PCB 16

/* 示例2：调整内存大小 */
// #define MEM_SIZE (64*1024)
// #define MEM_MAX_TX_SIZE ((MEM_SIZE*5)/6)
// #define MEM_MAX_RX_SIZE ((MEM_SIZE*3)/4)

/* 示例3：启用调试 */
// #define LWIP_DEBUG 1
// #define TCP_DEBUG LWIP_DBG_ON

/* 示例4：调整 TCP 参数 */
// #define TCP_MSS 1460
// #define TCP_WND (32*TCP_MSS)
// #define TCP_SND_BUF (32*TCP_MSS)

/* 示例5：启用 HTTP 服务器 */
// #define LWIP_HTTPD 1
// #define LWIP_HTTPD_CUSTOM_FILES 1
// #define LWIP_HTTPD_DYNAMIC_HEADERS 1
