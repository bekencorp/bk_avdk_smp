set(incs . include )

armino_build_get_property(armino_path ARMINO_PATH)
set(nano_legacy ${armino_path}/properties/modules/verisilicon_nano/legacy)

set(srcs
        src/ap_main.c
        src/shoot4k_cli.c
)

if(CONFIG_BK_H264E)

        list(APPEND srcs
                ${nano_legacy}/bk_vpu/bk_h264e/src/h264_encoder_api.c
        )

        set_source_files_properties(
                ${nano_legacy}/bk_vpu/bk_h264e/src/h264_encoder_api.c
                PROPERTIES COMPILE_DEFINITIONS
                "BK_H265E;__FREERTOS__;SUBDEV_CHAR=1;FREERTOS=1;DUMMY_BUILD=0;ISP_PLATFORM=FPGA;OFFLINE_TEST=0;STATIC_BUILD=1;BUILD_SHARED_LIBS=OFF"
                COMPILE_OPTIONS
                "-Os;-Wno-int-to-pointer-cast;-Wno-pointer-to-int-cast;-Wno-unused-variable"
        )
        set_source_files_properties(
                src/shoot4k_cli.c
                PROPERTIES COMPILE_DEFINITIONS
                "__FREERTOS__;FREERTOS=1"
        )
endif()

set(priv_req media_service bk_cli avdk_utils avdk_driver bk_camera)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

if(CONFIG_BK_H264E)
        target_include_directories(${COMPONENT_LIB} BEFORE PRIVATE
                ${nano_legacy}/include
                ${nano_legacy}/bk_vpu/bk_h264e/include
        )
endif()
