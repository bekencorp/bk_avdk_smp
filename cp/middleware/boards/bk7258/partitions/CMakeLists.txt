armino_component_register(INCLUDE_DIRS ./)

if (CMAKE_BUILD_EARLY_EXPANSION)
return()
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_build)
armino_build_set_property(PARTITIONS_HDR_DIR ${GEN_DIR})
set(GEN_FILE ${GEN_DIR}/security.h)
set(CONFIG_FILE_LISTS
	bl1_control.json
	../csv/ota.csv
	../csv/security.csv
	../csv/ppc.csv
	../csv/mpc.csv
	)

armino_build_get_property(python PYTHON)
set(GEN_TOOL $ENV{ARMINO_TOOLS_PATH}/build_tools/build_process/bk_sdk/bk_prebuild.py)

# The active project's csv files override the board defaults at prebuild time,
# and the beken_utils generator turns those csv fields into security.h macros.
# Track both so editing them (e.g. sig_verify_en / flash_aes_type) regenerates
# security.h. The board soc name is taken from this file's parent directory so
# the app0 project config dir is used for every core build.
get_filename_component(BOARD_SOC_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(BOARD_SOC_NAME ${BOARD_SOC_DIR} NAME)
set(PROJECT_PARTITION_CFG_DIR $ENV{PROJECT_DIR}/partitions/${BOARD_SOC_NAME})
set(PROJECT_CONFIG_DEPS)
foreach(cfg security.csv ota.csv ppc.csv mpc.csv)
	if(EXISTS ${PROJECT_PARTITION_CFG_DIR}/${cfg})
		list(APPEND PROJECT_CONFIG_DEPS ${PROJECT_PARTITION_CFG_DIR}/${cfg})
	endif()
endforeach()

file(GLOB BEKEN_UTILS_SCRIPTS $ENV{ARMINO_TOOLS_PATH}/env_tools/beken_utils/scripts/*.py)

add_custom_command(OUTPUT ${GEN_FILE}
	COMMAND ${python} ${GEN_TOOL} $ENV{ARMINO_SOC}
	DEPENDS ${CONFIG_FILE_LISTS} ${PROJECT_CONFIG_DEPS} ${BEKEN_UTILS_SCRIPTS} ${GEN_TOOL}
)

add_custom_target(generated_files SOURCES ${GEN_FILE})
add_dependencies(${COMPONENT_LIB} generated_files)
target_include_directories(${COMPONENT_LIB} INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_build)

