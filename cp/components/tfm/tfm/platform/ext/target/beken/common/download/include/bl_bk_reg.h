
#ifndef _bk3000_CHIP_H_
#define _bk3000_CHIP_H_
#include "soc/bk7259/reg_base.h"

/* Bootloader build configuration (formerly bl_config.h, merged here).
 * These macros are kept for reference/compatibility; the bk7259 download
 * path does not currently branch on any of them. */
#define BOOTLOADER_VERSION_STR            "2021-08"
#define CHIP_NAME_STR                     "BK7236"
#define DELAY_1MS_CNT                     320  //riscv 26Mhz 160  //ba22 26Mhz 900  //arm 16MHz 130

#define CPU_BA22    0
#define CPU_ARM     0
#define CPU_RISCV   0
#define CPU_CM33    1

#define CHIP_BK3281 0
#define CHIP_BK3266 0
#define CHIP_BK3288 0
#define CHIP_BK3633 0
#define CHIP_BK7256 0
#define CHIP_BK7236 1

#define CONFIG_WDT  1

#define ENABLE   1
#define DISABLE  0

/* bk7259 flash XIP/execution base (see bk7259/partition/flash_layout.h
 * FLASH_BASE_ADDRESS). boot_update.c subtracts this from linked addresses to
 * derive the physical flash offset. reg_base.h does not define it for bk7259. */
#ifndef SOC_FLASH_MEM_BASE
#define SOC_FLASH_MEM_BASE               (0x04000000)
#endif

// AMBA Address Mapping
// AHB
#define BK3000_SYS_BASE_ADDR             (SOC_SYSTEM_REG_BASE)
#define BK3000_UART_BASE_ADDR            (SOC_UART0_REG_BASE)
#define BK3000_SPI_BASE_ADDR             (SOC_SPI_REG_BASE)
/* bk7259 has no legacy system WDT; download watchdog uses the AON WDT. */
#define BK3000_WDT_BASE_ADDR             (SOC_AON_WDT_REG_BASE)
#define REG_XVR_BASE_ADDR                (SOC_XVR_REG_BASE)
#define BK3000_PMU_BASE_ADDR             (SOC_AON_PMU_REG_BASE)
#define BK3000_GPIO_BASE_ADDR            (0x44000400)
#define BK3000_AWDT_BASE_ADDR            (0x44000600)
#define ANA_XVR_NUM                      (16)

//watchdog
#define BK3000_WDT_CONTROL               (*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+2*4)))
#define BK3000_WDT_CONFIG                (*((volatile unsigned long *)(BK3000_WDT_BASE_ADDR+4*4)))

// PMU define
#define BK3000_PMU_RESET_REASON_GET      (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x0*4)))

#define AON_PMU_REG01                    (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x1*4)))
#define AON_PMU_REG01_BIT_XTAL_MODE      (0x1 << 21)
#define AON_PMU_IS_XTAL_40M              ((AON_PMU_REG01 & AON_PMU_REG01_BIT_XTAL_MODE) == 1)
#define AON_PMU_REG41                    (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x41*4)))
#define AON_PMU_REG41_BIT_XTAL_SEL       (0x1 << 14)
#define AON_PMU_SET_XTAL_26M             AON_PMU_REG41 &= ~AON_PMU_REG41_BIT_XTAL_SEL
#define AON_PMU_SET_XTAL_40M             AON_PMU_REG41 |= AON_PMU_REG41_BIT_XTAL_SEL
#define AON_PMU_IS_XTAL_SEL_40M          ((AON_PMU_REG41 & AON_PMU_REG41_BIT_XTAL_SEL) == 1)

#define AON_PMU_REG03                    (*((volatile unsigned int *)(BK3000_PMU_BASE_ADDR + 0x3*4)))

#define AON_PMU_REG03_BIT_BOOT_FLAG      (0x1 << 0)
#define AON_PMU_REG03_SET_BF_PRIMARY     AON_PMU_REG03 |= AON_PMU_REG03_BIT_BOOT_FLAG
#define AON_PMU_REG03_CLR_BF_PRIMARY     AON_PMU_REG03 &= ~AON_PMU_REG03_BIT_BOOT_FLAG

#define DEEP_SLEEP_RESTART_BIT		     (0x1 << 1)
// 1: Boot from deep_sleep  0: Boot from POR
#define DEEP_SLEEP_RESTART               ((BK3000_PMU_RESET_REASON_GET & DEEP_SLEEP_RESTART_BIT ) != 0)

#define addSYSTEM_Reg0xa                 *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0xa*4))
//uart enable - bk7259: UART0 clock at bit[4] of reg 0xC
#define addSYSTEM_Reg0xc                 *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0xc*4))
#define UART0_ENABLE                     addSYSTEM_Reg0xc |= (1 << 4)
#define UART0_DISABLE                    addSYSTEM_Reg0xc &= ~(1 << 4)
#define UART0_RESET                      REG_APB3_UART_RST = 1

//spi enable - bk7259: SPI0 clock at bit[9] of reg 0xC
#define SPI0_ENABLE                      addSYSTEM_Reg0xc |= (1 << 9)
#define SPI0_DISABLE                     addSYSTEM_Reg0xc &= ~(1 << 9)

#define PER_INT_ENABLE                   *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x14*4))
#define UART0_INT_INDEX_NUM              (4)
#define UART0_INT_INDEX                  (0x1U << 4 )
#define UART0_INT_ENABLE                 PER_INT_ENABLE |=  UART0_INT_INDEX
#define UART0_INT_DISABLE                PER_INT_ENABLE &=  ~UART0_INT_INDEX

#define addSYSTEM_Reg0x8                *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x8*4))

#define addSYSTEM_Reg0xd                *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0xd*4))
// bk7259: Timer0 clock select in reg 0x9 bit[28] (1=XTAL)
#define addSYSTEM_Reg0x9                *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x9*4))
#define CKSEL_TIM0                      addSYSTEM_Reg0x9 |= (1 << 28)
// bk7259: Timer0 clock at bit[0] of reg 0xC
#define TIM0_ENABLE                     addSYSTEM_Reg0xc |= (1 << 0)

// flash_rw_sel & rebooot   ok
#define addSYSTEM_Reg0x2                 *((volatile unsigned long *) (BK3000_SYS_BASE_ADDR+0x2*4))
#define REBOOT                           addSYSTEM_Reg0x2 &= ~0x1
#define  SPI_FLASH_SEL                   (0x1U << 9 )
#define  SET_SPI_RW_FLASH                addSYSTEM_Reg0x2 |= SPI_FLASH_SEL
#define  SET_FLASHCTRL_RW_FLASH          addSYSTEM_Reg0x2 &= ~SPI_FLASH_SEL

// Pherial Register's Define  

//GPIO
#define BK3000_GPIO_0_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+0*4)))
#define BK3000_GPIO_1_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+1*4)))

#define BK3000_GPIO_6_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+6*4)))
#define BK3000_GPIO_7_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+7*4)))
#define BK3000_GPIO_8_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+8*4)))
#define BK3000_GPIO_9_CONFIG             (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+9*4)))

#define BK3000_GPIO_10_CONFIG            (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+10*4)))
#define BK3000_GPIO_11_CONFIG            (*((volatile unsigned long *)(BK3000_GPIO_BASE_ADDR+11*4)))

#define sft_GPIO_INPUT                   (0x0)
#define sft_GPIO_OUTPUT                  (0x1)
#define sft_GPIO_INPUT_ENABLE            (0x2)
#define sft_GPIO_OUTPUT_ENABLE           (0x3)
#define sft_GPIO_PULL_MODE               (0x4)
#define sft_GPIO_PULL_ENABLE             (0x5)
#define sft_GPIO_FUNCTION_ENABLE         (0x6)
#define sft_GPIO_INPUT_MONITOR           (0x7)

 /* defination of registers
*/
#define REG_APB3_UART_RST                (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x02*4)))
#define REG_APB3_UART_CFG                (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x04*4)))
#define REG_APB3_UART_FIFO_THRESHOLD     (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x05*4)))
#define REG_APB3_UART_FIFO_STATUS        (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x06*4)))
#define REG_APB3_UART_DATA               (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x07*4)))
#define REG_APB3_UART_INT_ENABLE         (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x08*4)))
#define REG_APB3_UART_INT_FLAG           (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x09*4)))
#define REG_APB3_UART_FC_CFG             (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x0A*4)))
#define REG_APB3_UART_WAKEUP_ENABLE      (*((volatile unsigned long *)(BK3000_UART_BASE_ADDR+0x0B*4)))

#define UART_WRITE_BYTE(v)               (REG_APB3_UART_DATA=v)
#define UART_READ_BYTE()                 ((REG_APB3_UART_DATA>>8)&0xff)

#define bit_UART_TX_FIFO_FULL            (1<<16)

#define bit_UART_TX_FIFO_EMPTY           (1<<17)
#define bit_UART_RX_FIFO_FULL            (1<<18)
#define bit_UART_RX_FIFO_EMPTY           (1<<19)
#define bit_UART_TX_WRITE_READY          (1<<20)
#define bit_UART_RX_READ_READY           (1<<21)

#define sft_UART_CONF_TX_ENABLE          (0)
#define sft_UART_CONF_RX_ENABLE          (1)
#define sft_UART_CONF_IRDA               (2)
#define sft_UART_CONF_UART_LEN           (3)
#define sft_UART_CONF_PAR_EN             (5)
#define sft_UART_CONF_PAR_MODE           (6)
#define sft_UART_CONF_STOP_LEN           (7)
#define sft_UART_CONF_CLK_DIVID          (8)

#define sft_UART_FIFO_CONF_TX_FIFO       (0)
#define sft_UART_FIFO_CONF_RX_FIFO       (8)
#define sft_UART_FIFO_CONF_DETECT_TIME   (16)


#define bit_UART_INT_TX_NEED_WRITE       ( 1 << 0 )
#define bit_UART_INT_RX_NEED_READ        ( 1 << 1 )
#define bit_UART_INT_RX_OVER_FLOW        ( 1 << 2 )
#define bit_UART_INT_RX_PAR_ERR          ( 1 << 3 )
#define bit_UART_INT_RX_STOP_ERR         ( 1 << 4 )
#define bit_UART_INT_TX_STOP_END         ( 1 << 5 )
#define bit_UART_INT_RX_STOP_END         ( 1 << 6 )
#define bit_UART_INT_RXD_WAKEUP          ( 1 << 7 )

#define UART_TX_FIFO_SIZE                (128)
#define UART_RX_FIFO_SIZE                (128)
#define UART_TX_FIFO_COUNT               (REG_APB3_UART_FIFO_STATUS&0xff)
#define UART_RX_FIFO_COUNT               ((REG_APB3_UART_FIFO_STATUS>>8)&0xff)
#define UART_TX_FIFO_FULL                (REG_APB3_UART_FIFO_STATUS&0x00010000)
#define UART_TX_FIFO_EMPTY               (REG_APB3_UART_FIFO_STATUS&0x00020000)
#define UART_RX_FIFO_FULL                (REG_APB3_UART_FIFO_STATUS&0x00040000)
#define UART_RX_FIFO_EMPTY               (REG_APB3_UART_FIFO_STATUS&0x00080000)
#define UART_TX_WRITE_READY              (REG_APB3_UART_FIFO_STATUS&0x00100000)
#define UART_RX_READ_READY               (REG_APB3_UART_FIFO_STATUS&0x00200000)

#define UART_BAUDRATE_3250000            (3250000)
#define UART_BAUDRATE_3000000            (3000000)
#define UART_BAUDRATE_2000000            (2000000)
#define UART_BAUDRATE_921600             (921600)  //
#define UART_BAUDRATE_460800             (460800)
#define UART_BAUDRATE_256000             (256000)
#define UART_BAUDRATE_230400             (230400)  //
#define UART_BAUDRATE_115200             (115200)  //default
#define UART_BAUDRATE_9600               (9600)     //
#define UART_BAUDRATE_3000               (3000)     //
#define UART_BAUDRATE_3250               (3250)     //

#define UART_BAUDRATE_DEFAULE            (UART_BAUDRATE_115200) //UART_BAUDRATE_3000000
#define UART_CLOCK_FREQ_24M              (24000000)
#define UART_CLOCK_FREQ_26M              (26000000)
#define UART_CLOCK_FREQ_40M              (40000000)
#define UART_CLOCK_FREQ_48M              (48000000)
#define UART_CLOCK_FREQ_52M              (52000000)
#define UART_CLOCK_FREQ_80M              (80000000)
#define UART_CLOCK_FREQ                  (UART_CLOCK_FREQ_26M)
#define Beken_Write_Register(addr, data) (*((volatile u_int32 *)(addr))) = (u_int32)(data);
#define Beken_Read_Register(addr)        (*(volatile u_int32 *)(addr));

#endif
// end of file


