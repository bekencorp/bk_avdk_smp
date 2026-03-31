// Copyright 2020-2024 Beken
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

#include <stdint.h>
#include  "mshc_regs.h"

#include <components/log.h>

#define SDIOD_TAG "sdio_dwc"
#define SDIOD_LOGI(...) BK_LOGI(SDIOD_TAG, ##__VA_ARGS__)
#define SDIOD_LOGW(...) BK_LOGW(SDIOD_TAG, ##__VA_ARGS__)
#define SDIOD_LOGE(...) BK_LOGE(SDIOD_TAG, ##__VA_ARGS__)
#define SDIOD_LOGD(...) BK_LOGD(SDIOD_TAG, ##__VA_ARGS__)


#ifdef CONFIG_CPU64_BIT
typedef unsigned long addr_t;
#else
typedef unsigned int addr_t;
#endif

#define XUINT8      0
#define XUINT16     1
#define XUINT32     2
#define XINT8       3
#define XINT16      4
#define XINT32      5
#define XSTRING     6
#define XFLOAT      7

#define ALIGN_4BYTE 4
#define ALIGN_8BYTE 8
#define ALIGN_16BYTE    16

#define XSTATE0     0
#define XSTATE1     1
#define XSTATE2     2
#define XSTATE3     3
#define XSTATE4     4
#define XSTATE5     5
#define XSTATE6     6
#define XSTATE7     7
#define XSTATE8     8
#define XSTATE9     9
#define XSTATE10    10
#define XSTATE11    11
#define XSTATE12    12
#define XSTATE13    13
#define XSTATE14    14
#define XSTATE15    15
#define XSTATE16    16
#define XSTATE17    17
#define XSTATE18    18
#define XSTATE19    19
#define XSTATE20    20
#define XSTATE21    21
#define XSTATE22    22
#define XSTATE23    23
#define XSTATE24    24

#define BIT_ON      1
#define BIT_OFF     0

#define ERR_INVARG      -1
#define ERR_NORESOURCE  -2

#define MAX_CMD_PARAMS                 64
#define XSTATE_CMD_INIT                XSTATE0
#define XSTATE_CMD_SUBMIT              XSTATE1
#define XSTATE_WAIT_IO_COMPLETE        XSTATE2
#define XSTATE_CMD_DONE                XSTATE3

#define XUSER_PAT     0
#define XRAND         1

struct cmd_param_s {
    /* cmd validity bit */
    uint8_t is_valid;
    /* cmd and status */
    uint8_t cmd;
    uint8_t status;
    uint8_t completed;
    /* response type and response data */
    uint8_t resp_type;
    uint32_t resp[4];
    /* data buffer and datalen */
    uint8_t *databuf;
    uint32_t datalen;
    /* cmd parameter len and parameters */
    uint8_t param_len;
    uint8_t param[MAX_CMD_PARAMS];
};

struct cmd_param_t {
    uint8_t in_use;
    uint8_t state;
    struct cmd_param_s cur_cmd; /* if valid execute current command */
    struct cmd_param_s pre_cmd; /* if valid, execute pre-cmd before executing current cmd */
    struct cmd_param_s post_cmd; /*if valid, execute post-cmd after executing current cmd */
};

#define sdio_mshc_0_base     0x48040000
#define sdio_mshc_1_base     0x48050000

//********user reg **********//
#define sdio_reg0(addr)                                   *((volatile unsigned int *)  (addr + 0x700))
#define sdio_reg1(addr)                                   *((volatile unsigned int *)  (addr + 0x704))
#define sdio_reg2(addr)                                   *((volatile unsigned int *)  (addr + 0x708))
#define sdio_reg3(addr)                                   *((volatile unsigned int *)  (addr + 0x70c))
#define sdio_reg4(addr)                                   *((volatile unsigned int *)  (addr + 0x710))
#define sdio_reg5(addr)                                   *((volatile unsigned int *)  (addr + 0x714))

//*******sd card cmd*********//
#define CMD0   0
#define CMD1   1
#define CMD2   2
#define CMD3   3
#define CMD6   6
#define CMD7   7
#define CMD8   8
#define CMD9   9
#define CMD11  11
#define CMD13  13
#define CMD16  16
#define CMD17  17
#define CMD18  18
#define CMD21  21
#define CMD23  23
#define CMD24  24
#define CMD25  25
#define CMD41  41
#define CMD51  51
#define CMD55  55

#define EMMC_CARD  1
#define SD_CARD    0
#define DATA_WIDTH1      1
#define DATA_WIDTH4      4
#define DATA_WIDTH8      8

#define CMD6_SDR_1BIT    0
#define CMD6_SDR_4BIT    2
#define CMD6_SDR_8BIT    3
#define CMD6_DDR_4BIT    6
#define CMD6_DDR_8BIT    7

#define clk320M  1
#define clkAPLL  0

#define VER4_EN  1
// eof

