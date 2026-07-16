set(incs ./
    common
    jpegd/include
    h264d/include
    pp/include
)

set(srcs "")

# Hardware decoder controller - always built when component is enabled
list(APPEND srcs
    common/bk_decode_pp_helper.c
    common/hw_decoder_ctlr.c
)

# Standalone PP (NV12 -> RGB) converter
if (CONFIG_BK_DECODER)
    list(APPEND srcs
        pp/src/bk_pp.c
        pp/src/bk_pp_ctlr.c
    )
endif()

# JPEG decoder (vcdec-based)
if (CONFIG_BK_DECODER)
    list(APPEND srcs
        jpegd/src/bk_jpeg_decode.c
        jpegd/src/bk_jpeg_decode_frame_ctlr.c
        jpegd/src/bk_jpeg_decode_flexa_ctlr.c
        jpegd/src/bk_jpeg_decode_utils.c
        h264d/src/bk_h264_decode.c
        h264d/src/bk_h264_decode_frame_ctlr.c
        h264d/src/bk_h264_decode_frame_zerocopy_ctlr.c
        h264d/src/bk_h264_decode_flexa_ctlr.c
    )
endif()

set(priv_req avdk_driver bk_common avdk_utils media_service h264d_fbpool)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

