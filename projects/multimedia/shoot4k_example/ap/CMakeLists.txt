set(incs . include )

set(srcs
        src/ap_main.c
        src/shoot4k_cli.c
)

if(CONFIG_BK_H264E)
        set_source_files_properties(
                src/shoot4k_cli.c
                PROPERTIES COMPILE_DEFINITIONS
                "__FREERTOS__;FREERTOS=1"
        )
endif()

set(priv_req media_service bk_cli avdk_utils avdk_driver bk_camera bk_libs)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
