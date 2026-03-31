// Copyright 2020-2021 Beken
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DMA_V_WORK_MODE_SINGLE       0x0
#define DMA_V_WORK_MODE_REPEAT       0x1

#define DMA_V_PRIO_MODE_ROUND_ROBIN      0x0
#define DMA_V_PRIO_MODE_FIXED_PRIO       0x1

#define DMA_V_REQ_MUX_DTCM             (0x0)   // DTCM/AHB_MEM Req
#define DMA_V_REQ_MUX_UART1            (0x1)   // Uart0 tx Req (UART1对应原本的UART0)
#define DMA_V_REQ_MUX_UART1_RX         (0x2)   // Uart0 rx Req
#define DMA_V_REQ_MUX_UART2            (0x3)   // Uart1 tx Req
#define DMA_V_REQ_MUX_UART2_RX         (0x4)   // Uart1 rx Req
#define DMA_V_REQ_MUX_UART3            (0x5)   // Uart2 tx Req
#define DMA_V_REQ_MUX_UART3_RX         (0x6)   // Uart2 rx Req
#define DMA_V_REQ_MUX_UART4            (0x7)   // Uart3 tx Req
#define DMA_V_REQ_MUX_UART4_RX         (0x8)   // Uart3 rx Req
#define DMA_V_REQ_MUX_GSPI0            (0x9)   // SPI0 tx Req
#define DMA_V_REQ_MUX_GSPI0_RX         (0xA)   // SPI0 rx Req
#define DMA_V_REQ_MUX_GSPI1            (0xB)   // SPI1 tx Req
#define DMA_V_REQ_MUX_GSPI1_RX         (0xC)   // SPI1 rx Req
#define DMA_V_REQ_MUX_GSPI2            (0xD)   // SPI2 tx Req
#define DMA_V_REQ_MUX_GSPI2_RX         (0xE)   // SPI2 rx Req
#define DMA_V_REQ_MUX_AUD_MIC0         (0xF)   // aud mic0 Req
#define DMA_V_REQ_MUX_AUD_SPK0         (0x10)  // aud spk0 Req
#define DMA_V_REQ_MUX_AUD_SPK0_HINT    (0x11)  // aud spk0 hint Req
#define DMA_V_REQ_MUX_I2S0             (0x12)  // i2s0 tx Req
#define DMA_V_REQ_MUX_I2S0_RX          (0x13)  // i2s0 rx Req
#define DMA_V_REQ_MUX_I2S1             (0x14)  // i2s1 tx Req
#define DMA_V_REQ_MUX_I2S1_RX          (0x15)  // i2s1 rx Req
#define DMA_V_REQ_MUX_I2S2             (0x16)  // i2s2 tx Req
#define DMA_V_REQ_MUX_I2S2_RX          (0x17)  // i2s2 rx Req
#define DMA_V_REQ_MUX_I2S3             (0x18)  // i2s3 tx Req
#define DMA_V_REQ_MUX_I2S3_RX          (0x19)  // i2s3 rx Req
#define DMA_V_REQ_MUX_SPDIF0           (0x1A)  // spdif0 Req
#define DMA_V_REQ_MUX_SPDIF1           (0x1B)  // spdif1 Req
#define DMA_V_REQ_MUX_SADC_RX          (0x1C)  // sadc rx Req
#define DMA_V_REQ_MUX_AUD_SPK0_CALL    (0x1D)  // aud spk0 call Req
#define DMA_V_REQ_MUX_AUD_MIC1         (0x1E)  // aud mic1 Req
#define DMA_V_REQ_MUX_AUD_SPK1_A2DP    (0x1F)  // aud spk1 a2dp Req
#define DMA_V_REQ_MUX_AUD_SPK1_HINT    (0x20)  // aud spk1 hint dma Req
#define DMA_V_REQ_MUX_AUD_SPK1_CALL    (0x21)  // aud spk1 call Req
#define DMA_V_REQ_MUX_GSPI3            (0x22)  // SPI3 tx Req
#define DMA_V_REQ_MUX_GSPI3_RX         (0x23)  // SPI3 rx Req
#define DMA_V_REQ_MUX_UART5            (0x24)  // Uart4 tx Req
#define DMA_V_REQ_MUX_UART5_RX         (0x25)  // Uart4 rx Req
#define DMA_V_REQ_MUX_I2S4             (0x26)  // i2s4 tx Req
#define DMA_V_REQ_MUX_I2S4_RX          (0x27)  // i2s4 rx Req
#define DMA_V_REQ_MUX_XDAC0            (0x28)  // xdac0 tx Req
#define DMA_V_REQ_MUX_XDAC1            (0x29)  // xdac1 tx Req
// 0x2A and above: Reserved

#define DMA_FLUSH_SRC_BUF_POS          (17)
#define DMA_FINISH_INT_POS             (18)
#define DMA_HALF_FINISH_INT_POS        (19)
#define DMA_BUS_ERR_INT_POS            (20)

#ifdef __cplusplus
}
#endif

