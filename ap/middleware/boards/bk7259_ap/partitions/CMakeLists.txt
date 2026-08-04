armino_component_register(INCLUDE_DIRS ./)

if (CMAKE_BUILD_EARLY_EXPANSION)
return()
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_build)
armino_build_set_property(PARTITIONS_HDR_DIR ${GEN_DIR})
set(GEN_FILE ${GEN_DIR}/security.h)
set(GEN_PARTITION_FILE ${GEN_DIR}/partitions_gen.h)
set(CONFIG_FILE_LISTS
	bl1_control.json
	../csv/ota.csv
	../csv/security.csv
	../csv/ppc.csv
	../csv/mpc.csv
	)
set(PROJECT_PARTITIONS_CSV $ENV{PROJECT_BUILD_DIR}/partitions/partitions.csv)
set(PROJECT_SECURITY_CSV $ENV{PROJECT_DIR}/partitions/bk7259/security.csv)
if(EXISTS ${PROJECT_SECURITY_CSV})
	list(APPEND CONFIG_FILE_LISTS ${PROJECT_SECURITY_CSV})
endif()
set(PROJECT_PPC_CSV $ENV{PROJECT_DIR}/partitions/$ENV{ARMINO_SOC}/ppc.csv)
set(PROJECT_PPRO_DEFAULT_CSV $ENV{PROJECT_DIR}/partitions/$ENV{ARMINO_SOC}/ppro_default.csv)
foreach(PROJECT_PPC_CONFIG ${PROJECT_PPC_CSV} ${PROJECT_PPRO_DEFAULT_CSV})
	if(EXISTS ${PROJECT_PPC_CONFIG})
		list(APPEND CONFIG_FILE_LISTS ${PROJECT_PPC_CONFIG})
	endif()
endforeach()

armino_build_get_property(python PYTHON)
set(GEN_TOOL $ENV{ARMINO_TOOLS_PATH}/build_tools/build_process/bk_sdk/bk_prebuild.py)

add_custom_command(OUTPUT ${GEN_FILE} ${GEN_PARTITION_FILE}
	COMMAND ${python} ${GEN_TOOL} $ENV{ARMINO_SOC}
	DEPENDS ${CONFIG_FILE_LISTS} ${PROJECT_PARTITIONS_CSV} ${GEN_TOOL}
)

add_custom_target(generated_files DEPENDS ${GEN_FILE} ${GEN_PARTITION_FILE})
add_dependencies(${COMPONENT_LIB} generated_files)
target_include_directories(${COMPONENT_LIB} INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_build)

