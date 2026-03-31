/**
 * @file i3c_reg.h
 * @brief I3C register offsets and bit definitions. SoC layer for Driver-HAL-LL model.
 *        I3C controller: word indices per I3C.txt. Beken: word indices per I3C_beken.txt.
 */
#ifndef I3C_REG_H
#define I3C_REG_H

#include <stdint.h>
#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* I3C controller: 4 bytes per word, base = SOC_I3C_IP_REG_BASE */
#define I3C_IP_WORD_SIZE         4u

/* I3C controller register word indices (I3C.txt) */
#define I3C_R_CTRL               0x04u
#define I3C_R_PRESCL_CTRL0       0x05u
#define I3C_R_PRESCL_CTRL1       0x06u
#define I3C_R_MST_IER            0x08u
#define I3C_R_MST_ICR            0x0Bu
#define I3C_R_MST_ISR            0x0Cu
#define I3C_R_MST_STATUS0        0x0Du
#define I3C_R_CMDR               0x0Eu
#define I3C_R_IBIR               0x0Fu
#define I3C_R_SLV_IER            0x10u
#define I3C_R_SLV_ICR            0x13u
#define I3C_R_SLV_ISR            0x14u
#define I3C_R_SLV_STATUS0        0x15u
#define I3C_R_SLV_STATUS1        0x16u
#define I3C_R_SLV_IBI_CTRL       0x17u
#define I3C_R_CMD0_FIFO          0x18u
#define I3C_R_CMD1_FIFO          0x19u
#define I3C_R_TX_FIFO            0x1Au
#define I3C_R_TX_FIFO_STATUS     0x1Bu
#define I3C_R_RX_FIFO_STATUS     0x1Fu
#define I3C_R_RX_FIFO            0x20u
#define I3C_R_IBI_DATA_FIFO      0x21u
#define I3C_R_SLV_DDR_TX_FIFO   0x22u
#define I3C_R_SLV_DDR_RX_FIFO   0x23u
#define I3C_R_CMD_IBI_THR_CTRL   0x24u
#define I3C_R_TX_RX_THR_CTRL     0x25u
#define I3C_R_SLV_DDR_TX_RX_THR  0x26u
#define I3C_R_FLUSH_CTRL         0x27u
#define I3C_R_SLV_CTRL           0x28u
#define I3C_R_DEVS_CTRL          0x2Eu
#define I3C_R_SIR_MAP_BASE       0x60u
#define I3C_R_DEV_TABLE_BASE     0x30u   /* RR0/RR1/RR2 at 0x30+i*4, 0x31+i*4, 0x32+i*4 per dev */

/* Beken strap block: word indices (I3C_beken.txt) */
#define I3C_BEKEN_R_SW_RESET      2u
#define I3C_BEKEN_R_DEVICE_ROLE   4u
#define I3C_BEKEN_R_DCR           5u
#define I3C_BEKEN_R_MRL           6u
#define I3C_BEKEN_R_MWL           7u
#define I3C_BEKEN_R_MXDS          8u
#define I3C_BEKEN_R_PID_MFR       9u
#define I3C_BEKEN_R_PID_INST      10u
#define I3C_BEKEN_R_BUS_AVAIL     11u
#define I3C_BEKEN_R_BUS_IDLE      12u
#define I3C_BEKEN_R_STAT_ADDR     13u
#define I3C_BEKEN_R_FLOW_CTRL     14u
#define I3C_BEKEN_R_RST_TIME      15u
#define I3C_BEKEN_R_XTIME         16u
#define I3C_BEKEN_R_TCAM_XDEL     18u

/* Beken R2 bits */
#define I3C_BEKEN_SW_RESET       (1u << 0)
#define I3C_BEKEN_CLKG_BPS_BEKEN (1u << 1)
#define I3C_BEKEN_CLKG_BPS_CDN   (1u << 2)

/* CTRL bits */
#define I3C_CTRL_DEV_EN          (1u << 31)
#define I3C_CTRL_MODE_MASK       (0x3u << 24)
#define I3C_CTRL_MODE_MASTER     (0x2u << 24)

/* MST_STATUS0 bits */
#define I3C_MST_STATUS0_CMDR_EMP (1u << 0)
#define I3C_MST_STATUS0_RX_EMP   (1u << 2)
#define I3C_MST_STATUS0_IBIR_EMP (1u << 3)
#define I3C_MST_STATUS0_HALTED   (1u << 17)

/* MST_ISR / MST_IER / MST_ICR bits */
#define I3C_MST_ISR_RX_THR       (1u << 7)
#define I3C_MST_ISR_IBIR_THR     (1u << 8)
#define I3C_MST_ISR_IBIR_UNF     (1u << 9)
#define I3C_MST_ISR_IBIR_OVF     (1u << 10)
#define I3C_MST_ISR_IBI_ALL      (I3C_MST_ISR_IBIR_THR | I3C_MST_ISR_IBIR_UNF | I3C_MST_ISR_IBIR_OVF)

/* IBIR field masks */
#define I3C_IBIR_TYPE_MASK       0x3u
#define I3C_IBIR_TYPE_IBI        0x0u
#define I3C_IBIR_XFER_BYTES_SHIFT 2
#define I3C_IBIR_XFER_BYTES_MASK (0x1Fu << I3C_IBIR_XFER_BYTES_SHIFT)
#define I3C_IBIR_SLAVE_ID_SHIFT  8
#define I3C_IBIR_SLAVE_ID_MASK   (0xFu << I3C_IBIR_SLAVE_ID_SHIFT)
#define I3C_IBIR_RESP_BIT        (1u << 12)

/* SLV_IBI_CTRL bits */
#define I3C_SLV_IBI_CTRL_REQ     (1u << 8)
#define I3C_SLV_IBI_CTRL_IBI_SEL (1u << 9)
#define I3C_SLV_IBI_CTRL_IBI_PL_SHIFT 16u
#define I3C_SLV_IBI_CTRL_IBI_ID_MASK  0xFu

#ifdef __cplusplus
}
#endif

#endif /* I3C_REG_H */
