// Copyright 2025-2026 Beken
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

#include <driver/isp_base.h>
#include <os/os.h>

#define ISP_FRAME_CNT_MAX (2)
#define ISP_INPUT_SENSOR_NAME "GC2053_1080P_LINEAR"

#include <modules/veri_isp/vsios_type.h>
#include <vsi_comm_video.h>
#include <vsi_comm_isp.h>
#include <modules/private/veri_isp/vsi_comm_sns.h>
#include <modules/private/veri_isp/mpi_isp_sbi.h>
#include <mpi_isp.h>
#include <modules/private/veri_isp/mpi_isp_mi.h>
#include <modules/private/veri_isp/vsios_i2c.h>

typedef struct vsiISP_PUB_ATTR_S {
    ISP_SNS_OBJ_S      *pSnsObj;
    vsi_u8_t           port_id;
    ISP_INPUT_TYPE_E   ispInputType;
    ISP_MODE_E         ispMode;
    ISP_HDR_MODE_E     hdrMode;
    ISP_STICH_MODE_E   stichMode;
    PIXEL_FORMAT_E     pixelFormat;
    vsi_u32_t          snsFps;
    RECT_S             snsRect;
    RECT_S             inFormRect;
    RECT_S             outFormRect;
    RECT_S             iSRect;
    vsios_i2c_attr_t   i2c_attr;
    vsi_u8_t           mipi_data_type;
} ISP_PUB_ATTR_S;

typedef struct vsiISP_PUB_MAP_S {
    uint16_t id;
    const struct vsiISP_PUB_ATTR_S *pPubAttr;
} ISP_PUB_MAP_S;

typedef struct vsiISP_INPUT_ENUM_S
{
    vsi_u8_t index;
    char szName[64];
} ISP_INPUT_ENUM_S;

typedef struct vsiISP_INPUT_S
{
    char szName[64];
} ISP_INPUT_S;

typedef struct {
    uint8_t buf_cnt;
    uint8_t chnl_id;
    uint8_t port_id;
    uint8_t enable_flexa;// 0/1:disable/enable
    uint8_t work_mode;//0: frame mode; 1: flexa mode
    uint16_t width;
    uint16_t height;
    uint16_t format;
    uint32_t clk;
    char *name;
    /** Drop first N complete frames after channel open (AE warmup), 0 = disabled. */
    uint8_t skip_frames;
} isp_config_ext_t;

typedef struct {
    uint8_t enable : 1;
    uint8_t enable_flexa : 1;
    uint8_t sbi_enable_pending : 1;
    uint8_t buf_cnt;
    uint8_t total_line;
    uint8_t line;
    ISP_CHN channel;
    ISP_CHN_ATTR_S chn_attr;
    ISP_SBI_ATTR_S sbi_attr;
    uint8_t *base_addr;
    uint32_t y_addr;
    uint32_t u_addr;
    uint32_t v_addr;
    uint32_t sequence;
    uint8_t skip_frames_remaining; /**< Frames left to drop (AE warmup); no ISR callback to upper layer. */
    uint8_t skip_active;           /**< Current frame is being dropped. */
    uint8_t warmup_done;           /**< Latched 1 only after this channel finished a configured (>0) skip countdown, or inherited an already-warmed peer at open. A skip=0 channel is NOT a warmup authority. */
    uint8_t *frame_buffer[ISP_FRAME_CNT_MAX];
    uint8_t malloc_flag;
} isp_channel_config_t;

typedef struct {
    uint8_t state;
    uint8_t isr_enable;
    uint8_t sensor_sns_registered; /**< paired with VSI_MPI_ISP_SnsRegCallBack in bk_isp_port_init */
    uint8_t port_pipeline_inited;  /**< bitmap (per ISP_PORT_CNT): module pipeline+mutex lazily inited in bk_isp_port_init */
    ISP_DEV dev;
    ISP_PORT port;
    beken_mutex_t isp_mutex;
    beken_semaphore_t isp_sem;
    void *pub_attr[ISP_PORT_CNT];
    isp_channel_config_t chn[ISP_CHN_CNT];
    int (*pop_buf) (ISP_CHN chn, VIDEO_BUF_S *pBuf, uint32_t timeMs);
    int (*free_buf) (ISP_CHN chn, VIDEO_BUF_S *pBuf);
} isp_control_t;

typedef void *isp_handle_t;



#ifdef __cplusplus
}
#endif
