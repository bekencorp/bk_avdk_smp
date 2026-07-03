set(incs include decode legacy/include modules/container_parser modules/video_decoder modules/audio_decoder)
set(srcs "")

if (CONFIG_AVI_PLAYER)
    list(APPEND srcs
        src/bk_video_player_ctlr.c
        src/bk_video_player_engine.c
        src/bk_video_player_playlist.c
        common/bk_video_player_buffer_pool.c
        common/bk_video_player_file_list.c
        common/bk_video_player_pipeline.c
        src/bk_video_player_container_parse.c
        src/bk_video_player_audio_decode.c
        src/bk_video_player_video_decode.c
    )

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AVI_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_avi_parser.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_MP4_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_mp4_parser.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AVI_PARSER OR CONFIG_BK_VIDEO_PLAYER_ENABLE_MP4_PARSER)
        list(APPEND srcs modules/container_parser/bk_video_player_jpeg_probe.c)
    endif()

    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_JPEG_VIDEO_DECODER)
        list(APPEND srcs modules/video_decoder/bk_video_player_jpeg_decoder.c)
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER)
        list(APPEND srcs
            modules/video_decoder/bk_video_player_h264_flexa_decoder.c
            modules/video_decoder/bk_video_player_h264_frame_decoder.c
            modules/video_decoder/bk_video_player_h264_frame_zerocopy_decoder.c
        )
    endif()
    if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER)
        list(APPEND srcs modules/audio_decoder/bk_video_player_aac_decoder.c)
    endif()
endif()

set(_priv_requires bk_common include avdk_utils lvgl bk_vfs)
if (CONFIG_BK_VIDEO_PLAYER_ENABLE_AAC_AUDIO_DECODER)
    list(APPEND _priv_requires aacdec)
endif()
if (CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER)
    # The HW H264 decoder runs Flexa segment mode bonded to the GPU (VG-Lite)
    # for NV12 -> compressed ARGB conversion. Pull in bk_gpu for the GPU
    # controller and media_service for bk_flexa_h264d_gpu_bond_*.
    list(APPEND _priv_requires bk_gpu media_service)
endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES "${_priv_requires}"
)

