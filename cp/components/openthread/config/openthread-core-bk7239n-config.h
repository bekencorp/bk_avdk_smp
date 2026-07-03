// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 *   This file includes bk7239n compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_BK7239N_CONFIG_H_
#define OPENTHREAD_CORE_BK7239N_CONFIG_H_

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * The bk7236n platform provides an otPlatLog() function.
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "BK7239N"
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VENDOR_OUI
 *
 * The Organizationally Unique Identifier for the vendor.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VENDOR_OUI
#define OPENTHREAD_CONFIG_STACK_VENDOR_OUI 0xfffff1   //fix me
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 32
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 160
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD 6
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 3
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 32
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPREND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Define to 1 if you want to enable software transmission security logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
 *
 * Define to 1 to enable software transmission target time logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
 *
 * Define to 1 to enable software reception target time logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
 *
 * Define to 1 to enable otPlatFlash* APIs to support non-volatile storage.
 *
 * When defined to 1, the platform MUST implement the otPlatFlash* APIs instead of the otPlatSettings* APIs.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (4096 * sizeof(void *))
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS
 *
 * The size of heap buffer when DTLS is disabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS 4000
#endif

/**
 * @def OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
 *
 * Define as 1 to enable the time synchronization service feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#define OPENTHREAD_CONFIG_TIME_SYNC_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *  min:640
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 648
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 648
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing, it must be set
 * to support following features:
 *    1. Time synchronization service feature (i.e., OPENTHREAD_CONFIG_TIME_SYNC_ENABLE is set).
 *
 * @note If it's enabled, plaforms must support interrupt context and concurrent access AES.
 *
 */
//#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#ifndef OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 1
#endif
//#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
 *
 * Reception scheduling and ramp up time needed for the CSL receiver to be ready, in units of microseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD 320
#endif

//#define OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US  6000

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
 *
 * The minimum time (in microseconds) before the MHR start that the radio should be in receive state and ready to
 * properly receive in order to properly receive any IEEE 802.15.4 frame.
 *
 * Preamble (last 40 us) + SFD (32 us) + PHR (32 us).
 *
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD 104
#endif

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
 *
 * The minimum time (in microseconds) after the MHR start that the radio should be in receive state in order
 * to properly receive any IEEE 802.15.4 frame.
 *
 * Set to zero since nRF radio will automatically extend the duration of the receive window once the SHR has
 * been detected.
 *
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER 0
#endif

/*
 * Suppress the ARMCC warning on unreachable statement,
 * e.g. break after assert(false) or ExitNow() macro.
 */
#if defined(__CC_ARM)
_Pragma("diag_suppress=111") _Pragma("diag_suppress=128")
#endif

//The following configurations were added as needed during the porting process by BEKEN.

/**
 * @def OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
 *
 * Define as 1 to enable bultin-mbedtls.
 *
 * Note that the OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS determines whether to use bultin-mbedtls as well as
 * whether to manage mbedTLS internally, such as memory allocation and debug.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
#define OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TCP_ENABLE
 *
 * Define to 0 to disable TCP
 *
 */
#ifndef OPENTHREAD_CONFIG_TCP_ENABLE
#define OPENTHREAD_CONFIG_TCP_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_DEBG
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_INIT
#define OPENTHREAD_CONFIG_LOG_LEVEL_INIT OT_LOG_LEVEL_CRIT
#endif

#ifndef OPENTHREAD_CONFIG_LOG_PKT_DUMP
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP  1
#endif

#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM  1
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE  1
#endif

#define OPENTHREAD_CONFIG_TP_LP_ENABLE 1

#define OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE 1

#define OPENTHREAD_CONFIG_THREAD_VERSION OT_THREAD_VERSION_1_4

#define OPENTHREAD_CONFIG_DIAG_ENABLE 1
// #define OPENTHREAD_CONFIG_MESH_DIAG_ENABLE 1

#define OPENTHREAD_CONFIG_ECDSA_ENABLE 1
#define OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE 1
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#define OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE 0
#define OPENTHREAD_CONFIG_MLR_ENABLE 1
#define OPENTHREAD_CONFIG_DUA_ENABLE 1
#define OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE 1
#define OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES 5

#define OPENTHREAD_CONFIG_SRP_SERVER_ENABLE 0
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 0
#define OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE 0

#define OPENTHREAD_CONFIG_PING_SENDER_ENABLE 1

#if OPENTHREAD_FTD
#define OPENTHREAD_CONFIG_COMMISSIONER_ENABLE 1
#define OPENTHREAD_CONFIG_JOINER_ENABLE 1
#endif

#if OPENTHREAD_MTD
#define OPENTHREAD_CONFIG_JOINER_ENABLE 1
#endif

#define OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE 1
#define OPENTHREAD_CONFIG_COAP_API_ENABLE 1

#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 1
#endif

#define OPENTHREAD_CONFIG_MAC_FILTER_ENABLE 1
#define OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE 1

#define OPENTHREAD_CONFIG_NET_DIAG_VENDOR_INFO_SET_API_ENABLE 1
#define OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE 1
#define OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE 1
#define OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE 1

//for homekit
#if CONFIG_HOMEKIT
#ifndef OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
#define OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE 1
#endif
#ifndef OPENTHREAD_CONFIG_IP6_MAX_ASSEMBLED_DATAGRAM
#define OPENTHREAD_CONFIG_IP6_MAX_ASSEMBLED_DATAGRAM 4096
#endif
#endif

#endif // OPENTHREAD_CORE_BK7239N_CONFIG_H_
