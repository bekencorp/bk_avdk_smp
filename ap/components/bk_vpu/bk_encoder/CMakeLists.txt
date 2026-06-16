set(incs ./
    h264e/include
    jpege/include
    h264e_stream/include
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
        jpege/src/bk_jpeg_encode.c
        jpege/src/bk_jpeg_encode_frame_ctlr.c
        jpege/src/bk_jpeg_encode_hw_flexa_ctlr.c
        jpege/src/bk_jpeg_encode_sw_flexa_ctlr.c
    )
endif()

if (CONFIG_H264E_STREAM_SESSION)
    list(APPEND srcs
        h264e_stream/src/h264e_stream_session.c
        h264e_stream/src/h264e_stream_protocol.c
        h264e_stream/src/h264e_stream_push.c
    )
endif()

set(priv_req bk_common avdk_driver bk_libs media_service avdk_utils)
set(req "")

# if (CONFIG_H264E_STREAM_SESSION)
#     list(APPEND priv_req bk_network_transfer)
# endif()

list(APPEND priv_req bk_network_transfer json)
armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES "${req}" PRIV_REQUIRES "${priv_req}")


