set(incs .)

set(srcs)

if (CONFIG_ADK_AEC_V3_ALGORITHM)

    if (CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2)
        list(APPEND srcs
                v2.0/aec_v3_algorithm_v2.c
            )
        if (CONFIG_AEC_RUN_ON_M52)
            list(APPEND srcs
                    v2.0/audio_mp52_ipc_ap.c
                )
        endif()
    else()
        list(APPEND srcs
                v1.0/aec_v3_algorithm.c
            )
    endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

if (CONFIG_ADK_AEC_V3_ALGORITHM)
armino_build_get_property(armino_soc ARMINO_SOC)
set(ap_libs_path $ENV{ARMINO_PATH}/components/bk_libs/${armino_soc}/libs)

get_filename_component(lib_name ${ap_libs_path}/libaec_v3.a NAME)
add_prebuilt_library(${lib_name} "${ap_libs_path}/libaec_v3.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()
