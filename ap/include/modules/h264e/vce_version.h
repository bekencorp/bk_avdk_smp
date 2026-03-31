/*--------------------------------------------------------------------------------
--                                                                              --
--       This software is confidential and proprietary and may be used          --
--        only as expressly authorized by a licensing agreement from            --
--                                                                              --
--                            Verisilicon.                                      --
--                                                                              --
--                   (C) COPYRIGHT 2020 VERISILICON                             --
--                            ALL RIGHTS RESERVED                               --
--                                                                              --
--                 The entire notice above must be reproduced                   --
--                  on all copies and should not be removed.                    --
--                                                                              --
--------------------------------------------------------------------------------*/

/* This file is generated. Please don't edit, don't change. */

#define VCESW_VERSION_MAJOR 2
#define VCESW_VERSION_MINOR 4
#define VCESW_VERSION_MICRO 101

#define VCESW_VESION_CODE (((VCESW_VERSION_MAJOR & 0xff)<<24) \
                         | ((VCESW_VERSION_MINOR & 0xff)<<16) \
                         | (VCESW_VERSION_MICRO & 0xffff))

/**
 * Evaluates to %TRUE if the version of VCE API is greater than
 * @major, @minor and @revision
 */
#define VCESW_CHECK_VERSION(major,minor,micro) \
        (VCESW_VERSION_MAJOR > (major) || \
         (VCESW_VERSION_MAJOR == (major) && VCESW_VERSION_MINOR > (minor)) || \
         (VCESW_VERSION_MAJOR == (major) && VCESW_VERSION_MINOR == (minor) && \
          VCESW_VERSION_MICRO >= (micro)))

/** Set P4CL# Directly */
#define VCENC_BUILD_CLNUM 908923

