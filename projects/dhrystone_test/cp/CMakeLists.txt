set(incs . dhrystone)
set(srcs cp_main.c)

if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()

# Dhrystone 2.1 (DMIPS): the sources and the CLI command live in this project
# (cp/dhrystone) so the benchmark moves with the project. dhry_cli.c registers
# the "dhrystone" command itself, no longer through bk_cli.
if (CONFIG_DHRYSTONE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dhrystone/dhry_1.c")
	list(APPEND srcs dhrystone/dhry_cli.c dhrystone/dhry_glue.c dhrystone/dhry_1.c dhrystone/dhry_2.c)
	# Adapter macros applied to the official dhry_1.c only; the official source stays untouched.
	set_source_files_properties(dhrystone/dhry_1.c PROPERTIES COMPILE_DEFINITIONS
		"main=dhrystone_main;printf=dhry_printf;scanf=dhry_scanf;DHRY_PORT=1;HZ=1000000")
	# The official sources are K&R C (implicit int, implicit declarations, malloc() prototype),
	# so build them as C89 and do not let warnings become errors.
	set_source_files_properties(dhrystone/dhry_1.c dhrystone/dhry_2.c PROPERTIES
		COMPILE_OPTIONS "-std=gnu89;-Wno-error")
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli)
