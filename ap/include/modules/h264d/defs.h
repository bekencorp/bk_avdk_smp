/*------------------------------------------------------------------------------
--         Copyright (c) 2015, VeriSilicon Inc. All rights reserved.          --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef __DEFS_H_INCLUDED_
#define __DEFS_H_INCLUDED_

#ifdef _ASSERT_USED
#include <assert.h>
#endif

#include <common/bk_include.h>
//#include <stdio.h>
#include "os/mem.h"
#include "basetype.h"
#include <stdbool.h>

// #define malloc(a)                    pvPortMalloc(a)
// #define realloc(a,b)                 pvPortRealloc(a,b)
// #define calloc(a,b)                  pvPortCalloc(a,b)
// #define free(a)                      vPortFree(a)    //7259debug@zhilei
// #define malloc(a)                    os_malloc(a)
// #define realloc(a,b)                 os_realloc(a,b)
// #define calloc(a,b)                  os_zalloc(a*b)
// #define free(a)                      os_free(a)    
#define TRUE true
#define FALSE false

/*------------------------------------------------------------------------------
    Generic data type stuff
------------------------------------------------------------------------------*/


// typedef enum {
//     FALSE = 0,
//     TRUE  = 1
// } bool;

#define __INLINE inline

/*------------------------------------------------------------------------------
    Defines
------------------------------------------------------------------------------*/
#define PP_MAX_CHANNELS         (4)
#define PP_MAX_FILENAME         (256)
#define PP_MAX_SCALED_WIDTH     (8192)
#define PP_MAX_MASKS            (2)
#define PP_MAX_RGB_BYTES        (4)
#define PP_INVALID_CODE         (-1)
#define PP_MAX_FILTER_SIZE      (15*15)
#define PP_MAX_WORD_WIDTH_BYTES (8)

#define PP_DEFAULT_OUT_YUV      ("pp_out.yuv")
#define PP_DEFAULT_OUT_RGB      ("pp_out.rgb")

/* Environment variables */
#define ENV_TEST_DATA_FORMAT    ("TEST_DATA_FORMAT")
#define ENV_PP_MODEL_HOME       ("POST_PROCESSING_MODEL_HOME")
#define ENV_TESTDATA_OUT        ("TESTDATA_OUT")
#define ENV_TEST_DATA_FILES     ("TEST_DATA_FILES")
#define ENV_CASE_NBR            ("CASE_NBR")
/* Test data formats */
#define ENV_TDFMT_FPGA          ("FPGA")
#define ENV_TDFMT_HEX           ("HEX")
#define ENV_TDFMT_TRC           ("TRC")
/* Test data files */
#define ENV_TDFILE_TOPLEVEL     ("TOPLEVEL")
#define ENV_TDFILE_ALL          ("ALL")

/* Color value used in masks without PiP */
#define PP_MASK_VALUE           (0)

/* macro for assertion, used only if compiler flag _ASSERT_USED is defined */
#undef ASSERT /* just to avoid conflicts */
#ifdef _ASSERT_USED
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) if(!(expr)) { bk_printf("assert failed, file: %s line: %d :: %s.\n", __FILE__, __LINE__, #expr);}
#endif

/*------------------------------------------------------------------------------
    General purpose macros
------------------------------------------------------------------------------*/
#undef MIN /* just to avoid conflicts */
#define MIN(a,b) ((a)<(b)?(a):(b))
#undef MAX /* just to avoid conflicts */
#define MAX(a,b) ((a)>(b)?(a):(b))
#define SATURATE(a,b,c) (((c)<(a))?(a):(((c)>(b))?(b):(c)))

/*------------------------------------------------------------------------------
    Pixel format macros and bits
------------------------------------------------------------------------------*/
/* Bits to identify image format */
#define PP_FMT_RGB                  (1<<16)
#define PP_FMT_YUV                  (1<<17)
#define PP_FMT_INTERLEAVED          (1<<18)
#define PP_FMT_YUV_CH_INTERLEAVED   (1<<19)
#define PP_FMT_ROTATED              (1<<20)
#define PP_FMT_START_CH             (1<<21)
#define PP_FMT_CR_FIRST             (1<<22)
#define PP_FMT_TILED_4              (1<<23)
/* Macros to identify image format */
#define PP_IS_RGB(a)                ( (a) & PP_FMT_RGB )
#define PP_IS_YUV(a)                ( (a) & PP_FMT_YUV )
#define PP_IS_INTERLEAVED(a)        ( (a) & PP_FMT_INTERLEAVED )
#define PP_IS_TILED_4(a)            ( (a) & PP_FMT_TILED_4 )
#define PP_IS_YUV_CH_INTERLEAVED(a) ( (a) & PP_FMT_YUV_CH_INTERLEAVED )
#define PP_IS_RGB_PLANAR(a)         ( (a) & PP_FMT_RGB_PLANAR )
#define PP_HAS_CHROMA(a)            ( PP_IS_YUV(a) && ((a) != YUV400 ))
/* Macro to return internal format for image format (i.e. discard interleaving
   etc. stuff */
#define PP_INTERNAL_FMT(a)          ( (a) & ( 0xFFFF | PP_FMT_RGB | PP_FMT_YUV ) )

/*------------------------------------------------------------------------------
    Pixel format enumerator.
------------------------------------------------------------------------------*/
typedef enum {
    /* YUV formats */
    YUV400      = PP_FMT_YUV | 400,
    YUV420      = PP_FMT_YUV | 420,
    YUV422      = PP_FMT_YUV | 422,
    YUV444      = PP_FMT_YUV | 444,
    YUV411      = PP_FMT_YUV | 411,
    /* YUV formats, all channels 11interleaved */
    YUYV422     = PP_FMT_INTERLEAVED | YUV422,
    YVYU422     = PP_FMT_INTERLEAVED | YUV422 | PP_FMT_CR_FIRST,
    UYVY422     = PP_FMT_INTERLEAVED | YUV422 | PP_FMT_START_CH,
    VYUY422     = PP_FMT_INTERLEAVED | YUV422 | PP_FMT_START_CH | PP_FMT_CR_FIRST,
    AYCBCR      = PP_FMT_INTERLEAVED | YUV444,
    /* YUV formats, Cb and Cr interleaved */
    YUV420C     = PP_FMT_YUV_CH_INTERLEAVED | YUV420,
    YUV422C     = PP_FMT_YUV_CH_INTERLEAVED | YUV422,
    YUV444C     = PP_FMT_YUV_CH_INTERLEAVED | YUV444,
    YUV411C     = PP_FMT_YUV_CH_INTERLEAVED | YUV411,
    /* rotated */
    YUV422CR    = YUV422C | PP_FMT_ROTATED,
    /* tiled */
    YUYV422T4   = YUYV422 | PP_FMT_TILED_4,
    YVYU422T4   = YVYU422 | PP_FMT_TILED_4,
    UYVY422T4   = UYVY422 | PP_FMT_TILED_4,
    VYUY422T4   = VYUY422 | PP_FMT_TILED_4,
    /* RGB formats */
    RGBP        = PP_FMT_RGB | 24,
    RGB         = PP_FMT_INTERLEAVED | RGBP
} PixelFormat;

/*------------------------------------------------------------------------------
    Modes to determine alpha source values
------------------------------------------------------------------------------*/
typedef enum {
    ALPHA_SEQUENTIAL,
    ALPHA_CHANNEL_0,
    ALPHA_CHANNEL_1,
    ALPHA_CHANNEL_2,
    ALPHA_CHANNEL_3,
    ALPHA_CONSTANT
} AlphaSource;


/*------------------------------------------------------------------------------
    Test data format enumeration.
------------------------------------------------------------------------------*/
typedef enum {
    PP_TRC_NONE = 0,
    PP_TRC_FPGA,
    PP_TRC_HEX,
    PP_TRC_TRC
} TestDataFormat;

/*------------------------------------------------------------------------------
    Scaling parameters, output from the scaling module
------------------------------------------------------------------------------*/
typedef struct {
    u32 W, H;
    u32 Winv, Hinv;
    u32 Ch, Cv;
    u32 hKernelParams[PP_MAX_SCALED_WIDTH][4]; /* horiz.interp.kernel params */
    u32 vKernelParams[PP_MAX_SCALED_WIDTH][2]; /* vert.interp.kernel params  */
    u32 hKernelCoeffs[PP_MAX_SCALED_WIDTH][4]; /* horiz.interp.kernel coeffs */
    u32 vKernelCoeffs[PP_MAX_SCALED_WIDTH][2]; /* vert.interp.kernel coeffs  */
    u32 weight[PP_MAX_SCALED_WIDTH]; /* should be height but doesn't matter */
} ScaleParams;

/*------------------------------------------------------------------------------
    Test data files enumeration.
------------------------------------------------------------------------------*/
typedef enum {
    PP_FILES_NONE = 0,
    PP_FILES_TOPLEVEL,
    PP_FILES_ALL
} TestDataFiles;

/*------------------------------------------------------------------------------
    RGB format structure.
------------------------------------------------------------------------------*/
typedef struct {
    /* Bit sizes */
    u32 pad[PP_MAX_CHANNELS+1];
    u32 chan[PP_MAX_CHANNELS];
    /* Total bytes per pel */
    u32 bytes;
    /* Channel order */
    u32 order[PP_MAX_CHANNELS];
    /* Data for trace module
       0 = R, 1 = G, 3 = B */
    u32 offs[PP_MAX_CHANNELS];
    u32 mask[PP_MAX_CHANNELS];
} RgbFormat;

/*------------------------------------------------------------------------------
    RGB format structure.
------------------------------------------------------------------------------*/
typedef struct {
    u32     byteOrder[PP_MAX_WORD_WIDTH_BYTES];
    u32     bytes;
    u32     wordWidth;
} ByteFormat;

/*------------------------------------------------------------------------------
    Pixel data, denotes a single frame.
------------------------------------------------------------------------------*/
typedef struct {
    u32         width,
                height,
                fieldOutput,
                ditheringEna;
    PixelFormat fmt;

    u8 * base;
    /* Declare picture data. Union structure is used for some code clarity;
       chan[0] maps to R in RGB and Y in YUV formats etc. */
    union {
        u8 ** chan[PP_MAX_CHANNELS];
        struct { u8 **Y, **Cb, **Cr; } yuv;
        struct { u8 **R, **G, **B, **A; } rgb;
    };
} PixelData;


/*------------------------------------------------------------------------------
    Color conversion constants
------------------------------------------------------------------------------*/
typedef struct {
    u32     videoRange;
    /* Contrast/brightness/etc adjusting coefficients and values */
    i32     a1, a2, a, b, c, d, e, f;
    i32     off1, off2, thr1, thr2;
    /* "Intuitive" values, these are mapped to above values */
    i32     brightness, contrast, saturation;
} ColorConversion;

/*------------------------------------------------------------------------------
    Overlay
------------------------------------------------------------------------------*/
typedef struct {
    u32     enabled;
    i32     x0, y0;
    PixelData   data;
    char    fileName[PP_MAX_FILENAME+1];
    PixelFormat pixelFormat;
    AlphaSource alphaSource;
} Overlay;

/*------------------------------------------------------------------------------
    Image area, used for masking
------------------------------------------------------------------------------*/
typedef struct {
    i32     x0, y0, x1, y1;
    Overlay overlay;
} Area;

/*------------------------------------------------------------------------------
    FIR
------------------------------------------------------------------------------*/
typedef struct {
    i32     coeffs[ PP_MAX_FILTER_SIZE ];
    i32     divisor;
    u32     size; /* 3, 5, 7.. */
} FilterData;

/*------------------------------------------------------------------------------
    Deinterlacing parameters
------------------------------------------------------------------------------*/
typedef struct {
    u32     enable;
    u32     blendEnable;
    u32     edgeDetectValue;
    u32     threshold;
} Deinterlacing;

/*------------------------------------------------------------------------------
    Range mapping parameters
------------------------------------------------------------------------------*/
typedef struct {
    u32     enableY;
    u32     coeffY;
    u32     enableC;
    u32     coeffC;
} RangeMapping;

/*------------------------------------------------------------------------------
    Trace file data
------------------------------------------------------------------------------*/
typedef enum {
    TRC_HEX,
    TRC_TRC,
} TraceFormat;

/*------------------------------------------------------------------------------
    PP block parameters
------------------------------------------------------------------------------*/
typedef struct {

    ColorConversion cnv;

    /* PIP, zoom, etc. areas. */
    Area    * mask[ PP_MAX_MASKS ];
    i32     pipx0, pipy0;
    i32     zoomx0, zoomy0;
    u32     maskCount;

    /* Input/output files */
    char inputFile[PP_MAX_FILENAME+1];
    char outputFile[PP_MAX_FILENAME+1];
    PixelFormat inputFormat, outputFormat;
    char pipBackground[PP_MAX_FILENAME+1];
    bool pipeline;
    bool filterEnabled;
    bool jpeg;
    bool crop8Right;
    bool crop8Down;

    RgbFormat   outRgbFmt;
    /* output byte formats */
    ByteFormat  byteFmt;
    ByteFormat  bigEndianFmt;
    /* input byte formats */
    ByteFormat  byteFmtInput;
    ByteFormat  bigEndianFmtInput;
    ByteFormat  stdByteFmt;

    u32 ppInWidth, ppInHeight;
    u32 ppOrigWidth, ppOrigHeight;

    /* Frames */
    i32     firstFrame,
            frames;

    /* Rotation */
    i32     rotation;

    u32     inputTraceMode;

    /* High-pass filter data */
    FilterData * firFilter;

    /* Pixel datas */
    PixelData   input,
                output,
                * pip, /* pip frame, if used */
                * pipTmp,
                * pipYuv, /* Yuv pip frame, if pip and rgb used. */
                * zoom; /* zoom area, if used */

    Deinterlacing deint;
    RangeMapping range;
    u32 inputStruct;
} PpParams;


#endif /* __DEFS_H_INCLUDED_ */
