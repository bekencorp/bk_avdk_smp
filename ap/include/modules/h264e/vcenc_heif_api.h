
/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            VeriSilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2022 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--                                                                            --
--  Abstract : Include API for saving hevc stream as heif file                --
--                                                                            --
------------------------------------------------------------------------------*/
#ifndef _VCENC_HEIF_API_H_
#define _VCENC_HEIF_API_H_

#ifdef HEIF_SUPPORT

#include "hevcencapi.h"
#include "vcenc_heif_writer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup api_heif_vce
 *
 * @{
 */

/** Defines grid modes. */
typedef enum {
  /** Disables grid modes. */
	HEIF_GRID_DISABLE = 0,
	/** Takes each input image as a grid for composing the output image. */
	HEIF_GRID_INPUT_TILE = 1,
	/** Splits input image into multiple grids by indicated numbers of grids (rowsxcols) for encoding and storage. */
	HEIF_GRID_INPUT_PIC = 2,
	/** Splits input image into multiple grids by indicated gird image size (widthxheight) for encoding and storage.  */
	HEIF_GRID_INPUT_PIC_WIDTH = 3
}VCENCHeifGridModeType;

/** \brief Saves the OSD configurations of the undivied image, for display OSD at grid mode 2/3. */
typedef struct VCEncHeifOSDConfig_{
	u32 overlayEnables;		/**< \brief Any of the OSD regions is enabled. */
	/** \brief The OSD regions. */
	VCEncOverlayArea overlayArea[MAX_OVERLAY_NUM];

	/* OSD_MAP */
	u32 osdMapEnable;
	char *osdMapInput;
	u32 osdMapStride;
	u32 osdMapAlpha[MAX_OSDMAP_COLOR_NUM];
	u32 osdMapY[MAX_OSDMAP_COLOR_NUM];
	u32 osdMapU[MAX_OSDMAP_COLOR_NUM];
	u32 osdMapV[MAX_OSDMAP_COLOR_NUM];
}VCEncHeifOSDConfig;

/** \brief Contains the HEIF configurations parsed from an input configuration file. */
typedef struct VCEncHeifConfig_ {
  /** \brief The media data type. */
  VCEncHeifDecInfoType DecInfoType;
  /** \brief The path to the HEIF output file. */
  char output_file[256];
  /** \brief The width of the encoded image, or the grid image when grid mode was enabled. */
  int img_width;
  /** \brief The height of the encoded image, or the grid image when grid mode was enabled. */
  int img_height;
  /** \brief The total number of images saved to the HEIF output file. */
  u32 total_numbers;
  /** \brief The sequence number of the primary image item. */
  u32 primary_number;
  /** \brief The sequence number of the hidden image item. */
  u32 hidden_number;
  /** \brief Whether to save thumbnail images to the HEIF output file.
   *  \n <tt>false</tt>: do not save.
   *  \n <tt>true</tt>: save. */
  bool thumb_enable;
  /** \brief Whether to crop images before saving them to the HEIF output file.
   *  \n <tt>false</tt>: do not crop.
   *  \n <tt>true</tt>: crop. */
  bool crop_enable;
  /** \brief The cropping configurations. */
  VCEncHeifCropCfg crop_config;
  /** \brief Whether to mirror images before saving them to the HEIF output file.
   *  \n <tt>false</tt>: do not mirror.
   *  \n <tt>true</tt>: mirror. */
  bool mirror_enable;
  /** \brief The mirroring type. */
  VCEncHeifMirrorType mirror_type;
  /** \brief Whether to rotate images before saving them to the HEIF output file.
   *  \n <tt>false</tt>: do not rotate.
   *  \n <tt>true</tt>: rotate. */
  bool rotate_enable;
 /** \brief The angle of rotation, in degrees.
  *  \n Valid values include <tt>90</tt>, <tt>180</tt>, and <tt>270</tt>. */
  int angle;
  /** \brief The grid mode. */
  VCENCHeifGridModeType grid_mode;
  /** \brief The image grid configurations. */
  VCEncHeifGridCfg grid_config;
  /** \brief The OSD configurations. */
  VCEncHeifOSDConfig OSD_config;
  /** \brief Whether to overlay images before saving them to the HEIF output file.
   *  \n <tt>false</tt>: do not overlay.
   *  \n <tt>true</tt>: overlay. */
  bool overlay_enable;
  /** \brief The overlaying configurations. */
  VCEncHeifOverlayCfg overlay_config;
  /** \brief Whether to add a group to the HEIF output file.
   *  \n <tt>false</tt>: do not add.
   *  \n <tt>true</tt>: add. */
  bool add_group;
  /** \brief Whether to use an auxiliary image for complementing the master image.
   *  \n <tt>false</tt>: do not use.
   *  \n <tt>true</tt>: use. */
  bool auxiliary_enable;
  /** \brief The type of the auxiliary image. */
  VCEncHeifAuxiliaryType auxType;
  /** \brief Whether to add an edit list to each sequence.
   *  \n <tt>false</tt>: do not add.
   *  \n <tt>true</tt>: add. */
  bool editList_enable;
  /** \brief The path to the file that stores the HEVC (H.265) stream of the auxiliary image. */
  char slave_file[256];
} VCEncHeifConfig;

/** \brief Defines an HEIF instance. */
typedef struct VCEncHeifInst_ {
  /** \brief The HEIF configurations parsed from the input configuration file. */
  VCEncHeifConfig *heif_config;
  /** \brief The HEIF writer handle. */
  VCEncHeifWriterInst writer_inst;
  /** \brief The number of images input to the encoder. */
  u32 input_numbers;
  /** \brief The number of image items saved to the HEIF output file. */
  u32 saved_numbers;
} VCEncHeifInst;

/**
  * Parses HEIF configurations from a file, and fills VCEncHeifInst.heif_config accordingly.
  *
  * \param [in] cfgfile The path to the HEIF configuration file.
  * \param [in] heif_inst The HEIF instance.
  * \param [in] codecType The video codec format, only support hevc(heif) & av1(avif) now.
  *
  * \return \ref VCEncHeifRet
  */
VCEncHeifRet VCENCHeifParseConfig(char *cfgfile, VCEncHeifInst *heif_inst, VCEncVideoCodecFormat codecType);

/**
  * Creates and initializes an HEIF instance.
  *
  * After this API is successfully called, an HEIF writer is also initialized and can be used
  * through <tt>VCEncHeifInst.writer_inst</tt>.
  *
  * \param [in] heif_inst The HEIF instance.
  * \param [in] width The width of the heif output image.
  * \param [in] height The height of the heif output image.
  * \param [in] input_numbers Input image number, for grid_mode = HEIF_GRID_INPUT_TILE, shoud greater
  *             than or equal to grid_cols x grid_rows.
  *
  * \return \ref VCEncHeifRet
  */
VCEncHeifRet VCEncHeifInit(VCEncHeifInst *heif_inst, i32 width, i32 height, u32 input_numbers);

/**
  * Obtains a grid image for input to an encoder.
  *
  * \param [in] encoder The encoder instance.
  * \param [in] heif_inst The HEIF instance.
  *
  * \return \ref VCEncHeifRet
  */
VCEncHeifRet VCEncHeifSetPrpForGrid(VCEncInst encoder, VCEncHeifInst *heif_inst        );


/**
  * Sets the hevc decoder configurations for an HEIF output file.
  *
  * \param [in] writer_inst The HEIF writer handle.
  * \param [in] bufs The stream buffer.
  * \param [in] offset The stream offset.
  * \param [in] size The size of stream data, in bytes.
  *
  * \return None
  */
void VCEncHeifSetHevcDecConfig(VCEncHeifWriterInst writer_inst, VCEncStrmBufs *bufs,
                           u32 offset, u32 size);


/**
  * Writes encoded stream to a file.
  *
  * \param [in] heif_inst The HEIF instance.
  * \param [in] bufs The stream buffer.
  * \param [in] offset The stream offset.
  * \param [in] streamSize The size of the stream data, in bytes.
  *
  * \return \ref VCEncHeifRet
  */
VCEncHeifRet VCEncHeifWriteStream(VCEncHeifInst *heif_inst, VCEncStrmBufs *bufs,
                                  u32 offset, u32 streamSize);

/**
  * Releases the writer of an HEIF instance.
  *
  * \param [in] heif_inst The HEIF instance.
  *
  * \return None
  */
void VCEncHeifCloseWriter(VCEncHeifInst *heif_inst);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif
