set(incs ./
    h264e/include
    jpege/include
    common
)

set(srcs "")

# Hardware encoder controller (always built)
list(APPEND srcs
    common/hw_encoder_ctlr.c
)

# H.264 and JPEG encoder sources when CONFIG_BK_ENCODER
if (CONFIG_BK_ENCODER)
    list(APPEND srcs
        h264e/src/bk_h264_encode.c
        h264e/src/bk_h264_encode_frame_ctlr.c
        h264e/src/bk_h264_encode_hw_flexa_ctlr.c
        h264e/src/bk_h264_encode_sw_flexa_ctlr.c
        h264e/src/h264e_driver.c
        jpege/src/bk_jpeg_encode.c
        jpege/src/bk_jpeg_encode_frame_ctlr.c
        jpege/src/bk_jpeg_encode_hw_flexa_ctlr.c
        jpege/src/bk_jpeg_encode_sw_flexa_ctlr.c
    )
endif()

set(priv_req bk_common avdk_driver bk_libs media_service avdk_utils)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


