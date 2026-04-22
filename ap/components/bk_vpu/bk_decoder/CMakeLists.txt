set(incs ./
    common
    jpegd/include
)

set(srcs "")

# Hardware decoder controller - always built when component is enabled
list(APPEND srcs
    common/hw_decoder_ctlr.c
)

# JPEG decoder (vcdec-based)
if (CONFIG_BK_DECODER)
    list(APPEND srcs
        jpegd/src/bk_jpeg_decode.c
        jpegd/src/bk_jpeg_decode_frame_ctlr.c
        jpegd/src/bk_jpeg_decode_flexa_ctlr.c
    )
endif()

set(priv_req avdk_driver bk_common avdk_utils media_service)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

