#-------------------------------------------------------------------------------
# Copyright (c)     2023-2028, Arm Limited. All rights reserved.
# Copyright (c)     2023-2028 Cypress Semiconductor Corporation (an Infineon company)
# or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(version)
include(remote_library)
include(utils)

if(EXISTS ${CMAKE_SOURCE_DIR}/localrepos.cmake)
    message(WARNING "Building using local repositories from \"" ${CMAKE_SOURCE_DIR}/localrepos.cmake "\"")
    include(${CMAKE_SOURCE_DIR}/localrepos.cmake)
endif()

############################ CONFIGURATION #####################################
include(config/pre_config.cmake)

if(USE_KCONFIG_TOOL)
    include(${CMAKE_SOURCE_DIR}/config/kconfig.cmake)
else()
    include(${CMAKE_SOURCE_DIR}/config/set_config.cmake)
endif()

include(config/post_config.cmake)

############################### Compiler configuration #########################

include(${TFM_TOOLCHAIN_FILE})

set(CMAKE_C_COMPILER_FORCED true)
set(CMAKE_CXX_COMPILER_FORCED true)

project("Trusted Firmware M" VERSION ${TFM_VERSION} LANGUAGES C CXX ASM)
tfm_toolchain_reload_compiler()

# Generate armino_config.h from the Armino/SDK Kconfig values passed in from the
# parent build (e.g. -DCONFIG_TFM_RAM_SIZE=${CONFIG_CPU0_SPE_RAM_SIZE}). Every
# secure-world consumer (region_defs.h, bk_tfm_mpc.c, the .ld scatter files that
# include region_defs.h) includes "armino_config.h", so the SPE RAM size is
# driven solely by the external defconfig and needs no hardcode in sdkconfig.h.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/armino_config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config/armino_config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/config)

add_subdirectory(lib/ext)
add_subdirectory(lib/fih)
add_subdirectory(tools)
add_subdirectory(secure_fw)

add_subdirectory(interface)
if(BL2)
    add_subdirectory(bl2)
endif()

if(BL1 AND PLATFORM_DEFAULT_BL1)
    add_subdirectory(bl1/bl1_2)
    add_subdirectory(bl1/bl1_1)
endif()

add_subdirectory(platform)

if(CRYPTO_HW_ACCELERATOR)
    add_subdirectory(platform/ext/accelerator)
endif()

if(IS_DIRECTORY ${CONFIG_TFM_TEST_DIR})
    add_subdirectory(${CONFIG_TFM_TEST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tf-m-tests)
endif()

############################ Config Check ######################################

include(${CMAKE_SOURCE_DIR}/config/check_config.cmake)

################################################################################

include(cmake/install.cmake)
