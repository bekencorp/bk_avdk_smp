set(incs ./
    h264e/include
    common
)

set(srcs "")

# 硬件编码器控制器 - 总是编译
list(APPEND srcs
    common/hw_encoder_ctlr.c
)

# H.264 编码器 CONFIG_H264E
if (CONFIG_BK_ENCODER)
    list(APPEND srcs
        h264e/src/bk_h264_encode.c
        h264e/src/bk_h264_encode_frame_ctlr.c
        h264e/src/bk_h264_encode_hw_flexa_ctlr.c
        h264e/src/bk_h264_encode_sw_flexa_ctlr.c
        h264e/src/h264e_driver.c
    )
endif()

set(priv_req bk_common avdk_driver bk_libs media_service avdk_utils)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


