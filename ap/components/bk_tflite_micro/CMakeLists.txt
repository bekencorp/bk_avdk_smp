set(incs )
set(srcs )

# cmake_minimum_required(VERSION 3.5)
if (CONFIG_TFLITE_MICRO)

set(TENSORFLOW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tflite-micro)

set(MAKEFILE_DIR ${TENSORFLOW_ROOT}/tensorflow/lite/micro/tools/make)

set(DOWNLOADS_DIR ${MAKEFILE_DIR}/downloads)

get_filename_component(COMPILER_DIR ${CMAKE_C_COMPILER} DIRECTORY)


list(APPEND incs 
	${TENSORFLOW_ROOT}
	${DOWNLOADS_DIR}/gemmlowp
	${DOWNLOADS_DIR}/flatbuffers/include
	${DOWNLOADS_DIR}/kissfft
	${DOWNLOADS_DIR}/ruy
)

# Add Ethos-U driver headers if NPU is enabled
if (CONFIG_NPU)
	list(APPEND incs 
		${DOWNLOADS_DIR}/ethos_u_core_driver/include
		${DOWNLOADS_DIR}/cmsis/CMSIS/Core/Include
        include
	)

    list(APPEND srcs 
        src/bk_ethosu.c
    )
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")

if (CONFIG_TFLITE_MICRO)

target_compile_options(${COMPONENT_LIB} INTERFACE -DTF_LITE_STATIC_MEMORY)

include(ExternalProject)

set(target "cortex_m_generic")
set(optimized_kernel_dir "cmsis_nn")

# Detect target architecture. Default to cortex-m33, but force cortex-m55 whenever
# the NPU is enabled because Arm Ethos-U65 kernels are built for Cortex-M55 regardless
# of CONFIG_SOC_ARCH.
set(target_arch "cortex-m33")
set(target_arch_reason "default fallback")

if(NOT DEFINED CONFIG_SOC_ARCH OR CONFIG_SOC_ARCH STREQUAL "")
    message(WARNING "TFLM: CONFIG_SOC_ARCH is undefined or empty, using default target_arch=${target_arch}")
    set(target_arch_reason "missing CONFIG_SOC_ARCH fallback")
else()
    if(CONFIG_SOC_ARCH STREQUAL "cm55")
        set(target_arch "cortex-m55")
        set(target_arch_reason "CONFIG_SOC_ARCH=cm55")
    elseif(CONFIG_SOC_ARCH STREQUAL "cm33")
        set(target_arch_reason "CONFIG_SOC_ARCH=cm33")
    else()
        # Unknown value: keep default and print a warning for visibility.
        message(WARNING "TFLM: Unknown CONFIG_SOC_ARCH=${CONFIG_SOC_ARCH}, using default target_arch=${target_arch}")
        set(target_arch_reason "unknown CONFIG_SOC_ARCH fallback")
    endif()
endif()

if(CONFIG_NPU)
    set(target_arch "cortex-m55")
    set(target_arch_reason "CONFIG_NPU enabled -> force cortex-m55 for Ethos-U65")
endif()

# Capture toolchain -mcpu/-mfloat-abi flags so that the generated library matches the
# application ABI (avoid soft/hard float mismatches when linking).
set(tflm_detected_mcpu "")
set(tflm_detected_float_abi "")
set(_tflm_flag_snapshot "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}")
if(_tflm_flag_snapshot MATCHES "-mcpu=([^ ]+)")
    set(tflm_detected_mcpu "${CMAKE_MATCH_1}")
endif()
if(_tflm_flag_snapshot MATCHES "-mfloat-abi=([^ ]+)")
    set(tflm_detected_float_abi "${CMAKE_MATCH_1}")
endif()

if(NOT CONFIG_NPU AND target_arch MATCHES "^cortex-m55" AND tflm_detected_float_abi STREQUAL "soft")
    if(tflm_detected_mcpu MATCHES "cortex-m55\\+nodsp")
        set(target_arch "cortex-m55+nodsp+nofp")
        set(target_arch_reason "Toolchain flags -mcpu=${tflm_detected_mcpu}, -mfloat-abi=${tflm_detected_float_abi}")
    else()
        set(target_arch "cortex-m55+nofp")
        set(target_arch_reason "Toolchain flag -mfloat-abi=${tflm_detected_float_abi}")
    endif()
endif()

set(tflm_make_extra_args)
if(tflm_detected_float_abi)
    list(APPEND tflm_make_extra_args "FLOAT=${tflm_detected_float_abi}")
endif()

# Calculate project include path and config path for tflite-micro Makefile
# From ap/components/bk_tflite_micro to ap/include
get_filename_component(PROJECT_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../include" ABSOLUTE)
list(APPEND tflm_make_extra_args "PROJECT_INCLUDE_PATH=${PROJECT_INCLUDE_PATH}")

# Add build config directory for sdkconfig.h
# sdkconfig.h is generated in build_dir/config/sdkconfig.h
armino_build_get_property(build_dir BUILD_DIR)
get_filename_component(PROJECT_CONFIG_PATH "${build_dir}/config" ABSOLUTE)
list(APPEND tflm_make_extra_args "PROJECT_CONFIG_PATH=${PROJECT_CONFIG_PATH}")

# Put TFLM object/lib output under this project's BUILD_DIR instead of
# ${TENSORFLOW_ROOT}/gen. Multiple projects/jobs sharing the same SDK tree used to
# race on the same gen/ tree (rm -rf / parallel writes), which breaks CI.
get_filename_component(TFLM_HOST_GENDIR "${build_dir}/tflite_micro_gen" ABSOLUTE)
list(APPEND tflm_make_extra_args "BASE_GENDIR=${TFLM_HOST_GENDIR}")

message(STATUS "TFLM: Using target_arch=${target_arch} (${target_arch_reason})")
message(STATUS "TFLM: Per-build gen directory (BASE_GENDIR): ${TFLM_HOST_GENDIR}")
message(STATUS "TFLM: Project include path: ${PROJECT_INCLUDE_PATH}")
message(STATUS "TFLM: Project config path: ${PROJECT_CONFIG_PATH}")

# Parallel jobs for the embedded TensorFlow Lite Micro Makefile (GNU make -j).
# With GEN isolated per BUILD_DIR, parallel projects no longer contend on the same gen/ tree;
# BK_TFLM_MAKE_JOBS still overrides microlite make parallelism.
if(DEFINED ENV{BK_TFLM_MAKE_JOBS} AND NOT "$ENV{BK_TFLM_MAKE_JOBS}" STREQUAL "")
    set(TFLM_MAKE_JOBS "$ENV{BK_TFLM_MAKE_JOBS}")
else()
    cmake_host_system_information(RESULT _tflm_nproc QUERY NUMBER_OF_LOGICAL_CORES)
    if(NOT _tflm_nproc OR _tflm_nproc LESS 1)
        set(_tflm_nproc 4)
    endif()
    if(_tflm_nproc GREATER 8)
        set(TFLM_MAKE_JOBS 8)
    else()
        set(TFLM_MAKE_JOBS ${_tflm_nproc})
    endif()
endif()
message(STATUS "TFLM: microlite make parallelism: -j${TFLM_MAKE_JOBS} (set BK_TFLM_MAKE_JOBS to override)")

# Configure build parameters based on NPU support
if (CONFIG_NPU)
    # Enable Ethos-U65 NPU support
    # CO_PROCESSOR=ethos_u will make TFLM prioritize NPU-supported operators
    # OPTIMIZED_KERNEL_DIR=cmsis_nn provides fallback for unsupported operators
    set(co_processor "ethos_u")
    set(ethosu_arch "u65")
    # When CO_PROCESSOR is set, the generated directory includes the coprocessor name
    # Use detected target_arch in the path
    set(TFLM_LIB_DIR ${TFLM_HOST_GENDIR}/cortex_m_generic_${target_arch}_default_cmsis_nn_ethos_u_gcc/lib/libtensorflow-microlite.a)
    get_filename_component(_tflm_lib_dir ${TFLM_LIB_DIR} DIRECTORY)
    get_filename_component(TFLM_VARIANT_DIR ${_tflm_lib_dir} DIRECTORY)

    # Note: Cache API adaptation is now implemented in bk_ethosu.c
    # ETHOSU_EXTRA_FLAGS is no longer needed as cache functions are conditionally
    # compiled based on CONFIG_NPU_CACHE in bk_ethosu.c
    set(make_env_cmd ${CMAKE_COMMAND} -E env)
    
    add_custom_command(OUTPUT ${TFLM_LIB_DIR}
        COMMAND rm -rf ${TFLM_VARIANT_DIR}
        COMMAND ${make_env_cmd} make -f ${MAKEFILE_DIR}/Makefile ${tflm_make_extra_args} TARGET_TOOLCHAIN_ROOT=${COMPILER_DIR}/ TARGET=${target} TARGET_ARCH=${target_arch} OPTIMIZED_KERNEL_DIR=${optimized_kernel_dir} CO_PROCESSOR=${co_processor} ETHOSU_ARCH=${ethosu_arch} microlite -j${TFLM_MAKE_JOBS}
        WORKING_DIRECTORY ${TENSORFLOW_ROOT}
        VERBATIM
        USES_TERMINAL
        COMMENT "Build tflm project with Ethos-U65 NPU support (arch: ${target_arch})"
    )
else()
    # Use CMSIS-NN only (no NPU)
    # Use detected target_arch in the path
    set(TFLM_LIB_DIR ${TFLM_HOST_GENDIR}/cortex_m_generic_${target_arch}_default_cmsis_nn_gcc/lib/libtensorflow-microlite.a)
    get_filename_component(_tflm_lib_dir ${TFLM_LIB_DIR} DIRECTORY)
    get_filename_component(TFLM_VARIANT_DIR ${_tflm_lib_dir} DIRECTORY)

    add_custom_command(OUTPUT ${TFLM_LIB_DIR}
        COMMAND rm -rf ${TFLM_VARIANT_DIR}
        COMMAND make -f ${MAKEFILE_DIR}/Makefile ${tflm_make_extra_args} TARGET=${target} TARGET_ARCH=${target_arch} OPTIMIZED_KERNEL_DIR=${optimized_kernel_dir} microlite -j${TFLM_MAKE_JOBS}
        WORKING_DIRECTORY ${TENSORFLOW_ROOT}
        VERBATIM
        USES_TERMINAL
        COMMENT "Build tflm project with CMSIS-NN only (arch: ${target_arch})"
    )
endif()
add_custom_target(tflm_project DEPENDS ${TFLM_LIB_DIR})

add_library(tflm STATIC IMPORTED GLOBAL)
add_dependencies(tflm tflm_project)
set_target_properties(tflm PROPERTIES IMPORTED_LOCATION ${TFLM_LIB_DIR})
target_link_libraries(${COMPONENT_LIB} INTERFACE tflm)
endif()
