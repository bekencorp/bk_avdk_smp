/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon Inc.                                --
--                                                                            --
--                   (C) COPYRIGHT 2015 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
-- Description : Hevc SEI Messages.
--
------------------------------------------------------------------------------*/

#ifndef VCENC_HDR_INFO_H
#define VCENC_HDR_INFO_H

#include "base_type.h"

#define DEBUG_JSON


#if defined(__cplusplus)
extern "C" {
#endif

/* see ss_hdr_t35 in file vcenc_api_main.h for feature description. */

/**
 * \addtogroup api_helper
 *
 * @{
 */

/**
 * \brief Contains the information in Bezier curve packets defined by HDR10+.
 */
typedef struct {
  /** \brief <tt>tone_mapping_flag[w]</tt> indicates whether the metadata of the tone mapping
   *  function in the w-th processing window is present. */
  u32 tone_mapping_flag[3];
  /** \brief <tt>knee_point_x[w]</tt> indicates the X coordinate of the separation point
   *  between the linear part and the curved part of the tone mapping function in the w-th
   *  processing window. */
  u32 knee_point_x[3];
  /** \brief <tt>knee_point_y[w]</tt> indicates the Y coordinate of the separation point
   *  between the linear part and the curved part of the tone mapping function in the w-th
   *  processing window. */
  u32 knee_point_y[3];
  /** \brief <tt>num_bezier_curve_anchors[w]</tt> indicates the number of the intermediate
   *  anchor parameters of the tone mapping function in the w-th processing window. */
  u32 num_bezier_curve_anchors[3];
  /** \brief <tt>bezier_curve_anchors[w*15+i]</tt> indicates the i-th intermediate anchor
   *  parameter of the tone mapping function in the w-th processing window in the scene. */
  u32 bezier_curve_anchors[3 * 15];
  /** \brief <tt>color_saturation_mapping_flag[w]</tt> indicates the color saturation mapping
   *  flag for the w-th processing window.
   *  \n <tt>0</tt>: compliant with SMPTE ST 2094-40.
   *  \n <tt>1</tt>: reserved for future use. */
  u32 color_saturation_mapping_flag[3];
  /** \brief <tt>color_saturation_weight[w]</tt> indicates the color saturation gain in the
   *  w-th processing window in the scene. */
  u32 color_saturation_weight[3];
} BezierDict;

/**
 * \brief Contains the information in Mastering packets defined by HDR10+.
 */
typedef struct {
  /** \brief The actual peak luminance flag of the mastering display.
   *  \n <tt>0</tt>: compliant with SMPTE ST 2094-40.
   *  \n <tt>1</tt>: reserved for future use. */
  u32 mastering_display_actual_peak_luminance_flag;
  /** \brief The number of rows in the <tt>MasteringDict.mastering_display_actual_peak_luminance</tt>
   *  array. */
  u32 num_rows_mastering_display_actual_peak_luminance;
  /** \brief The number of columns in the <tt>MasteringDict.mastering_display_actual_peak_luminance</tt>
   *  array. */
  u32 num_cols_mastering_display_actual_peak_luminance;
  /** \brief The normalized actual peak luminance of the mastering display. */
  u32 mastering_display_actual_peak_luminance[31 * 31];
} MasteringDict;

/**
 * \brief Contains the information in DistDict packets defined by HDR10+.
 */
typedef struct {
  /** \brief The maximum linearized RGB values in each processing window in the scene.
   *  \n <tt>maxscl[w*3+i]</tt> indicates the maximum value for the i-th color component of
   *  R, G, and B in the w-th processing window. */
  u32 maxscl[3 * 3];
  /** \brief <tt>average_maxrgb[w]</tt> indicates the average of maximum linearized RGB values
   *  in the w-th processing window in the scene. */
  u32 average_maxrgb[3];
  /** \brief <tt>num_distribution_maxrgb_percentiles[w]</tt> indicates the number of maximum
   *  linearized RGB values at given percentiles in the w-th processing window in the scene. */
  u32 num_distribution_maxrgb_percentiles[3];
  /** \brief <tt>distribution_maxrgb_percentages[w*15+i]</tt> indicates the integer percentage
   *  of maximum linearized RGB values at the i-th percentile in the w-th processing window in
   *  the scene. */
  u32 distribution_maxrgb_percentages[3 * 15];
  /** \brief <tt>distribution_maxrgb_percentiles[w*15+i]</tt> indicates the number of maximum
   *  linearized RGB values at the i-th percentile in the w-th processing window in the scene. */
  u32 distribution_maxrgb_percentiles[3 * 15];
  /** \brief <tt>fraction_bright_pixels[w]</tt> indicates the fraction of selected pixels in
   *  the picture that contains the brightest pixel in the w-th processing window in the scene. */
  u32 fraction_bright_pixels[3];
} DistDict;

/**
 * \brief Contains the information in PeakLum packets defined by HDR10+.
 */
typedef struct {
  /** \brief The number of rows in the <tt>PeakLum.targeted_system_display_actual_peak_luminance</tt>
   *  array. */
  u32 num_rows_targeted_system_display_actual_peak_luminance;
  /** \brief The number of columns in the <tt>PeakLum.targeted_system_display_actual_peak_luminance</tt>
   *  array. */
  u32 num_cols_targeted_system_display_actual_peak_luminance;
  /** \brief The normalized actual peak luminance of targeted system display. */
  u32 targeted_system_display_actual_peak_luminance[31 * 31];
} PeakLum;

/**
 * \brief Contains the information of a processing window in ProcWindows packets defined by HDR10+.
 */
typedef struct {
  /** \brief The X coordinate of the top-left pixel of the processing window. */
  u32 window_upper_left_corner_x;
  /** \brief The Y coordinate of the top-left pixel of the processing window. */
  u32 window_upper_left_corner_y;
  /** \brief The X coordinate of the bottom-left pixel of the processing window. */
  u32 window_lower_left_corner_x;
  /** \brief The Y coordinate of the bottom-left pixel of the processing window. */
  u32 window_lower_left_corner_y;
  /** \brief The X coordinate of the center position of concentric internal and external ellipses
   *  of the elliptical pixel selector in the processing window. */
  u32 center_of_elipse_x;
  /** \brief The Y coordinate of the center position of concentric internal and external ellipses
   *  of the elliptical pixel selector in the processing window. */
  u32 center_of_elipse_y;
  /** \brief The clockwise rotation angle in degrees, relative to the positive direction of the
   *  X-axis of the concentric internal and external ellipses of the elliptical pixel selector in
   *  the processing window. */
  u32 rotation_angle;
  /** \brief The semi-major axis value, in pixels, of the internal ellipse of the elliptical pixel
   *  selector in the processing window. */
  u32 semimajor_axis_internal_elipse;
  /** \brief The semi-major axis value, in pixels, of the external ellipse of the elliptical pixel
   *  selector in the processing window. */
  u32 semimajor_axis_external_elipse;
  /** \brief The semi-minor axis value, in pixels, of the external ellipse of the elliptical pixel
   *  selector in the processing window. */
  u32 semiminor_axis_external_elipse;
  /** \brief The method to combine rendered pixels in the processing window with at least one
   *  elliptical pixel selector.
   *  \n <tt>0</tt>: weighted averaging.
   *  \n <tt>1</tt>: layering. */
  u32 overlap_process_option;
} ProcWindows;

/**
 * \brief Contains the information of an extended display mapping metadata block in ExtDmDataBlock
 * packets defined by Dolby Vision.
 */
typedef struct {
  /** \brief A length used to derive the size of the extended display mapping metadata block payload
   *  in bytes.
   *  \n The value range is from <tt>0</tt> to <tt>1023</tt>, inclusive. */
  u32 ext_block_length;
  /** \brief The level of payload contained in the extended display mapping metadata block.
   *  \n The value range is from <tt>0</tt> to <tt>255</tt>, inclusive. */
  u32 ext_block_level;
  /** \brief The minimum luminance value of the current picture in 12-bit PQ encoding.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive. */
  u32 min_PQ;
  /** \brief The maximum luminance value of the current picture in 12-bit PQ encoding.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive. */
  u32 max_PQ;
  /** \brief The midpoint luminance value of the current picture in 12-bit PQ encoding.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive. */
  u32 avg_PQ;
  /** \brief The maximum luminance value of a target display in 12-bit PQ encoding.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive. */
  u32 target_max_PQ;
  /** \brief The slope metadata.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive.
   *  \n If trim_slope is not present, the value is <tt>2048</tt>. */
  u32 trim_slope;
  /** \brief The offset metadata.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive.
   *  \n If trim_offset is not present, the value is <tt>2048</tt>. */
  u32 trim_offset;
  /** \brief The power metadata.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive.
   *  \n If trim_power is not present, the value is <tt>2048</tt>. */
  u32 trim_power;
  /** \brief The chroma weight metadata.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive.
   *  \n If trim_chroma_weight is not present, the value is <tt>2048</tt>. */
  u32 trim_chroma_weight;
  /** \brief The saturation gain metadata.
   *  \n The value range is from <tt>0</tt> to <tt>4095</tt>, inclusive.
   *  \n If trim_saturation_gain is not present, the value is <tt>2048</tt>. */
  u32 trim_saturation_gain;
  /** \brief Reserved. */
  u32 ms_weight;
  /** \brief The X offset between the left border of the active area and the leftmost
   *  selected pixel of the current picture.
   *  \n The value range is from <tt>0</tt> to <tt>8191</tt>, inclusive. For details,
   *  see ProcessingWindow definitions in SMPTE ST 2094-10. */
  u32 active_area_left_offset;
  /** \brief The X offset between the right border of the active area and the rightmost
   *  selected pixel of the current picture.
   *  \n The value range is from <tt>0</tt> to <tt>8191</tt>, inclusive. For details,
   *  see ProcessingWindow definitions in SMPTE ST 2094-10. */
  u32 active_area_right_offset;
  /** \brief The Y offset between the top border of the active area and the topmost
   *  selected pixel of the current picture.
   *  \n The value range is from <tt>0</tt> to <tt>8191</tt>, inclusive. For details,
   *  see ProcessingWindow definitions in SMPTE ST 2094-10. */
  u32 active_area_top_offset;
  /** \brief The Y offset between the bottom border of the active area and the bottommost
   *  selected pixel of the current picture.
   *  \n The value range is from <tt>0</tt> to <tt>8191</tt>, inclusive. For details,
   *  see ProcessingWindow definitions in SMPTE ST 2094-10. */
  u32 active_area_bottom_offset;
} ExtDmDataBlock;

/**
 * \brief Contains the information in MetaData packets defined by Dolby Vision.
 */
typedef struct {
  /** \brief The number of extended display mapping metadata blocks. */
  u32 num_ext_blocks;
  /** \brief The information of each extended display mapping metadata block. */
  ExtDmDataBlock ext_dm_data_block[100];
} MetaData;

/**
 * \brief HDR10Info member: HDR10 plus data packet
 * \n See \ref HDR10Info
 */
typedef struct {
  /** \brief The country code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_country_code;
  /** \brief The terminal provider code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_terminal_provider_code;
  /** \brief The terminal provider oriented code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_terminal_provider_oriented_code;
  /** \brief The application identifier. */
  u32 application_identifier;
  /** \brief The application version. */
  u32 application_version;
  /** \brief The number of windows.
   *  \n A maximum of three windows are supported. */
  u32 num_windows;
  /** \brief The information in processing window packets of HDR10+, with
   *  <tt>processing_windows[w]</tt> for the w-th processing window. */
  ProcWindows processing_windows[3];
  /** \brief The maximum luminance for display, in the unit of 0.0001 candelas
   *  per square meter. */
  u32 targeted_system_display_maximum_luminance;
  /** \brief The actual peak luminance flag of the targeted system display.
   *  \n <tt>0</tt>: compliant with SMPTE ST 2094-40.
   *  \n <tt>1</tt>: reserved for future use. */
  u32 targeted_system_display_actual_peak_luminance_flag;
  /** \brief The information in actual peak luminance packets of HDR10+. */
  PeakLum targeted_system_display_actual_peak_luminance;
  /** \brief The information in distribution packets of HDR10+. */
  DistDict distribution;
  /** \brief The information in mastering packets of HDR10+. */
  MasteringDict mastering;
  /** \brief The information in bezier curve packets of HDR10+. */
  BezierDict bezier_curve;
} hdr10_plus_attr;

/**
 * \brief HDR10Info member: Dolby vision data packet
 * \n See \ref HDR10Info
 */
typedef struct {
  /** \brief The country code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_country_code;
  /** \brief The terminal provider code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_terminal_provider_code;
  /** \brief The terminal provider oriented code defined in ITU-T Rec. T.35. */
  u32 itu_t_t35_terminal_provider_oriented_code;
  /** \brief The application identifier. */
  u32 application_identifier;
  /** \brief The application version. */
  u32 application_version;
  /** \brief The data type code defined by Dolby Vision. */
  u32 data_type_code;
  /** \brief The metadata refresh flag defined by Dolby Vision.
   *  \n This field is used to identify whether the data to be parsed is compatible
   *  with HDR10+ or Dolby Vision.
   *  \n <tt>0</tt>: HDR10+.
   *  \n <tt>1</tt>: Dolby Vision. */
  u32 metadata_refrensh_flag;
  /** \brief The metadata packets of Dolby Vision. */
  MetaData metadata;
} dolby_vision_attr;

/**
 * \brief Same data struct for save HDR10+ and Dolby's info
 */
typedef struct {
  /** \brief current frame number */
  u32 frame_num;
  /** \brief HDR date type[1,2:1], 1: HDR10 plus; 2: dolby vision; 3: both in same frame*/
  u32 hdr_type;
  hdr10_plus_attr       hdr10_attr;
  dolby_vision_attr     dolby_attr;
} HDR10Info;

enum {
  HDR_TYPE_HDR10_PLUS = 1,          /**< \brief HDR date type is HDR10 PLUS */
  HDR_TYPE_DOLBY_VISION = 2,        /**< \brief HDR date type is Dolby vision */
  HDR_TYPE_BOTH_HDR10_DOLBY = 3     /**< \brief HDR date type is MIX of HDR10 Plus and Dolby vision */
};

#ifndef HDR_HELPER_SUPPORT
#define VCEncPutHDR10Info(hdr10_info, buffer_size, buffer, hdrType)
#else

/**
 * Fills a buffer with HDR10 information according to the SMPTE standard as T35 packets.
 *
 * \param [in] hdr10_info The HDR information to be coded into T35 packets.
 * \param [in] buffer_size The size of the buffer.
 * \param [out] buffer A pointer to the buffer filled with T35 data.
 *
 * \return The number of bytes filled in the buffer
*/
int VCEncPutHDR10Info(HDR10Info *hdr10_info, u32 buffer_size, u8 *buffer, u8 hdrType);
#endif


/** @} */

#if defined(__cplusplus)
}
#endif

#endif /* VCENC_HDR_INFO_H */

