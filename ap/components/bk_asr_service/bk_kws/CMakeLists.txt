set(incs)
if (CONFIG_SOC_BK7258)
    set(KWS_MODEL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bk7258")
elseif(CONFIG_SOC_BK7259)
    set(KWS_MODEL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bk7259")
else()
    set(KWS_MODEL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bk7258")
endif()

set(KWS_MODEL_FROM_UDISK_ENABLED 0)
set(KWS_MODEL_PROJECT_DECONFIG_CANDIDATES
    "$ENV{PROJECT_DIR}/ap/config/bk7259_ap/defconfig"
    "${CMAKE_SOURCE_DIR}/ap/config/bk7259_ap/defconfig"
)
foreach(defconfig_path ${KWS_MODEL_PROJECT_DECONFIG_CANDIDATES})
    if (EXISTS "${defconfig_path}")
        file(STRINGS "${defconfig_path}" defconfig_has_kws_udisk REGEX "^CONFIG_BEKEN_KWS_MODEL_FROM_UDISK=y$")
        if (defconfig_has_kws_udisk)
            set(KWS_MODEL_FROM_UDISK_ENABLED 1)
        endif()
    endif()
endforeach()

if (CONFIG_BEKEN_KWS_MODEL_FROM_UDISK OR KWS_MODEL_FROM_UDISK_ENABLED)
    set(LIB_KWS_MODEL_FILE_PATH "${KWS_MODEL_LIB_DIR}/libkws_model_vfs.a")
    message(STATUS "KWS model lib: ${LIB_KWS_MODEL_FILE_PATH} (VFS/file)")
else()
    set(LIB_KWS_MODEL_FILE_PATH "${KWS_MODEL_LIB_DIR}/libkws_model.a")
    message(STATUS "KWS model lib: ${LIB_KWS_MODEL_FILE_PATH} (built-in array)")
endif()

armino_component_register(INCLUDE_DIRS "${incs}")

get_filename_component(lib_name ${LIB_KWS_MODEL_FILE_PATH} NAME)
file(COPY ${LIB_KWS_MODEL_FILE_PATH}
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_prebuilt_library(${lib_name} "${LIB_KWS_MODEL_FILE_PATH}")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
