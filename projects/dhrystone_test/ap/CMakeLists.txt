set(incs . dhrystone)
set(srcs ap_main.c)

set(priv_req bk_network_provisioning bk_cli)

# Dhrystone 2.1 (DMIPS): the sources and the CLI command live in this project
# (ap/dhrystone) so the benchmark moves with the project. dhry_cli.c registers
# the "dhrystone" command itself, no longer through bk_cli.
if (CONFIG_DHRYSTONE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dhrystone/dhry_1.c")
	list(APPEND srcs dhrystone/dhry_cli.c dhrystone/dhry_glue.c dhrystone/dhry_1.c dhrystone/dhry_2.c)

	# Number of benchmark instances; Dhrystone 2.1 is not reentrant (all mutable
	# state is in file-scope globals), so the second instance is built from the
	# same official sources with every symbol renamed (see dhrystone/dhry_1_ctx1.c).
	if(DEFINED ENV{DHRYSTONE_CONTEXTS})
		set(dhrystone_contexts "$ENV{DHRYSTONE_CONTEXTS}")
	else()
		set(dhrystone_contexts "2")
	endif()
	if(NOT (dhrystone_contexts GREATER 1))
		set(dhrystone_contexts 1)
	else()
		set(dhrystone_contexts 2)
	endif()
	if(NOT CONFIG_SOC_SMP)
		set(dhrystone_contexts 1)
	endif()

	# Optimisation level of the benchmark code itself (dhry_1.c / dhry_2.c).
	if(DEFINED ENV{DHRYSTONE_OPT_FLAG})
		set(DHRY_BENCH_OPT "$ENV{DHRYSTONE_OPT_FLAG}")
	else()
		set(DHRY_BENCH_OPT "-Os")
	endif()
	set(DHRY_OPT_LABEL "${DHRY_BENCH_OPT}")

	# Code/data placement, driven by environment variables (default: Flash XIP
	# code, internal SRAM heap data):
	#   DHRYSTONE_IRAM_CODE=1   code in internal SRAM (.iram)
	#   DHRYSTONE_PSRAM_CODE=1  code in PSRAM (.psram.code)
	#   DHRYSTONE_SRAM3_DATA=1  Records in the fixed SRAM3 arena (0x2c100000)
	#   DHRYSTONE_SRAM4_DATA=1  Records in the fixed SRAM4 arena (0x2c140000)
	#   DHRYSTONE_DTCM_DATA=1   Records in DTCM (.dtcm_noinit)
	set(DHRY_PLACE_FLAGS "")
	set(DHRY_CODE_PLACEMENT "FLASH-XIP")
	set(DHRY_MEM_LOCATION "SRAM-HEAP")
	if(DEFINED ENV{DHRYSTONE_IRAM_CODE} AND "$ENV{DHRYSTONE_IRAM_CODE}" STREQUAL "1")
		list(APPEND DHRY_PLACE_FLAGS "-DDHRY_IRAM_CODE=1")
		set(DHRY_CODE_PLACEMENT "IRAM")
	elseif(DEFINED ENV{DHRYSTONE_PSRAM_CODE} AND "$ENV{DHRYSTONE_PSRAM_CODE}" STREQUAL "1")
		list(APPEND DHRY_PLACE_FLAGS "-DDHRY_PSRAM_CODE=1")
		set(DHRY_CODE_PLACEMENT "PSRAM")
	endif()
	if(DEFINED ENV{DHRYSTONE_SRAM3_DATA} AND "$ENV{DHRYSTONE_SRAM3_DATA}" STREQUAL "1")
		list(APPEND DHRY_PLACE_FLAGS "-DDHRY_SRAM3_DATA=1")
		set(DHRY_MEM_LOCATION "SRAM3")
	elseif(DEFINED ENV{DHRYSTONE_SRAM4_DATA} AND "$ENV{DHRYSTONE_SRAM4_DATA}" STREQUAL "1")
		list(APPEND DHRY_PLACE_FLAGS "-DDHRY_SRAM4_DATA=1")
		set(DHRY_MEM_LOCATION "SRAM4")
	elseif(DEFINED ENV{DHRYSTONE_DTCM_DATA} AND "$ENV{DHRYSTONE_DTCM_DATA}" STREQUAL "1")
		list(APPEND DHRY_PLACE_FLAGS "-DDHRY_DTCM_DATA=1")
		set(DHRY_MEM_LOCATION "DTCM")
	endif()

	# CPU frequency the command boosts to; follows the same knob as the clock boost
	# performed by bk_cli before calling dhrystone_run().
	if(DEFINED ENV{COREMARK_CPU_FREQ_MHZ})
		set(dhry_cpu_freq_mhz "$ENV{COREMARK_CPU_FREQ_MHZ}")
	else()
		set(dhry_cpu_freq_mhz "480")
	endif()

	# Adapter macros applied to the official sources only; they stay untouched.
	set_source_files_properties(dhrystone/dhry_1.c PROPERTIES COMPILE_DEFINITIONS
		"main=dhrystone_main;printf=dhry_printf;scanf=dhry_scanf;DHRY_PORT=1;HZ=1000000")
	set_source_files_properties(dhrystone/dhry_glue.c PROPERTIES COMPILE_DEFINITIONS
		"DHRY_OPT_LABEL=\"${DHRY_OPT_LABEL}\";DHRY_CODE_PLACEMENT=\"${DHRY_CODE_PLACEMENT}\";DHRY_MEM_LOCATION=\"${DHRY_MEM_LOCATION}\";DHRY_CONTEXTS=${dhrystone_contexts};DHRY_CPU_FREQ_MHZ=${dhry_cpu_freq_mhz}")
	# The CLI wrapper needs the same CPU frequency to pick the clock enum and to
	# print the boost message.
	set_source_files_properties(dhrystone/dhry_cli.c PROPERTIES COMPILE_DEFINITIONS
		"DHRY_CPU_FREQ_MHZ=${dhry_cpu_freq_mhz}")
	# The official sources are K&R C (implicit int, implicit declarations, malloc()
	# prototype), so build them as C89 and do not let warnings become errors.
	set_source_files_properties(dhrystone/dhry_1.c dhrystone/dhry_2.c PROPERTIES
		COMPILE_OPTIONS "-std=gnu89;-Wno-error;${DHRY_BENCH_OPT}")

	# Second instance for the SMP dual-core run: the same official files included
	# from a wrapper, with every file-scope symbol renamed.
	if(dhrystone_contexts EQUAL 2)
		list(APPEND srcs dhrystone/dhry_1_ctx1.c dhrystone/dhry_2_ctx1.c)
		set(DHRY_CTX1_DEFS
			"main=dhrystone_main1;printf=dhry_printf1;scanf=dhry_scanf1;dhry_malloc=dhry_malloc1;times=dhry_times1;DHRY_PORT=1;HZ=1000000"
			"Ptr_Glob=Ptr_Glob_1;Next_Ptr_Glob=Next_Ptr_Glob_1;Int_Glob=Int_Glob_1;Bool_Glob=Bool_Glob_1"
			"Ch_1_Glob=Ch_1_Glob_1;Ch_2_Glob=Ch_2_Glob_1;Arr_1_Glob=Arr_1_Glob_1;Arr_2_Glob=Arr_2_Glob_1"
			"Reg=Reg_1;time_info=time_info_1;Begin_Time=Begin_Time_1;End_Time=End_Time_1;User_Time=User_Time_1"
			"Microseconds=Microseconds_1;Dhrystones_Per_Second=Dhrystones_Per_Second_1"
			"Proc_1=Proc_1_1;Proc_2=Proc_2_1;Proc_3=Proc_3_1;Proc_4=Proc_4_1;Proc_5=Proc_5_1"
			"Proc_6=Proc_6_1;Proc_7=Proc_7_1;Proc_8=Proc_8_1"
			"Func_1=Func_1_1;Func_2=Func_2_1;Func_3=Func_3_1")
		set_source_files_properties(dhrystone/dhry_1_ctx1.c dhrystone/dhry_2_ctx1.c PROPERTIES
			COMPILE_DEFINITIONS "${DHRY_CTX1_DEFS}"
			COMPILE_OPTIONS "-std=gnu89;-Wno-error;${DHRY_BENCH_OPT}")
	endif()

	# Must come after the COMPILE_OPTIONS above, which would overwrite an appended value.
	if(DHRY_PLACE_FLAGS)
		set(DHRY_CTX_SRCS dhrystone/dhry_glue.c dhrystone/dhry_1.c dhrystone/dhry_2.c)
		if(dhrystone_contexts EQUAL 2)
			list(APPEND DHRY_CTX_SRCS dhrystone/dhry_1_ctx1.c dhrystone/dhry_2_ctx1.c)
		endif()
		set_property(SOURCE ${DHRY_CTX_SRCS}
			APPEND PROPERTY COMPILE_OPTIONS "${DHRY_PLACE_FLAGS}")
	endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
