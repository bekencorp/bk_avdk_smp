set(incs include vi5302)
set(srcs)

if (CONFIG_TOF_ENABLE)
	if (CONFIG_TOF_VI5302_ENABLE)
		list(APPEND srcs
			bk_tof_vi5302.c
			vi5302/VI5302_User_Handle.c
			vi5302/VI5302_API.c
			vi5302/VI5302_Firmware.c
			vi5302/VI5302_System_Data.c
			vi5302/VI5302_Algorithm.c
			vi5302/VI5302_Efuse.c
		)
	endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)

# Official Visionics sources trip -Werror=unused-*; keep vendor code intact.
if (CONFIG_TOF_VI5302_ENABLE)
	set_source_files_properties(
		vi5302/VI5302_API.c
		vi5302/VI5302_Firmware.c
		vi5302/VI5302_System_Data.c
		vi5302/VI5302_Algorithm.c
		vi5302/VI5302_Efuse.c
		PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function"
	)
endif()
