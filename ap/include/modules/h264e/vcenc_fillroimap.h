/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2021 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef __VCENCFILLROIMAP_H__
#define __VCENCFILLROIMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup api_helper
 *
 * @{
 */

/** Specifies quantization parameter (QP) value types. */
enum RoiQpType_e {
  /** QP delta. */
  QP_TYPE_DELTA=0,
  /** Absolute QP. */
  QP_TYPE_ABS=1
};

/** Specifies CU modes. */
enum RoiCuMode_e {
  /** The inter mode. */
  CU_MODE_INTER=0,
  /** The intra mode. */
  CU_MODE_INTRA=1,
  /** The skip mode. */
  CU_MODE_SKIP=2,
  /** The IPCM mode. */
  CU_MODE_IPCM=3,
};

/** Specifies partition modes for intra-mode CU. */
enum RoiIntraPartType_e {
  /** 2Nx2N. */
  ROI_PART_2N_2N_I = 0,
  /** NxN. */
  ROI_PART_N_N_I,
};

/** Specifies partition modes for inter-mode CU. */
enum RoiInterPartType_e {
  /** Automatic mode. */
  ROI_INTERNAL_AUTOMODE = 0,
  /** 2Nx2N. */
  ROI_PART_2N_2N = 1,
  /** 2NxN. */
  ROI_PART_2N_N,
  /** Nx2N. */
  ROI_PART_N_2N,
  /** 2NxNU. */
  ROI_PART_2N_NU,
  /** 2NxND. */
  ROI_PART_2N_ND,
  /** NLx2N. */
  ROI_PART_NL_2N,
  /** NRx2N. */
  ROI_PART_NR_2N,
};

/** \brief Defines a ROI and contains its attributes. */
typedef struct {
  /** \brief The X coordinate of the top-left corner of the ROI. */
  u16 x0;
  /** \brief The Y coordinate of the top-left corner of the ROI. */
  u16 y0;
  /** \brief The width of the ROI. */
  u16 dx;
  /** \brief The height of the ROI. */
  u16 dy;
  /** \brief The QP delta or the absolute QP value, according to the setting of
   *  <tt>RoiRect.qp_type</tt>. */
  u8 qp;
  /** \brief The value type of <tt>RoiRect.qp</tt>.
   *  \n For the available value types, see Section <em> \ref RoiQpType_e</em>. */
  u8 qp_type;
  /** \brief Whether the setting of <tt>RoiRect.qp</tt> is valid.
   *  \n <tt>0</tt>: invalid.
   *  \n <tt>1</tt>: valid. */
  u8 qp_valid;
  /** \brief The CU size used to encode in the specified ROI. */
  u8 cu_size;
  /** \brief Whether the setting of <tt>RoiRect.cu_size</tt> is valid.
   *  \n <tt>0</tt>: invalid.
   *  \n <tt>1</tt>: valid. */
  u8 cu_size_valid;
  /** \brief The CU mode used to encode the specified ROI.
   *  \n For the available CU modes, see Section <em> \ref RoiCuMode_e</em>. */
  u8 mode;
  /** \brief The partition mode used to encode the specified ROI.
   *  \n The available partition modes vary according to the CU mode. For details, see
   *  Section <em> \ref RoiIntraPartType_e</em> and Section <em> \ref RoiInterPartType_e</em>. */
  i8 part;
  /** \brief Whether the settings of <tt>RoiRect.mode</tt> and <tt>RoiRect.part</tt> are valid.
   *  \n <tt>0</tt>: invalid.
   *  \n <tt>1</tt>: valid. */
  u8 mode_valid;
  /** \brief Obsoleted. */
  u8 intra_part_valid;
  /** \brief (Under development) The prediction unit (PU) information. */
  union {
    /** \brief The inter PU information for two PUs. */
    struct {
      /** \brief The direction. */
      i32 dir;
      /** \brief The index of the reference used in each of the two reference lists. */
      u8 ref_idx[2];
      /** \brief The horizontal motion vector (MV) for the two references. */
      u8 mvx[2];
      /** \brief The vertical MV for the two references. */
      u8 mvy[2];
    } inter[2];
    /** \brief The intra PU information. */
    struct {
      /** \brief The luma modes, only luma_mode[0] valid for \ref ROI_PART_2N_2N_I and all
       *  entries valid for \ref ROI_PART_N_N_I. */
      i32 luma_mode[4];
      /** \brief The chroma mode. For the available values, see ITU-T Rec. H.265 and H.264. */
      i32 chroma_mode;
    } intra;
  } info;
} RoiRect;

#ifndef ROI_BUILD_SUPPORT
#define VCEncFillRoiMap(inst, roi, roimap_format, roimap_delta_qpmemory, \
                        blk_side_length)                                 \
  (0)
#else
/**
 * Fills the ROI map buffer according to the provided ROI configurations.
 *
 * \param [in] inst The encoder instance.
 * \param [in] roi The ROI and its attributes.
 * \param [in] format The format used to fill the buffer.
 *                    \n Only formats 1 to 4 are supported. For details, see <em>Hantro VC9000E
 *                       Series Memory Buffer and Format Organization</em>.
 * \param [in] buffer The ROI map buffer to be filled.
 * \param [in] blk_size The size of the unit block for the ROI map.
 *
 * \return \ref VCEncRet
 */
VCEncRet VCEncFillRoiMap(const void *inst, const RoiRect *roi, u8 format,
                   u8 *buffer, u32 blk_size);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __VCENCFILLROIMAP_H__ */

