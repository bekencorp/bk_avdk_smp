/*****************************************************************************
 * Copyright (c) 2024 by VeriSilicon Holdings Co., Ltd. ("VeriSilicon")
 * All Rights Reserved.
 *
 * The material in this file is confidential and contains trade secrets of
 * VeriSilicon.  This is proprietary information owned or licensed by
 * VeriSilicon.  No part of this work may be disclosed, reproduced, copied,
 * transmitted, or used in any way for any purpose, without the express
 * written permission of VeriSilicon.
 *
 *****************************************************************************/

#ifndef VSI_AWB_SP_STAT_H
#define VSI_AWB_SP_STAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Block-average statistics shared between the AP-side frame consumer and the
 * ISP 3A AWB algorithm.
 *
 * The AP side samples the 1920x1080 MP frame on a 160x90 grid (one sample
 * every 12th pixel in both directions) and folds it into 32x18 blocks of 5x5
 * samples each. For every block the mean Y, Cb and Cr are stored; the
 * resulting [576][3] array is published here and read back by
 * VSI_ISP_AwbRun().
 */
#define VSI_AWB_SP_STAT_BLOCKS_X    (32U)
#define VSI_AWB_SP_STAT_BLOCKS_Y    (18U)
#define VSI_AWB_SP_STAT_BLOCK_W     (5U)
#define VSI_AWB_SP_STAT_BLOCK_H     (5U)
#define VSI_AWB_SP_STAT_WIDTH       (VSI_AWB_SP_STAT_BLOCKS_X * VSI_AWB_SP_STAT_BLOCK_W)
#define VSI_AWB_SP_STAT_HEIGHT      (VSI_AWB_SP_STAT_BLOCKS_Y * VSI_AWB_SP_STAT_BLOCK_H)
#define VSI_AWB_SP_STAT_BLOCK_CNT   (VSI_AWB_SP_STAT_BLOCKS_X * VSI_AWB_SP_STAT_BLOCKS_Y)

/* Index of the component in vsi_awb_sp_block_stat_t.mean[]. */
#define VSI_AWB_SP_STAT_MEAN_Y      (0U)
#define VSI_AWB_SP_STAT_MEAN_CB     (1U)
#define VSI_AWB_SP_STAT_MEAN_CR     (2U)

typedef struct vsiAWB_SP_BLOCK_STAT_S {
    uint32_t frame_id;                              /**< \brief Monotonic SP frame counter, 0 before the first update. */
    uint8_t mean[VSI_AWB_SP_STAT_BLOCK_CNT][3];    /**< \brief [block][Y, Cb, Cr] block averages. */
} vsi_awb_sp_block_stat_t;

/**
 * @brief Publish one frame of SP block statistics.
 *
 * Safe to call from a task while VSI_ISP_AwbSpStatGet() is running in the ISP
 * interrupt: the new sample is written into the inactive buffer and the
 * active index is swapped only after the write completes.
 *
 * @param mean     [VSI_AWB_SP_STAT_BLOCK_CNT][3] block averages (Y, Cb, Cr).
 * @param frame_id SP frame counter associated with @p mean.
 */
void VSI_ISP_AwbSpStatUpdate(const uint8_t mean[VSI_AWB_SP_STAT_BLOCK_CNT][3],
                             uint32_t frame_id);

/**
 * @brief Get the latest stable SP block statistics.
 *
 * @return Pointer to the latest published sample, or NULL before the first
 *         VSI_ISP_AwbSpStatUpdate() call.
 */
const vsi_awb_sp_block_stat_t *VSI_ISP_AwbSpStatGet(void);

#ifdef __cplusplus
}
#endif

#endif /* VSI_AWB_SP_STAT_H */
