set(incs include)
set(srcs "")

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs
		src/bk_asr.c
		src/bk_asr_service.c
	)

if (CONFIG_WANSON_ASR OR CONFIG_WANSON_ARMINO_ASR)
    set(incs wanson)
	list(APPEND incs
		wanson
	)
	list(APPEND srcs
		wanson/bk_wanson_asr_intf.c
	)
endif()

if (CONFIG_BEKEN_KWS)
    set(incs bk_kws)
	list(APPEND incs
		bk_kws
	)
	list(APPEND srcs
		bk_kws/bk_kws_asr_intf.c
	)

	# KWS model prebuilt lib ships next to the KWS interface (bk_kws/), in two
	# variants: libkws_model.a (built-in array) and libkws_model_vfs.a (from
	# udisk/VFS). Select the right one per project config so both internal and
	# released builds pick the correct variant (this component lives under
	# ap/components and is kept in the released SDK).
	if(CONFIG_SOC_BK7259)
		set(KWS_MODEL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bk_kws/bk7259")
	endif()

	if (CONFIG_BEKEN_KWS_MODEL_FROM_UDISK)
		set(LIB_KWS_MODEL_FILE_PATH "${KWS_MODEL_LIB_DIR}/libkws_model_vfs.a")
		message(STATUS "KWS model lib: ${LIB_KWS_MODEL_FILE_PATH} (VFS/file)")
	else()
		set(LIB_KWS_MODEL_FILE_PATH "${KWS_MODEL_LIB_DIR}/libkws_model.a")
		message(STATUS "KWS model lib: ${LIB_KWS_MODEL_FILE_PATH} (built-in array)")
	endif()

endif()

endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include wanson wanson_armino bk_tflite_micro
)

if (CONFIG_BEKEN_KWS AND LIB_KWS_MODEL_FILE_PATH AND EXISTS ${LIB_KWS_MODEL_FILE_PATH})
	get_filename_component(kws_model_lib_name ${LIB_KWS_MODEL_FILE_PATH} NAME)
	add_prebuilt_library(${kws_model_lib_name} "${LIB_KWS_MODEL_FILE_PATH}")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${kws_model_lib_name})
endif()
