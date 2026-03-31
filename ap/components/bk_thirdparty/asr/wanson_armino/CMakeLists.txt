set(incs)

if (CONFIG_WANSON_ARMINO_ASR)

	list(APPEND incs
		include
		armino
	)

if (CONFIG_BK7259_ASR_DEBUG)
	set(srcs asrfst.c
	    )
endif()
if (CONFIG_WANSON_ASR_WORDS_VERSION_V1)
	list(APPEND incs
		armino/words_version_v1
	)
endif()

if (CONFIG_WANSON_ASR_WORDS_VERSION_V2)
	list(APPEND incs
		armino/words_version_v2
	)
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V1)
	list(APPEND incs
		armino/group_version_words_v1/
	)
endif()

if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V2)
	list(APPEND incs
		armino/group_version_words_v2/
	)
endif()

endif()	#(CONFIG_WANSON_ARMINO_ASR)

if(CONFIG_WANSON_CN_LICENSE)
	list(APPEND incs
		include
		armino
	)
endif()

if (CONFIG_BK7259_ASR_DEBUG)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
else()
armino_component_register(INCLUDE_DIRS "${incs}")
endif()

if (CONFIG_WANSON_ARMINO_ASR)

if (CONFIG_WANSON_ASR_GROUP_VERSION)
if (CONFIG_WANSON_CN_LICENSE)
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrfst_with_auth.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrfst_with_auth.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
else()
if (CONFIG_BK7259_ASR_DEBUG)

else()
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrfst.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrfst.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()
endif()
else()
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrbase.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/armino/libasrbase.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

endif()

if(CONFIG_WANSON_CN_LICENSE)
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/license/libwanson_license.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/license/libwanson_license.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

