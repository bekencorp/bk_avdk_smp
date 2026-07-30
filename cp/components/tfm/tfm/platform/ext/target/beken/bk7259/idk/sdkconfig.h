/*
 * Automatically generated file. DO NOT EDIT.
 * Beken IoT Development Framework (BEKEN-ARMINO) Configuration Header
 */
#pragma once

#define CONFIG_SOC_BK7259 1
#define CONFIG_SOC_STR "bk7259"
#define CONFIG_SOC_ARCH "cm52"
#define CONFIG_TOOLCHAIN_PATH "/opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin"
#define CONFIG_TOOLCHAIN_PREFIX "arm-none-eabi-"
#define CONFIG_ASSERT_IGNORE 1
#define CONFIG_LWIP 1
#define CONFIG_LWIP_V2_1 1
#define CONFIG_NO_HOSTED 1
#define CONFIG_BLUETOOTH 1
#define CONFIG_ALI_MQTT 1
#define CONFIG_OTA_HTTP 1
#define CONFIG_AT_CMD 1
#define CONFIG_ATE 1
#define CONFIG_ATE_GPIO_ID 0
#define CONFIG_ATE_GPIO_PULL_UP 1
#define CONFIG_BLE 1
#define CONFIG_BLE_5_X 1
#define CONFIG_CLI 1
#define CONFIG_BKREG 1
#define CONFIG_MAX_COMMANDS 255
#define CONFIG_IPERF_TEST 1
#define CONFIG_EFUSE 1
#define CONFIG_ICU 0
#define CONFIG_COMPONENTS_WPA_TWT_TEST 1
#define CONFIG_MCU_PS 1
#define CONFIG_DEEP_PS 1
#define CONFIG_TICK_CALI 1
#define CONFIG_STA_PS 1
#define CONFIG_BK_NETIF 1
#define CONFIG_NON_OS 1
#define CONFIG_APP_MAIN_TASK_PRIO 4
#define CONFIG_APP_MAIN_TASK_STACK_SIZE 4096
#define CONFIG_BASE_MAC_FROM_RF_OTP_FLASH 1
#define CONFIG_RANDOM_MAC_ADDR 1
#define CONFIG_WIFI_ENABLE 1
#define CONFIG_WIFI6_CODE_STACK 1
#define CONFIG_WIFI6 1
#define CONFIG_WIFI4 1
#define CONFIG_MSDU_RESV_HEAD_LENGTH 96
#define CONFIG_MSDU_RESV_TAIL_LENGTH 16
#define CONFIG_TASK_RECONNECT_PRIO 4
#define CONFIG_WIFI6_IP_DEBUG 1
#define CONFIG_CMSIS 1
#define CONFIG_DVP_CAMERA_I2C_ID 0
#define CONFIG_DEMOS_IPERF 1
#define CONFIG_HTTP 1
#define CONFIG_DHCP 1
#define CONFIG_TASK_LWIP_PRIO 4
#define CONFIG_LWIP_MEM_REDUCE 1
#define CONFIG_TEMP_DETECT 1
#define CONFIG_INT_WDT 1
#define CONFIG_INT_WDT_PERIOD_MS 1000
#define CONFIG_TASK_WDT 1
#define CONFIG_TASK_WDT_PERIOD_MS 60000
#define CONFIG_UART1 1
#define CONFIG_UART2 1
#define CONFIG_PRINT_PORT_UART1 1
#define CONFIG_UART_PRINT_PORT 0
#define CONFIG_UART_ATE_PORT 1

/* UART GPIO pin map. The SDK gpio_map.h (pulled in via uart_ll.h) maps
 * UARTx_*_PIN to these CONFIG_* macros, which are normally generated into the
 * project sdkconfig.h from soc/bk7259/Kconfig. The TF-M secure build uses this
 * static idk/sdkconfig.h instead, so mirror the Kconfig default pin numbers. */
#define CONFIG_UART0_TX_PIN  11
#define CONFIG_UART0_RX_PIN  10
#define CONFIG_UART0_CTS_PIN 12
#define CONFIG_UART0_RTS_PIN 13
#define CONFIG_UART1_TX_PIN  0
#define CONFIG_UART1_RX_PIN  1
#define CONFIG_UART2_TX_PIN  41
#define CONFIG_UART2_RX_PIN  40
#define CONFIG_UART3_TX_PIN  0
#define CONFIG_UART3_RX_PIN  1
#define CONFIG_UART4_TX_PIN  41
#define CONFIG_UART4_RX_PIN  40
#define CONFIG_UART5_TX_PIN  65
#define CONFIG_UART5_RX_PIN  64
#define CONFIG_PRINTF_BUF_SIZE 256
#define CONFIG_KFIFO_SIZE 128
#define CONFIG_TRNG_SUPPORT 1
#define CONFIG_SDIO_HOST 1
#define CONFIG_SDIO_HOST_DEFAULT_CLOCK_FREQ 200000
#define CONFIG_MAC_PHY_BYPASS 1
#define CONFIG_SPI_STATIS 1
#define CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY 1
#define CONFIG_QSPI_STATIS 1
#define CONFIG_AON_RTC 1
#define CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT 4
#define CONFIG_GENERAL_DMA 1
#define CONFIG_JPEGENC_HW 1
#define CONFIG_CALENDAR 1
#define CONFIG_MPC 1
#define CONFIG_PRRO 1
#define CONFIG_XTAL_FREQ_26M 1
#define CONFIG_XTAL_FREQ 26000000
#define CONFIG_DCO_FREQ 120000000
#define CONFIG_CPU_FREQ_HZ 120000000
#define CONFIG_SYSTEM_CTRL 1
#define CONFIG_I2C 1
#define CONFIG_I2C_SUPPORT_ID_BITS 1
#define CONFIG_FM_I2C 1
#define CONFIG_PWM 1
#define CONFIG_TIMER 1
#define CONFIG_TIMER_COUNTER 1
#define CONFIG_TIMER_SUPPORT_ID_BITS 0
#define CONFIG_SARADC 1
#define CONFIG_SARADC_NEED_FLUSH 1
#define CONFIG_FLASH 1
#define CONFIG_FLASH_QUAD_ENABLE 1
#define CONFIG_LZMA1900 1

#ifdef DOMAIN_NS
#define CONFIG_SPE 0
#else
#define CONFIG_SPE 1
#endif

#define CONFIG_TFM_TAKE_FOR_BL2  1
#define CONFIG_ENABLE_DEBUG      1
#define CONFIG_FPGA              0
#define CONFIG_ISR_REG_DISABLE   1
#define CONFIG_BK_PRINTF_DISABLE 1
#define CONFIG_STDIO_PRINTF      1
#define CONFIG_STDIO_PRINTF_BUF_SIZE 48
#define CONFIG_TFM_BL2_CRC       1

// #ifndef TFM_ISOLATION_LEVEL
// #define TFM_ISOLATION_LEVEL 3
// #endif
#define CONFIG_TFM_SYS_LL_NSC 0
#define CONFIG_TFM_AON_PMU_LL_NSC 0

#define CONFIG_SYS_CPU0 1
#define CONFIG_CPU_CNT  1

/* CONFIG_TFM_RAM_SIZE (secure-world / SPE RAM carve-out) is intentionally NOT
 * defined here. It is generated into armino_config.h by CMake configure_file()
 * from the external Armino/SDK Kconfig (projects/.../cp/config/bk7259/defconfig
 * CONFIG_CPU0_SPE_RAM_SIZE), so the SPE RAM size is configured in one place. */
#define CONFIG_UART_DEEPSLEEP_CB_SUPPORT 0
#define CONFIG_UART_PM_CB_SUPPORT 0
// #define CONFIG_CUSTOMIZED_PRINTF "log_port.h"

#define CONFIG_VOLT_DETECT  1
#define CONFIG_VBAT_THRESHOLD_LOW 2200
#define CONFIG_LP_VOL 0x6

#define CONFIG_SUPPORT_IO_MATRIX 1
#define CONFIG_IO_MATRIX_VER2_0 1

#define CONFIG_SARADC_V1P2 1
