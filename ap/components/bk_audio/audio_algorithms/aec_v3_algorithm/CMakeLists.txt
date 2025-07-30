set(incs .)

set(srcs)

if (CONFIG_ADK_AEC_V3_ALGORITHM)

list(APPEND srcs
		aec_v3_algorithm.c
	)


endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

if (CONFIG_ADK_AEC_V3_ALGORITHM)
set(ap_libs_path $ENV{ARMINO_PATH}/components/bk_libs/bk7258_ap/libs)

set(aec_v3_lib $ENV{ARMINO_PATH}/properties/modules/audio_codec/aec_v3/bk7258/libaec_v3.a)
file(COPY
"${aec_v3_lib}"
DESTINATION
"${ap_libs_path}"
)
get_filename_component(lib_name ${ap_libs_path}/libaec_v3.a NAME)
add_prebuilt_library(${lib_name} "${ap_libs_path}/libaec_v3.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()
