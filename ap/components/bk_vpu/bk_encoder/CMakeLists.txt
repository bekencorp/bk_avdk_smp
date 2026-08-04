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
    common/h264_encode_vcenc_rate_ctrl.c
    common/h264_encode_osd.c
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
        jpege/src/bk_jpeg_encode_sw_frame_ctlr.c
        jpege/src/bk_jpeg_encode_hw_flexa_ctlr.c
        jpege/src/bk_jpeg_encode_sw_flexa_ctlr.c
    )
    if (CONFIG_BK_H264E_DEBUG_SEI)
        list(APPEND srcs
            common/h264_debug_sei.c
        )
    endif()
endif()

if (CONFIG_H264E_STREAM_SESSION)
    list(APPEND srcs
        h264e_stream/src/h264e_stream_session.c
        h264e_stream/src/h264e_stream_schema.c
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

if (CONFIG_BK_ENCODER)
    armino_build_get_property(armino_soc ARMINO_SOC)

    set(video_libs)
    if(CONFIG_VIDEO_DTCM)
        if(CONFIG_VIDEO_DTCM_ALL)
            list(APPEND video_libs
                libtjpg_enc_dtcm.a
            )
        else()
            list(APPEND video_libs
                libtjpg_enc_dtcm_part.a
            )
        endif()
    else()
        list(APPEND video_libs
            libtjpg_enc_sram.a
        )
    endif()

    foreach(lib_name ${video_libs})
        add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${armino_soc}/libs/${lib_name}")
        target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
    endforeach()
endif()


