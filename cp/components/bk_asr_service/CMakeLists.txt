set(incs include)
set(srcs "")
set(priv_requires bk_common bk_audio include)

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

endif()

endif()

if (CONFIG_ADK_OBS_UTIL)
	list(APPEND priv_requires audio_utils)
endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_requires}"
)
